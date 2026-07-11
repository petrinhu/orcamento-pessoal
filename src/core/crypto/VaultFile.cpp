#include "VaultFile.h"

#include "core/crypto/SodiumCryptoService.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QtEndian>

#include <cstring>
#include <optional>

namespace {

constexpr unsigned char MAGIC_V2[4] = {'P', 'B', 'D', 0x02};
constexpr qsizetype MAGIC_SIZE = 4;
constexpr qsizetype LEN_SIZE   = 2;   // uint16 BE

QString toB64(const QByteArray &b)
{
    return QString::fromLatin1(b.toBase64());
}

// Decodifica base64 ESTRITO — string vazia decodifica para QByteArray()
// vazio (válido); caracteres/padding inválidos → nullopt. Diferencia-se de
// QByteArray::fromBase64() "solto" (que ignora lixo em silêncio) — aqui um
// header adulterado com base64 quebrado precisa ser detectável.
std::optional<QByteArray> decodeB64Strict(const QString &s)
{
    const auto r = QByteArray::fromBase64Encoding(s.toLatin1(), QByteArray::AbortOnBase64DecodingErrors);
    if (!r)
        return std::nullopt;
    return r.decoded;
}

} // namespace

QByteArray VaultFile::frameV2(const VaultHeaderData &hdr, const QByteArray &innerBlobV1)
{
    QJsonObject o;
    o[QStringLiteral("u")]   = hdr.username;
    o[QStringLiteral("sp")]  = toB64(hdr.saltPw);
    o[QStringLiteral("sr")]  = toB64(hdr.saltRk);
    o[QStringLiteral("ops")] = hdr.kdfOps;
    // size_t não cabe em QJsonValue (int/double) sem perda acima de 2^53 —
    // mem = 256MB (kdfMem real) fica MUITO abaixo disso; double representa
    // exatamente. Mesma convenção do users.json (UserAccountRepository).
    o[QStringLiteral("mem")] = static_cast<double>(hdr.kdfMem);
    o[QStringLiteral("wp")]  = toB64(hdr.wrappedKeyByPass);
    o[QStringLiteral("wr")]  = toB64(hdr.wrappedKeyByRecovery);

    const QByteArray headerJson = QJsonDocument(o).toJson(QJsonDocument::Compact);

    // header_len é uint16 (máx 65535B). O header real (username + 2 salts +
    // 2 wraps em base64) fica na casa de 300-400B — este método só é
    // chamado com dado que o próprio processo gerou (não é caminho
    // adversarial). Clampar + avisar em vez de assert/exceção (CONTRACT
    // §6.4): nunca crasha, mas um header absurdamente grande ficaria
    // truncado/ilegível no parseV2 seguinte — bug de chamador, não de I/O.
    qsizetype size = headerJson.size();
    if (size > 0xFFFF) {
        qWarning("VaultFile::frameV2: header JSON de %lld bytes excede uint16 — truncando (BUG do chamador)",
                 static_cast<long long>(size));
        size = 0xFFFF;
    }
    const quint16 headerLen = static_cast<quint16>(size);

    uchar lenBuf[LEN_SIZE];
    qToBigEndian<quint16>(headerLen, lenBuf);

    QByteArray result;
    result.reserve(MAGIC_SIZE + LEN_SIZE + headerLen + innerBlobV1.size());
    result.append(reinterpret_cast<const char *>(MAGIC_V2), static_cast<int>(MAGIC_SIZE));
    result.append(reinterpret_cast<const char *>(lenBuf), static_cast<int>(LEN_SIZE));
    result.append(headerJson.left(headerLen));
    result.append(innerBlobV1);
    return result;
}

bool VaultFile::isV2(const QByteArray &fileBytes)
{
    if (fileBytes.size() < MAGIC_SIZE)
        return false;
    return std::memcmp(fileBytes.constData(), MAGIC_V2, static_cast<size_t>(MAGIC_SIZE)) == 0;
}

VaultFile::Format VaultFile::detect(const QByteArray &fileBytes)
{
    if (isV2(fileBytes))
        return Format::V2;
    if (SodiumCryptoService::isNewFormat(fileBytes))
        return Format::V1;
    return Format::Unknown;
}

VaultFile::ParseResult VaultFile::parseV2(const QByteArray &fileBytes)
{
    ParseResult result;

    if (!isV2(fileBytes))
        return result;
    if (fileBytes.size() < MAGIC_SIZE + LEN_SIZE)
        return result;   // truncado antes mesmo do header_len

    const quint16 headerLen = qFromBigEndian<quint16>(fileBytes.constData() + MAGIC_SIZE);

    const qsizetype headerStart = MAGIC_SIZE + LEN_SIZE;
    const qsizetype headerEnd   = headerStart + static_cast<qsizetype>(headerLen);
    // Núcleo da validação anti out-of-bounds: header_len forjado (maior que
    // o arquivo) OU arquivo truncado no meio do header — mesma checagem
    // cobre os dois casos, sem nunca fatiar além do buffer real.
    if (headerEnd > fileBytes.size())
        return result;

    const QByteArray headerJson = fileBytes.mid(headerStart, headerLen);

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(headerJson, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject())
        return result;

    const QJsonObject o = doc.object();

    if (!o.value(QStringLiteral("u")).isString())   return result;
    if (!o.value(QStringLiteral("sp")).isString())  return result;
    if (!o.value(QStringLiteral("sr")).isString())  return result;
    if (!o.value(QStringLiteral("ops")).isDouble()) return result;
    if (!o.value(QStringLiteral("mem")).isDouble()) return result;
    if (!o.value(QStringLiteral("wp")).isString())  return result;
    if (!o.value(QStringLiteral("wr")).isString())  return result;

    const auto sp = decodeB64Strict(o.value(QStringLiteral("sp")).toString());
    const auto sr = decodeB64Strict(o.value(QStringLiteral("sr")).toString());
    const auto wp = decodeB64Strict(o.value(QStringLiteral("wp")).toString());
    const auto wr = decodeB64Strict(o.value(QStringLiteral("wr")).toString());
    if (!sp || !sr || !wp || !wr)
        return result;

    VaultHeaderData hdr;
    hdr.username             = o.value(QStringLiteral("u")).toString();
    hdr.saltPw               = *sp;
    hdr.saltRk               = *sr;
    hdr.kdfOps               = o.value(QStringLiteral("ops")).toInt();
    hdr.kdfMem               = static_cast<size_t>(o.value(QStringLiteral("mem")).toDouble());
    hdr.wrappedKeyByPass     = *wp;
    hdr.wrappedKeyByRecovery = *wr;

    result.ok = true;
    result.header = hdr;
    result.innerBlob = fileBytes.mid(headerEnd);
    return result;
}
