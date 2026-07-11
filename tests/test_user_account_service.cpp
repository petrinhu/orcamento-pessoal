// PeuBud - TDD do UserAccountService (ADR-001 #212, Fatia 2 + Fatia 4).
// Regra de negócio de gestão de usuários: envelope encryption, invite-key
// (marcador descartável — D3), recovery key, sessão de assinatura do admin,
// a âncora anti-pubkey-swap no SecretStoreService (item E do brief da
// Fatia 1/2), e a integração `.pbd` v2 autocontido (Fatia 4: createAdmin/
// completeFirstLogin bootstrapam o cofre via DatabaseManager; verifyLogin/
// resetPasswordViaRecovery leem/escrevem o header direto no `.pbd`). NÃO
// testa I/O bruto de JSON (isso é test_user_account_repository) nem o
// framing do `.pbd` v2 em si (isso é test_vault_file/
// test_database_manager_vault_v2).
//
// Todo teste aqui que chama UserAccountService::createAdmin() grava a
// âncora do admin_pubkey no SecretStoreService (libsecret -> Secret
// Service via D-Bus). Em CI headless não há D-Bus session nem KWallet/
// GNOME Keyring — o mesmo achado documentado em test_tabshell_seguranca
// ("secret_password_{store,lookup}_sync crasha o processo" sem D-Bus). Por
// isso o guard fica em init() (roda antes de CADA teste desta suíte, não
// só de um específico como em test_tabshell_seguranca).
//
// T0 (Fatia 4, pré-requisito de infra): createAdmin()/completeFirstLogin()
// agora abrem/fecham o singleton DatabaseManager pra bootstrapar o `.pbd`
// v2 do cofre — sem isolar HOME/XDG_DATA_HOME (mesmo padrão de
// test_working_db_path.cpp), este teste escreveria no ambiente REAL de
// quem roda o ctest. cleanup() fecha o singleton defensivamente entre
// testes (reusado no processo inteiro da suíte).

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDateTime>
#include <QFile>

#include "services/UserAccountService.h"
#include "repositories/UserAccountRepository.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/crypto/VaultFile.h"
#include "core/database/DatabaseManager.h"
#include "core/secret/SecretStoreService.h"
#include "core/session/UserSession.h"
#include "core/settings/AppSettings.h"

class TestUserAccountService : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void slugifyEUserSessionDelegam();
    void createAdminEVerifyLoginERecovery();
    void fluxoInviteCompleto();
    void adminNaoObtemKDeTerceiroComSenhaErrada();
    void completeFirstLoginSemAdminOnline();
    void verifyLoginGatePorArquivo();
    void resetPasswordViaRecoverySemAdminOnlinePreservaInnerBlob();
    void removeUserRecusaUltimoAdmin();
    void usernameExistsPegaColisaoDeSlug();
    void pubkeySwapDetectado();

private:
    QString        m_appName; // nome único por-execução — isola do keychain real e de execuções anteriores
    QTemporaryDir *m_home = nullptr;
};

void TestUserAccountService::init()
{
    if (qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))
        QSKIP("sem Secret Service/D-Bus (libsecret) — UserAccountService::createAdmin toca "
              "o keychain real (âncora anti-pubkey-swap) — teste não roda em CI headless");

    m_appName = QStringLiteral("peubud-test-useracct-%1")
        .arg(QDateTime::currentMSecsSinceEpoch());

    // (T0, Fatia 4) createAdmin()/completeFirstLogin() agora abrem/fecham
    // DatabaseManager (bootstrapVault) -- isolar HOME/XDG_DATA_HOME em
    // QTemporaryDir, senão savePathFixo()/workingDbDir() apontam pro
    // ambiente real de quem roda o ctest.
    m_home = new QTemporaryDir();
    QVERIFY(m_home->isValid());
    qputenv("HOME", m_home->path().toUtf8());
    qputenv("XDG_DATA_HOME", (m_home->path() + QStringLiteral("/.local/share")).toUtf8());
}

