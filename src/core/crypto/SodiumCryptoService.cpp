#include "SodiumCryptoService.h"

#include <sodium.h>
#include <stdexcept>
#include <cstring>

// ── Format constants ──────────────────────────────────────────────────────────

static constexpr unsigned char MAGIC[4] = {'P', 'B', 'D', 0x01};
static constexpr int MAGIC_SIZE   = 4;
static constexpr int ALGO_SIZE    = 1;
static constexpr int NONCE_SIZE   = 24;   // XChaCha20 nonce; AES-GCM uses first 12
static constexpr int HEADER_SIZE  = MAGIC_SIZE + ALGO_SIZE + NONCE_SIZE;  // 29 bytes
static constexpr int TAG_SIZE     = 16;

static constexpr unsigned char ALGO_AESGCM    = 0;
static constexpr unsigned char ALGO_XCHACHA20 = 1;

// ── Argon2id parameters ───────────────────────────────────────────────────────
// MODERATE preset: ops=3, mem=256 MB.
// Rationale for this app:
//   • Financial data — high sensitivity; offline brute-force is a realistic threat.
//   • Login once per session on i5-12500H → ~1-2 s latency is acceptable.
//   • 256 MB per guess makes GPU farms extremely expensive.
//   • Exceeds SECURITY.md minimum (≥64 MB, ≥3 iterations).
static constexpr unsigned long long ARGON2ID_OPS = crypto_pwhash_OPSLIMIT_MODERATE;
static constexpr size_t             ARGON2ID_MEM = crypto_pwhash_MEMLIMIT_MODERATE;

// ── SodiumCryptoService ───────────────────────────────────────────────────────

SodiumCryptoService::SodiumCryptoService()
{
    if (sodium_init() < 0)
        throw std::runtime_error("libsodium init failed");
    m_useAesGcm = (crypto_aead_aes256gcm_is_available() != 0);
}

bool SodiumCryptoService::aesNiAvailable()
{
    if (sodium_init() < 0) return false;
    return crypto_aead_aes256gcm_is_available() != 0;
}

bool SodiumCryptoService::isNewFormat(const QByteArray &data)
{
    if (data.size() < MAGIC_SIZE) return false;
    return memcmp(data.constData(), MAGIC, MAGIC_SIZE) == 0;
}

QByteArray SodiumCryptoService::encrypt(const QByteArray &plaintext, const QByteArray &key)
{
#ifdef QT_DEBUG
    return plaintext;
#endif

    if (key.size() != 32)
        throw std::runtime_error("Key must be 32 bytes");

    // Generate fresh nonce (24 bytes; AES-GCM only consumes the first 12)
    QByteArray nonce(NONCE_SIZE, '\0');
    randombytes_buf(nonce.data(), static_cast<size_t>(NONCE_SIZE));

    const unsigned char algo = m_useAesGcm ? ALGO_AESGCM : ALGO_XCHACHA20;

    // libsodium AEAD appends the 16-byte tag at the end of ciphertext
    const unsigned long long cipherLen =
        static_cast<unsigned long long>(plaintext.size()) + TAG_SIZE;
    QByteArray cipher(static_cast<int>(cipherLen), '\0');
    unsigned long long actualLen = 0;

    int ret = -1;
    if (m_useAesGcm) {
        ret = crypto_aead_aes256gcm_encrypt(
            reinterpret_cast<unsigned char *>(cipher.data()), &actualLen,
            reinterpret_cast<const unsigned char *>(plaintext.constData()),
            static_cast<unsigned long long>(plaintext.size()),
            nullptr, 0, nullptr,
            reinterpret_cast<const unsigned char *>(nonce.constData()),   // 12 bytes used
            reinterpret_cast<const unsigned char *>(key.constData()));
    } else {
        ret = crypto_aead_xchacha20poly1305_ietf_encrypt(
            reinterpret_cast<unsigned char *>(cipher.data()), &actualLen,
            reinterpret_cast<const unsigned char *>(plaintext.constData()),
            static_cast<unsigned long long>(plaintext.size()),
            nullptr, 0, nullptr,
            reinterpret_cast<const unsigned char *>(nonce.constData()),
            reinterpret_cast<const unsigned char *>(key.constData()));
    }

    if (ret != 0)
        throw std::runtime_error("Encryption failed");

    cipher.resize(static_cast<int>(actualLen));

    // Assemble: magic(4) + algo(1) + nonce(24) + cipher(N+16)
    QByteArray result;
    result.reserve(HEADER_SIZE + cipher.size());
    result.append(reinterpret_cast<const char *>(MAGIC), MAGIC_SIZE);
    result.append(static_cast<char>(algo));
    result.append(nonce);
    result.append(cipher);
    return result;
}

