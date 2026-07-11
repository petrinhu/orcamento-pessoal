#pragma once

#include "core/interfaces/ICryptoService.h"

#include <optional>

// Encryption/decryption via libsodium.
//
// Cipher:
//   AES-256-GCM          (algo ID 0) when AES-NI is present at runtime
//   XChaCha20-Poly1305   (algo ID 1) otherwise
//
// Key derivation: Argon2id — OPSLIMIT_MODERATE (ops=3), MEMLIMIT_MODERATE (256 MB)
//
// File format (new):
//   [4 bytes magic 'P','B','D',0x01]
//   [1 byte  algo  0=AES-GCM / 1=XChaCha20]
//   [24 bytes nonce — AES-GCM uses first 12; XChaCha20 uses all 24]
//   [ciphertext + 16-byte AEAD tag]
//
// Legacy files (no magic) are detected by DatabaseManager and handled with
// AesCryptoService (AES-256-CBC + PBKDF2) for backward compatibility.
class SodiumCryptoService final : public ICryptoService {
public:
    SodiumCryptoService();

    QByteArray encrypt(const QByteArray &plaintext, const QByteArray &key) override;
    QByteArray decrypt(const QByteArray &ciphertext, const QByteArray &key) override;
    QByteArray deriveKey(const QString &password, const QByteArray &salt) override;
    QByteArray generateSalt() override;

    // (ADR-001 #212 §3.3 Foundation) Overload de deriveKey com params Argon2id
    // explícitos — os wraps de K (por senha e por recovery key) exigem
    // ops/mem gravados por-usuário no registro (kdf_params, permite
    // auto-rehash futuro), em vez do default MODERATE fixo usado por
    // deriveKey(QString,QByteArray) acima. Esse overload de 2 args passa a
    // ser conveniência: chama este com ARGON2ID_OPS/ARGON2ID_MEM (MODERATE).
    QByteArray deriveKey(const QByteArray &password, const QByteArray &salt,
                         unsigned long long opsLimit, size_t memLimit) const;

    // Generaliza generateSalt() — N bytes aleatórios via randombytes_buf
    // (K de 32B, recovery key de 20B, salts, etc.).
    QByteArray randomBytes(size_t n) const;

    // ── Envelope encryption (wrap/unwrap de K) — ADR-001 §3.2 ───────────────
    // crypto_secretbox_easy (XSalsa20-Poly1305), NÃO o AEAD do .pbd: o AEAD
    // grande (crypto_aead_aes256gcm_*) exige AES-NI em runtime sem fallback
    // — um wrap feito com AES-NI ficaria ilegível ao restaurar backup numa
    // máquina sem AES-NI. secretbox é portável e o payload é sempre pequeno
    // (32-64B), sem ganho em reusar o AEAD grande.
    // Formato: nonce(24B) || ciphertext(secret.size()+16B MAC).
    QByteArray wrapKey(const QByteArray &secret32, const QByteArray &wrapKey32) const;
    // nullopt = tag inválida (senha/recovery key errada) — erro esperado,
    // por retorno, SEM exceção (CONTRACT §6.4: optional para falha esperada).
    std::optional<QByteArray> unwrapKey(const QByteArray &wrapped, const QByteArray &wrapKey32) const;

    // ── Assinatura Ed25519 — integridade do users.json (ADR-001 §3.6/D-C) ──
    // Amarra escrita no registro a "quem tem a senha do admin" (a signing
    // key do admin é embrulhada como K, por senha+recovery). Verificação
    // (pubkey) não exige segredo — roda antes de qualquer auth, na tela de
    // login: assimétrica, não HMAC+libsecret, porque uma HMAC-key no cofre
    // do sistema fica acessível via D-Bus a qualquer processo da MESMA
    // sessão Linux — não isolaria admin de user na mesma máquina.
    struct SignKeypair {
        QByteArray publicKey;  // 32B — crypto_sign_PUBLICKEYBYTES
        QByteArray secretKey;  // 64B — crypto_sign_SECRETKEYBYTES
    };
    SignKeypair generateSignKeypair() const;
    QByteArray sign(const QByteArray &message, const QByteArray &secretKey64) const;
    bool verify(const QByteArray &message, const QByteArray &signature64,
                const QByteArray &publicKey32) const;

    // True if the AES-NI instruction set is available on this CPU.
    static bool aesNiAvailable();

    // True if `data` starts with the new-format magic bytes.
    static bool isNewFormat(const QByteArray &data);

private:
    bool m_useAesGcm;
};
