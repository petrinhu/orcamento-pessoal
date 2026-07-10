// PeuBud - varredura dominio FIN-01/#146 (Onda U-1): dois casos CONFIRMADOS de
// perda de dado silenciosa por QString null em coluna TEXT NOT NULL, achados
// durante a varredura dos 14 repos restantes (alem de Meta/Divida).
//
// Caso 1 (TransacaoRecorrenteRepository): RecorrenciaDialog::resultado() faz
// `t.categoria.clear(); t.tipoPgto.clear();` quando tipo=="entrada" (e
// `t.fonte.clear()` quando tipo=="gasto") -- QString::clear() torna a string
// NULL, nao apenas vazia. Como um dos dois ramos SEMPRE limpa um desses
// campos, TODA transacao recorrente (de qualquer tipo) violava a constraint
// `TEXT NOT NULL DEFAULT ''` de fonte/categoria/tipo_pgto.
//
// Caso 2 (LancamentoInvestimentoRepository): InvestimentoDialog::lancamentos()
// faz `catNome.clear();` quando o combo de categoria esta no item "sem
// categoria" (catId<0) -- selecao comum/default. `l.categoria` (TEXT NOT NULL
// DEFAULT '') recebia NULL explicito.
//
// Fix: RepositoryUtils.h::naoNulo() aplicado nos binds TEXT NOT NULL desses
// dois repos (mesma tecnica do fix principal em Meta/Divida).

#include <QtTest/QtTest>
#include <QByteArray>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "core/AppBootstrap.h"
#include "models/TransacaoRecorrente.h"
#include "models/LancamentoInvestimento.h"
#include "repositories/TransacaoRecorrenteRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"

class TestRepositoryNaoNuloDomino : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    QSqlDatabase db() const { return DatabaseManager::instance().db(); }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(m_tmp.path()));
        // (#192) AppSettings::workingDbPath() usa QStandardPaths::AppDataLocation
        // (HOME/XDG_DATA_HOME), nao mais CWD-relativo -- isolar HOME tambem,
        // senao o .db em claro deste teste vaza pro ambiente real do dev.
        m_prevHome    = qgetenv("HOME");
        m_prevHomeSet = qEnvironmentVariableIsSet("HOME");
        qputenv("HOME", m_tmp.path().toUtf8());

        // (#200/ARQ-01) registra os providers de schema Mid/Backend ANTES
        // do open() -- ver AppBootstrap.h.
        AppBootstrap::registerCoreSchemaProviders();

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-repo-naonulo-domino"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());
    }

    void cleanupTestCase()
    {
        if (m_prevHomeSet)
            qputenv("HOME", m_prevHome);
        else
            qunsetenv("HOME");
        if (!m_prevCwd.isEmpty())
            QDir::setCurrent(m_prevCwd);
    }

    void init()
    {
        QSqlQuery q(db());
        QVERIFY(q.exec(QStringLiteral("DELETE FROM transacoes_recorrentes")));
        QVERIFY(q.exec(QStringLiteral("DELETE FROM lancamentos_investimentos")));
    }

    // Caso 1: recorrencia de ENTRADA -- RecorrenciaDialog::resultado() faz
    // t.categoria.clear() e t.tipoPgto.clear() nesse ramo (campos "so para
    // gasto"). Reproduz o clear() exatamente como o dialog faz.
    void insertRecorrenteEntradaComCamposClearedPersiste()
    {
        TransacaoRecorrente t;
        t.descricao   = QStringLiteral("Salario");
        t.tipo        = QStringLiteral("entrada");
        t.valor       = 500000;
        t.fonte       = QStringLiteral("Empregador");
        t.frequencia  = QStringLiteral("mensal");
        t.dataInicio  = QDate::currentDate();
        t.obs         = QStringLiteral("");
        // Ramo "entrada" do dialog: categoria/tipoPgto sao clear()ados (null).
        t.categoria.clear();
        t.tipoPgto.clear();
        QVERIFY(t.categoria.isNull());
        QVERIFY(t.tipoPgto.isNull());

        const bool ok = TransacaoRecorrenteRepository().insert(t);
        QVERIFY2(ok, "insert deveria persistir recorrencia de entrada com categoria/tipoPgto null");
        QVERIFY(t.id > 0);

        const auto lido = TransacaoRecorrenteRepository().findById(t.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->categoria, QString::fromLatin1(""));
        QCOMPARE(lido->tipoPgto,  QString::fromLatin1(""));
    }

    // Ramo "gasto" do dialog: fonte e clear()ada (null).
    void insertRecorrenteGastoComFonteClearedPersiste()
    {
        TransacaoRecorrente t;
        t.descricao  = QStringLiteral("Aluguel");
        t.tipo       = QStringLiteral("gasto");
        t.valor      = 150000;
        t.categoria  = QStringLiteral("moradia");
        t.tipoPgto   = QStringLiteral("boleto");
        t.frequencia = QStringLiteral("mensal");
        t.dataInicio = QDate::currentDate();
        t.fonte.clear();
        QVERIFY(t.fonte.isNull());

        const bool ok = TransacaoRecorrenteRepository().insert(t);
        QVERIFY2(ok, "insert deveria persistir recorrencia de gasto com fonte null");
        QVERIFY(t.id > 0);

        const auto lido = TransacaoRecorrenteRepository().findById(t.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->fonte, QString::fromLatin1(""));
    }

    // Caso 2: InvestimentoDialog::lancamentos() faz catNome.clear() quando o
    // combo esta em "sem categoria" (catId<0) -- selecao default/comum.
    void insertInvestimentoSemCategoriaPersiste()
    {
        LancamentoInvestimento l;
        l.mes          = QDate::currentDate().month();
        l.ano          = QDate::currentDate().year();
        l.dia          = 1;
        l.nome         = QStringLiteral("PETR4");
        l.tipoAtivo    = QStringLiteral("acao");
        l.tipoOperacao = QStringLiteral("aporte");
        l.valor        = 100000;
        l.ticker       = QStringLiteral("PETR4");
        // Ramo catId<0 do dialog: catNome.clear() -> categoria fica null.
        l.categoria.clear();
        QVERIFY(l.categoria.isNull());

        const bool ok = LancamentoInvestimentoRepository().insert(l);
        QVERIFY2(ok, "insert deveria persistir lancamento sem categoria (categoria null)");
        QVERIFY(l.id > 0);

        const auto lido = LancamentoInvestimentoRepository().findById(l.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->categoria, QString::fromLatin1(""));
    }
};

QTEST_MAIN(TestRepositoryNaoNuloDomino)
#include "test_repository_naonulo_domino.moc"
