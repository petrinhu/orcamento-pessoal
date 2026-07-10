// PeuBud - durabilidade do banco (#191, FIN-02, Onda U-1).
//
// BUG (auditoria 2026-06-22, §3.1 FIN-02): DatabaseManager usava
// journal_mode=MEMORY (journal em RAM -- crash no meio de uma transacao pode
// corromper o .db) e encryptAndSave() cifrava o .db em claro por cima do
// ultimo .pbd bom SEM checar PRAGMA integrity_check antes -- um .db
// sutilmente corrompido (ex.: bitrot, gravacao parcial externa -- nao
// necessariamente um crash do processo) seria cifrado e sobrescreveria o
// cofre bom, causando perda de dado silenciosa no proprio caminho de
// "salvar com sucesso".
//
// FIX: journal_mode=DELETE (crash-safe -- recupera via rollback journal no
// proximo open; journal e TRANSITORIO, ao contrario de WAL que deixa um
// -wal residente entre sessoes) + PRAGMA integrity_check via
// DatabaseManager::checkFileIntegrity() como GATE unico dentro de
// encryptAndSave() (aplica uniformemente a save()/close()/exportToFile()):
// se o .db reprovar, o save e ABORTADO e o .pbd anterior permanece intacto.
//
// Isolamento: HOME e CWD do processo sao redirecionados para um
// QTemporaryDir. savePathFixo()/autosavePathFixo() (#181) usam
// QDir::homePath() (~/peubud/save/<slug>/...) e (#192) o .db em claro usa
// AppSettings::workingDbPath() (QStandardPaths::AppDataLocation, baseado em
// HOME/XDG_DATA_HOME) -- sem isolar HOME, este teste escreveria no
// ~/peubud/save/ E no ~/.local/share/ REAIS de quem roda o ctest.

#include <QtTest/QtTest>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QString>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "core/AppBootstrap.h"
#include "core/settings/AppSettings.h"

