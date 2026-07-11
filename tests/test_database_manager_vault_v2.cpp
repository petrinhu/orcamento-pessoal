// PeuBud - ADR-001 #212 (Fatia 4, T3): DatabaseManager v2.
//
// setVaultHeader()/vaultHeader()/vaultIsV2() + open() com autodetect
// (V2/V1/legado AES-CBC) + encryptAndSave() envelopando em `.pbd` v2 +
// close() resetando o header (singleton reusado entre logins).
//
// Provas (spec docs/design/fatia4-integracao-vault-v2.md, T3):
//   (a) setVaultHeader + open(inexistente) + close() -> magic PBD\x02,
//       header recuperável do arquivo.
//   (b) reabertura com o mesmo K -> dado E header intactos (sem chamar
//       setVaultHeader() de novo -- open() restaura do próprio arquivo).
//   (c) `.pbd` v1 (sem header v2) continua abrindo SEM REGRESSÃO.
//   (d) `.pbd` legado (AES-256-CBC/PBKDF2) continua abrindo via keyLegacy
//       SEM REGRESSÃO -- migração transparente pro formato novo no close().
//
// Isolamento: HOME/XDG_DATA_HOME redirecionados para QTemporaryDir por
// teste (mesmo padrão de test_working_db_path.cpp) -- sem isso, este teste
// escreveria `.pbd`/`.db` no ambiente REAL de quem roda o ctest. cleanup()
// fecha o singleton defensivamente entre testes.

#include <QtTest/QtTest>

#include <QByteArray>
#include <QCoreApplication>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>

#include "core/crypto/AesCryptoService.h"
#include "core/crypto/KeyDerivation.h"
#include "core/crypto/SodiumCryptoService.h"
#include "core/crypto/VaultFile.h"
#include "core/database/DatabaseManager.h"
#include "core/settings/AppSettings.h"

class TestDatabaseManagerVaultV2 : public QObject
{
    Q_OBJECT

private:
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;
    QByteArray    m_prevXdgData;
    bool          m_prevXdgDataSet = false;
    QTemporaryDir *m_home = nullptr;

    static VaultHeaderData amostraHeader(SodiumCryptoService &crypto, const QString &username,
                                          const QByteArray &K)
    {
        VaultHeaderData hdr;
        hdr.username = username;
        hdr.saltPw   = crypto.randomBytes(16);
        hdr.saltRk   = crypto.randomBytes(16);
        hdr.kdfOps   = 3;
        hdr.kdfMem   = 268435456; // 256 MB — MODERATE
        // wraps arbitrários (só precisam existir/round-tripar; não são
        // desembrulhados neste teste — isso é responsabilidade de
        // UserAccountService, testado em test_user_account_service.cpp).
        hdr.wrappedKeyByPass     = crypto.wrapKey(K, crypto.randomBytes(32));
        hdr.wrappedKeyByRecovery = crypto.wrapKey(K, crypto.randomBytes(32));
        return hdr;
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
        m_home = new QTemporaryDir();
        QVERIFY(m_home->isValid());
        qputenv("HOME", m_home->path().toUtf8());
        qputenv("XDG_DATA_HOME", (m_home->path() + QStringLiteral("/.local/share")).toUtf8());
    }

    void cleanup()
    {
        // (T0) Singleton reusado entre testes -- fecha defensivamente antes
        // de destruir o HOME isolado, senão um teste que falhou no meio
        // (sem chamar close()) vaza o header/chave pro próximo teste.
        DatabaseManager::instance().close();
        delete m_home;
        m_home = nullptr;
    }

    // ── (a) setVaultHeader + open(inexistente) + close() -> magic v2 ───────

