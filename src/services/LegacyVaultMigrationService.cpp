#include "LegacyVaultMigrationService.h"

#include "core/crypto/AesCryptoService.h"
#include "core/database/DatabaseManager.h"
#include "core/settings/AppSettings.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include <sodium.h>

namespace {

// ── RAII: zera um QByteArray sensível ao sair de escopo ────────────────────
// Mesma técnica de UserAccountService.cpp (ScopedWipe) — duplicada aqui
// (não exportada de lá, é um detalhe de implementação privado do .cpp) em
// vez de acoplar os dois .cpp por um header interno só para isto.
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

// Mesma chave de salt usada por LoginDialog::onEntrar (APP_NAME + "/salt",
// appName "orcamento-pessoal") — a migração precisa derivar EXATAMENTE as
// mesmas chaves candidatas que o login atual derivaria para o mesmo
// username, ou não conseguirá abrir o cofre legado.
const QString kSaltSuffix = QStringLiteral("/salt");

// (ADR-001 §6/D6) Réplica EXATA do algoritmo de slug PRÉ-Fatia-4 de
// UserSession::slug() (bug "João" -> "joo": não decompunha NFD antes do
// filtro [a-z0-9_]). Usada só como FALLBACK de compatibilidade — a fonte
// única de verdade do slug ATUAL é UserAccountService::slugify() (NFD +
// remove Mark_NonSpacing, "João" -> "joao").
QString slugAntigo(const QString &username)
{
    return username.toLower()
                   .replace(QLatin1Char(' '), QLatin1Char('_'))
                   .remove(QRegularExpression(QStringLiteral("[^a-z0-9_]")));
}

// (ADR-001 §7/D6) Relocaliza um `.pbd` do caminho do slug ANTIGO para o
// caminho do slug NOVO — reduz o resto do fluxo de migração a um único
// caminho (sempre newSlug), em vez de dois branches de materialização.
//
// SEGURANÇA DE DADO: copia (nunca move) via temp+rename atômico para o
// destino novo; só DEPOIS de o destino estar durável é que a fonte é
// renomeada para `.bak` (nunca deletada). Em qualquer ponto da operação, os
// bytes originais existem em pelo menos um lugar no disco:
//   - crash antes do rename do temp -> destino: fonte intacta no lugar
//     original, nada mudou (o `.new` fica órfão, sobrescrito na próxima
//     tentativa);
//   - crash entre o rename do temp -> destino e o rename fonte -> .bak:
//     cofre existe em DOIS lugares (destino novo completo + fonte original
//     intacta) — nunca em NENHUM;
//   - falha ao renomear fonte -> .bak (ex.: FS read-only): não fatal, loga
//     aviso — o destino novo já tem uma cópia válida, é o suficiente para
//     a migração prosseguir.
bool relocalizarParaSlugNovo(const QString &pbdPathAntigo, const QString &pbdPathNovo)
{
    const QString tmp = pbdPathNovo + QStringLiteral(".new");
    QFile::remove(tmp); // resíduo de tentativa anterior, se houver
    if (!QFile::copy(pbdPathAntigo, tmp)) {
        qWarning() << "LegacyVaultMigrationService: falha ao copiar cofre do slug antigo"
                   << pbdPathAntigo << "->" << tmp;
        return false;
    }
    if (!QFile::rename(tmp, pbdPathNovo)) {
        qWarning() << "LegacyVaultMigrationService: falha ao renomear cópia relocalizada"
                   << tmp << "->" << pbdPathNovo;
        QFile::remove(tmp);
        return false;
    }

    const QString bak = pbdPathAntigo + QStringLiteral(".bak");
    if (!QFile::rename(pbdPathAntigo, bak)) {
        qWarning() << "LegacyVaultMigrationService: não foi possível renomear o cofre do slug"
                      " antigo para .bak —" << pbdPathAntigo << "(cofre já replicado em"
                   << pbdPathNovo << ", migração prossegue)";
    }
    return true;
}

} // namespace

LegacyVaultMigrationService::LegacyVaultMigrationService(UserAccountService &accounts,
                                                          SodiumCryptoService &crypto,
                                                          SecretStoreService &secrets)
    : m_accounts(accounts)
    , m_crypto(crypto)
    , m_secrets(secrets)
{
}

