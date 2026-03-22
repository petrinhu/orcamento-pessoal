#include "AesCryptoService.h"
#include "KeyDerivation.h"

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdexcept>

// Layout of ciphertext on disk:
//   [16 bytes IV][N bytes ciphertext]

static constexpr int IV_SIZE = 16;

QByteArray AesCryptoService::encrypt(const QByteArray &plaintext, const QByteArray &key)
{
#ifdef QT_DEBUG
    return plaintext; // no-op in debug
#endif

    QByteArray iv(IV_SIZE, '\0');
    if (RAND_bytes(reinterpret_cast<unsigned char *>(iv.data()), IV_SIZE) != 1)
        throw std::runtime_error("IV generation failed");

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    QByteArray ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH, '\0');
    int len1 = 0, len2 = 0;

    EVP_EncryptInit_ex(ctx,
        EVP_aes_256_cbc(), nullptr,
        reinterpret_cast<const unsigned char *>(key.constData()),
        reinterpret_cast<const unsigned char *>(iv.constData()));

    EVP_EncryptUpdate(ctx,
        reinterpret_cast<unsigned char *>(ciphertext.data()), &len1,
        reinterpret_cast<const unsigned char *>(plaintext.constData()), plaintext.size());

    EVP_EncryptFinal_ex(ctx,
        reinterpret_cast<unsigned char *>(ciphertext.data()) + len1, &len2);

    EVP_CIPHER_CTX_free(ctx);
    ciphertext.resize(len1 + len2);

    return iv + ciphertext;
}

QByteArray AesCryptoService::decrypt(const QByteArray &data, const QByteArray &key)
{
#ifdef QT_DEBUG
    return data; // no-op in debug
#endif

    if (data.size() < IV_SIZE)
        throw std::runtime_error("Ciphertext too short");

    QByteArray iv         = data.left(IV_SIZE);
    QByteArray ciphertext = data.mid(IV_SIZE);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) throw std::runtime_error("EVP_CIPHER_CTX_new failed");

    QByteArray plaintext(ciphertext.size() + EVP_MAX_BLOCK_LENGTH, '\0');
    int len1 = 0, len2 = 0;

    EVP_DecryptInit_ex(ctx,
        EVP_aes_256_cbc(), nullptr,
        reinterpret_cast<const unsigned char *>(key.constData()),
        reinterpret_cast<const unsigned char *>(iv.constData()));

    EVP_DecryptUpdate(ctx,
        reinterpret_cast<unsigned char *>(plaintext.data()), &len1,
        reinterpret_cast<const unsigned char *>(ciphertext.constData()), ciphertext.size());

    if (EVP_DecryptFinal_ex(ctx,
            reinterpret_cast<unsigned char *>(plaintext.data()) + len1, &len2) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES decrypt failed — wrong key or corrupt data");
    }

    EVP_CIPHER_CTX_free(ctx);
    plaintext.resize(len1 + len2);

    return plaintext;
}

QByteArray AesCryptoService::deriveKey(const QString &password, const QByteArray &salt)
{
    return KeyDerivation::deriveKey(password, salt);
}

QByteArray AesCryptoService::generateSalt()
{
    return KeyDerivation::generateSalt();
}
