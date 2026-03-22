#pragma once

#include <QByteArray>
#include <QString>

// PBKDF2-SHA256 key derivation — isolated per SRP
namespace KeyDerivation {

constexpr int KEY_SIZE   = 32; // 256 bits
constexpr int SALT_SIZE  = 16; // 128 bits
constexpr int ITERATIONS = 600000;

QByteArray deriveKey(const QString &password, const QByteArray &salt,
                     int keySize = KEY_SIZE, int iterations = ITERATIONS);
QByteArray generateSalt(int size = SALT_SIZE);

} // namespace KeyDerivation
