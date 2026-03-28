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
    if (salt.size() != static_cast<int>(crypto_pwhash_SALTBYTES))
        throw std::runtime_error("Salt must be crypto_pwhash_SALTBYTES (16) bytes");

    const QByteArray passBytes = password.toUtf8();
    QByteArray key(32, '\0');

    const int ret = crypto_pwhash(
        reinterpret_cast<unsigned char *>(key.data()), 32,
        passBytes.constData(), static_cast<unsigned long long>(passBytes.size()),
        reinterpret_cast<const unsigned char *>(salt.constData()),
        ARGON2ID_OPS, ARGON2ID_MEM,
        crypto_pwhash_ALG_ARGON2ID13);

    if (ret != 0)
        throw std::runtime_error("Argon2id key derivation failed — out of memory?");

    return key;
}

QByteArray SodiumCryptoService::generateSalt()
{
    // crypto_pwhash_SALTBYTES == 16, same as the legacy KeyDerivation::SALT_SIZE.
    // Existing salts stored in the keychain are compatible.
    QByteArray salt(static_cast<int>(crypto_pwhash_SALTBYTES), '\0');
    randombytes_buf(salt.data(), crypto_pwhash_SALTBYTES);
    return salt;
}