void TestUserAccountService::cleanup()
{
    // (T0) Singleton reusado entre testes -- fecha defensivamente ANTES de
    // destruir o HOME isolado (um teste que falhou no meio, sem chamar
    // close(), vazaria header/chave pro próximo teste da suíte).
    DatabaseManager::instance().close();

    delete m_home;
    m_home = nullptr;

    if (m_appName.isEmpty())
        return; // teste skipado em init() — nada a limpar

    // Cada teste usa um appName único (timestamp), então isto não deveria
    // colidir com outra execução — mas remove mesmo assim, sem deixar
    // resíduo no keychain real do sistema que roda o teste.
    SecretStoreService secrets(m_appName);
    secrets.remove(QStringLiteral("__admin_pubkey__"));
}

// ── T1: AppSettings::slugify + delegações (UserSession, UserAccountService) ─

void TestUserAccountService::slugifyEUserSessionDelegam()
{
    QCOMPARE(AppSettings::slugify(QStringLiteral("João")), QStringLiteral("joao"));
    QCOMPARE(UserAccountService::slugify(QStringLiteral("João")), QStringLiteral("joao"));

    // UserSession::slug() delega à mesma fonte — regressão do bug "joo"
    // (implementação antiga não decompunha Unicode antes do filtro
    // [a-z0-9_], então "João" virava "joo" em vez de "joao").
    UserSession::instance().login(QStringLiteral("João"), QByteArray());
    QCOMPARE(UserSession::instance().slug(), QStringLiteral("joao"));
    UserSession::instance().logout();
}

// ── createAdmin -> verifyLogin Ok+Admin; recovery key recupera K ───────────

void TestUserAccountService::createAdminEVerifyLoginERecovery()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);
    UserAccountService svc(repo, crypto, secrets);

    QVERIFY(svc.isFirstRun());

    const auto created = svc.createAdmin(QStringLiteral("petrus"), QStringLiteral("SenhaForte123!"));
    QVERIFY2(created.ok, qPrintable(created.error));
    QCOMPARE(created.recoveryKeyPlaintext.size(), UserAccountService::kRecoveryKeyBytes);
    QVERIFY(!svc.isFirstRun());

    // (Fatia 4) o cofre do admin já existe e é v2 assim que createAdmin() retorna.
    const QString pbdPath = AppSettings::savePathFixo(UserAccountService::slugify(QStringLiteral("petrus")));
    QVERIFY(QFile::exists(pbdPath));

    const auto login = svc.verifyLogin(QStringLiteral("petrus"), QStringLiteral("SenhaForte123!"));
    QCOMPARE(static_cast<int>(login.status), static_cast<int>(UserAccountService::LoginResult::Ok));
    QVERIFY(login.role == Role::Admin);
    QCOMPARE(login.dataKey.size(), UserAccountService::kKeyBytes);

    // Recovery: instância NOVA do Service (sem sessão nenhuma) — o admin é
    // o próprio alvo, então a recovery key dele já basta pra auto-assinar
    // a regravação (via wrappedSignKeyByRecovery), sem depender de sessão.
    UserAccountService svcFresh(repo, crypto, secrets);
    const auto reset = svcFresh.resetPasswordViaRecovery(
        QStringLiteral("petrus"), created.recoveryKeyPlaintext, QStringLiteral("OutraSenhaForte456!"));
    QVERIFY2(reset.ok, qPrintable(reset.error));

    const auto loginNovo = svcFresh.verifyLogin(QStringLiteral("petrus"), QStringLiteral("OutraSenhaForte456!"));
    QCOMPARE(static_cast<int>(loginNovo.status), static_cast<int>(UserAccountService::LoginResult::Ok));
    QCOMPARE(loginNovo.dataKey, login.dataKey); // K preservado — recovery não rotaciona K
}

// ── Fluxo invite completo (happy path) ──────────────────────────────────────

