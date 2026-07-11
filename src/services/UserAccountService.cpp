#include "UserAccountService.h"

#include "core/settings/AppSettings.h"

#include <QFile>
#include <QRegularExpression>

#include <sodium.h>

namespace {

// ── RAII: zera um QByteArray sensível ao sair de escopo ────────────────────
// .data() (não-const) força detach COW antes de zerar — se o buffer for
// compartilhado (ex.: cópia de UserAccountService::m_adminSignKey), só a
// CÓPIA local é apagada, nunca o original (mesma técnica de
// UserSession::logout()).
struct ScopedWipe {
    QByteArray &buf;
    explicit ScopedWipe(QByteArray &b) : buf(b) {}
    ~ScopedWipe()
    {
        if (!buf.isEmpty())
            sodium_memzero(buf.data(), static_cast<size_t>(buf.size()));
    }
};

constexpr unsigned long long kModerateOps = crypto_pwhash_OPSLIMIT_MODERATE;
constexpr size_t             kModerateMem = crypto_pwhash_MEMLIMIT_MODERATE;

constexpr int kMinPasswordLen = 12; // ADR-001 §2 (líder confirmou ≥12 em 2026-07-11)

// ── Crockford Base32 (exclui I/L/O/U) ───────────────────────────────────────
constexpr const char *kCrockfordAlphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

int crockfordValue(QChar c)
{
    const QChar upper = c.toUpper();
    for (int i = 0; i < 32; ++i) {
        if (QLatin1Char(kCrockfordAlphabet[i]) == upper)
            return i;
    }
    return -1;
}

int findUserIndex(const QVector<UserAccountRecord> &users, const QString &username)
{
    for (int i = 0; i < users.size(); ++i) {
        if (users[i].username.compare(username, Qt::CaseSensitive) == 0)
            return i;
    }
    return -1;
}

} // namespace

UserAccountService::UserAccountService(UserAccountRepository &repo, SodiumCryptoService &crypto,
                                        SecretStoreService &secrets)
    : m_repo(repo)
    , m_crypto(crypto)
    , m_secrets(secrets)
{
}

UserAccountService::~UserAccountService()
{
    clearSession();
}

void UserAccountService::clearSession()
{
    if (!m_adminSignKey.isEmpty())
        sodium_memzero(m_adminSignKey.data(), static_cast<size_t>(m_adminSignKey.size()));
    m_adminSignKey.clear();
}

// ── Recovery/invite key encoding ────────────────────────────────────────────

QString UserAccountService::encodeRecoveryKey(const QByteArray &raw)
{
    if (raw.size() != kRecoveryKeyBytes)
        return QString();

    // 160 bits -> 32 grupos de 5 bits, MSB primeiro, sem padding (20*8=160=32*5 exato).
    int symbols[32];
    for (int group = 0; group < 32; ++group) {
        int value = 0;
        for (int i = 0; i < 5; ++i) {
            const int bit          = group * 5 + i;
            const int byteIndex    = bit / 8;
            const int bitInByte    = 7 - (bit % 8);
            const int bitVal       = (static_cast<unsigned char>(raw[byteIndex]) >> bitInByte) & 1;
            value = (value << 1) | bitVal;
        }
        symbols[group] = value;
    }

    int checksum = 0;
    for (int s : symbols) checksum += s;
    checksum %= 32;

    QString out;
    out.reserve(32 + 8 + 2); // 32 símbolos + 7 hífens de grupo + 1 hífen do checksum + 1 símbolo
    for (int i = 0; i < 32; ++i) {
        if (i > 0 && i % 4 == 0)
            out += QLatin1Char('-');
        out += QChar(QLatin1Char(kCrockfordAlphabet[symbols[i]])).toLower();
    }
    out += QLatin1Char('-');
    out += QChar(QLatin1Char(kCrockfordAlphabet[checksum])).toLower();
    return out;
}

