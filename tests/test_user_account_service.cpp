// PeuBud - TDD do UserAccountService (ADR-001 #212, Fatia 2).
// Regra de negócio de gestão de usuários: envelope encryption, invite-key,
// recovery key, sessão de assinatura do admin, e a âncora anti-pubkey-swap
// no SecretStoreService (item E do brief da tarefa). NÃO testa I/O bruto de
// JSON (isso é test_user_account_repository, Fatia 1).
//
// Todo teste aqui chama UserAccountService::createAdmin(), que grava a
// âncora do admin_pubkey no SecretStoreService (libsecret -> Secret
// Service via D-Bus). Em CI headless não há D-Bus session nem KWallet/
// GNOME Keyring — o mesmo achado documentado em test_tabshell_seguranca
// ("secret_password_{store,lookup}_sync crasha o processo" sem D-Bus). Por
// isso o guard fica em init() (roda antes de CADA teste desta suíte, não
// só de um específico como em test_tabshell_seguranca).

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QDateTime>

#include "services/UserAccountService.h"
#include "repositories/UserAccountRepository.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/secret/SecretStoreService.h"

class TestUserAccountService : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void createAdminEVerifyLoginERecovery();
    void fluxoInviteCompleto();
    void adminNaoObtemKDeTerceiroComSenhaErrada();
    void resetPasswordViaRecoveryPreservaK();
    void removeUserRecusaUltimoAdmin();
    void usernameExistsPegaColisaoDeSlug();
    void pubkeySwapDetectado();

private:
    QString m_appName; // nome único por-execução — isola do keychain real e de execuções anteriores
};

void TestUserAccountService::init()
{
    if (qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))
        QSKIP("sem Secret Service/D-Bus (libsecret) — UserAccountService::createAdmin toca "
              "o keychain real (âncora anti-pubkey-swap) — teste não roda em CI headless");

    m_appName = QStringLiteral("peubud-test-useracct-%1")
        .arg(QDateTime::currentMSecsSinceEpoch());
}

void TestUserAccountService::cleanup()
{
    if (m_appName.isEmpty())
        return; // teste skipado em init() — nada a limpar

    // Cada teste usa um appName único (timestamp), então isto não deveria
    // colidir com outra execução — mas remove mesmo assim, sem deixar
    // resíduo no keychain real do sistema que roda o teste.
    SecretStoreService secrets(m_appName);
    secrets.remove(QStringLiteral("__admin_pubkey__"));
}

// ── 1. createAdmin -> verifyLogin Ok+Admin; recovery key recupera K ────────

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

// ── 2. Fluxo invite completo ────────────────────────────────────────────

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

    // (Limitação temporária desta fatia — ver nota de design no topo de
    // UserAccountService.h) completeFirstLogin() precisa de uma sessão com
    // assinatura disponível; svc ainda tem m_adminSignKey cacheada (nenhum
    // clearSession() chamado ainda) — simula "mesma sessão de processo",
    // cenário plausível (telas de admin+onboarding sem reiniciar o app).
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

// ── 3. Admin NÃO obtém K de terceiro (senha errada de outro user falha) ────

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

// ── 4. resetPasswordViaRecovery preserva K ──────────────────────────────

void TestUserAccountService::resetPasswordViaRecoveryPreservaK()
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

    const auto invited = svc.createUser(QStringLiteral("carla"), Role::User);
    QVERIFY2(invited.ok, qPrintable(invited.error));
    const auto firstLogin = svc.completeFirstLogin(
        QStringLiteral("carla"), invited.inviteKeyPlaintext, QStringLiteral("SenhaDaCarla123!"));
    QVERIFY2(firstLogin.ok, qPrintable(firstLogin.error));

    // svc ainda tem a sessão do admin cacheada -- necessário pra
    // resetPasswordViaRecovery de usuário COMUM nesta fatia (ver nota de
    // design; admin recuperando a PRÓPRIA senha, teste 1, é auto-suficiente).
    const auto reset = svc.resetPasswordViaRecovery(
        QStringLiteral("carla"), firstLogin.recoveryKeyPlaintext, QStringLiteral("SenhaNovaDaCarla456!"));
    QVERIFY2(reset.ok, qPrintable(reset.error));

    const auto loginSenhaAntiga = svc.verifyLogin(QStringLiteral("carla"), QStringLiteral("SenhaDaCarla123!"));
    QCOMPARE(static_cast<int>(loginSenhaAntiga.status), static_cast<int>(UserAccountService::LoginResult::WrongPassword));

    const auto loginSenhaNova = svc.verifyLogin(QStringLiteral("carla"), QStringLiteral("SenhaNovaDaCarla456!"));
    QCOMPARE(static_cast<int>(loginSenhaNova.status), static_cast<int>(UserAccountService::LoginResult::Ok));
    QCOMPARE(loginSenhaNova.dataKey, firstLogin.dataKey); // mesmo K -- cofre idêntico antes/depois
}

// ── 5. removeUser recusa o último admin ─────────────────────────────────

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

// ── 6. usernameExists pega colisão de slug ("João" vs "joao") ──────────────

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

// ── 7. pubkey-swap: troca o admin_pubkey no arquivo + re-assina com o novo
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
