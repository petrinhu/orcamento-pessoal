#pragma once

#include "core/interfaces/ICryptoService.h"

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

    // True if the AES-NI instruction set is available on this CPU.
    static bool aesNiAvailable();

    // True if `data` starts with the new-format magic bytes.
    static bool isNewFormat(const QByteArray &data);

private:
    bool m_useAesGcm;
};
