// PeuBud - TDD do LegacyVaultMigrationService (ADR-001 #212 §7, Fatia 5).
//
// Migração de cofres single-user PRÉ-ADR-001 (qualquer nome digitado no
// login criava um `.pbd` implicitamente, cifrado direto por Argon2id(senha)
// — v1-novo — ou, mais antigo, AES-256-CBC/PBKDF2(senha) — legado) para o
// modelo admin+envelope (`.pbd` v2). Prova central: o dado financeiro
// SOBREVIVE à migração nos dois formatos (v1-novo e legado), senha errada
// NÃO toca o `.pbd` original, e o fallback de slug antigo (D6, username
// acentuado) acha e migra corretamente, preservando o cofre antigo como
// `.bak`.
//
// Isolamento: HOME/XDG_DATA_HOME redirecionados para QTemporaryDir por
// teste (mesmo padrão de test_database_manager_vault_v2.cpp/
// test_user_account_service.cpp) -- sem isso, este teste escreveria
// `.pbd`/`.db`/users.json no ambiente REAL de quem roda o ctest. Todo teste
// aqui toca o keychain real (libsecret -> Secret Service via D-Bus) para o
// salt legado + a âncora anti-pubkey-swap -- QSKIP em CI headless sem
// D-Bus, mesmo achado documentado em test_tabshell_seguranca/
// test_user_account_service.

#include <QtTest/QtTest>

#include <QByteArray>
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>

#include "core/crypto/AesCryptoService.h"
#include "core/crypto/KeyDerivation.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/crypto/VaultFile.h"
#include "core/database/DatabaseManager.h"
#include "core/secret/SecretStoreService.h"
#include "core/settings/AppSettings.h"
#include "repositories/UserAccountRepository.h"
#include "services/LegacyVaultMigrationService.h"
#include "services/UserAccountService.h"

class TestLegacyVaultMigration : public QObject
{
    Q_OBJECT

private:
    QByteArray     m_prevHome;
    bool           m_prevHomeSet = false;
    QByteArray     m_prevXdgData;
    bool           m_prevXdgDataSet = false;
    QTemporaryDir *m_home = nullptr;
    QString        m_appName; // nome único por-teste — isola do keychain real

    // Fabrica bytes PLAINTEXT de um `.db` SQLite válido com a tabela
    // "cartoes" e 1 linha conhecida (`nomeCartao`) — usado para fabricar os
    // dois formatos legados (v1-novo Argon2id e legado AES-CBC), cifrados
    // de formas diferentes por cada teste. Retorna QByteArray vazio em
    // qualquer falha (caller faz QVERIFY(!bytes.isEmpty())).
    static QByteArray fabricarDbPlaintextComCartao(const QString &nomeCartao)
    {
        QTemporaryDir tmp;
        if (!tmp.isValid())
            return {};
        const QString dbFile = tmp.filePath(QStringLiteral("fixture.db"));
        const QString connName = QStringLiteral("legacymig_fixture_%1")
            .arg(QDateTime::currentMSecsSinceEpoch());

        {
            QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
            db.setDatabaseName(dbFile);
            if (!db.open()) {
                QSqlDatabase::removeDatabase(connName);
                return {};
            }
            QSqlQuery create(db);
            if (!create.exec(QStringLiteral(
                    "CREATE TABLE cartoes (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "apelido TEXT NOT NULL UNIQUE)"))) {
                db.close();
                QSqlDatabase::removeDatabase(connName);
                return {};
            }
            QSqlQuery ins(db);
            ins.prepare(QStringLiteral("INSERT INTO cartoes (apelido) VALUES (?)"));
            ins.addBindValue(nomeCartao);
            if (!ins.exec()) {
                db.close();
                QSqlDatabase::removeDatabase(connName);
                return {};
            }
            db.close();
        }
        QSqlDatabase::removeDatabase(connName);

        QFile f(dbFile);
        if (!f.open(QIODevice::ReadOnly))
            return {};
        const QByteArray bytes = f.readAll();
        f.close();
        return bytes;
    }

    // Réplica do algoritmo de slug PRÉ-Fatia-4 (mesma cópia que
    // LegacyVaultMigrationService.cpp usa internamente) — só para o teste
    // de fallback montar a fixture no caminho ANTIGO.
    static QString slugAntigoLocal(const QString &username)
    {
        return username.toLower()
                       .replace(QLatin1Char(' '), QLatin1Char('_'))
                       .remove(QRegularExpression(QStringLiteral("[^a-z0-9_]")));
    }

private slots:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(QStringLiteral("petrinhuTest"));
        QCoreApplication::setApplicationName(QStringLiteral("PeuBudTest"));
        m_prevHome       = qgetenv("HOME");
        m_prevHomeSet    = qEnvironmentVariableIsSet("HOME");
        m_prevXdgData    = qgetenv("XDG_DATA_HOME");
        m_prevXdgDataSet = qEnvironmentVariableIsSet("XDG_DATA_HOME");
    }