void TestUserAccountService::fluxoInviteCompleto()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);
    UserAccountService svc(repo, crypto, secrets);

    QVERIFY(svc.createAdmin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).ok);
    QCOMPARE(static_cast<int>(svc.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).status),
             static_cast<int>(UserAccountService::LoginResult::Ok));

    const auto invited = svc.createUser(QStringLiteral("maria"), Role::User);
    QVERIFY2(invited.ok, qPrintable(invited.error));
    QCOMPARE(invited.inviteKeyPlaintext.size(), UserAccountService::kRecoveryKeyBytes);

    // (Fatia 4) completeFirstLogin() é 100% self-service — não depende mais
    // de m_adminSignKey, mesmo com svc ainda "logado" como admin (a chamada
    // funciona de qualquer instância).
    const auto firstLogin = svc.completeFirstLogin(
        QStringLiteral("maria"), invited.inviteKeyPlaintext, QStringLiteral("SenhaDaMaria789!"));
    QVERIFY2(firstLogin.ok, qPrintable(firstLogin.error));
    QCOMPARE(firstLogin.dataKey.size(), UserAccountService::kKeyBytes);
    QCOMPARE(firstLogin.recoveryKeyPlaintext.size(), UserAccountService::kRecoveryKeyBytes);
    QVERIFY2(firstLogin.recoveryKeyPlaintext != invited.inviteKeyPlaintext,
        "recovery key definitiva não deveria reusar o invite key (ADR-001 §3.5)");

    svc.clearSession();

    const auto mariaLogin = svc.verifyLogin(QStringLiteral("maria"), QStringLiteral("SenhaDaMaria789!"));
    QCOMPARE(static_cast<int>(mariaLogin.status), static_cast<int>(UserAccountService::LoginResult::Ok));
    QVERIFY(mariaLogin.role == Role::User);
    QCOMPARE(mariaLogin.dataKey, firstLogin.dataKey); // K do login == K do 1º login
}

// ── Admin NÃO obtém K de terceiro (senha errada de outro user falha) ───────

void TestUserAccountService::adminNaoObtemKDeTerceiroComSenhaErrada()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);
    UserAccountService svc(repo, crypto, secrets);

    QVERIFY(svc.createAdmin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).ok);
    QCOMPARE(static_cast<int>(svc.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).status),
             static_cast<int>(UserAccountService::LoginResult::Ok));

    const auto invited = svc.createUser(QStringLiteral("bob"), Role::User);
    QVERIFY2(invited.ok, qPrintable(invited.error));
    const auto firstLogin = svc.completeFirstLogin(
        QStringLiteral("bob"), invited.inviteKeyPlaintext, QStringLiteral("SenhaDoBob123!"));
    QVERIFY2(firstLogin.ok, qPrintable(firstLogin.error));
    QVERIFY(!firstLogin.dataKey.isEmpty());

    // Ninguém (nem o processo do admin) consegue K de "bob" sem a senha
    // real dele -- nenhum caminho de verifyLogin devolve dataKey != Ok.
    const auto tentativa = svc.verifyLogin(QStringLiteral("bob"), QStringLiteral("SenhaErrada999!"));
    QCOMPARE(static_cast<int>(tentativa.status), static_cast<int>(UserAccountService::LoginResult::WrongPassword));
    QVERIFY2(tentativa.dataKey.isEmpty(), "WrongPassword nunca deveria devolver dataKey preenchida");
}

// ── T6 (prova central #1): completeFirstLogin SEM admin online ─────────────

