// PeuBud - smoke test de schema pos-#200/ARQ-01.
//
// ARQ-01 moveu a criacao de 4 tabelas (proventos_recebidos, ajustes_corporativos,
// fechamentos_mensais, audit_log) de chamadas diretas dentro de
// DatabaseManager::createSchema() para providers registrados via
// DatabaseManager::registerSchemaProvider() (composicao em AppBootstrap).
// Este teste prova que a inversao de dependencia NAO perdeu nenhuma tabela:
// banco novo -> as 16 tabelas inline do Foundation + as 4 via provider
// existem todas apos open().

#include <QtTest/QtTest>

#include <QByteArray>
#include <QDir>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "core/AppBootstrap.h"

class TestSchemaSmoke : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    bool tabelaExiste(const QString &nome)
    {
        QSqlQuery q(DatabaseManager::instance().db());
        q.prepare(QStringLiteral(
            "SELECT 1 FROM sqlite_master WHERE type='table' AND name = ?"));
        q.addBindValue(nome);
        if (!q.exec()) return false;
        return q.next();
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(m_tmp.path()));
        m_prevHome    = qgetenv("HOME");
        m_prevHomeSet = qEnvironmentVariableIsSet("HOME");
        qputenv("HOME", m_tmp.path().toUtf8());

        // (#200/ARQ-01) ponto de composicao — sem isso, os 4 providers
        // Mid/Backend nunca seriam chamados por createSchema().
        AppBootstrap::registerCoreSchemaProviders();

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-schema-smoke"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());
    }

    void cleanupTestCase()
    {
        // Nao chama close() (evitaria cifragem em build release); so restaura
        // ambiente. QTemporaryDir remove tudo no destrutor.
        if (m_prevHomeSet) qputenv("HOME", m_prevHome);
        else qunsetenv("HOME");
        QDir::setCurrent(m_prevCwd);
    }

    // As 16 tabelas criadas inline em DatabaseManager::createSchema().
    void tabelasInlineDoFoundationExistem()
    {
        const QStringList inline16 = {
            QStringLiteral("cartoes"),
            QStringLiteral("compras"),
            QStringLiteral("parcelas"),
            QStringLiteral("fontes_renda"),
            QStringLiteral("lancamentos_entradas"),
            QStringLiteral("lancamentos_gastos"),
            QStringLiteral("orcamento_itens"),
            QStringLiteral("lancamentos_investimentos"),
            QStringLiteral("transacoes_recorrentes"),
            QStringLiteral("categorias"),
            QStringLiteral("pagadores"),
            QStringLiteral("dividas"),
            QStringLiteral("metas"),
            QStringLiteral("lembretes"),
            QStringLiteral("precos_ativos"),
            QStringLiteral("cotacoes_tempo_real"),
        };
        QCOMPARE(inline16.size(), 16);
        for (const QString &t : inline16) {
            QVERIFY2(tabelaExiste(t),
                qPrintable(QStringLiteral("tabela inline ausente: %1").arg(t)));
        }
    }

    // As 4 tabelas Mid/Backend, agora criadas via
    // DatabaseManager::registerSchemaProvider (AppBootstrap).
    void tabelasViaProviderMidBackendExistem()
    {
        const QStringList viaProvider = {
            QStringLiteral("proventos_recebidos"),    // #66 ProventoRepository
            QStringLiteral("ajustes_corporativos"),    // #65 AjusteCorporativoService
            QStringLiteral("fechamentos_mensais"),     // #88 FechamentoMensalService
            QStringLiteral("audit_log"),               // #92 AuditLogService
        };
        QCOMPARE(viaProvider.size(), 4);
        for (const QString &t : viaProvider) {
            QVERIFY2(tabelaExiste(t),
                qPrintable(QStringLiteral("tabela via provider ausente: %1").arg(t)));
        }
    }
};

QTEST_MAIN(TestSchemaSmoke)
#include "test_schema_smoke.moc"