    void cleanupTestCase()
    {
        if (m_prevHomeSet) qputenv("HOME", m_prevHome); else qunsetenv("HOME");
        if (m_prevXdgDataSet) qputenv("XDG_DATA_HOME", m_prevXdgData); else qunsetenv("XDG_DATA_HOME");
    }

    void init()
    {
        if (qEnvironmentVariableIsEmpty("DBUS_SESSION_BUS_ADDRESS"))
            QSKIP("sem Secret Service/D-Bus (libsecret) — LegacyVaultMigrationService toca "
                  "o keychain real (salt legado + âncora anti-pubkey-swap) — teste não "
                  "roda em CI headless");

        m_appName = QStringLiteral("peubud-test-legacymig-%1")
            .arg(QDateTime::currentMSecsSinceEpoch());

        m_home = new QTemporaryDir();
        QVERIFY(m_home->isValid());
        qputenv("HOME", m_home->path().toUtf8());
        qputenv("XDG_DATA_HOME", (m_home->path() + QStringLiteral("/.local/share")).toUtf8());
    }

    void cleanup()
    {
        // Singleton reusado entre testes -- fecha defensivamente ANTES de
        // destruir o HOME isolado (mesmo padrão de test_database_manager_
        // vault_v2.cpp/test_user_account_service.cpp).
        DatabaseManager::instance().close();
        delete m_home;
        m_home = nullptr;

        if (m_appName.isEmpty())
            return; // teste skipado em init() — nada a limpar

        SecretStoreService secrets(m_appName);
        secrets.remove(QStringLiteral("__admin_pubkey__"));
        for (const QString &u : {QStringLiteral("petrus"), QStringLiteral("carol"),
                                  QStringLiteral("bob"), QStringLiteral("joão"),
                                  QStringLiteral("diana")}) {
            secrets.remove(u.toLower() + QStringLiteral("/salt"));
        }
    }

    // ── 1: .pbd v1-novo (Argon2id) migra e preserva o dado ─────────────────