std::optional<QByteArray> UserAccountService::decodeRecoveryKey(const QString &formatted)
{
    QString cleaned;
    cleaned.reserve(formatted.size());
    for (const QChar &c : formatted) {
        if (c == QLatin1Char('-') || c.isSpace())
            continue;
        cleaned += c;
    }
    if (cleaned.size() != 33) // 32 símbolos de dado + 1 checksum
        return std::nullopt;

    int symbols[33];
    for (int i = 0; i < 33; ++i) {
        const int v = crockfordValue(cleaned.at(i));
        if (v < 0)
            return std::nullopt; // caractere fora do alfabeto Crockford
        symbols[i] = v;
    }

    int checksum = 0;
    for (int i = 0; i < 32; ++i) checksum += symbols[i];
    checksum %= 32;
    if (checksum != symbols[32])
        return std::nullopt; // checksum não bate — erro de transcrição

    QByteArray raw(kRecoveryKeyBytes, '\0');
    for (int group = 0; group < 32; ++group) {
        const int value = symbols[group];
        for (int i = 0; i < 5; ++i) {
            const int bit       = group * 5 + i;
            const int byteIndex = bit / 8;
            const int bitInByte = 7 - (bit % 8);
            const int bitVal    = (value >> (4 - i)) & 1;
            if (bitVal)
                raw[byteIndex] = static_cast<char>(static_cast<unsigned char>(raw[byteIndex]) | (1 << bitInByte));
        }
    }
    return raw;
}

// ── Slug ─────────────────────────────────────────────────────────────────

QString UserAccountService::slugify(const QString &username)
{
    // NFD decompõe "ã" em 'a' + combining tilde; removemos as marcas de
    // combinação (categoria Mark_NonSpacing) ANTES do filtro [a-z0-9_], daí
    // "João" -> "joao" (não "joo", que seria o resultado de só remover
    // tudo fora de a-z0-9_ sem decompor — comportamento atual de
    // UserSession::slug(), que este método deliberadamente NÃO reusa por
    // não ser uma função livre e por essa fatia não tocar UserSession.*).
    const QString decomposed = username.normalized(QString::NormalizationForm_D);
    QString semAcento;
    semAcento.reserve(decomposed.size());
    for (const QChar &c : decomposed) {
        if (c.category() != QChar::Mark_NonSpacing)
            semAcento += c;
    }
    return semAcento.toLower()
                     .replace(QLatin1Char(' '), QLatin1Char('_'))
                     .remove(QRegularExpression(QStringLiteral("[^a-z0-9_]")));
}

// ── Consultas ────────────────────────────────────────────────────────────

bool UserAccountService::isFirstRun() const
{
    return m_repo.isEmpty();
}

bool UserAccountService::usernameExists(const QString &username) const
{
    const auto loaded = m_repo.loadAll();
    if (!loaded.ok)
        return false; // registro tampered/ausente — não é "existe"; verifyLogin cobre integridade

    const QString targetSlug = slugify(username);
    for (const auto &u : loaded.users) {
        if (u.username.compare(username, Qt::CaseSensitive) == 0)
            return true; // igualdade exata
        if (slugify(u.username) == targetSlug)
            return true; // colisão de slug normalizado (ex.: "João" vs "joao")
    }
    return false;
}

QVector<UserPublicInfo> UserAccountService::listUsers() const
{
    QVector<UserPublicInfo> out;
    const auto loaded = m_repo.loadAll();
    if (!loaded.ok)
        return out; // fail-secure — lista vazia; caller usa verifyLogin para diagnosticar tampered

    out.reserve(loaded.users.size());
    for (const auto &u : loaded.users) {
        UserPublicInfo info;
        info.username = u.username;
        info.role     = u.role;
        // (D-B) qualquer conta pode ter cofre próprio — hasVault reflete se
        // o .pbd já existe no caminho derivado do slug (não abre/decifra
        // nada aqui, só QFile::exists — sem tocar DatabaseManager).
        info.hasVault = QFile::exists(AppSettings::savePathFixo(slugify(u.username)));
        out.append(info);
    }
    return out;
}

bool UserAccountService::pubKeyMatchesAnchor(const QByteArray &filePubKey) const
{
    const QByteArray anchor = m_secrets.retrieve(QString::fromLatin1(kAdminPubKeyAnchorKey));
    return !anchor.isEmpty() && anchor == filePubKey;
}

// ── Bootstrap do admin (1º run) ─────────────────────────────────────────────

