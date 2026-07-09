// PeuBud - #192 (Onda U, DIST-03/SEC-02): .db de trabalho em local FIXO.
//
// BUG (auditoria 2026-06-22, §4 DIST-03 = §3.6 SEC-02): DatabaseManager
// computava `m_dbPath = "data/.<slug>.db"` RELATIVO ao CWD do processo. O
// `.desktop` do pacote nao seta `Path=`, entao o lancamento via menu abre o
// processo com CWD imprevisivel ($HOME, "/", ou o que o DE escolher) -> o
// diretorio "data/" e criado num lugar aleatorio (ou a criacao falha, ex.:
// CWD sem permissao de escrita) -> quebra 100% na 1a execucao do pacote
// instalado.
//
// FIX: AppSettings::workingDbDir()/workingDbPath() usam
// QStandardPaths::AppDataLocation (Linux: $XDG_DATA_HOME/<org>/<app>, fixo
// e independente de onde o processo foi lancado), com mkpath + chmod 0700
// garantidos a cada chamada.
//
// Isolamento: HOME e XDG_DATA_HOME sao redirecionados para um QTemporaryDir
// -- sem isso, este teste escreveria no ~/.local/share/ REAL de quem roda
// o ctest.

#include <QtTest/QtTest>

#include <QByteArray>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTemporaryDir>

#include <sys/stat.h>

#include "core/settings/AppSettings.h"

class TestWorkingDbPath : public QObject
{
    Q_OBJECT

private:
    QByteArray m_prevHome;
    bool       m_prevHomeSet = false;
    QByteArray m_prevXdgData;
    bool       m_prevXdgDataSet = false;

private slots:
    void initTestCase()
    {
        // Nomes deterministicos p/ QStandardPaths::AppDataLocation incluir
        // um subcaminho previsivel dentro do XDG_DATA_HOME isolado.
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

    // ── Formato do caminho ───────────────────────────────────────────────

    void workingDbPath_terminaComPontoSlugPontoDb()
    {
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        qputenv("XDG_DATA_HOME", xdg.path().toUtf8());

        const QString path = AppSettings::workingDbPath(QStringLiteral("petrus"));
        QVERIFY(path.endsWith(QStringLiteral("/.petrus.db")));
    }

    void workingDbPath_slugsDiferentes_arquivosDiferentesNoMesmoDir()
    {
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        qputenv("XDG_DATA_HOME", xdg.path().toUtf8());

        const QString path1 = AppSettings::workingDbPath(QStringLiteral("alice"));
        const QString path2 = AppSettings::workingDbPath(QStringLiteral("bob"));

        QVERIFY(path1 != path2);
        QCOMPARE(QFileInfo(path1).absolutePath(), QFileInfo(path2).absolutePath());
    }

    // ── O CERNE DO BUG: independencia do CWD ────────────────────────────
    // Antes do fix: m_dbPath = "data/.<slug>.db" (relativo) -- mudar o CWD
    // entre as duas chamadas mudaria o caminho retornado (ou pior: o
    // resolveria contra um CWD sem permissao de escrita, quebrando o
    // lancamento via .desktop). Pos-fix: caminho fixo, independe do CWD.

    void workingDbPath_naoDependeDoCwdDoProcesso()
    {
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        qputenv("XDG_DATA_HOME", xdg.path().toUtf8());

        QTemporaryDir cwd1, cwd2;
        QVERIFY(cwd1.isValid() && cwd2.isValid());
        const QString cwdOriginal = QDir::currentPath();

        QVERIFY(QDir::setCurrent(cwd1.path()));
        const QString pathDeCwd1 = AppSettings::workingDbPath(QStringLiteral("slugtest"));

        QVERIFY(QDir::setCurrent(cwd2.path()));
        const QString pathDeCwd2 = AppSettings::workingDbPath(QStringLiteral("slugtest"));

        QDir::setCurrent(cwdOriginal);

        QCOMPARE(pathDeCwd1, pathDeCwd2);
    }

    void workingDbPath_eSempreCaminhoAbsoluto()
    {
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        qputenv("XDG_DATA_HOME", xdg.path().toUtf8());

        const QString path = AppSettings::workingDbPath(QStringLiteral("petrus"));
        QVERIFY2(QDir::isAbsolutePath(path),
                 qPrintable(QStringLiteral("esperado caminho absoluto, obtido: %1").arg(path)));
        QVERIFY2(!path.startsWith(QStringLiteral("data/")),
                 "regressao DIST-03: caminho voltou a ser relativo a \"data/\"");
    }

    // ── mkpath + permissao 0700 ──────────────────────────────────────────

    void workingDbDir_mkpathCriaDiretorioComPermissao0700()
    {
        QTemporaryDir xdg;
        QVERIFY(xdg.isValid());
        qputenv("XDG_DATA_HOME", xdg.path().toUtf8());

        const QString dir = AppSettings::workingDbDir();
        QVERIFY(QDir(dir).exists());

        struct stat st{};
        QVERIFY(::stat(QFile::encodeName(dir).constData(), &st) == 0);
        QCOMPARE(static_cast<unsigned>(st.st_mode & 0777), static_cast<unsigned>(S_IRWXU));
    }
};

QTEST_GUILESS_MAIN(TestWorkingDbPath)
#include "test_working_db_path.moc"