void TestUserAccountService::completeFirstLoginSemAdminOnline()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);

    UserAccountService svcAdmin(repo, crypto, secrets);
    QVERIFY(svcAdmin.createAdmin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).ok);
    QCOMPARE(static_cast<int>(svcAdmin.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).status),
             static_cast<int>(UserAccountService::LoginResult::Ok));

    const auto invited = svcAdmin.createUser(QStringLiteral("erika"), Role::User);
    QVERIFY2(invited.ok, qPrintable(invited.error));

    // Admin desloga -- prova central: completeFirstLogin() NÃO depende dele.
    svcAdmin.clearSession();

    // Instância NOVA, m_adminSignKey NUNCA preenchida nesta instância.
    UserAccountService svcSemSessao(repo, crypto, secrets);
    const auto firstLogin = svcSemSessao.completeFirstLogin(
        QStringLiteral("erika"), invited.inviteKeyPlaintext, QStringLiteral("SenhaDaErika789!"));
    QVERIFY2(firstLogin.ok, qPrintable(firstLogin.error));
    QCOMPARE(firstLogin.dataKey.size(), UserAccountService::kKeyBytes);

    const auto login = svcSemSessao.verifyLogin(QStringLiteral("erika"), QStringLiteral("SenhaDaErika789!"));
    QCOMPARE(static_cast<int>(login.status), static_cast<int>(UserAccountService::LoginResult::Ok));
    QCOMPARE(login.dataKey, firstLogin.dataKey);

    // Prova de segurança (D3, decisão do líder 2026-07-11): re-desembrulhar
    // wrappedMarkerByInvite com a invite key ORIGINAL prova posse do
    // convite, mas o valor obtido NUNCA é o K real -- o marcador é
    // criptograficamente desvinculado do K gerado no 1º login.
    const auto loaded = repo.loadAll();
    QVERIFY(loaded.ok);
    int idx = -1;
    for (int i = 0; i < loaded.users.size(); ++i) {
        if (loaded.users[i].username == QStringLiteral("erika")) { idx = i; break; }
    }
    QVERIFY2(idx >= 0, "registro de \"erika\" deveria continuar no users.json (vestigial)");

    const UserAccountRecord &erikaRec = loaded.users[idx];
    QByteArray kdfInvite = crypto.deriveKey(invited.inviteKeyPlaintext, erikaRec.saltInvite,
                                            static_cast<unsigned long long>(erikaRec.kdfOpsInvite),
                                            erikaRec.kdfMemInvite);
    const auto marker = crypto.unwrapKey(erikaRec.wrappedMarkerByInvite, kdfInvite);
    QVERIFY2(marker.has_value(), "invite key original ainda deveria desembrulhar o marcador (não expira)");
    QVERIFY2(*marker != firstLogin.dataKey,
        "marcador desembrulhado NUNCA deveria bater com o K real (D3, marcador descartável)");
}

// ── T7: verifyLogin — gate por EXISTÊNCIA DO ARQUIVO ────────────────────────

void TestUserAccountService::verifyLoginGatePorArquivo()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);

    UserAccountService svcAdmin(repo, crypto, secrets);
    QVERIFY(svcAdmin.createAdmin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).ok);
    QCOMPARE(static_cast<int>(svcAdmin.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).status),
             static_cast<int>(UserAccountService::LoginResult::Ok));

    const auto invited = svcAdmin.createUser(QStringLiteral("fabio"), Role::User);
    QVERIFY2(invited.ok, qPrintable(invited.error));

    // (b) sem cofre + invitePending true -> InvitePending.
    const auto antesDoFirstLogin = svcAdmin.verifyLogin(QStringLiteral("fabio"), QStringLiteral("qualquer"));
    QCOMPARE(static_cast<int>(antesDoFirstLogin.status), static_cast<int>(UserAccountService::LoginResult::InvitePending));

    svcAdmin.clearSession();
    UserAccountService svcSemSessao(repo, crypto, secrets);
    const auto firstLogin = svcSemSessao.completeFirstLogin(
        QStringLiteral("fabio"), invited.inviteKeyPlaintext, QStringLiteral("SenhaDoFabio789!"));
    QVERIFY2(firstLogin.ok, qPrintable(firstLogin.error));

    // (a) invitePending continua true (vestigial, D3 passo 10) no registro,
    // mas o cofre já existe -- existência do arquivo é autoritativa.
    const auto loadedPos = repo.loadAll();
    QVERIFY(loadedPos.ok);
    bool achouFabio = false;
    for (const auto &u : loadedPos.users) {
        if (u.username == QStringLiteral("fabio")) {
            QVERIFY2(u.invitePending, "invitePending deveria continuar true (vestigial) -- D3 passo 10");
            achouFabio = true;
        }
    }
    QVERIFY(achouFabio);

    const auto depoisDoFirstLogin = svcSemSessao.verifyLogin(QStringLiteral("fabio"), QStringLiteral("SenhaDoFabio789!"));
    QCOMPARE(static_cast<int>(depoisDoFirstLogin.status), static_cast<int>(UserAccountService::LoginResult::Ok));

    // (c) admin sem cofre -> IntegrityError (fail-secure; invitePending do
    // admin é sempre false, mas o gate por arquivo dispara igual).
    QVERIFY(QFile::remove(AppSettings::savePathFixo(UserAccountService::slugify(QStringLiteral("admin")))));
    const auto adminSemCofre = svcSemSessao.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!"));
    QCOMPARE(static_cast<int>(adminSemCofre.status), static_cast<int>(UserAccountService::LoginResult::IntegrityError));
}