UserAccountService::CreateAdminResult UserAccountService::createAdmin(const QString &username,
                                                                       const QString &password)
{
    CreateAdminResult res;

    if (username.trimmed().isEmpty()) {
        res.error = QStringLiteral("nome de usuário vazio");
        return res;
    }
    if (!isFirstRun()) {
        res.error = QStringLiteral("já existe um registro de usuários — não é o primeiro run");
        return res;
    }
    if (password.size() < kMinPasswordLen) {
        res.error = QStringLiteral("senha deve ter ao menos %1 caracteres").arg(kMinPasswordLen);
        return res;
    }

    UserAccountRecord admin;
    admin.username = username;
    admin.role     = Role::Admin;
    admin.saltPw   = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    admin.saltRk   = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    admin.kdfOps   = static_cast<int>(kModerateOps);
    admin.kdfMem   = kModerateMem;
    admin.invitePending = false;

    QByteArray K = m_crypto.randomBytes(kKeyBytes);
    ScopedWipe wipeK(K);

    QByteArray passBytes = password.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, admin.saltPw, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfPass(kdfPass);

    const QByteArray recoveryKey = m_crypto.randomBytes(kRecoveryKeyBytes); // vai pro caller — não wipar aqui
    QByteArray kdfRecovery = m_crypto.deriveKey(recoveryKey, admin.saltRk, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfRecovery(kdfRecovery);

    admin.wrappedKeyByPass     = m_crypto.wrapKey(K, kdfPass);
    admin.wrappedKeyByRecovery = m_crypto.wrapKey(K, kdfRecovery);

    SodiumCryptoService::SignKeypair signKp = m_crypto.generateSignKeypair();
    ScopedWipe wipeSignSecret(signKp.secretKey);

    admin.wrappedSignKeyByPass     = m_crypto.wrapKey(signKp.secretKey, kdfPass);
    admin.wrappedSignKeyByRecovery = m_crypto.wrapKey(signKp.secretKey, kdfRecovery);

    // (item E) grava a âncora ANTES do saveAll: se o processo morrer entre
    // os dois passos, o pior caso é users.json inexistente + âncora órfã no
    // keychain (isFirstRun() continua true, próxima tentativa de
    // createAdmin sobrescreve a âncora — sem risco de segurança, só
    // resíduo). O inverso (users.json com admin mas sem âncora) seria pior
    // — abriria a janela de pubkey-swap que este item existe pra fechar.
    if (!m_secrets.store(QString::fromLatin1(kAdminPubKeyAnchorKey), signKp.publicKey)) {
        res.error = QStringLiteral("falha ao gravar a âncora do admin no keychain do sistema");
        return res;
    }

    QVector<UserAccountRecord> users{admin};
    if (!m_repo.saveAll(users, signKp.publicKey, signKp.secretKey)) {
        res.error = QStringLiteral("falha ao gravar users.json");
        return res;
    }

    res.ok = true;
    res.recoveryKeyPlaintext = recoveryKey;
    return res;
}

// ── Criação de usuário via invite key (admin logado) ───────────────────────

UserAccountService::CreateUserResult UserAccountService::createUser(const QString &username, Role role)
{
    CreateUserResult res;

    if (m_adminSignKey.isEmpty()) {
        res.error = QStringLiteral("operação exige admin logado");
        return res;
    }
    if (username.trimmed().isEmpty()) {
        res.error = QStringLiteral("nome de usuário vazio");
        return res;
    }
    if (usernameExists(username)) {
        res.error = QStringLiteral("nome de usuário já existe (ou colide por slug)");
        return res;
    }

    const auto loaded = m_repo.loadAll();
    if (!loaded.ok) {
        res.error = QStringLiteral("registro de usuários corrompido/adulterado — não é possível criar usuário");
        return res;
    }
    if (!pubKeyMatchesAnchor(loaded.adminPubKey)) {
        res.error = QStringLiteral("admin_pubkey do arquivo não confere com a âncora do keychain — possível adulteração");
        return res;
    }

    UserAccountRecord novo;
    novo.username = username;
    novo.role     = role;
    novo.kdfOps   = static_cast<int>(kModerateOps);
    novo.kdfMem   = kModerateMem;
    // saltPw/wrappedKeyByPass ficam vazios — só existem após completeFirstLogin.
    novo.saltRk   = m_crypto.randomBytes(crypto_pwhash_SALTBYTES); // salt do invite key (reaproveita o slot "_rk")
    novo.invitePending = true;

    QByteArray K = m_crypto.randomBytes(kKeyBytes);
    ScopedWipe wipeK(K);

    const QByteArray inviteKey = m_crypto.randomBytes(kRecoveryKeyBytes); // vai pro caller — não wipar
    QByteArray kdfInvite = m_crypto.deriveKey(inviteKey, novo.saltRk, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfInvite(kdfInvite);

    // (ADR-001 §3.5) K embrulhada SÓ por KDF(invite_key) neste momento —
    // reaproveita o slot wrappedKeyByRecovery até completeFirstLogin trocar
    // por um wrap de recovery key definitivo.
    novo.wrappedKeyByRecovery = m_crypto.wrapKey(K, kdfInvite);

    QVector<UserAccountRecord> users = loaded.users;
    users.append(novo);
    if (!m_repo.saveAll(users, loaded.adminPubKey, m_adminSignKey)) {
        res.error = QStringLiteral("falha ao gravar users.json");
        return res;
    }

    res.ok = true;
    res.inviteKeyPlaintext = inviteKey;
    return res;
}

// ── 1º login do usuário convidado (define a própria senha) ─────────────────

UserAccountService::FirstLoginResult UserAccountService::completeFirstLogin(
    const QString &username, const QByteArray &inviteKey, const QString &newPassword)
{
    FirstLoginResult res;

    if (newPassword.size() < kMinPasswordLen) {
        res.error = QStringLiteral("senha deve ter ao menos %1 caracteres").arg(kMinPasswordLen);
        return res;
    }

    const auto loaded = m_repo.loadAll();
    if (!loaded.ok) {
        res.error = QStringLiteral("registro de usuários corrompido/adulterado");
        return res;
    }
    if (!pubKeyMatchesAnchor(loaded.adminPubKey)) {
        res.error = QStringLiteral("admin_pubkey do arquivo não confere com a âncora do keychain — possível adulteração");
        return res;
    }

    // Ver nota de design no topo de UserAccountService.h: persistir exige
    // resignar o registro central, e só a signing key do admin produz uma
    // assinatura válida. Limitação temporária desta fatia — NÃO expõe a
    // senha do novo usuário ao admin (ela nunca passa pelo código dele),
    // só a assinatura de metadados depende de m_adminSignKey estar
    // disponível nesta instância do Service.
    if (m_adminSignKey.isEmpty()) {
        res.error = QStringLiteral(
            "não foi possível concluir: persistência exige uma sessão com "
            "assinatura disponível nesta instância (ADR-001 §8 passo 6 — "
            ".pbd v2 auto-contido — fecha essa dependência numa fatia futura)");
        return res;
    }

    const int idx = findUserIndex(loaded.users, username);
    if (idx < 0) {
        res.error = QStringLiteral("usuário não encontrado");
        return res;
    }

    UserAccountRecord u = loaded.users[idx];
    if (!u.invitePending) {
        res.error = QStringLiteral("usuário não está com convite pendente");
        return res;
    }

    QByteArray kdfInvite = m_crypto.deriveKey(inviteKey, u.saltRk,
                                              static_cast<unsigned long long>(u.kdfOps), u.kdfMem);
    ScopedWipe wipeKdfInvite(kdfInvite);

    const auto unwrapped = m_crypto.unwrapKey(u.wrappedKeyByRecovery, kdfInvite);
    if (!unwrapped) {
        res.error = QStringLiteral("invite key inválida");
        return res;
    }
    const QByteArray K = *unwrapped; // vai para res.dataKey — não wipar

    QByteArray newSaltPw = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    QByteArray passBytes = newPassword.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, newSaltPw, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfPass(kdfPass);

    // (ADR-001 §3.5) recovery key NOVA e definitiva — não reusa o invite key.
    const QByteArray newRecoveryKey = m_crypto.randomBytes(kRecoveryKeyBytes); // vai pro caller
    QByteArray newSaltRk = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    QByteArray kdfRecovery = m_crypto.deriveKey(newRecoveryKey, newSaltRk, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfRecovery(kdfRecovery);

    u.wrappedKeyByPass     = m_crypto.wrapKey(K, kdfPass);
    u.saltPw               = newSaltPw;
    u.wrappedKeyByRecovery = m_crypto.wrapKey(K, kdfRecovery);
    u.saltRk               = newSaltRk;
    u.kdfOps                = static_cast<int>(kModerateOps);
    u.kdfMem                = kModerateMem;
    u.invitePending          = false;

    QVector<UserAccountRecord> users = loaded.users;
    users[idx] = u;
    if (!m_repo.saveAll(users, loaded.adminPubKey, m_adminSignKey)) {
        res.error = QStringLiteral("falha ao gravar users.json");
        return res;
    }

    res.ok = true;
    res.dataKey = K;
    res.recoveryKeyPlaintext = newRecoveryKey;
    return res;
}

// ── Login ────────────────────────────────────────────────────────────────

UserAccountService::LoginResult UserAccountService::verifyLogin(const QString &username,
                                                                  const QString &password)
{
    LoginResult res;

    const auto loaded = m_repo.loadAll();
    if (!loaded.ok) {
        res.status = LoginResult::IntegrityError;
        return res;
    }
    if (!pubKeyMatchesAnchor(loaded.adminPubKey)) {
        res.status = LoginResult::IntegrityError;
        return res;
    }

    const int idx = findUserIndex(loaded.users, username);
    if (idx < 0) {
        // (ADR-001 §2) mensagem explícita de "usuário não existe" é OK
        // (app local, sem superfície de rede) — mas ainda roda um
        // deriveKey dummy pra manter latência ~constante entre os
        // caminhos de falha (defesa barata, sem custo de design).
        QByteArray dummySalt = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
        QByteArray passBytes = password.toUtf8();
        ScopedWipe wipePassBytes(passBytes);
        QByteArray dummy = m_crypto.deriveKey(passBytes, dummySalt, kModerateOps, kModerateMem);
        ScopedWipe wipeDummy(dummy);

        res.status = LoginResult::UserNotFound;
        return res;
    }

    const UserAccountRecord &u = loaded.users[idx];
    if (u.invitePending) {
        res.status = LoginResult::InvitePending;
        return res;
    }

    QByteArray passBytes = password.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, u.saltPw,
                                            static_cast<unsigned long long>(u.kdfOps), u.kdfMem);
    ScopedWipe wipeKdfPass(kdfPass);

    const auto unwrapped = m_crypto.unwrapKey(u.wrappedKeyByPass, kdfPass);
    if (!unwrapped) {
        res.status = LoginResult::WrongPassword;
        return res;
    }

    res.dataKey = *unwrapped;
    res.role    = u.role;
    res.status  = LoginResult::Ok;

    if (u.role == Role::Admin) {
        const auto unwrappedSign = m_crypto.unwrapKey(u.wrappedSignKeyByPass, kdfPass);
        if (!unwrappedSign) {
            // Mesma senha que abriu K deveria abrir a signing key também
            // (mesmo wrapKey32) — se não abrir, o registro está
            // parcialmente corrompido. Fail-secure: não deixa a sessão
            // "logada sem poder assinar" passar batido.
            res.status = LoginResult::IntegrityError;
            res.dataKey.clear();
            return res;
        }
        m_adminSignKey = *unwrappedSign;
    }

    return res;
}

// ── Remoção de usuário ──────────────────────────────────────────────────

bool UserAccountService::removeUser(const QString &username)
{
    if (m_adminSignKey.isEmpty())
        return false;

    const auto loaded = m_repo.loadAll();
    if (!loaded.ok)
        return false;
    if (!pubKeyMatchesAnchor(loaded.adminPubKey))
        return false;

    const int idx = findUserIndex(loaded.users, username);
    if (idx < 0)
        return false;

    if (loaded.users[idx].role == Role::Admin) {
        int adminCount = 0;
        for (const auto &u : loaded.users)
            if (u.role == Role::Admin) ++adminCount;
        if (adminCount <= 1)
            return false; // recusa remover o último admin — travaria a instalação
    }

    QVector<UserAccountRecord> updated = loaded.users;
    updated.removeAt(idx);
    return m_repo.saveAll(updated, loaded.adminPubKey, m_adminSignKey);
}

// ── Reset de senha via recovery key ─────────────────────────────────────

UserAccountService::RecoveryResult UserAccountService::resetPasswordViaRecovery(
    const QString &username, const QByteArray &recoveryKey, const QString &newPassword)
{
    RecoveryResult res;

    if (newPassword.size() < kMinPasswordLen) {
        res.error = QStringLiteral("senha deve ter ao menos %1 caracteres").arg(kMinPasswordLen);
        return res;
    }

    const auto loaded = m_repo.loadAll();
    if (!loaded.ok) {
        res.error = QStringLiteral("registro de usuários corrompido/adulterado");
        return res;
    }
    if (!pubKeyMatchesAnchor(loaded.adminPubKey)) {
        res.error = QStringLiteral("admin_pubkey do arquivo não confere com a âncora do keychain — possível adulteração");
        return res;
    }

    const int idx = findUserIndex(loaded.users, username);
    if (idx < 0) {
        res.error = QStringLiteral("usuário não encontrado");
        return res;
    }

    UserAccountRecord u = loaded.users[idx];
    if (u.invitePending) {
        res.error = QStringLiteral("usuário ainda não completou o 1º login (convite pendente)");
        return res;
    }

    QByteArray kdfRecovery = m_crypto.deriveKey(recoveryKey, u.saltRk,
                                                static_cast<unsigned long long>(u.kdfOps), u.kdfMem);
    ScopedWipe wipeKdfRecovery(kdfRecovery);

    const auto unwrapped = m_crypto.unwrapKey(u.wrappedKeyByRecovery, kdfRecovery);
    if (!unwrapped) {
        res.error = QStringLiteral("recovery key inválida");
        return res;
    }
    const QByteArray K = *unwrapped; // K nunca muda (D-D/§2) — não wipar, só reusada pros re-wraps abaixo

    // Quem assina a regravação do registro central (ver nota de design no
    // .h): se o alvo É o admin, a própria recovery key dele já desembrulha
    // a signing key — auto-suficiente, sem depender de sessão alguma
    // (cobre o cenário real: admin esqueceu a senha e está deslogado).
    // Se o alvo é um usuário comum, precisa de uma sessão de admin já
    // autenticada nesta instância (mesma limitação temporária de
    // completeFirstLogin — ver ADR-001 §8 passo 6).
    QByteArray signSecretForSaving;
    ScopedWipe wipeSignSecretForSaving(signSecretForSaving);

    if (u.role == Role::Admin) {
        const auto unwrappedSign = m_crypto.unwrapKey(u.wrappedSignKeyByRecovery, kdfRecovery);
        if (!unwrappedSign) {
            res.error = QStringLiteral("falha ao recuperar a signing key do admin via recovery key — registro corrompido?");
            return res;
        }
        signSecretForSaving = *unwrappedSign;
    } else {
        if (m_adminSignKey.isEmpty()) {
            res.error = QStringLiteral(
                "não foi possível concluir: reset de usuário comum exige uma "
                "sessão de admin disponível nesta instância (mesma limitação "
                "temporária de completeFirstLogin — ver ADR-001 §8 passo 6)");
            return res;
        }
        signSecretForSaving = m_adminSignKey; // cópia COW — wipe local não afeta o membro (ver ScopedWipe)
    }

    QByteArray newSaltPw = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    QByteArray passBytes = newPassword.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, newSaltPw, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfPass(kdfPass);

    u.wrappedKeyByPass = m_crypto.wrapKey(K, kdfPass);
    u.saltPw           = newSaltPw;
    u.kdfOps             = static_cast<int>(kModerateOps);
    u.kdfMem             = kModerateMem;
    // wrappedKeyByRecovery/saltRk permanecem intactos — recovery não rotaciona.

    if (u.role == Role::Admin) {
        // Signing key em si não muda — só o wrap POR SENHA dela precisa
        // ser regravado (o wrap por recovery já estava correto, é o que
        // acabamos de usar pra abrir tudo).
        u.wrappedSignKeyByPass = m_crypto.wrapKey(signSecretForSaving, kdfPass);
    }

    QVector<UserAccountRecord> updated = loaded.users;
    updated[idx] = u;
    if (!m_repo.saveAll(updated, loaded.adminPubKey, signSecretForSaving)) {
        res.error = QStringLiteral("falha ao gravar users.json");
        return res;
    }

    res.ok = true;
    return res;
}
