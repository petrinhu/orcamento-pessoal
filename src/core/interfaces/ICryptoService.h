#pragma once

#include <QByteArray>
#include <QString>

class ICryptoService {
public:
    virtual ~ICryptoService() = default;

    virtual QByteArray encrypt(const QByteArray &plaintext, const QByteArray &key) = 0;
    virtual QByteArray decrypt(const QByteArray &ciphertext, const QByteArray &key) = 0;
    virtual QByteArray deriveKey(const QString &password, const QByteArray &salt) = 0;
    virtual QByteArray generateSalt() = 0;
};