// ── T8 (prova central #2): resetPasswordViaRecovery SEM admin online ───────

void TestUserAccountService::resetPasswordViaRecoverySemAdminOnlinePreservaInnerBlob()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);

    UserAccountService svc1(repo, crypto, secrets); // instância 1: cria admin+invite
    QVERIFY(svc1.createAdmin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).ok);
    QCOMPARE(static_cast<int>(svc1.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).status),
             static_cast<int>(UserAccountService::LoginResult::Ok));
    const auto invited = svc1.createUser(QStringLiteral("gabriela"), Role::User);
    QVERIFY2(invited.ok, qPrintable(invited.error));
    svc1.clearSession();

    UserAccountService svc2(repo, crypto, secrets); // instância 2: completa 1º login
    const auto firstLogin = svc2.completeFirstLogin(
        QStringLiteral("gabriela"), invited.inviteKeyPlaintext, QStringLiteral("SenhaDaGabriela789!"));
    QVERIFY2(firstLogin.ok, qPrintable(firstLogin.error));

    const QString vaultPath = AppSettings::savePathFixo(UserAccountService::slugify(QStringLiteral("gabriela")));
    QFile antes(vaultPath);
    QVERIFY(antes.open(QIODevice::ReadOnly));
    const QByteArray bytesAntes = antes.readAll();
    antes.close();
    const auto parsedAntes = VaultFile::parseV2(bytesAntes);
    QVERIFY(parsedAntes.ok);

    UserAccountService svc3(repo, crypto, secrets); // instância 3: reset via recovery, SEM sessão nenhuma
    const auto reset = svc3.resetPasswordViaRecovery(
        QStringLiteral("gabriela"), firstLogin.recoveryKeyPlaintext, QStringLiteral("SenhaNovaDaGabriela456!"));
    QVERIFY2(reset.ok, qPrintable(reset.error));

    QFile depois(vaultPath);
    QVERIFY(depois.open(QIODevice::ReadOnly));
    const QByteArray bytesDepois = depois.readAll();
    depois.close();
    const auto parsedDepois = VaultFile::parseV2(bytesDepois);
    QVERIFY(parsedDepois.ok);

    // Prova "não re-cifra" (D5 passo 7): innerBlob idêntico byte-a-byte
    // antes/depois -- só o header (saltPw/wrappedKeyByPass) mudou.
    QCOMPARE(parsedDepois.innerBlob, parsedAntes.innerBlob);
    QVERIFY2(parsedDepois.header.wrappedKeyByPass != parsedAntes.header.wrappedKeyByPass,
        "wrappedKeyByPass deveria mudar (nova senha)");
    QCOMPARE(parsedDepois.header.wrappedKeyByRecovery, parsedAntes.header.wrappedKeyByRecovery);
    QCOMPARE(parsedDepois.header.saltRk, parsedAntes.header.saltRk);

    const auto loginSenhaAntiga = svc3.verifyLogin(QStringLiteral("gabriela"), QStringLiteral("SenhaDaGabriela789!"));
    QCOMPARE(static_cast<int>(loginSenhaAntiga.status), static_cast<int>(UserAccountService::LoginResult::WrongPassword));

    const auto loginSenhaNova = svc3.verifyLogin(QStringLiteral("gabriela"), QStringLiteral("SenhaNovaDaGabriela456!"));
    QCOMPARE(static_cast<int>(loginSenhaNova.status), static_cast<int>(UserAccountService::LoginResult::Ok));
    QCOMPARE(loginSenhaNova.dataKey, firstLogin.dataKey); // mesmo K -- cofre idêntico antes/depois
}