QByteArray SodiumCryptoService::decrypt(const QByteArray &data, const QByteArray &key)
{
#ifdef QT_DEBUG
    return data;
#endif

    if (!isNewFormat(data))
        throw std::runtime_error("Not a PBD v1 file — missing magic bytes");

    if (data.size() < HEADER_SIZE + TAG_SIZE)
        throw std::runtime_error("Ciphertext too short");

    if (key.size() != 32)
        throw std::runtime_error("Key must be 32 bytes");

    const unsigned char algo = static_cast<unsigned char>(data[MAGIC_SIZE]);
    const QByteArray nonce   = data.mid(MAGIC_SIZE + ALGO_SIZE, NONCE_SIZE);
    const QByteArray cipher  = data.mid(HEADER_SIZE);

    if (cipher.size() < TAG_SIZE)
        throw std::runtime_error("Ciphertext too short after header");

    const unsigned long long plainLen =
        static_cast<unsigned long long>(cipher.size()) - TAG_SIZE;
    QByteArray plain(static_cast<int>(plainLen), '\0');
    unsigned long long actualLen = 0;

    int ret = -1;
    if (algo == ALGO_AESGCM) {
        ret = crypto_aead_aes256gcm_decrypt(
            reinterpret_cast<unsigned char *>(plain.data()), &actualLen,
            nullptr,
            reinterpret_cast<const unsigned char *>(cipher.constData()),
            static_cast<unsigned long long>(cipher.size()),
            nullptr, 0,
            reinterpret_cast<const unsigned char *>(nonce.constData()),
            reinterpret_cast<const unsigned char *>(key.constData()));
    } else if (algo == ALGO_XCHACHA20) {
        ret = crypto_aead_xchacha20poly1305_ietf_decrypt(
            reinterpret_cast<unsigned char *>(plain.data()), &actualLen,
            nullptr,
            reinterpret_cast<const unsigned char *>(cipher.constData()),
            static_cast<unsigned long long>(cipher.size()),
            nullptr, 0,
            reinterpret_cast<const unsigned char *>(nonce.constData()),
            reinterpret_cast<const unsigned char *>(key.constData()));
    } else {
        throw std::runtime_error("Unknown algorithm ID in file header");
    }

    if (ret != 0)
        throw std::runtime_error("Decryption failed — wrong key or corrupt data");

    plain.resize(static_cast<int>(actualLen));
    return plain;
}

QByteArray SodiumCryptoService::deriveKey(const QString &password, const QByteArray &salt)
{
    // Cópia mutável da senha em bytes, para podermos zerá-la após derivar.
    // QByteArray::data() força o detach (cópia única, COW), garantindo que o
    // sodium_memzero abaixo apague ESTE buffer e não um compartilhado.
    QByteArray passBytes = password.toUtf8();

    // Conveniência sobre o overload de 4 args — defaults MODERATE (mesmos de
    // sempre). (ADR-001 §3.3): os wraps de K usam o overload de 4 args
    // diretamente, com os ops/mem gravados no registro por-usuário.
    const QByteArray key = deriveKey(passBytes, salt, ARGON2ID_OPS, ARGON2ID_MEM);

    // (#128) Zera a senha em claro assim que não é mais necessária (após a
    // derivação), independentemente de sucesso/falha. Reduz a janela em que a
    // senha do usuário fica residente em memória.
    sodium_memzero(passBytes.data(), static_cast<size_t>(passBytes.size()));

    return key;
}

QByteArray SodiumCryptoService::deriveKey(const QByteArray &password, const QByteArray &salt,
                                          unsigned long long opsLimit, size_t memLimit) const
{
    if (salt.size() != static_cast<int>(crypto_pwhash_SALTBYTES))
        throw std::runtime_error("Salt must be crypto_pwhash_SALTBYTES (16) bytes");

    QByteArray key(32, '\0');
    const int ret = crypto_pwhash(
        reinterpret_cast<unsigned char *>(key.data()), 32,
        password.constData(), static_cast<unsigned long long>(password.size()),
        reinterpret_cast<const unsigned char *>(salt.constData()),
        opsLimit, memLimit,
        crypto_pwhash_ALG_ARGON2ID13);

    if (ret != 0)
        throw std::runtime_error("Argon2id key derivation failed — out of memory?");

    return key;
}

QByteArray SodiumCryptoService::generateSalt()
{
    // crypto_pwhash_SALTBYTES == 16, same as the legacy KeyDerivation::SALT_SIZE.
    // Existing salts stored in the keychain are compatible.
    return randomBytes(crypto_pwhash_SALTBYTES);
}

QByteArray SodiumCryptoService::randomBytes(size_t n) const
{
    QByteArray b(static_cast<int>(n), '\0');
    randombytes_buf(b.data(), n);
    return b;
}

