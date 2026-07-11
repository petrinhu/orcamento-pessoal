// PeuBud - TDD do framing puro do .pbd v2 auto-contido (ADR-001 #212 §5.2,
// Fatia 3 — Foundation). Isolado: NÃO toca DatabaseManager nem o caminho
// vivo do banco — só o serializador/parser VaultFile (sem estado, sem I/O).

#include <QtTest/QtTest>

#include "core/crypto/SodiumCryptoService.h"
#include "core/crypto/VaultFile.h"

class TestVaultFile : public QObject
{
    Q_OBJECT

private slots:
    void roundTripCamposIdenticos();
    void roundTripUsernameUtf8();
    void roundTripInnerBlobVazio();
    void roundTripInnerBlobGrande();

    void detectReconheceV2();
    void detectReconheceV1();
    void detectRetornaUnknownParaLixo();

    void parseV2FalhaComArquivoTruncadoNoMeioDoHeader();
    void parseV2FalhaComHeaderLenMaiorQueArquivo();
    void parseV2FalhaComJsonMalformado();
    void parseV2FalhaComCampoAusente();
    void parseV2FalhaComBase64Invalido();
    void parseV2FalhaComMagicErrado();

private:
    static VaultHeaderData amostraHeader();
};

VaultHeaderData TestVaultFile::amostraHeader()
{
    SodiumCryptoService crypto;
    VaultHeaderData h;
    h.username             = QStringLiteral("petrus");
    h.saltPw                = crypto.randomBytes(16);
    h.saltRk                = crypto.randomBytes(16);
    h.kdfOps                = 3;
    h.kdfMem                = 268435456; // 256MB
    h.wrappedKeyByPass      = crypto.wrapKey(crypto.randomBytes(32), crypto.randomBytes(32));
    h.wrappedKeyByRecovery  = crypto.wrapKey(crypto.randomBytes(32), crypto.randomBytes(32));
    return h;
}

void TestVaultFile::roundTripCamposIdenticos()
{
    const VaultHeaderData hdr = amostraHeader();
    const QByteArray inner = QByteArrayLiteral("blob-v1-simulado-qualquer-coisa");

    const QByteArray framed = VaultFile::frameV2(hdr, inner);
    QVERIFY(VaultFile::isV2(framed));

    const auto parsed = VaultFile::parseV2(framed);
    QVERIFY(parsed.ok);
    QCOMPARE(parsed.header.username, hdr.username);
    QCOMPARE(parsed.header.saltPw, hdr.saltPw);
    QCOMPARE(parsed.header.saltRk, hdr.saltRk);
    QCOMPARE(parsed.header.kdfOps, hdr.kdfOps);
    QCOMPARE(static_cast<qint64>(parsed.header.kdfMem), static_cast<qint64>(hdr.kdfMem));
    QCOMPARE(parsed.header.wrappedKeyByPass, hdr.wrappedKeyByPass);
    QCOMPARE(parsed.header.wrappedKeyByRecovery, hdr.wrappedKeyByRecovery);
    QCOMPARE(parsed.innerBlob, inner);
}

void TestVaultFile::roundTripUsernameUtf8()
{
    VaultHeaderData hdr = amostraHeader();
    hdr.username = QStringLiteral("João Ção 日本語");
    const QByteArray inner = QByteArrayLiteral("x");

    const QByteArray framed = VaultFile::frameV2(hdr, inner);
    const auto parsed = VaultFile::parseV2(framed);
    QVERIFY(parsed.ok);
    QCOMPARE(parsed.header.username, hdr.username);
}

void TestVaultFile::roundTripInnerBlobVazio()
{
    const VaultHeaderData hdr = amostraHeader();
    const QByteArray inner;

    const QByteArray framed = VaultFile::frameV2(hdr, inner);
    const auto parsed = VaultFile::parseV2(framed);
    QVERIFY(parsed.ok);
    QVERIFY(parsed.innerBlob.isEmpty());
}

void TestVaultFile::roundTripInnerBlobGrande()
{
    SodiumCryptoService crypto;
    const VaultHeaderData hdr = amostraHeader();
    const QByteArray inner = crypto.randomBytes(1024 * 1024); // ~1MB aleatorio

    const QByteArray framed = VaultFile::frameV2(hdr, inner);
    const auto parsed = VaultFile::parseV2(framed);
    QVERIFY(parsed.ok);
    QCOMPARE(parsed.innerBlob.size(), inner.size());
    QCOMPARE(parsed.innerBlob, inner);
}

void TestVaultFile::detectReconheceV2()
{
    const VaultHeaderData hdr = amostraHeader();
    const QByteArray framed = VaultFile::frameV2(hdr, QByteArrayLiteral("inner"));
    QCOMPARE(VaultFile::detect(framed), VaultFile::Format::V2);
}

