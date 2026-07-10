// PeuBud - #199 (Onda U, QA-07): 6 repos de dinheiro apontados pela
// auditoria como "CRUD sem prova INSERT->SELECT" (Divida/Meta/Orcamento/
// Price/Lembrete/Pagador). Divida e Meta ja tinham suite propria (regressao
// FIN-01/#146 -- test_divida_repository.cpp / test_meta_repository.cpp), que
// TAMBEM exercita insert->findById. Este arquivo cobre os 4 restantes:
// OrcamentoRepository, PriceRepository, LembreteRepository, PagadorRepository.
//
// Foco: prova de round-trip INSERT->SELECT com atencao especial aos campos
// monetarios em centavos (qint64) -- nenhuma perda/arredondamento no
// caminho SQLite (INTEGER nativo, sem cast para double em nenhum ponto).

#include <QtTest/QtTest>
#include <QByteArray>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "core/AppBootstrap.h"
#include "models/ItemOrcamento.h"
#include "models/PrecoAtivo.h"
#include "models/Lembrete.h"
#include "models/Pagador.h"
#include "repositories/OrcamentoRepository.h"
#include "repositories/PriceRepository.h"
#include "repositories/LembreteRepository.h"
#include "repositories/PagadorRepository.h"

class TestReposSemCobertura : public QObject
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
        // (HOME/XDG_DATA_HOME), nao mais CWD-relativo -- isolar HOME tambem.
        m_prevHome    = qgetenv("HOME");
        m_prevHomeSet = qEnvironmentVariableIsSet("HOME");
        qputenv("HOME", m_tmp.path().toUtf8());

        // (#200/ARQ-01) registra os providers de schema Mid/Backend ANTES
        // do open() -- ver AppBootstrap.h.
        AppBootstrap::registerCoreSchemaProviders();

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-repos-sem-cobertura"));
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
        QVERIFY(q.exec(QStringLiteral("DELETE FROM orcamento_itens")));
        QVERIFY(q.exec(QStringLiteral("DELETE FROM precos_ativos")));
        QVERIFY(q.exec(QStringLiteral("DELETE FROM lembretes")));
        QVERIFY(q.exec(QStringLiteral("DELETE FROM pagadores")));
    }

    // ── OrcamentoRepository ──────────────────────────────────────────────────

    void orcamentoInsertFindByIdPreservaCentavos()
    {
        ItemOrcamento item;
        item.nome         = QStringLiteral("Salario");
        item.tipo         = QStringLiteral("receita");
        item.valorMensal  = 500000; // R$ 5.000,00
        item.categoriaRef = QStringLiteral("*");

        QVERIFY2(OrcamentoRepository().insert(item), "insert falhou");
        QVERIFY(item.id > 0);

        const auto lido = OrcamentoRepository().findById(item.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->nome, QStringLiteral("Salario"));
        QCOMPARE(lido->tipo, QStringLiteral("receita"));
        QCOMPARE(lido->valorMensal, qint64(500000));
        QCOMPARE(lido->categoriaRef, QStringLiteral("*"));
    }

    void orcamentoFindAllOrdenaReceitaAntesDeDespesa()
    {
        ItemOrcamento despesa; despesa.nome = QStringLiteral("Aluguel");
        despesa.tipo = QStringLiteral("despesa"); despesa.valorMensal = 150000;
        ItemOrcamento receita; receita.nome = QStringLiteral("Freela");
        receita.tipo = QStringLiteral("receita"); receita.valorMensal = 80000;

        QVERIFY(OrcamentoRepository().insert(despesa));
        QVERIFY(OrcamentoRepository().insert(receita));

        const auto todos = OrcamentoRepository().findAll();
        QCOMPARE(todos.size(), 2);
        QCOMPARE(todos.first().tipo, QStringLiteral("receita"));
    }

    // ── PriceRepository ──────────────────────────────────────────────────────

    void priceUpsertFindByTickerAndDatePreservaCentavos()
    {
        PrecoAtivo p;
        p.ticker     = QStringLiteral("PETR4");
        p.data       = QDate(2025, 3, 14);
        p.abertura   = 2550;
        p.maxima     = 2699;
        p.minima     = 2501;
        p.fechamento = 2599;
        p.volume     = 1234500;
        p.fonte      = QStringLiteral("cotahist");
        p.atualizado = QDateTime(QDate(2025, 3, 14), QTime(18, 0));

        QVERIFY2(PriceRepository().upsert(p), "upsert falhou");

        const auto lido = PriceRepository().findByTickerAndDate(
            QStringLiteral("PETR4"), QDate(2025, 3, 14));
        QVERIFY(lido.has_value());
        QCOMPARE(lido->abertura,   qint64(2550));
        QCOMPARE(lido->maxima,     qint64(2699));
        QCOMPARE(lido->minima,     qint64(2501));
        QCOMPARE(lido->fechamento, qint64(2599));
        QCOMPARE(lido->volume,     qint64(1234500));
    }

    void priceUpsertBatchTransacaoAtomica()
    {
        QList<PrecoAtivo> lote;
        for (int dia = 1; dia <= 3; ++dia) {
            PrecoAtivo p;
            p.ticker     = QStringLiteral("VALE3");
            p.data       = QDate(2025, 1, dia);
            p.fechamento = 6000 + dia;
            p.fonte      = QStringLiteral("cotahist");
            lote << p;
        }

        QVERIFY2(PriceRepository().upsertBatch(lote), "upsertBatch falhou");
        QCOMPARE(PriceRepository().countByTicker(QStringLiteral("VALE3")), 3);

        const auto serie = PriceRepository().findByTickerRange(
            QStringLiteral("VALE3"), QDate(2025, 1, 1), QDate(2025, 1, 3));
        QCOMPARE(serie.size(), 3);
        QCOMPARE(serie.first().fechamento, qint64(6001));
    }

    // ── LembreteRepository ───────────────────────────────────────────────────

    void lembreteInsertFindByIdPreservaCentavos()
    {
        Lembrete l;
        l.descricao        = QStringLiteral("Fatura do cartao");
        l.valor             = 89990; // R$ 899,90
        l.dataVenc          = QDate::currentDate().addDays(10).toString(Qt::ISODate);
        l.frequencia        = QStringLiteral("mensal");
        l.debitoAutomatico  = true;
        l.status            = QStringLiteral("pendente");
        l.obs               = QStringLiteral("");

        QVERIFY2(LembreteRepository().insert(l), "insert falhou");
        QVERIFY(l.id > 0);

        const auto lido = LembreteRepository().findById(l.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->descricao, QStringLiteral("Fatura do cartao"));
        QCOMPARE(lido->valor, qint64(89990));
        QCOMPARE(lido->debitoAutomatico, true);
        QCOMPARE(lido->status, QStringLiteral("pendente"));
    }

    void lembreteFindAtivosFiltraStatus()
    {
        Lembrete pendente; pendente.descricao = QStringLiteral("A");
        pendente.valor = 100; pendente.status = QStringLiteral("pendente");
        pendente.dataVenc = QDate::currentDate().toString(Qt::ISODate);
        Lembrete pago; pago.descricao = QStringLiteral("B");
        pago.valor = 200; pago.status = QStringLiteral("pago");
        pago.dataVenc = QDate::currentDate().toString(Qt::ISODate);

        QVERIFY(LembreteRepository().insert(pendente));
        QVERIFY(LembreteRepository().insert(pago));

        const auto ativos = LembreteRepository().findAtivos();
        QCOMPARE(ativos.size(), 1);
        QCOMPARE(ativos.first().descricao, QStringLiteral("A"));
    }

    // ── PagadorRepository ────────────────────────────────────────────────────

    void pagadorInsertFindByIdPersiste()
    {
        Pagador p;
        p.nome = QStringLiteral("Maria Silva");
        p.tipo = QStringLiteral("pagador");
        p.obs  = QStringLiteral("");

        QVERIFY2(PagadorRepository().insert(p), "insert falhou");
        QVERIFY(p.id > 0);

        const auto lido = PagadorRepository().findById(p.id);
        QVERIFY(lido.has_value());
        QCOMPARE(lido->nome, QStringLiteral("Maria Silva"));
        QCOMPARE(lido->tipo, QStringLiteral("pagador"));
    }

    void pagadorFindOrCreateIdempotente()
    {
        const int id1 = PagadorRepository().findOrCreate(QStringLiteral("Joao"));
        QVERIFY(id1 > 0);
        const int id2 = PagadorRepository().findOrCreate(QStringLiteral("joao")); // case-insensitive
        QCOMPARE(id2, id1);

        const auto todos = PagadorRepository().findAll();
        QCOMPARE(todos.size(), 1);
    }
};

QTEST_MAIN(TestReposSemCobertura)
#include "test_repos_sem_cobertura.moc"
