#include "KeyDerivation.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>

namespace KeyDerivation {

QByteArray deriveKey(const QString &password, const QByteArray &salt,
                     int keySize, int iterations)
{
    QByteArray passBytes = password.toUtf8();
    QByteArray key(keySize, '\0');

    int ok = PKCS5_PBKDF2_HMAC(
        passBytes.constData(), passBytes.size(),
        reinterpret_cast<const unsigned char *>(salt.constData()), salt.size(),
        iterations,
        EVP_sha256(),
        keySize,
        reinterpret_cast<unsigned char *>(key.data())
    );

    if (!ok)
        throw std::runtime_error("PBKDF2 key derivation failed");

    return key;
}

QByteArray generateSalt(int size)
{
    QByteArray salt(size, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), size) != 1)
        throw std::runtime_error("RAND_bytes failed");
    return salt;
}

} // namespace KeyDerivation