class TestDbDurabilidade : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    // Chave de teste: 32 bytes fixos. AES-256-GCM/XChaCha20 exigem
    // exatamente 32 bytes (SodiumCryptoService::encrypt/decrypt); nao
    // precisa ser derivada de senha real para este teste de durabilidade.
    static QByteArray chaveTeste() { return QByteArray(32, char(0x5A)); }

    QSqlDatabase db() const { return DatabaseManager::instance().db(); }

    // Caminho do .db em claro que DatabaseManager::open() usa internamente
    // (#192: AppSettings::workingDbPath() -- QStandardPaths::AppDataLocation,
    // isolado por HOME neste teste -- substitui o antigo "data/.<slug>.db"
    // relativo ao CWD, CWD-relative sendo exatamente o bug DIST-03/SEC-02).
    static QString dbPlanoPath(const QString &slug)
    {
        return AppSettings::workingDbPath(slug);
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd     = QDir::currentPath();
        m_prevHome    = qgetenv("HOME");
        m_prevHomeSet = qEnvironmentVariableIsSet("HOME");
        QVERIFY(QDir::setCurrent(m_tmp.path()));
        // Isola savePathFixo()/autosavePathFixo() dentro do QTemporaryDir --
        // nunca tocar o ~/peubud/ real de quem roda o ctest.
        qputenv("HOME", m_tmp.path().toUtf8());

        // (#200/ARQ-01) registra os providers de schema Mid/Backend ANTES de
        // qualquer DatabaseManager::instance().open() desta suíte -- ver
        // AppBootstrap.h. Idempotente; uma única chamada cobre todos os
        // open() dos slots abaixo.
        AppBootstrap::registerCoreSchemaProviders();
    }

    void cleanupTestCase()
    {
        // Fecha qualquer conexao ainda aberta (best-effort; alguns testes
        // deliberadamente deixam o banco em estado corrompido).
        if (DatabaseManager::instance().isOpen())
            DatabaseManager::instance().close();

        if (m_prevHomeSet)
            qputenv("HOME", m_prevHome);
        else
            qunsetenv("HOME");
        if (!m_prevCwd.isEmpty())
            QDir::setCurrent(m_prevCwd);
    }

    // ── PRAGMA journal_mode ──────────────────────────────────────────────

    void openUsaJournalModeDelete()
    {
        const QString slug = QStringLiteral("durab-journal");
        QVERIFY2(DatabaseManager::instance().open(slug, chaveTeste()),
                 "open() falhou");

        QSqlQuery q(db());
        QVERIFY(q.exec(QStringLiteral("PRAGMA journal_mode")));
        QVERIFY(q.next());
        QCOMPARE(q.value(0).toString().toLower(), QStringLiteral("delete"));

        QVERIFY2(DatabaseManager::instance().close(),
                 "close() deveria persistir com sucesso (banco integro)");
    }

    // ── round-trip normal sob journal_mode=DELETE ───────────────────────
    // Prova que a troca MEMORY -> DELETE nao regride o ciclo save -> reabre
    // -> dado intacto (a mudanca de journal so afeta durabilidade em
    // crash, nunca a leitura/escrita normal).

    void roundTripNormalPreservaDadoAposSaveEReabertura()
    {
        const QString slug = QStringLiteral("durab-roundtrip");
        QVERIFY2(DatabaseManager::instance().open(slug, chaveTeste()),
                 "open() falhou");

        QSqlQuery ins(db());
        ins.prepare(QStringLiteral("INSERT INTO cartoes(apelido) VALUES(:a)"));
        ins.bindValue(QStringLiteral(":a"), QStringLiteral("Nubank Platinum"));
        QVERIFY2(ins.exec(), qPrintable(ins.lastError().text()));

        QVERIFY2(DatabaseManager::instance().close(),
                 "close() deveria ter persistido com sucesso (banco integro)");

        // Reabre do zero -- decripta o .pbd recem-gravado.
        QVERIFY2(DatabaseManager::instance().open(slug, chaveTeste()),
                 "reabertura falhou");

        QSqlQuery sel(db());
        QVERIFY(sel.exec(QStringLiteral(
            "SELECT apelido FROM cartoes WHERE apelido = 'Nubank Platinum'")));
        QVERIFY2(sel.next(),
                 "dado inserido antes do close() nao sobreviveu ao round-trip");
        QCOMPARE(sel.value(0).toString(), QStringLiteral("Nubank Platinum"));

        QVERIFY(DatabaseManager::instance().close());
    }

    // ── checkFileIntegrity: unidade pura (sem sessao do DatabaseManager) ──

    void checkFileIntegrityRejeitaArquivoInexistente()
    {
        QVERIFY(!DatabaseManager::checkFileIntegrity(
            m_tmp.filePath(QStringLiteral("nao-existe.db"))));
    }

    void checkFileIntegrityRejeitaArquivoCorrompido()
    {
        const QString path = m_tmp.filePath(QStringLiteral("corrompido.db"));
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArrayLiteral("isso claramente nao e um banco SQLite valido"));
        f.close();

        QVERIFY(!DatabaseManager::checkFileIntegrity(path));
    }

    void checkFileIntegrityAceitaBancoValido()
    {
        const QString path = m_tmp.filePath(QStringLiteral("valido.db"));
        {
            QSqlDatabase aux = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                                          QStringLiteral("aux_valido"));
            aux.setDatabaseName(path);
            QVERIFY(aux.open());
            QSqlQuery q(aux);
            QVERIFY(q.exec(QStringLiteral("CREATE TABLE t(id INTEGER PRIMARY KEY)")));
            aux.close();
        }
        QSqlDatabase::removeDatabase(QStringLiteral("aux_valido"));

        QVERIFY(DatabaseManager::checkFileIntegrity(path));
    }

    // ── O TESTE CENTRAL do #191/FIN-02 ──────────────────────────────────
    // Corrompe deliberadamente o .db em claro de uma sessao aberta e prova
    // que save()/close() RECUSAM persistir por cima do ultimo .pbd bom.
    // ANTES da fix: encryptAndSave() nao checava integridade -- teria
    // cifrado o lixo e sobrescrito o cofre bom (o bug exato da auditoria).

    void saveRecusaPersistirSobreDbCorrompidoPreservaVaultAnterior()
    {
        const QString slug = QStringLiteral("durab-corrupcao");
        QVERIFY2(DatabaseManager::instance().open(slug, chaveTeste()),
                 "open() falhou");

        QSqlQuery ins(db());
        ins.prepare(QStringLiteral("INSERT INTO cartoes(apelido) VALUES(:a)"));
        ins.bindValue(QStringLiteral(":a"), QStringLiteral("Cofre Bom"));
        QVERIFY2(ins.exec(), qPrintable(ins.lastError().text()));

        QVERIFY2(DatabaseManager::instance().save(),
                 "1o save() (banco integro) deveria ter sucesso");

        const QString pbdPath = AppSettings::savePathFixo(slug);
        QByteArray pbdBomBytes;
        {
            QFile pbdFile(pbdPath);
            QVERIFY(pbdFile.open(QIODevice::ReadOnly));
            pbdBomBytes = pbdFile.readAll();
        }
        QVERIFY2(!pbdBomBytes.isEmpty(), "cofre bom deveria ter conteudo");

        // Corrompe o .db em claro DIRETAMENTE no disco (simula bitrot /
        // gravacao parcial externa -- cenario distinto de crash-do-processo,
        // que o journal_mode=DELETE cobre; aqui o arquivo ja esta corrompido
        // ANTES do save() ser chamado).
        const QString planoPath = dbPlanoPath(slug);
        {
            QFile plano(planoPath);
            QVERIFY2(plano.exists(), "arquivo .db em claro deveria existir");
            QVERIFY(plano.open(QIODevice::ReadWrite));
            plano.resize(20); // trunca no meio do cabecalho SQLite (100 bytes)
            plano.close();
        }

        // ESPEC pos-fix: save() detecta via integrity_check e RECUSA
        // persistir.
        QVERIFY2(!DatabaseManager::instance().save(),
                 "save() deveria RECUSAR persistir um .db corrompido");

        {
            QFile pbdFile2(pbdPath);
            QVERIFY(pbdFile2.open(QIODevice::ReadOnly));
            QCOMPARE(pbdFile2.readAll(), pbdBomBytes); // cofre anterior intacto
        }

        // close() usa o MESMO gate (dentro de encryptAndSave) -- tambem deve
        // recusar, sem sobrescrever o cofre bom no caminho de saida da app.
        QVERIFY2(!DatabaseManager::instance().close(),
                 "close() deveria RECUSAR persistir um .db corrompido");

        {
            QFile pbdFile3(pbdPath);
            QVERIFY(pbdFile3.open(QIODevice::ReadOnly));
            QCOMPARE(pbdFile3.readAll(), pbdBomBytes); // ainda intacto apos close()
        }

        // close() ja fechou a conexao "main" (mesmo tendo recusado
        // persistir) -- nenhum handle pendente para o proximo teste.
    }
};

QTEST_MAIN(TestDbDurabilidade)
#include "test_db_durabilidade.moc"