    void setVaultHeaderOpenInexistenteClose_geraMagicV2ComHeaderRecuperavel()
    {
        SodiumCryptoService crypto;
        const QByteArray K = crypto.randomBytes(32);
        const VaultHeaderData hdr = amostraHeader(crypto, QStringLiteral("alice"), K);

        DatabaseManager::instance().setVaultHeader(hdr);
        QVERIFY(DatabaseManager::instance().vaultIsV2());
        QCOMPARE(DatabaseManager::instance().vaultHeader().username, hdr.username);

        const QString slug = QStringLiteral("alice");
        QVERIFY(!QFile::exists(AppSettings::savePathFixo(slug))); // pré-condição: cofre NÃO existe
        QVERIFY(DatabaseManager::instance().open(slug, K));
        QVERIFY(DatabaseManager::instance().isOpen());
        // open() num arquivo inexistente NÃO toca m_vaultHeader/m_vaultIsV2
        // (D4) — continua primed com o que setVaultHeader() injetou.
        QVERIFY(DatabaseManager::instance().vaultIsV2());
        QVERIFY(DatabaseManager::instance().close());

        const QString pbdPath = AppSettings::savePathFixo(slug);
        QVERIFY(QFile::exists(pbdPath));

        QFile f(pbdPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray bytes = f.readAll();
        f.close();

        QVERIFY2(VaultFile::isV2(bytes), "close() deveria ter gravado magic PBD\\x02");
        const auto parsed = VaultFile::parseV2(bytes);
        QVERIFY(parsed.ok);
        QCOMPARE(parsed.header.username, hdr.username);
        QCOMPARE(parsed.header.saltPw, hdr.saltPw);
        QCOMPARE(parsed.header.saltRk, hdr.saltRk);
        QCOMPARE(parsed.header.kdfOps, hdr.kdfOps);
        QCOMPARE(static_cast<qint64>(parsed.header.kdfMem), static_cast<qint64>(hdr.kdfMem));
        QCOMPARE(parsed.header.wrappedKeyByPass, hdr.wrappedKeyByPass);
        QCOMPARE(parsed.header.wrappedKeyByRecovery, hdr.wrappedKeyByRecovery);

        // close() reseta o header do singleton (D4).
        QVERIFY(!DatabaseManager::instance().vaultIsV2());
    }

    // ── (b) reabrir com o mesmo K -> dado E header intactos ─────────────────

    void reabreComMesmoK_dadoEHeaderIntactos()
    {
        SodiumCryptoService crypto;
        const QByteArray K = crypto.randomBytes(32);
        const VaultHeaderData hdr = amostraHeader(crypto, QStringLiteral("bob"), K);
        const QString slug = QStringLiteral("bob");

        DatabaseManager::instance().setVaultHeader(hdr);
        QVERIFY(DatabaseManager::instance().open(slug, K));
        {
            QSqlQuery ins(DatabaseManager::instance().db());
            QVERIFY2(ins.exec(QStringLiteral("INSERT INTO cartoes (apelido) VALUES ('Nubank')")),
                     qPrintable(ins.lastError().text()));
        }
        QVERIFY(DatabaseManager::instance().close());

        // Reabre SEM chamar setVaultHeader() de novo — open() precisa
        // restaurar o header do PRÓPRIO ARQUIVO (D4).
        QVERIFY(DatabaseManager::instance().open(slug, K));
        QVERIFY(DatabaseManager::instance().vaultIsV2());
        QCOMPARE(DatabaseManager::instance().vaultHeader().username, hdr.username);
        QCOMPARE(DatabaseManager::instance().vaultHeader().wrappedKeyByPass, hdr.wrappedKeyByPass);

        QSqlQuery sel(DatabaseManager::instance().db());
        QVERIFY(sel.exec(QStringLiteral("SELECT apelido FROM cartoes")));
        QVERIFY(sel.next());
        QCOMPARE(sel.value(0).toString(), QStringLiteral("Nubank"));
        QVERIFY(!sel.next()); // só 1 linha — dado não duplicado/corrompido

        QVERIFY(DatabaseManager::instance().close());
    }

    // ── (c) .pbd v1 existente abre SEM REGRESSÃO ────────────────────────────

    void pbdV1Existente_abreSemRegressao()
    {
        SodiumCryptoService crypto;
        const QByteArray key = crypto.randomBytes(32);
        const QString slug = QStringLiteral("carol");

        // Sem setVaultHeader() -> grava v1 puro (m_vaultIsV2 default false).
        QVERIFY(DatabaseManager::instance().open(slug, key));
        QVERIFY(!DatabaseManager::instance().vaultIsV2());
        {
            QSqlQuery ins(DatabaseManager::instance().db());
            QVERIFY2(ins.exec(QStringLiteral("INSERT INTO cartoes (apelido) VALUES ('Itau')")),
                     qPrintable(ins.lastError().text()));
        }
        QVERIFY(DatabaseManager::instance().close());

        const QString pbdPath = AppSettings::savePathFixo(slug);
        QFile f(pbdPath);
        QVERIFY(f.open(QIODevice::ReadOnly));
        const QByteArray bytes = f.readAll();
        f.close();
        QVERIFY(!VaultFile::isV2(bytes));
        QVERIFY2(SodiumCryptoService::isNewFormat(bytes), "deveria ser v1 (magic PBD\\x01)");

        // Reabre — branch V1/legado de open() precisa resetar
        // m_vaultHeader/m_vaultIsV2 (D4), não vazar de um teste anterior.
        QVERIFY(DatabaseManager::instance().open(slug, key));
        QVERIFY(!DatabaseManager::instance().vaultIsV2());

        QSqlQuery sel(DatabaseManager::instance().db());
        QVERIFY(sel.exec(QStringLiteral("SELECT apelido FROM cartoes")));
        QVERIFY(sel.next());
        QCOMPARE(sel.value(0).toString(), QStringLiteral("Itau"));

        QVERIFY(DatabaseManager::instance().close());
    }

    // ── (d) .pbd legado (AES-256-CBC/PBKDF2) abre via keyLegacy SEM REGRESSÃO ──

    void pbdLegadoAesCbc_abreViaKeyLegacySemRegressaoEMigraTransparente()
    {
        const QString slug = QStringLiteral("dave");
        const QString pbdPath = AppSettings::savePathFixo(slug); // mkpath da pasta-pai

        // Fabrica um `.pbd` legado real: AES-256-CBC + PBKDF2-SHA256 (o
        // formato pré-Sodium, produzido hoje só por instalações antigas —
        // nenhum código atual GRAVA nesse formato, só LÊ pra compat).
        const QByteArray saltLegado = KeyDerivation::generateSalt();
        const QByteArray keyLegacy  = KeyDerivation::deriveKey(QStringLiteral("SenhaLegado123!"), saltLegado);
        AesCryptoService legacyCrypto;
        // SQLite trata um arquivo vazio como banco novo válido — plaintext
        // vazio é suficiente pra fabricar um `.db` "legado" plausível sem
        // depender de bytes reais de um schema antigo.
        const QByteArray cipherLegado = legacyCrypto.encrypt(QByteArray(), keyLegacy);

        QFile fOut(pbdPath);
        QVERIFY(fOut.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(fOut.write(cipherLegado), static_cast<qint64>(cipherLegado.size()));
        fOut.close();

        QVERIFY(!VaultFile::isV2(cipherLegado));
        QVERIFY2(!SodiumCryptoService::isNewFormat(cipherLegado), "fixture deveria ser legado (sem magic)");

        SodiumCryptoService crypto;
        const QByteArray keyNewFutura = crypto.randomBytes(32);

        QVERIFY(DatabaseManager::instance().open(slug, keyNewFutura, keyLegacy));
        QVERIFY(!DatabaseManager::instance().vaultIsV2());
        {
            QSqlQuery ins(DatabaseManager::instance().db());
            QVERIFY2(ins.exec(QStringLiteral("INSERT INTO cartoes (apelido) VALUES ('LegacyBank')")),
                     qPrintable(ins.lastError().text()));
        }
        // close() migra transparente pro formato novo (m_needsReencrypt não
        // é sequer necessário aqui — encryptAndSave() SEMPRE grava no
        // formato novo; m_needsReencrypt só existe hoje/historicamente).
        QVERIFY(DatabaseManager::instance().close());

        QFile fPos(pbdPath);
        QVERIFY(fPos.open(QIODevice::ReadOnly));
        const QByteArray bytesPos = fPos.readAll();
        fPos.close();
        QVERIFY2(SodiumCryptoService::isNewFormat(bytesPos), "deveria ter migrado pro formato novo (Sodium)");
        QVERIFY2(!VaultFile::isV2(bytesPos), "sem setVaultHeader(), a migração produz v1 puro, não v2");

        // Reabre já no formato novo — confirma que o dado inserido sob o
        // formato legado sobreviveu à migração transparente.
        QVERIFY(DatabaseManager::instance().open(slug, keyNewFutura));
        QVERIFY(!DatabaseManager::instance().vaultIsV2());
        QSqlQuery sel(DatabaseManager::instance().db());
        QVERIFY(sel.exec(QStringLiteral("SELECT apelido FROM cartoes")));
        QVERIFY(sel.next());
        QCOMPARE(sel.value(0).toString(), QStringLiteral("LegacyBank"));

        QVERIFY(DatabaseManager::instance().close());
    }
};

QTEST_GUILESS_MAIN(TestDatabaseManagerVaultV2)
#include "test_database_manager_vault_v2.moc"
