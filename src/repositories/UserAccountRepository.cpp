#include "UserAccountRepository.h"

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QSaveFile>

#include <sodium.h>   // crypto_sign_PUBLICKEYBYTES/BYTES
#include <sys/stat.h> // chmod — restringe o arquivo a 0600 (contém wraps de K)
#include <unistd.h>   // fsync

namespace {

QString roleToString(Role r)
{
    return r == Role::Admin ? QStringLiteral("admin") : QStringLiteral("user");
}

std::optional<Role> roleFromString(const QString &s)
{
    if (s == QStringLiteral("admin")) return Role::Admin;
    if (s == QStringLiteral("user"))  return Role::User;
    return std::nullopt;
}

QJsonObject userToJson(const UserAccountRecord &u)
{
    QJsonObject kdf;
    kdf[QStringLiteral("salt_pw")] = QString::fromLatin1(u.saltPw.toBase64());
    kdf[QStringLiteral("salt_rk")] = QString::fromLatin1(u.saltRk.toBase64());
    kdf[QStringLiteral("ops")]     = static_cast<qint64>(u.kdfOps);
    kdf[QStringLiteral("mem")]     = static_cast<qint64>(u.kdfMem);

    QJsonObject o;
    o[QStringLiteral("username")]                = u.username;
    o[QStringLiteral("role")]                    = roleToString(u.role);
    o[QStringLiteral("kdf_params")]               = kdf;
    o[QStringLiteral("wrapped_key_by_pass")]      = QString::fromLatin1(u.wrappedKeyByPass.toBase64());
    o[QStringLiteral("wrapped_key_by_recovery")]  = QString::fromLatin1(u.wrappedKeyByRecovery.toBase64());
    o[QStringLiteral("invite_pending")]           = u.invitePending;
    return o;
}

// nullopt = registro malformado (campo ausente/tipo errado) — o chamador
// trata como adulteração (fail-secure), não como crash.
std::optional<UserAccountRecord> userFromJson(const QJsonValue &v)
{
    if (!v.isObject()) return std::nullopt;
    const QJsonObject o = v.toObject();

    if (!o.value(QStringLiteral("username")).isString())
        return std::nullopt;

    const auto roleOpt = roleFromString(o.value(QStringLiteral("role")).toString());
    if (!roleOpt) return std::nullopt;

    if (!o.value(QStringLiteral("kdf_params")).isObject())
        return std::nullopt;
    const QJsonObject kdf = o.value(QStringLiteral("kdf_params")).toObject();

    UserAccountRecord u;
    u.username             = o.value(QStringLiteral("username")).toString();
    u.role                 = *roleOpt;
    u.saltPw               = QByteArray::fromBase64(kdf.value(QStringLiteral("salt_pw")).toString().toLatin1());
    u.saltRk                = QByteArray::fromBase64(kdf.value(QStringLiteral("salt_rk")).toString().toLatin1());
    u.kdfOps                = kdf.value(QStringLiteral("ops")).toInt();
    u.kdfMem                = static_cast<size_t>(kdf.value(QStringLiteral("mem")).toDouble());
    u.wrappedKeyByPass       = QByteArray::fromBase64(o.value(QStringLiteral("wrapped_key_by_pass")).toString().toLatin1());
    u.wrappedKeyByRecovery   = QByteArray::fromBase64(o.value(QStringLiteral("wrapped_key_by_recovery")).toString().toLatin1());
    u.invitePending          = o.value(QStringLiteral("invite_pending")).toBool();
    return u;
}

// Conteúdo canônico assinado: {admin_pubkey, users, version} — EXCLUI
// "signature" (a própria coisa que ele autentica). QJsonDocument::toJson
// (Compact), no backend QCborMap do Qt6, serializa as chaves de QJsonObject
// em ordem ALFABÉTICA — confirmado por teste ad-hoc (não é insertion-order).
// Logo, montar o QJsonObject e chamar toJson(Compact) já produz a forma
// canônica exigida pelo ADR-001 §5.1 ("chaves ordenadas e sem espaços") sem
// nenhuma ordenação manual adicional.
QByteArray canonicalContent(int version, const QByteArray &adminPubKey,
                            const QVector<UserAccountRecord> &users)
{
    QJsonArray arr;
    for (const auto &u : users)
        arr.append(userToJson(u));

    QJsonObject top;
    top[QStringLiteral("version")]      = version;
    top[QStringLiteral("admin_pubkey")] = QString::fromLatin1(adminPubKey.toBase64());
    top[QStringLiteral("users")]        = arr;

    return QJsonDocument(top).toJson(QJsonDocument::Compact);
}

} // namespace

UserAccountRepository::UserAccountRepository(SodiumCryptoService &crypto, QString filePath)
    : m_crypto(crypto)
    , m_filePath(std::move(filePath))
{
}

bool UserAccountRepository::exists() const
{
    return QFile::exists(m_filePath);
}

bool UserAccountRepository::isEmpty() const
{
    if (!exists())
        return true;
    const LoadResult r = loadAll();
    return !r.ok || r.users.isEmpty();
}

