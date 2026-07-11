#include "UserAccountService.h"

#include "core/database/DatabaseManager.h"
#include "core/settings/AppSettings.h"

#include <QFile>

#include <sodium.h>
#include <unistd.h>   // fsync — rewriteVaultHeader() (D5, escrita atômica)

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
    // (ADR-001 §6/D6, Fatia 4) Delega à fonte única de verdade —
    // AppSettings::slugify (compartilhada com UserSession::slug() e com o
    // path do cofre, savePathFixo). Algoritmo (NFD + remove
    // Mark_NonSpacing + lowercase + [^a-z0-9_]) documentado lá.
    return AppSettings::slugify(username);
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

// ── Bootstrap de cofre `.pbd` v2 (D3, Fatia 4) ──────────────────────────────
// Usado por createAdmin() e completeFirstLogin() — setVaultHeader() ANTES
// do open() (bootstrap: o arquivo ainda não existe, então open() preserva o
// header primed — ver DatabaseManager::setVaultHeader()); close() em
// seguida materializa o `.pbd` v2 real no disco (1º save). Qualquer falha
// aborta sem deixar o singleton aberto — o caller nunca persiste
// `users.json` apontando pra um cofre que não foi materializado.
bool UserAccountService::bootstrapVault(const QString &slug, const VaultHeaderData &header,
                                        const QByteArray &K)
{
    DatabaseManager::instance().setVaultHeader(header);
    if (!DatabaseManager::instance().open(slug, K)) {
        DatabaseManager::instance().close(); // defensivo — reseta o header primed em caso de falha parcial
        return false;
    }
    return DatabaseManager::instance().close();
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
    // Defensivo — bootstrapVault() abre/fecha o singleton DatabaseManager;
    // se esta instância de processo já tem outro cofre aberto, recusa em
    // vez de fazer clobbering (mesmo guard de completeFirstLogin, D3).
    if (DatabaseManager::instance().isOpen()) {
        res.error = QStringLiteral("operação exige nenhuma sessão de cofre aberta nesta instância");
        return res;
    }

    // (ADR-001 §8 passo 6/D1, Fatia 4) Os wraps de K por senha/recovery
    // moram no header `.pbd` v2 — não mais no UserAccountRecord.
    VaultHeaderData header;
    header.username = username;
    header.saltPw    = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    header.saltRk    = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    header.kdfOps    = static_cast<int>(kModerateOps);
    header.kdfMem    = kModerateMem;

    QByteArray K = m_crypto.randomBytes(kKeyBytes);
    ScopedWipe wipeK(K);

    QByteArray passBytes = password.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, header.saltPw, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfPass(kdfPass);

    const QByteArray recoveryKey = m_crypto.randomBytes(kRecoveryKeyBytes); // vai pro caller — não wipar aqui
    QByteArray kdfRecovery = m_crypto.deriveKey(recoveryKey, header.saltRk, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfRecovery(kdfRecovery);

    header.wrappedKeyByPass     = m_crypto.wrapKey(K, kdfPass);
    header.wrappedKeyByRecovery = m_crypto.wrapKey(K, kdfRecovery);

    SodiumCryptoService::SignKeypair signKp = m_crypto.generateSignKeypair();
    ScopedWipe wipeSignSecret(signKp.secretKey);

    UserAccountRecord admin;
    admin.username     = username;
    admin.role         = Role::Admin;
    admin.invitePending = false;
    admin.wrappedSignKeyByPass     = m_crypto.wrapKey(signKp.secretKey, kdfPass);
    admin.wrappedSignKeyByRecovery = m_crypto.wrapKey(signKp.secretKey, kdfRecovery);

    // (item E) grava a âncora ANTES do cofre/registro: se o processo morrer
    // no meio, o pior caso é users.json/.pbd ausentes + âncora órfã no
    // keychain (isFirstRun() continua true, próxima tentativa de
    // createAdmin sobrescreve a âncora — sem risco de segurança, só
    // resíduo). O inverso (registro com admin mas sem âncora) seria pior —
    // abriria a janela de pubkey-swap que este item existe pra fechar.
    if (!m_secrets.store(QString::fromLatin1(kAdminPubKeyAnchorKey), signKp.publicKey)) {
        res.error = QStringLiteral("falha ao gravar a âncora do admin no keychain do sistema");
        return res;
    }

    // (D3) Bootstrapa o .pbd v2 do admin ANTES de gravar users.json —
    // falha aqui aborta sem tocar o registro central (estado limpo,
    // retentável; a próxima tentativa de createAdmin reescreve tudo).
    const QString slug = slugify(username);
    if (!bootstrapVault(slug, header, K)) {
        res.error = QStringLiteral("falha ao inicializar o cofre do admin (.pbd v2)");
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
    novo.username     = username;
    novo.role         = role;
    novo.invitePending = true;
    novo.saltInvite    = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    novo.kdfOpsInvite  = static_cast<int>(kModerateOps);
    novo.kdfMemInvite  = kModerateMem;

    // (D3, decisão do líder 2026-07-11) A invite key embrulha um MARCADOR
    // ALEATÓRIO descartável de 32B — NUNCA o K real. O K real só nasce em
    // completeFirstLogin(), gerado pelo PRÓPRIO usuário; o admin nunca gera
    // nem processa K de ninguém, nem indiretamente (o marcador desembrulhado
    // é jogado fora, nunca vira dado de fato — prova de segurança T6).
    QByteArray marker = m_crypto.randomBytes(kKeyBytes);
    ScopedWipe wipeMarker(marker);

    const QByteArray inviteKey = m_crypto.randomBytes(kRecoveryKeyBytes); // vai pro caller — não wipar
    QByteArray kdfInvite = m_crypto.deriveKey(inviteKey, novo.saltInvite, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfInvite(kdfInvite);

    novo.wrappedMarkerByInvite = m_crypto.wrapKey(marker, kdfInvite);

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
// (ADR-001 §8 passo 6/D3, Fatia 4) 100% self-service — NÃO toca
// users.json, então não depende de m_adminSignKey. O admin pode estar
// offline/deslogado o tempo todo.

UserAccountService::FirstLoginResult UserAccountService::completeFirstLogin(
    const QString &username, const QByteArray &inviteKey, const QString &newPassword)
{
    FirstLoginResult res;

    if (newPassword.size() < kMinPasswordLen) {
        res.error = QStringLiteral("senha deve ter ao menos %1 caracteres").arg(kMinPasswordLen);
        return res;
    }
    // Defensivo — bootstrapVault() abre/fecha o singleton DatabaseManager;
    // se esta instância de processo já tem outro cofre aberto (ex.: sessão
    // do admin logado com o próprio cofre financeiro aberto), recusa em
    // vez de fazer clobbering.
    if (DatabaseManager::instance().isOpen()) {
        res.error = QStringLiteral("operação exige nenhuma sessão de cofre aberta nesta instância");
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

    const UserAccountRecord &u = loaded.users[idx];
    if (!u.invitePending) {
        res.error = QStringLiteral("usuário não está com convite pendente");
        return res;
    }

    const QString slug      = slugify(username);
    const QString vaultPath = AppSettings::savePathFixo(slug);
    if (QFile::exists(vaultPath)) {
        // (D3 passo 2 — idempotência) já existe .pbd para este slug: ou o
        // 1º login já foi concluído antes (invitePending stale, D3 passo
        // 10) ou houve colisão de slug — recusa em vez de sobrescrever um
        // cofre real.
        res.error = QStringLiteral("já existe um cofre para este usuário — 1º login já concluído?");
        return res;
    }

    QByteArray kdfInvite = m_crypto.deriveKey(inviteKey, u.saltInvite,
                                              static_cast<unsigned long long>(u.kdfOpsInvite), u.kdfMemInvite);
    ScopedWipe wipeKdfInvite(kdfInvite);

    // Unwrap só prova posse da invite key — o valor desembrulhado é um
    // MARCADOR descartável (D3), NUNCA o K real. Descartado aqui mesmo,
    // nunca vira res.dataKey (ver prova de segurança T6 no relatório).
    const auto unwrappedMarker = m_crypto.unwrapKey(u.wrappedMarkerByInvite, kdfInvite);
    if (!unwrappedMarker) {
        res.error = QStringLiteral("invite key inválida");
        return res;
    }

    // (D3 passo 5) K NOVO nasce aqui — gerado pelo PRÓPRIO usuário, sem
    // vínculo criptográfico com o marcador descartado acima.
    const QByteArray K = m_crypto.randomBytes(kKeyBytes); // vai para res.dataKey — não wipar

    VaultHeaderData header;
    header.username = username;
    header.saltPw    = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    header.saltRk    = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    header.kdfOps    = static_cast<int>(kModerateOps);
    header.kdfMem    = kModerateMem;

    QByteArray passBytes = newPassword.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, header.saltPw, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfPass(kdfPass);

    // (ADR-001 §3.5) recovery key NOVA e definitiva — não reusa o invite key.
    const QByteArray newRecoveryKey = m_crypto.randomBytes(kRecoveryKeyBytes); // vai pro caller
    QByteArray kdfRecovery = m_crypto.deriveKey(newRecoveryKey, header.saltRk, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfRecovery(kdfRecovery);

    header.wrappedKeyByPass     = m_crypto.wrapKey(K, kdfPass);
    header.wrappedKeyByRecovery = m_crypto.wrapKey(K, kdfRecovery);

    if (!bootstrapVault(slug, header, K)) {
        res.error = QStringLiteral("falha ao inicializar o cofre do usuário (.pbd v2)");
        return res;
    }

    // (D3 passo 10) users.json NÃO é gravado aqui de propósito — mutação de
    // senha/recovery de usuário comum toca só o .pbd dele (D1). invitePending
    // fica true, VESTIGIAL: o gate real de login (verifyLogin, D5) é por
    // EXISTÊNCIA DO ARQUIVO .pbd, não pela flag. É isso que fecha "1º login
    // sem admin/assinatura online" (prova T6).
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
    const QString slug      = slugify(username);
    const QString vaultPath = AppSettings::savePathFixo(slug);

    // (ADR-001 §8 passo 6/D5, Fatia 4) Gate por EXISTÊNCIA DO ARQUIVO —
    // autoritativo. invitePending só desempata o caso anômalo (registro diz
    // completo mas sem cofre no disco → fail-secure, NUNCA "senha errada").
    if (!QFile::exists(vaultPath)) {
        res.status = u.invitePending ? LoginResult::InvitePending : LoginResult::IntegrityError;
        return res;
    }

    QFile f(vaultPath);
    if (!f.open(QIODevice::ReadOnly)) {
        res.status = LoginResult::IntegrityError;
        return res;
    }
    const QByteArray fileBytes = f.readAll();
    f.close();

    const auto parsed = VaultFile::parseV2(fileBytes);
    if (!parsed.ok) {
        // Cofre pré-ADR-001 (v1 puro, sem header v2 — conta em migração,
        // Fatia 5, fora de escopo) OU corrompido; ambos caem em
        // IntegrityError aqui (fail-secure).
        res.status = LoginResult::IntegrityError;
        return res;
    }

    QByteArray passBytes = password.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, parsed.header.saltPw,
                                            static_cast<unsigned long long>(parsed.header.kdfOps),
                                            parsed.header.kdfMem);
    ScopedWipe wipeKdfPass(kdfPass);

    const auto unwrapped = m_crypto.unwrapKey(parsed.header.wrappedKeyByPass, kdfPass);
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

// ── Registro do dono de um cofre migrado como admin (ADR-001 §7, Fatia 5) ──

bool UserAccountService::registerMigratedAdmin(const UserAccountRecord &admin,
                                                const QByteArray &adminPubKey,
                                                const QByteArray &adminSignSecretKey)
{
    if (!isFirstRun())
        return false; // já existe registro — não é (mais) um 1º run/migração
    if (admin.role != Role::Admin)
        return false; // migração sempre promove o dono do cofre a admin

    QVector<UserAccountRecord> users{admin};
    return m_repo.saveAll(users, adminPubKey, adminSignSecretKey);
}

// ── Reset de senha via recovery key ─────────────────────────────────────
// (ADR-001 §8 passo 6/D5, Fatia 4) Operação de ARQUIVO PURA sobre o .pbd
// v2 do usuário — sem DatabaseManager (o innerBlob nunca é re-cifrado, só
// copiado verbatim), sem tocar users.json quando o alvo é usuário comum
// (logo, sem depender de m_adminSignKey nesse caso).

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
    const UserAccountRecord u = loaded.users[idx];

    // (D5, Fatia 4) Gate por EXISTÊNCIA DO ARQUIVO — mesma lógica de
    // verifyLogin (D5). NÃO checar `u.invitePending` aqui: desde D3 passo
    // 10, essa flag fica VESTIGIAL (true) para sempre depois do 1º login
    // bem-sucedido — checá-la recusaria reset de senha de todo usuário
    // comum que já tem cofre. O arquivo é a fonte de verdade.
    const QString slug      = slugify(username);
    const QString vaultPath = AppSettings::savePathFixo(slug);
    if (!QFile::exists(vaultPath)) {
        res.error = QStringLiteral("cofre do usuário não encontrado — convite ainda pendente?");
        return res;
    }

    QFile f(vaultPath);
    if (!f.open(QIODevice::ReadOnly)) {
        res.error = QStringLiteral("falha ao abrir o cofre do usuário");
        return res;
    }
    const QByteArray fileBytes = f.readAll();
    f.close();

    const auto parsed = VaultFile::parseV2(fileBytes);
    if (!parsed.ok) {
        res.error = QStringLiteral(
            "cofre em formato antigo (pré-ADR-001) — reset via recovery exige "
            "migração para o .pbd v2 primeiro (ADR-001 §8 passos 6-7, Fatia 5)");
        return res;
    }

    QByteArray kdfRecovery = m_crypto.deriveKey(recoveryKey, parsed.header.saltRk,
                                                static_cast<unsigned long long>(parsed.header.kdfOps),
                                                parsed.header.kdfMem);
    ScopedWipe wipeKdfRecovery(kdfRecovery);

    const auto unwrapped = m_crypto.unwrapKey(parsed.header.wrappedKeyByRecovery, kdfRecovery);
    if (!unwrapped) {
        res.error = QStringLiteral("recovery key inválida");
        return res;
    }
    const QByteArray K = *unwrapped; // K nunca muda (D-D/§2) — não wipar, reusada só pro re-wrap abaixo

    QByteArray newSaltPw = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    QByteArray passBytes = newPassword.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, newSaltPw, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfPass(kdfPass);

    VaultHeaderData newHeader = parsed.header;
    newHeader.saltPw           = newSaltPw;
    newHeader.wrappedKeyByPass = m_crypto.wrapKey(K, kdfPass);
    // wrappedKeyByRecovery/saltRk permanecem intactos — recovery não rotaciona (D-D/§2).

    // (D5 passo 7) Escrita atômica: temp + fsync + rename. innerBlob
    // preservado VERBATIM por rewriteVaultHeader() — K nunca muda, o .pbd
    // NÃO é re-cifrado, só o header é regravado.
    if (!rewriteVaultHeader(vaultPath, newHeader)) {
        res.error = QStringLiteral("falha ao regravar o cofre com a nova senha");
        return res;
    }

    if (u.role == Role::Admin) {
        // (D5 passo 5) Admin recuperando a PRÓPRIA senha: a própria
        // recovery key já desembrulha a signing key dele (mesmo
        // kdfRecovery) — auto-suficiente, sem depender de m_adminSignKey
        // nem de sessão alguma (cobre o cenário real: admin esqueceu a
        // senha e está deslogado).
        const auto unwrappedSign = m_crypto.unwrapKey(u.wrappedSignKeyByRecovery, kdfRecovery);
        if (!unwrappedSign) {
            res.error = QStringLiteral(
                "falha ao recuperar a signing key do admin via recovery key — registro corrompido?");
            return res;
        }
        QByteArray signSecret = *unwrappedSign;
        ScopedWipe wipeSignSecret(signSecret);

        UserAccountRecord updated = u;
        updated.wrappedSignKeyByPass = m_crypto.wrapKey(signSecret, kdfPass);

        QVector<UserAccountRecord> users = loaded.users;
        users[idx] = updated;
        if (!m_repo.saveAll(users, loaded.adminPubKey, signSecret)) {
            res.error = QStringLiteral("falha ao gravar users.json (wrap de senha da signing key)");
            return res;
        }
    }
    // Usuário comum: nenhum toque em users.json (D5 passo 6).

    res.ok = true;
    return res;
}

// ── Regrava só o header do .pbd v2, innerBlob verbatim (D5) ────────────────

bool UserAccountService::rewriteVaultHeader(const QString &vaultPath, const VaultHeaderData &newHeader)
{
    QFile in(vaultPath);
    if (!in.open(QIODevice::ReadOnly))
        return false;
    const QByteArray fileBytes = in.readAll();
    in.close();

    const auto parsed = VaultFile::parseV2(fileBytes);
    if (!parsed.ok)
        return false; // não deveria acontecer — já validado pelo caller antes de chegar aqui

    const QByteArray framed = VaultFile::frameV2(newHeader, parsed.innerBlob);

    const QString tmpPath = vaultPath + QStringLiteral(".new");
    QFile out(tmpPath);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    if (out.write(framed) != framed.size()) {
        out.close();
        QFile::remove(tmpPath);
        return false;
    }
    out.flush();
    ::fsync(out.handle()); // durabilidade — sobrevive a crash antes do rename atômico
    out.close();

    QFile::remove(vaultPath);
    return QFile::rename(tmpPath, vaultPath);
}