QByteArray SodiumCryptoService::wrapKey(const QByteArray &secret32, const QByteArray &wrapKey32) const
{
    // Invariante de tamanho de chave é erro de PROGRAMAÇÃO (o chamador
    // sempre deriva wrapKey32 via deriveKey, 32B fixos) — não confundir com
    // unwrapKey, onde o payload vem de disco/rede (pode ser adversarial) e
    // portanto retorna nullopt em vez de lançar.
    if (wrapKey32.size() != crypto_secretbox_KEYBYTES)
        throw std::runtime_error("wrapKey: wrapKey32 must be crypto_secretbox_KEYBYTES (32) bytes");

    const QByteArray nonce = randomBytes(crypto_secretbox_NONCEBYTES);
    QByteArray cipher(secret32.size() + crypto_secretbox_MACBYTES, '\0');

    crypto_secretbox_easy(
        reinterpret_cast<unsigned char *>(cipher.data()),
        reinterpret_cast<const unsigned char *>(secret32.constData()),
        static_cast<unsigned long long>(secret32.size()),
        reinterpret_cast<const unsigned char *>(nonce.constData()),
        reinterpret_cast<const unsigned char *>(wrapKey32.constData()));

    QByteArray result;
    result.reserve(nonce.size() + cipher.size());
    result.append(nonce);
    result.append(cipher);
    return result;
}

std::optional<QByteArray> SodiumCryptoService::unwrapKey(const QByteArray &wrapped,
                                                          const QByteArray &wrapKey32) const
{
    if (wrapKey32.size() != crypto_secretbox_KEYBYTES)
        return std::nullopt;
    if (wrapped.size() < crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES)
        return std::nullopt;

    const QByteArray nonce  = wrapped.left(crypto_secretbox_NONCEBYTES);
    const QByteArray cipher = wrapped.mid(crypto_secretbox_NONCEBYTES);
    QByteArray plain(cipher.size() - crypto_secretbox_MACBYTES, '\0');

    const int ret = crypto_secretbox_open_easy(
        reinterpret_cast<unsigned char *>(plain.data()),
        reinterpret_cast<const unsigned char *>(cipher.constData()),
        static_cast<unsigned long long>(cipher.size()),
        reinterpret_cast<const unsigned char *>(nonce.constData()),
        reinterpret_cast<const unsigned char *>(wrapKey32.constData()));

    // Tag inválida = senha/recovery key errada. Erro ESPERADO (não é bug do
    // programa) — nullopt, sem exceção.
    if (ret != 0)
        return std::nullopt;

    return plain;
}

SodiumCryptoService::SignKeypair SodiumCryptoService::generateSignKeypair() const
{
    SignKeypair kp;
    kp.publicKey.resize(crypto_sign_PUBLICKEYBYTES);
    kp.secretKey.resize(crypto_sign_SECRETKEYBYTES);
    crypto_sign_keypair(
        reinterpret_cast<unsigned char *>(kp.publicKey.data()),
        reinterpret_cast<unsigned char *>(kp.secretKey.data()));
    return kp;
}

QByteArray SodiumCryptoService::sign(const QByteArray &message, const QByteArray &secretKey64) const
{
    if (secretKey64.size() != crypto_sign_SECRETKEYBYTES)
        throw std::runtime_error("sign: secretKey64 must be crypto_sign_SECRETKEYBYTES (64) bytes");

    QByteArray signature(crypto_sign_BYTES, '\0');
    unsigned long long sigLen = 0;
    crypto_sign_detached(
        reinterpret_cast<unsigned char *>(signature.data()), &sigLen,
        reinterpret_cast<const unsigned char *>(message.constData()),
        static_cast<unsigned long long>(message.size()),
        reinterpret_cast<const unsigned char *>(secretKey64.constData()));
    signature.resize(static_cast<int>(sigLen));
    return signature;
}

bool SodiumCryptoService::verify(const QByteArray &message, const QByteArray &signature64,
                                 const QByteArray &publicKey32) const
{
    // Tamanho errado = adulteração/registro malformado — falha, não crash.
    if (signature64.size() != crypto_sign_BYTES || publicKey32.size() != crypto_sign_PUBLICKEYBYTES)
        return false;

    // crypto_sign_verify_detached já compara em tempo constante internamente
    // (Ed25519 do libsodium) — não precisa de sodium_memcmp adicional aqui.
    return crypto_sign_verify_detached(
        reinterpret_cast<const unsigned char *>(signature64.constData()),
        reinterpret_cast<const unsigned char *>(message.constData()),
        static_cast<unsigned long long>(message.size()),
        reinterpret_cast<const unsigned char *>(publicKey32.constData())) == 0;
}
