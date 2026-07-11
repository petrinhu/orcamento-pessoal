// PeuBud - TDD das primitivas de envelope/assinatura do ADR-001 #212
// (Fatia 1 — Foundation). Cobre os métodos NOVOS de SodiumCryptoService:
// deriveKey(4 args), wrapKey/unwrapKey (crypto_secretbox) e o par
// sign/verify (Ed25519). Não toca nos métodos pré-existentes (encrypt/
// decrypt/deriveKey(2 args)/generateSalt), já cobertos por outros testes.

#include <QtTest/QtTest>

#include "core/crypto/SodiumCryptoService.h"

class TestCryptoEnvelope : public QObject
{
    Q_OBJECT

private slots:
    void wrapUnwrapRoundTrip();
    void unwrapComChaveErradaRetornaNullopt();
    void unwrapComPayloadTruncadoRetornaNullopt();
    void deriveKeyDeterministicoMesmaSenhaMesmoSalt();
    void deriveKeySaltsDiferentesGeramChavesDiferentes();
    void signVerifyOk();
    void verifyFalhaComMensagemAdulterada();
    void verifyFalhaComPubkeyErrada();
    void verifyFalhaComAssinaturaTamanhoErrado();
};

void TestCryptoEnvelope::wrapUnwrapRoundTrip()
{
    SodiumCryptoService crypto;
    const QByteArray secret  = crypto.randomBytes(32);   // simula K
    const QByteArray wrapKey = crypto.randomBytes(32);   // simula Argon2id(senha)

    const QByteArray wrapped = crypto.wrapKey(secret, wrapKey);
    // nonce(24) + secret(32) + MAC(16) = 72 bytes, exatamente o ADR-001 §3.2.
    QCOMPARE(wrapped.size(), 24 + 32 + 16);

    const auto unwrapped = crypto.unwrapKey(wrapped, wrapKey);
    QVERIFY(unwrapped.has_value());
    QCOMPARE(*unwrapped, secret);
}

void TestCryptoEnvelope::unwrapComChaveErradaRetornaNullopt()
{
    SodiumCryptoService crypto;
    const QByteArray secret      = crypto.randomBytes(32);
    const QByteArray wrapKey     = crypto.randomBytes(32);
    const QByteArray outraChave  = crypto.randomBytes(32);

    const QByteArray wrapped = crypto.wrapKey(secret, wrapKey);
    const auto unwrapped = crypto.unwrapKey(wrapped, outraChave);
    QVERIFY(!unwrapped.has_value());
}

void TestCryptoEnvelope::unwrapComPayloadTruncadoRetornaNullopt()
{
    SodiumCryptoService crypto;
    const QByteArray wrapKey = crypto.randomBytes(32);
    // Menor que nonce(24)+MAC(16) — inválido por construção, sem chamar libsodium.
    QVERIFY(!crypto.unwrapKey(QByteArray(10, 'x'), wrapKey).has_value());
}

void TestCryptoEnvelope::deriveKeyDeterministicoMesmaSenhaMesmoSalt()
{
    SodiumCryptoService crypto;
    const QByteArray senha = QByteArrayLiteral("senha-de-teste-com-12-chars");
    const QByteArray salt  = crypto.randomBytes(16);
    // ops/mem no MÍNIMO absoluto do Argon2id (crypto_pwhash_OPSLIMIT_MIN=1,
    // MEMLIMIT_MIN=8192) — só para o teste rodar em milissegundos; produção
    // usa sempre MODERATE (ops=3, mem=256MB) via kdf_params do registro.
    const unsigned long long ops = 1;
    const size_t mem = 8192;

    const QByteArray k1 = crypto.deriveKey(senha, salt, ops, mem);
    const QByteArray k2 = crypto.deriveKey(senha, salt, ops, mem);
    QCOMPARE(k1.size(), 32);
    QCOMPARE(k1, k2);
}

void TestCryptoEnvelope::deriveKeySaltsDiferentesGeramChavesDiferentes()
{
    SodiumCryptoService crypto;
    const QByteArray senha = QByteArrayLiteral("senha-de-teste-com-12-chars");
    const QByteArray salt1 = crypto.randomBytes(16);
    const QByteArray salt2 = crypto.randomBytes(16);
    const unsigned long long ops = 1;
    const size_t mem = 8192;

    const QByteArray k1 = crypto.deriveKey(senha, salt1, ops, mem);
    const QByteArray k2 = crypto.deriveKey(senha, salt2, ops, mem);
    QVERIFY(k1 != k2);
}

void TestCryptoEnvelope::signVerifyOk()
{
    SodiumCryptoService crypto;
    const auto kp = crypto.generateSignKeypair();
    QCOMPARE(kp.publicKey.size(), 32);
    QCOMPARE(kp.secretKey.size(), 64);

    const QByteArray msg = QByteArrayLiteral("conteudo canonico de users.json");
    const QByteArray sig = crypto.sign(msg, kp.secretKey);
    QCOMPARE(sig.size(), 64);
    QVERIFY(crypto.verify(msg, sig, kp.publicKey));
}

void TestCryptoEnvelope::verifyFalhaComMensagemAdulterada()
{
    SodiumCryptoService crypto;
    const auto kp = crypto.generateSignKeypair();
    const QByteArray msg = QByteArrayLiteral("mensagem original assinada");
    const QByteArray sig = crypto.sign(msg, kp.secretKey);

    QByteArray msgAdulterada = msg;
    msgAdulterada[0] = static_cast<char>(msgAdulterada[0] ^ 0x01);
    QVERIFY(!crypto.verify(msgAdulterada, sig, kp.publicKey));
}

void TestCryptoEnvelope::verifyFalhaComPubkeyErrada()
{
    SodiumCryptoService crypto;
    const auto kpDono   = crypto.generateSignKeypair();
    const auto kpOutro  = crypto.generateSignKeypair();
    const QByteArray msg = QByteArrayLiteral("mensagem qualquer");
    const QByteArray sig = crypto.sign(msg, kpDono.secretKey);

    QVERIFY(!crypto.verify(msg, sig, kpOutro.publicKey));
}

void TestCryptoEnvelope::verifyFalhaComAssinaturaTamanhoErrado()
{
    SodiumCryptoService crypto;
    const auto kp = crypto.generateSignKeypair();
    const QByteArray msg = QByteArrayLiteral("mensagem");
    QVERIFY(!crypto.verify(msg, QByteArray(10, 'z'), kp.publicKey));
}

QTEST_GUILESS_MAIN(TestCryptoEnvelope)
#include "test_crypto_envelope.moc"
