#pragma once

#include "core/interfaces/ICryptoService.h"

// AES-256-CBC encryption/decryption via OpenSSL.
// In QT_DEBUG builds, encrypt/decrypt are pass-through (no actual crypto)
// to avoid password friction during development.
class AesCryptoService final : public ICryptoService {
public:
    QByteArray encrypt(const QByteArray &plaintext, const QByteArray &key) override;
    QByteArray decrypt(const QByteArray &ciphertext, const QByteArray &key) override;
    QByteArray deriveKey(const QString &password, const QByteArray &salt) override;
    QByteArray generateSalt() override;
};