// ── T9: removeUser recusa o último admin ────────────────────────────────

void TestUserAccountService::removeUserRecusaUltimoAdmin()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);
    UserAccountService svc(repo, crypto, secrets);

    QVERIFY(svc.createAdmin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).ok);
    QCOMPARE(static_cast<int>(svc.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).status),
             static_cast<int>(UserAccountService::LoginResult::Ok));

    QVERIFY2(!svc.removeUser(QStringLiteral("admin")), "não deveria permitir remover o único admin");
    QCOMPARE(svc.listUsers().size(), 1);

    const auto invited = svc.createUser(QStringLiteral("dave"), Role::User);
    QVERIFY2(invited.ok, qPrintable(invited.error));
    QVERIFY2(svc.removeUser(QStringLiteral("dave")), "usuário comum deveria poder ser removido");
    QCOMPARE(svc.listUsers().size(), 1);
}

// ── T9: usernameExists pega colisão de slug ("João" vs "joao") ─────────────

void TestUserAccountService::usernameExistsPegaColisaoDeSlug()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);
    UserAccountService svc(repo, crypto, secrets);

    QVERIFY(svc.createAdmin(QStringLiteral("João"), QStringLiteral("SenhaAdmin123!")).ok);

    QCOMPARE(UserAccountService::slugify(QStringLiteral("João")),
             UserAccountService::slugify(QStringLiteral("joao")));

    QVERIFY2(svc.usernameExists(QStringLiteral("joao")),
        "\"joao\" deveria colidir por slug normalizado com \"João\" já cadastrado");
    QVERIFY(svc.usernameExists(QStringLiteral("João"))); // igualdade exata
    QVERIFY(!svc.usernameExists(QStringLiteral("maria")));
}

// ── T9: pubkey-swap: troca o admin_pubkey no arquivo + re-assina com o novo
//      secret -> verifyLogin detecta via cruzamento com a âncora do keychain ──

void TestUserAccountService::pubkeySwapDetectado()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    SodiumCryptoService crypto;
    UserAccountRepository repo(crypto, tmp.filePath(QStringLiteral("users.json")));
    SecretStoreService secrets(m_appName);
    UserAccountService svc(repo, crypto, secrets);

    QVERIFY(svc.createAdmin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).ok);
    QCOMPARE(static_cast<int>(svc.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!")).status),
             static_cast<int>(UserAccountService::LoginResult::Ok));

    // Ataque CWE-269 avançado: troca o par Ed25519 INTEIRO e re-assina o
    // registro com o par NOVO. UserAccountRepository::loadAll() sozinho
    // aceitaria isso de bom grado (a assinatura bate com o admin_pubkey que
    // está NO PRÓPRIO ARQUIVO — auto-consistente). É exatamente o cenário
    // que a âncora no SecretStoreService (item E do brief) existe pra
    // fechar: o pubkey do arquivo precisa bater com a cópia gravada fora
    // do arquivo, no keychain do sistema.
    const auto loaded = repo.loadAll();
    QVERIFY(loaded.ok);
    const auto forjado = crypto.generateSignKeypair();
    QVERIFY(repo.saveAll(loaded.users, forjado.publicKey, forjado.secretKey));

    // Confirma que o ataque É indetectável só pelo repositório (baseline).
    const auto loadedPosAtaque = repo.loadAll();
    QVERIFY2(loadedPosAtaque.ok, "premissa do teste: o repo sozinho aceita o registro re-assinado (auto-consistente)");

    UserAccountService svcAtacado(repo, crypto, secrets); // instância nova, sem sessão prévia
    const auto login = svcAtacado.verifyLogin(QStringLiteral("admin"), QStringLiteral("SenhaAdmin123!"));
    QCOMPARE(static_cast<int>(login.status), static_cast<int>(UserAccountService::LoginResult::IntegrityError));
    QVERIFY(login.dataKey.isEmpty());
}

QTEST_GUILESS_MAIN(TestUserAccountService)
#include "test_user_account_service.moc"