bool LegacyVaultMigrationService::legacyVaultPresente(const QString &username,
                                                       QString *slugEncontrado) const
{
    if (slugEncontrado)
        slugEncontrado->clear();

    if (!m_accounts.isFirstRun())
        return false; // já existe registro admin+envelope — nada a migrar

    const QString slugAtual = UserAccountService::slugify(username);
    if (QFile::exists(AppSettings::savePathFixo(slugAtual))) {
        if (slugEncontrado) *slugEncontrado = slugAtual;
        return true;
    }

    const QString slugAntigoStr = slugAntigo(username);
    if (slugAntigoStr != slugAtual && QFile::exists(AppSettings::savePathFixo(slugAntigoStr))) {
        if (slugEncontrado) *slugEncontrado = slugAntigoStr;
        return true;
    }

    return false;
}

LegacyVaultMigrationService::MigrateResult
LegacyVaultMigrationService::migrarParaAdmin(const QString &username, const QString &password)
{
    MigrateResult res;

    if (!m_accounts.isFirstRun()) {
        res.error = QStringLiteral("já existe um registro de usuários — não é o primeiro run");
        return res;
    }
    // Defensivo — mesmo guard de createAdmin()/completeFirstLogin(): esta
    // operação abre/fecha o singleton DatabaseManager; se esta instância de
    // processo já tem outro cofre aberto, recusa em vez de clobbering.
    if (DatabaseManager::instance().isOpen()) {
        res.error = QStringLiteral("operação exige nenhuma sessão de cofre aberta nesta instância");
        return res;
    }

    const QString slugNovo = UserAccountService::slugify(username);

    // (D6) Localiza o cofre físico — slug atual primeiro, slug antigo
    // (pré-Fatia-4, sem NFD) como fallback. Se achado no slug antigo,
    // relocaliza (copy + .bak, lossless) ANTES de abrir — reduz o resto do
    // fluxo a um único caminho (slugNovo), inclusive pro close() mais
    // adiante materializar o `.pbd` v2 diretamente no destino certo.
    QString slugFisico;
    if (QFile::exists(AppSettings::savePathFixo(slugNovo))) {
        slugFisico = slugNovo;
    } else {
        const QString velho = slugAntigo(username);
        if (velho != slugNovo && QFile::exists(AppSettings::savePathFixo(velho)))
            slugFisico = velho;
    }
    if (slugFisico.isEmpty()) {
        res.error = QStringLiteral("nenhum cofre legado encontrado para este usuário");
        return res;
    }
    if (slugFisico != slugNovo) {
        if (!relocalizarParaSlugNovo(AppSettings::savePathFixo(slugFisico),
                                      AppSettings::savePathFixo(slugNovo))) {
            res.error = QStringLiteral(
                "falha ao relocalizar o cofre do slug antigo para o slug atual (ADR-001 §6/D6)");
            return res;
        }
    }

    // ── 1. Deriva as chaves candidatas do modelo antigo EXATAMENTE como o
    //       login atual (LoginDialog::onEntrar) ──────────────────────────
    const QString saltKey = username.toLower() + kSaltSuffix;
    const QByteArray salt = m_secrets.retrieve(saltKey);
    if (salt.isEmpty()) {
        res.error = QStringLiteral(
            "sem salt no cofre do sistema para este usuário — impossível derivar a chave antiga");
        return res;
    }

    // Argon2id MODERATE default — mesmo overload de 2 argumentos que
    // LoginDialog::onEntrar usa para o candidato v1-novo.
    const QByteArray keyNovoCandidato = m_crypto.deriveKey(password, salt);
    AesCryptoService legacyCrypto;
    const QByteArray keyLegadoCandidato = legacyCrypto.deriveKey(password, salt); // PBKDF2

    // ── 2. Abre reusando o caminho de abertura já existente ─────────────
    if (!DatabaseManager::instance().open(slugNovo, keyNovoCandidato, keyLegadoCandidato)) {
        DatabaseManager::instance().close(); // defensivo — nunca deixa header/chave vazar
        res.error = QStringLiteral("senha incorreta ou cofre corrompido — migração não realizada");
        return res;
    }

    // ── 3. .db de trabalho JÁ decifrado em memória — gera o envelope novo ──
    VaultHeaderData header;
    header.username = username;
    header.saltPw    = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    header.saltRk    = m_crypto.randomBytes(crypto_pwhash_SALTBYTES);
    header.kdfOps    = static_cast<int>(kModerateOps);
    header.kdfMem    = kModerateMem;

    const QByteArray K = m_crypto.randomBytes(UserAccountService::kKeyBytes); // vai para rekey() — não wipar

    QByteArray passBytes = password.toUtf8();
    ScopedWipe wipePassBytes(passBytes);
    QByteArray kdfPass = m_crypto.deriveKey(passBytes, header.saltPw, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfPass(kdfPass);

    const QByteArray recoveryKey = m_crypto.randomBytes(UserAccountService::kRecoveryKeyBytes); // vai pro caller
    QByteArray kdfRecovery = m_crypto.deriveKey(recoveryKey, header.saltRk, kModerateOps, kModerateMem);
    ScopedWipe wipeKdfRecovery(kdfRecovery);

    header.wrappedKeyByPass     = m_crypto.wrapKey(K, kdfPass);
    header.wrappedKeyByRecovery = m_crypto.wrapKey(K, kdfRecovery);

    SodiumCryptoService::SignKeypair signKp = m_crypto.generateSignKeypair();
    ScopedWipe wipeSignSecret(signKp.secretKey);

    // ── 4. Injeta o header + troca a chave do PRÓXIMO save()/close() ────
    DatabaseManager::instance().setVaultHeader(header);
    if (!DatabaseManager::instance().rekey(K)) {
        // Só pode falhar se o DB não estiver aberto — não deveria acontecer
        // aqui (open() acabou de retornar true). Defensivo mesmo assim.
        DatabaseManager::instance().close();
        res.error = QStringLiteral("falha interna ao trocar a chave do cofre (rekey)");
        return res;
    }

    // ── 5. Âncora do admin_pubkey — ANTES do registro (mesma ordem de
    //       UserAccountService::createAdmin) ─────────────────────────────
    if (!m_secrets.store(QString::fromLatin1(UserAccountService::kAdminPubKeyAnchorKey),
                         signKp.publicKey)) {
        DatabaseManager::instance().close(); // fecha sem persistir nada — retentável
        res.error = QStringLiteral("falha ao gravar a âncora do admin no cofre do sistema");
        return res;
    }

    // ── 6. close() materializa o `.pbd` v2 com K novo ────────────────────
    // m_slug == slugNovo sempre neste ponto (abrimos com slugNovo — o
    // cofre físico já foi relocalizado para lá no passo D6, se necessário)
    // — close() grava em AppSettings::savePathFixo(slugNovo) via o mesmo
    // idioma temp+rename atômico usado em qualquer save() normal: nunca
    // existe um estado onde o v2 está pela metade por cima do v1 (o `.pbd`
    // anterior só é removido DEPOIS do `.new` completo estar gravado e
    // pronto para o rename).
    if (!DatabaseManager::instance().close()) {
        res.error = QStringLiteral(
            "falha ao materializar o cofre .pbd v2 (integrity_check reprovou) — "
            "cofre original preservado, tente novamente");
        return res;
    }

    // ── 7. Registra o dono como admin ────────────────────────────────────
    UserAccountRecord admin;
    admin.username               = username;
    admin.role                   = Role::Admin;
    admin.invitePending          = false;
    admin.wrappedSignKeyByPass     = m_crypto.wrapKey(signKp.secretKey, kdfPass);
    admin.wrappedSignKeyByRecovery = m_crypto.wrapKey(signKp.secretKey, kdfRecovery);

    if (!m_accounts.registerMigratedAdmin(admin, signKp.publicKey, signKp.secretKey)) {
        // O `.pbd` v2 já foi materializado (passo 6), mas o registro não —
        // estado retentável: o próximo boot ainda vê isFirstRun()==true (o
        // registro nunca foi gravado) e o `.pbd` v2 já existe no caminho do
        // slug atual; uma nova tentativa de migração encontraria o cofre já
        // no formato v2 (não mais "legado") e falharia em
        // legacyVaultPresente()/open() por não achar cofre legado — cenário
        // residual raro (falha de disco entre os passos 6 e 7); documentado
        // como limitação conhecida, fora do escopo de auto-reparo desta
        // fatia (o dado financeiro em si NUNCA foi perdido, só a promoção a
        // admin ficaria pendente — recuperável manualmente).
        res.error = QStringLiteral("falha ao gravar o registro de usuários (users.json) — "
                                    "o cofre já foi migrado para .pbd v2, mas a promoção a "
                                    "admin não foi registrada; contate o suporte");
        return res;
    }

    res.ok = true;
    res.recoveryKeyPlaintext = recoveryKey;
    return res;
}