void TestVaultFile::detectReconheceV1()
{
    SodiumCryptoService crypto;
    const QByteArray key = crypto.randomBytes(32);
    const QByteArray v1Blob = crypto.encrypt(QByteArrayLiteral("plaintext qualquer"), key);
#ifdef QT_DEBUG
    // Em build debug, SodiumCryptoService::encrypt() é passthrough (retorna
    // o plaintext sem magic) — este teste de detecção de FRAMING precisa do
    // magic v1 real; construir o blob manualmente com o layout documentado
    // em SodiumCryptoService.h (magic + algo + nonce + cipher) não é
    // necessário: isNewFormat() só olha os 4 primeiros bytes, então
    // prefixamos o magic manualmente para simular um .pbd v1 real.
    const QByteArray magic1 = QByteArrayLiteral("PBD") + QByteArray(1, static_cast<char>(0x01));
    const QByteArray v1Real = magic1 + v1Blob;
    QCOMPARE(VaultFile::detect(v1Real), VaultFile::Format::V1);
#else
    QCOMPARE(VaultFile::detect(v1Blob), VaultFile::Format::V1);
#endif
}

void TestVaultFile::detectRetornaUnknownParaLixo()
{
    QCOMPARE(VaultFile::detect(QByteArrayLiteral("lixo-nao-e-nada-disso")), VaultFile::Format::Unknown);
    QCOMPARE(VaultFile::detect(QByteArray()), VaultFile::Format::Unknown);
}

void TestVaultFile::parseV2FalhaComArquivoTruncadoNoMeioDoHeader()
{
    const VaultHeaderData hdr = amostraHeader();
    const QByteArray framed = VaultFile::frameV2(hdr, QByteArrayLiteral("inner"));

    // Corta o arquivo bem no meio do header JSON (mantém magic+len intactos,
    // mas o buffer termina antes de header_len bytes).
    const QByteArray truncado = framed.left(framed.size() - 20);
    const auto parsed = VaultFile::parseV2(truncado);
    QVERIFY(!parsed.ok);
}

void TestVaultFile::parseV2FalhaComHeaderLenMaiorQueArquivo()
{
    const VaultHeaderData hdr = amostraHeader();
    QByteArray framed = VaultFile::frameV2(hdr, QByteArrayLiteral("inner"));

    // Forja header_len absurdamente grande (0xFFFF) sem alterar o resto —
    // headerEnd > fileBytes.size() deve barrar antes de qualquer fatiamento.
    framed[4] = static_cast<char>(0xFF);
    framed[5] = static_cast<char>(0xFF);
    const auto parsed = VaultFile::parseV2(framed);
    QVERIFY(!parsed.ok);
}

void TestVaultFile::parseV2FalhaComJsonMalformado()
{
    QByteArray fileBytes;
    fileBytes.append(QByteArrayLiteral("PBD"));
    fileBytes.append(static_cast<char>(0x02));
    const QByteArray badJson = QByteArrayLiteral("{ nao fecha");
    uchar lenBuf[2];
    qToBigEndian<quint16>(static_cast<quint16>(badJson.size()), lenBuf);
    fileBytes.append(reinterpret_cast<const char *>(lenBuf), 2);
    fileBytes.append(badJson);
    fileBytes.append(QByteArrayLiteral("inner"));

    const auto parsed = VaultFile::parseV2(fileBytes);
    QVERIFY(!parsed.ok);
}

void TestVaultFile::parseV2FalhaComCampoAusente()
{
    QByteArray fileBytes;
    fileBytes.append(QByteArrayLiteral("PBD"));
    fileBytes.append(static_cast<char>(0x02));
    // JSON válido mas SEM "wp".
    const QByteArray json = QByteArrayLiteral(
        R"({"u":"x","sp":"AAAA","sr":"AAAA","ops":3,"mem":268435456,"wr":"AAAA"})");
    uchar lenBuf[2];
    qToBigEndian<quint16>(static_cast<quint16>(json.size()), lenBuf);
    fileBytes.append(reinterpret_cast<const char *>(lenBuf), 2);
    fileBytes.append(json);
    fileBytes.append(QByteArrayLiteral("inner"));

    const auto parsed = VaultFile::parseV2(fileBytes);
    QVERIFY(!parsed.ok);
}

void TestVaultFile::parseV2FalhaComBase64Invalido()
{
    QByteArray fileBytes;
    fileBytes.append(QByteArrayLiteral("PBD"));
    fileBytes.append(static_cast<char>(0x02));
    // "sp" com caractere ilegal em base64 ('!').
    const QByteArray json = QByteArrayLiteral(
        R"({"u":"x","sp":"!!!!not-b64!!!!","sr":"AAAA","ops":3,"mem":268435456,"wp":"AAAA","wr":"AAAA"})");
    uchar lenBuf[2];
    qToBigEndian<quint16>(static_cast<quint16>(json.size()), lenBuf);
    fileBytes.append(reinterpret_cast<const char *>(lenBuf), 2);
    fileBytes.append(json);
    fileBytes.append(QByteArrayLiteral("inner"));

    const auto parsed = VaultFile::parseV2(fileBytes);
    QVERIFY(!parsed.ok);
}

void TestVaultFile::parseV2FalhaComMagicErrado()
{
    QByteArray fileBytes;
    fileBytes.append(QByteArrayLiteral("XYZ"));
    fileBytes.append(static_cast<char>(0x02));
    fileBytes.append(QByteArrayLiteral("resto-qualquer"));

    const auto parsed = VaultFile::parseV2(fileBytes);
    QVERIFY(!parsed.ok);
    QVERIFY(!VaultFile::isV2(fileBytes));
}

QTEST_GUILESS_MAIN(TestVaultFile)
#include "test_vault_file.moc"