UserAccountRepository::LoadResult UserAccountRepository::loadAll() const
{
    LoadResult result;
    m_lastLoadWasTampered = false;

    QFile f(m_filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        // Arquivo ausente/inacessível — instalação nova, NÃO é adulteração.
        return result;
    }
    const QByteArray raw = f.readAll();
    f.close();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "UserAccountRepository::loadAll: JSON malformado em" << m_filePath
                   << "—" << parseErr.errorString();
        result.tampered = true;
        m_lastLoadWasTampered = true;
        return result;
    }

    const QJsonObject top = doc.object();
    const int version             = top.value(QStringLiteral("version")).toInt(-1);
    const QByteArray adminPubKey  = QByteArray::fromBase64(top.value(QStringLiteral("admin_pubkey")).toString().toLatin1());
    const QByteArray signature    = QByteArray::fromBase64(top.value(QStringLiteral("signature")).toString().toLatin1());
    const QJsonArray usersJson    = top.value(QStringLiteral("users")).toArray();

    QVector<UserAccountRecord> users;
    users.reserve(usersJson.size());
    for (const QJsonValue &v : usersJson) {
        const auto u = userFromJson(v);
        if (!u) {
            qWarning() << "UserAccountRepository::loadAll: registro de usuário malformado em" << m_filePath;
            result.tampered = true;
            m_lastLoadWasTampered = true;
            return result;
        }
        users.append(*u);
    }

    if (adminPubKey.size() != crypto_sign_PUBLICKEYBYTES || signature.size() != crypto_sign_BYTES) {
        qWarning() << "UserAccountRepository::loadAll: admin_pubkey/signature com tamanho inválido em" << m_filePath;
        result.tampered = true;
        m_lastLoadWasTampered = true;
        return result;
    }

    const QByteArray canonical = canonicalContent(version, adminPubKey, users);
    if (!m_crypto.verify(canonical, signature, adminPubKey)) {
        // Fail-secure (ADR-001 §4): assinatura não bate → bloqueia, sem
        // devolver dado parcial nem tentar "consertar" o arquivo sozinho.
        qWarning() << "UserAccountRepository::loadAll: assinatura Ed25519 inválida em" << m_filePath
                   << "— registro corrompido ou adulterado";
        result.tampered = true;
        m_lastLoadWasTampered = true;
        return result;
    }

    result.ok = true;
    result.tampered = false;
    result.adminPubKey = adminPubKey;
    result.users = users;
    return result;
}

bool UserAccountRepository::saveAll(const QVector<UserAccountRecord> &users,
                                    const QByteArray &adminPubKey,
                                    const QByteArray &adminSignSecretKey)
{
    constexpr int kVersion = 1;

    if (adminPubKey.size() != crypto_sign_PUBLICKEYBYTES
        || adminSignSecretKey.size() != crypto_sign_SECRETKEYBYTES) {
        qWarning() << "UserAccountRepository::saveAll: admin_pubkey/secretKey com tamanho inválido — abortando";
        return false;
    }

    const QByteArray canonical = canonicalContent(kVersion, adminPubKey, users);
    const QByteArray signature = m_crypto.sign(canonical, adminSignSecretKey);

    QJsonArray arr;
    for (const auto &u : users)
        arr.append(userToJson(u));

    QJsonObject top;
    top[QStringLiteral("version")]      = kVersion;
    top[QStringLiteral("admin_pubkey")] = QString::fromLatin1(adminPubKey.toBase64());
    top[QStringLiteral("signature")]    = QString::fromLatin1(signature.toBase64());
    top[QStringLiteral("users")]        = arr;

    const QByteArray out = QJsonDocument(top).toJson(QJsonDocument::Indented);

    // Escrita atômica: QSaveFile grava num arquivo temporário e só faz
    // rename() para o destino final dentro de commit() — se o processo
    // morrer no meio do write, o users.json anterior permanece intacto.
    QSaveFile saveFile(m_filePath);
    saveFile.setDirectWriteFallback(false);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning() << "UserAccountRepository::saveAll: falha ao abrir" << m_filePath
                   << "para escrita —" << saveFile.errorString();
        return false;
    }
    if (saveFile.write(out) != out.size()) {
        qWarning() << "UserAccountRepository::saveAll: write incompleto em" << m_filePath;
        saveFile.cancelWriting();
        return false;
    }
    saveFile.flush();
    ::fsync(saveFile.handle());  // durabilidade — sobrevive a crash antes do rename atômico
    if (!saveFile.commit()) {
        qWarning() << "UserAccountRepository::saveAll: commit() falhou em" << m_filePath
                   << "—" << saveFile.errorString();
        return false;
    }

    // 0600 — arquivo contém wraps de K (mesmo protegidos por Argon2id+AEAD,
    // defesa em profundidade: só o dono do processo deve conseguir ler).
    ::chmod(QFile::encodeName(m_filePath).constData(), S_IRUSR | S_IWUSR);
    return true;
}

bool UserAccountRepository::lastLoadWasTampered() const
{
    return m_lastLoadWasTampered;
}