    void v1NovoArgon2id_migraEPreservaDado()
    {
        const QString username = QStringLiteral("petrus");
        const QString password = QStringLiteral("SenhaAntiga1!"); // senha LEGADA — não passa
                                                                    // pelo mínimo de 12 (ADR-001
                                                                    // §2) porque é reusada, não nova
        const QString slug = UserAccountService::slugify(username);

        SodiumCryptoService crypto;
        const QByteArray salt = crypto.generateSalt();
        SecretStoreService secrets(m_appName);
        QVERIFY(secrets.store(username.toLower() + QStringLiteral("/salt"), salt));

        // Mesma derivação que LoginDialog::onEntrar faz para o candidato v1-novo.
        const QByteArray key = crypto.deriveKey(password, salt);

        const QByteArray plainDb = fabricarDbPlaintextComCartao(QStringLiteral("BancoAntigoV1"));
        QVERIFY(!plainDb.isEmpty());
        const QByteArray cipher = crypto.encrypt(plainDb, key);
        QVERIFY(SodiumCryptoService::isNewFormat(cipher));
        QVERIFY(!VaultFile::isV2(cipher));

        const QString pbdPath = AppSettings::savePathFixo(slug);
        QFile fOut(pbdPath);
        QVERIFY(fOut.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(fOut.write(cipher), static_cast<qint64>(cipher.size()));
        fOut.close();

        QTemporaryDir tmpReg;
        QVERIFY(tmpReg.isValid());
        UserAccountRepository repo(crypto, tmpReg.filePath(QStringLiteral("users.json")));
        UserAccountService accounts(repo, crypto, secrets);
        QVERIFY(accounts.isFirstRun());

        LegacyVaultMigrationService migration(accounts, crypto, secrets);
        QVERIFY(migration.legacyVaultPresente(username));

        const auto result = migration.migrarParaAdmin(username, password);
        QVERIFY2(result.ok, qPrintable(result.error));
        QCOMPARE(result.recoveryKeyPlaintext.size(), UserAccountService::kRecoveryKeyBytes);
        QVERIFY(!accounts.isFirstRun());

        QFile fIn(pbdPath);
        QVERIFY(fIn.open(QIODevice::ReadOnly));
        const QByteArray bytesPos = fIn.readAll();
        fIn.close();
        QVERIFY2(VaultFile::isV2(bytesPos), "migração deveria ter gravado .pbd v2");

        // Login pós-migração com a MESMA senha antiga -> Ok + Admin, e o
        // dado gravado ANTES da migração sobrevive.
        const auto login = accounts.verifyLogin(username, password);
        QCOMPARE(static_cast<int>(login.status), static_cast<int>(UserAccountService::LoginResult::Ok));
        QVERIFY(login.role == Role::Admin);

        QVERIFY(DatabaseManager::instance().open(slug, login.dataKey));
        QSqlQuery sel(DatabaseManager::instance().db());
        QVERIFY2(sel.exec(QStringLiteral("SELECT apelido FROM cartoes")), qPrintable(sel.lastError().text()));
        QVERIFY(sel.next());
        QCOMPARE(sel.value(0).toString(), QStringLiteral("BancoAntigoV1"));
        QVERIFY(!sel.next()); // só 1 linha — dado não duplicado/corrompido
        QVERIFY(DatabaseManager::instance().close());
    }

    // ── 2: .pbd legado AES-CBC/PBKDF2 migra e preserva o dado ──────────────

    void legadoAesCbc_migraEPreservaDado()
    {
        const QString username = QStringLiteral("carol");
        const QString password = QStringLiteral("SenhaLegado123!");
        const QString slug = UserAccountService::slugify(username);

        const QByteArray salt = KeyDerivation::generateSalt(); // 16B — compatível com os 2 formatos
        SecretStoreService secrets(m_appName);
        QVERIFY(secrets.store(username.toLower() + QStringLiteral("/salt"), salt));

        const QByteArray keyLegacy = KeyDerivation::deriveKey(password, salt); // PBKDF2-SHA256, 600k iter

        const QByteArray plainDb = fabricarDbPlaintextComCartao(QStringLiteral("LegacyBank"));
        QVERIFY(!plainDb.isEmpty());
        AesCryptoService legacyCrypto;
        const QByteArray cipher = legacyCrypto.encrypt(plainDb, keyLegacy);
        QVERIFY(!VaultFile::isV2(cipher));
        QVERIFY2(!SodiumCryptoService::isNewFormat(cipher), "fixture deveria ser legado (sem magic)");

        const QString pbdPath = AppSettings::savePathFixo(slug);
        QFile fOut(pbdPath);
        QVERIFY(fOut.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(fOut.write(cipher), static_cast<qint64>(cipher.size()));
        fOut.close();

        SodiumCryptoService crypto;
        QTemporaryDir tmpReg;
        QVERIFY(tmpReg.isValid());
        UserAccountRepository repo(crypto, tmpReg.filePath(QStringLiteral("users.json")));
        UserAccountService accounts(repo, crypto, secrets);
        LegacyVaultMigrationService migration(accounts, crypto, secrets);

        QVERIFY(migration.legacyVaultPresente(username));
        const auto result = migration.migrarParaAdmin(username, password);
        QVERIFY2(result.ok, qPrintable(result.error));

        QFile fIn(pbdPath);
        QVERIFY(fIn.open(QIODevice::ReadOnly));
        const QByteArray bytesPos = fIn.readAll();
        fIn.close();
        QVERIFY2(VaultFile::isV2(bytesPos), "migração deveria ter gravado .pbd v2");

        const auto login = accounts.verifyLogin(username, password);
        QCOMPARE(static_cast<int>(login.status), static_cast<int>(UserAccountService::LoginResult::Ok));
        QVERIFY(login.role == Role::Admin);

        QVERIFY(DatabaseManager::instance().open(slug, login.dataKey));
        QSqlQuery sel(DatabaseManager::instance().db());
        QVERIFY2(sel.exec(QStringLiteral("SELECT apelido FROM cartoes")), qPrintable(sel.lastError().text()));
        QVERIFY(sel.next());
        QCOMPARE(sel.value(0).toString(), QStringLiteral("LegacyBank"));
        QVERIFY(!sel.next());
        QVERIFY(DatabaseManager::instance().close());
    }

    // ── 3: senha errada -> não migra, .pbd original intacto ────────────────

    void senhaErrada_naoMigraENaoCorrompeOriginal()
    {
        const QString username = QStringLiteral("bob");
        const QString passwordCerta = QStringLiteral("SenhaCerta123!");
        const QString slug = UserAccountService::slugify(username);

        SodiumCryptoService crypto;
        const QByteArray salt = crypto.generateSalt();
        SecretStoreService secrets(m_appName);
        QVERIFY(secrets.store(username.toLower() + QStringLiteral("/salt"), salt));
        const QByteArray key = crypto.deriveKey(passwordCerta, salt);

        const QByteArray plainDb = fabricarDbPlaintextComCartao(QStringLiteral("Original"));
        QVERIFY(!plainDb.isEmpty());
        const QByteArray cipher = crypto.encrypt(plainDb, key);

        const QString pbdPath = AppSettings::savePathFixo(slug);
        QFile fOut(pbdPath);
        QVERIFY(fOut.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(fOut.write(cipher), static_cast<qint64>(cipher.size()));
        fOut.close();

        QTemporaryDir tmpReg;
        QVERIFY(tmpReg.isValid());
        UserAccountRepository repo(crypto, tmpReg.filePath(QStringLiteral("users.json")));
        UserAccountService accounts(repo, crypto, secrets);
        LegacyVaultMigrationService migration(accounts, crypto, secrets);

        const auto result = migration.migrarParaAdmin(username, QStringLiteral("SenhaErrada999!"));
        QVERIFY(!result.ok);
        QVERIFY(!result.error.isEmpty());
        QVERIFY(result.recoveryKeyPlaintext.isEmpty());
        QVERIFY(accounts.isFirstRun()); // registro continua vazio — ninguém foi promovido a admin
        QVERIFY(!DatabaseManager::instance().isOpen()); // sessão não ficou pendurada aberta

        QFile fIn(pbdPath);
        QVERIFY(fIn.open(QIODevice::ReadOnly));
        const QByteArray bytesDepois = fIn.readAll();
        fIn.close();
        QCOMPARE(bytesDepois, cipher); // byte-a-byte intacto — não migrou, não corrompeu
        QVERIFY(!VaultFile::isV2(bytesDepois));
        QVERIFY2(SodiumCryptoService::isNewFormat(bytesDepois), "continua v1-novo — não virou v2");
    }

    // ── 4: legacyVaultPresente — true com .pbd + registro vazio, false com admin ──

    void legacyVaultPresente_trueComPbdERegistroVazio_falseComAdmin()
    {
        const QString username = QStringLiteral("diana");
        const QString slug = UserAccountService::slugify(username);

        SodiumCryptoService crypto;
        SecretStoreService secrets(m_appName);

        QTemporaryDir tmpReg;
        QVERIFY(tmpReg.isValid());
        UserAccountRepository repo(crypto, tmpReg.filePath(QStringLiteral("users.json")));
        UserAccountService accounts(repo, crypto, secrets);
        LegacyVaultMigrationService migration(accounts, crypto, secrets);

        // Sem .pbd nenhum -> false, mesmo com registro vazio.
        QVERIFY(!migration.legacyVaultPresente(username));

        // Cria um .pbd qualquer no caminho fixo do slug (conteúdo não
        // importa para este teste — legacyVaultPresente só faz QFile::exists).
        const QString pbdPath = AppSettings::savePathFixo(slug);
        QFile fOut(pbdPath);
        QVERIFY(fOut.open(QIODevice::WriteOnly | QIODevice::Truncate));
        fOut.write(QByteArrayLiteral("qualquer coisa"));
        fOut.close();

        QString slugAchado;
        QVERIFY(migration.legacyVaultPresente(username, &slugAchado));
        QCOMPARE(slugAchado, slug);

        // Cria um admin de verdade (usuário DIFERENTE) -> registro deixa de
        // estar vazio -> legacyVaultPresente("diana") vira false mesmo com
        // o .pbd dela ainda lá (gate é por isFirstRun(), não por usuário).
        QVERIFY2(accounts.createAdmin(QStringLiteral("outraconta"), QStringLiteral("SenhaForte123!")).ok,
                 "setup do teste: createAdmin deveria ter funcionado");
        QVERIFY(!migration.legacyVaultPresente(username));
    }

    // ── 5: fallback de slug antigo (D6) — username acentuado ───────────────

    void slugAntigoFallback_achaEMigraCofreAcentuado()
    {
        const QString username = QStringLiteral("João");
        const QString password = QStringLiteral("SenhaDoJoao1!");
        const QString slugNovo = UserAccountService::slugify(username);
        QCOMPARE(slugNovo, QStringLiteral("joao"));
        const QString slugVelho = slugAntigoLocal(username);
        QCOMPARE(slugVelho, QStringLiteral("joo")); // regressão do bug documentado no ADR-001 §6/D6
        QVERIFY(slugVelho != slugNovo);

        SodiumCryptoService crypto;
        const QByteArray salt = crypto.generateSalt();
        SecretStoreService secrets(m_appName);
        QVERIFY(secrets.store(username.toLower() + QStringLiteral("/salt"), salt));
        const QByteArray key = crypto.deriveKey(password, salt);

        const QByteArray plainDb = fabricarDbPlaintextComCartao(QStringLiteral("CofreDoJoao"));
        QVERIFY(!plainDb.isEmpty());
        const QByteArray cipher = crypto.encrypt(plainDb, key);

        // Grava sob o caminho FIXO do slug ANTIGO (pré-Fatia-4, sem NFD) —
        // simula uma instalação que já existia antes da correção do bug.
        const QString pbdPathAntigo = AppSettings::savePathFixo(slugVelho);
        QFile fOut(pbdPathAntigo);
        QVERIFY(fOut.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(fOut.write(cipher), static_cast<qint64>(cipher.size()));
        fOut.close();
        QVERIFY(!QFile::exists(AppSettings::savePathFixo(slugNovo))); // pré-condição do fallback

        QTemporaryDir tmpReg;
        QVERIFY(tmpReg.isValid());
        UserAccountRepository repo(crypto, tmpReg.filePath(QStringLiteral("users.json")));
        UserAccountService accounts(repo, crypto, secrets);
        LegacyVaultMigrationService migration(accounts, crypto, secrets);

        QString slugAchado;
        QVERIFY(migration.legacyVaultPresente(username, &slugAchado));
        QCOMPARE(slugAchado, slugVelho);

        const auto result = migration.migrarParaAdmin(username, password);
        QVERIFY2(result.ok, qPrintable(result.error));

        // v2 materializado no slug NOVO; o antigo virou .bak (nunca deletado).
        QVERIFY(QFile::exists(AppSettings::savePathFixo(slugNovo)));
        QVERIFY2(QFile::exists(pbdPathAntigo + QStringLiteral(".bak")),
                  "cofre do slug antigo deveria ter sido preservado como .bak");
        QVERIFY(!QFile::exists(pbdPathAntigo)); // saiu do caminho antigo (renomeado p/ .bak)

        QFile fIn(AppSettings::savePathFixo(slugNovo));
        QVERIFY(fIn.open(QIODevice::ReadOnly));
        const QByteArray bytesPos = fIn.readAll();
        fIn.close();
        QVERIFY2(VaultFile::isV2(bytesPos), "migração deveria ter gravado .pbd v2 no slug NOVO");

        const auto login = accounts.verifyLogin(username, password);
        QCOMPARE(static_cast<int>(login.status), static_cast<int>(UserAccountService::LoginResult::Ok));

        QVERIFY(DatabaseManager::instance().open(slugNovo, login.dataKey));
        QSqlQuery sel(DatabaseManager::instance().db());
        QVERIFY2(sel.exec(QStringLiteral("SELECT apelido FROM cartoes")), qPrintable(sel.lastError().text()));
        QVERIFY(sel.next());
        QCOMPARE(sel.value(0).toString(), QStringLiteral("CofreDoJoao"));
        QVERIFY(!sel.next());
        QVERIFY(DatabaseManager::instance().close());
    }
};

QTEST_GUILESS_MAIN(TestLegacyVaultMigration)
#include "test_legacy_vault_migration.moc"
