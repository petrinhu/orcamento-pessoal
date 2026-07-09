// PeuBud - suite de testes do FechamentoMensalService (#144).
//
// CRITICO: simulate() estava QUEBRADO — fazia SQL cru contra colunas
// inexistentes (lancamentos_entradas.valor/.data, parcelas.vencimento,
// lancamentos_investimentos.data) e o COALESCE(...,0) mascarava o erro,
// retornando SEMPRE zero. A correcao delega ao ResumoService (fonte unica,
// ja testada em #116), que agrega LIQUIDO com as colunas reais.
//
// Estes testes provam o COMPORTAMENTO esperado a partir da espec (#144):
//   - mes vazio -> tudo zero (nao falha por isso; o bug era o inverso);
//   - mes COM dados -> valores NAO-zero CORRETOS (entradas liquidas =
//     bruto - irpf - inss, gastos, parcelas, investimentos net, contagens,
//     saldo liquido). Este e exatamente o teste que pegaria o bug original.
//   - simulate deve bater com ResumoService::resumoDoMes (fonte unica);
//   - close() persiste o snapshot e findByPeriod() o recupera;
//   - reopen() apaga o snapshot.
//
// Setup de DB: espelha test_resumo.cpp — abre o singleton DatabaseManager num
// diretorio temporario, popula via API publica de producao, limpa as tabelas
// a cada teste. Valores em centavos (qint64); comparacoes exatas (QCOMPARE).

#include <QtTest/QtTest>

#include <QByteArray>
#include <QDate>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "models/Compra.h"
#include "models/FechamentoMensal.h"
#include "models/GastoAvulso.h"
#include "models/LancamentoEntrada.h"
#include "models/LancamentoInvestimento.h"
#include "models/Parcela.h"
#include "models/ResumoMensal.h"
#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "services/CompraService.h"
#include "services/EntradaService.h"
#include "services/FechamentoMensalService.h"
#include "services/ParcelaService.h"
#include "services/ResumoService.h"

class TestFechamento : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;
    QString       m_prevCwd;
    QByteArray    m_prevHome;
    bool          m_prevHomeSet = false;

    // ── Remove triggers de auditoria (mesmo workaround de test_resumo) ───────
    void dropTriggersAudit()
    {
        QSqlDatabase db = DatabaseManager::instance().db();
        QSqlQuery list(db);
        list.exec(QStringLiteral(
            "SELECT name FROM sqlite_master WHERE type='trigger' "
            "AND name LIKE 'audit_%'"));
        QStringList nomes;
        while (list.next()) nomes << list.value(0).toString();
        for (const QString &n : nomes) {
            QSqlQuery d(db);
            d.exec(QStringLiteral("DROP TRIGGER IF EXISTS %1").arg(n));
        }
    }

    void limparTabelas()
    {
        QSqlDatabase db = DatabaseManager::instance().db();
        const QStringList tabelas = {
            QStringLiteral("lancamentos_entradas"),
            QStringLiteral("lancamentos_gastos"),
            QStringLiteral("lancamentos_investimentos"),
            QStringLiteral("parcelas"),
            QStringLiteral("compras"),
            QStringLiteral("fontes_renda"),
            QStringLiteral("fechamentos_mensais"),
        };
        for (const QString &t : tabelas) {
            QSqlQuery q(db);
            q.exec(QStringLiteral("DELETE FROM %1").arg(t));
        }
    }

    // ── Helpers de insercao (via API publica de producao) ────────────────────
    void addEntrada(int mes, int ano, qint64 bruto, qint64 irpf, qint64 inss)
    {
        LancamentoEntrada l;
        l.mes        = mes;
        l.ano        = ano;
        l.dia        = 1;
        l.fonte      = QStringLiteral("Salario");
        l.categoria  = QString::fromLatin1("");
        l.valorBruto = bruto;
        l.irpf       = irpf;
        l.inss       = inss;
        l.obs        = QString::fromLatin1("");
        EntradaService svc;
        QVERIFY2(svc.salvarComFonte(l), "falha ao inserir LancamentoEntrada");
    }

    void addGasto(int mes, int ano, qint64 valor)
    {
        GastoAvulso g;
        g.mes       = mes;
        g.ano       = ano;
        g.dia       = 1;
        g.descricao = QStringLiteral("gasto");
        g.categoria = QString::fromLatin1("");
        g.tipoPgto  = QStringLiteral("pix");
        g.valor     = valor;
        g.obs       = QString::fromLatin1("");
        g.pagador   = QString::fromLatin1("");
        GastoAvulsoRepository repo;
        QVERIFY2(repo.insert(g), "falha ao inserir GastoAvulso");
    }

    void addInvestimento(int mes, int ano, const QString &op, qint64 valor)
    {
        LancamentoInvestimento inv;
        inv.mes          = mes;
        inv.ano          = ano;
        inv.dia          = 1;
        inv.nome         = QStringLiteral("PETR4");
        inv.tipoAtivo    = QStringLiteral("acao");
        inv.tipoOperacao = op;   // "aporte"|"resgate"|"rendimento"|"dividendo"
        inv.valor        = valor;
        inv.categoria    = QString::fromLatin1("");
        inv.obs          = QString::fromLatin1("");
        inv.ticker       = QString::fromLatin1("");
        LancamentoInvestimentoRepository repo;
        QVERIFY2(repo.insert(inv), "falha ao inserir LancamentoInvestimento");
    }

    Compra addCompraParcelada(int mesInicio, int anoInicio, int numParcelas,
                              qint64 valorParcela)
    {
        Compra c;
        c.nome           = QStringLiteral("Compra");
        c.valorTotal     = valorParcela * numParcelas;
        c.numParcelas    = numParcelas;
        c.parcelaInicial = 1;
        c.mesInicio      = mesInicio;
        c.anoInicio      = anoInicio;

        QList<Parcela> parcelas;
        for (int i = 1; i <= numParcelas; ++i) {
            Parcela p;
            p.numero = i;
            p.valor  = valorParcela;
            parcelas.append(p);
        }
        CompraService svc;
        const bool ok = svc.inserir(c, parcelas);
        Q_ASSERT(ok);
        Q_UNUSED(ok);
        return c;
    }

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

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-fechamento"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());

        // Mesmo bug de produto de #116/#142: triggers audit_* referenciam
        // colunas inexistentes e fazem QUALQUER insert falhar. Removidos no
        // setup para popular o banco (este teste apenas LE/agrega).
        dropTriggersAudit();
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
        limparTabelas();
    }

    // ───────────────────────────── simulate ─────────────────────────────────

    // ESPEC 1: mes vazio -> todos os campos 0 (caso degenerado valido).
    void mesVazioTudoZero()
    {
        QSqlDatabase db = DatabaseManager::instance().db();
        const FechamentoResumo r =
            FechamentoMensalService::simulate(db, QDate(2025, 6, 1));

        QCOMPARE(r.entradasCentavos,      qint64(0));
        QCOMPARE(r.gastosCentavos,        qint64(0));
        QCOMPARE(r.parcelasCentavos,      qint64(0));
        QCOMPARE(r.investimentosCentavos, qint64(0));
        QCOMPARE(r.saldoLiquidoCentavos,  qint64(0));
        QCOMPARE(r.numLancamentos,        0);
    }

    // ESPEC 2 (CRITICO — pega o bug original): mes COM dados retorna valores
    // NAO-zero CORRETOS. entradas LIQUIDAS (bruto-irpf-inss), gastos, parcelas,
    // investimentos net (resgates-aportes), contagens e saldo liquido.
    void mesComDadosRetornaValoresNaoZero()
    {
        // Entradas: bruto 500000, irpf 50000, inss 30000 -> liquida 420000.
        addEntrada(6, 2025, 500000, 50000, 30000);
        // Gastos: 120000.
        addGasto(6, 2025, 120000);
        // Parcela jun/2025: 80000 (1 parcela).
        addCompraParcelada(6, 2025, 1, 80000);
        // Investimentos: aporte 100000, resgate 30000 -> net = -70000.
        // dividendo NAO entra no net (D0).
        addInvestimento(6, 2025, QStringLiteral("aporte"),   100000);
        addInvestimento(6, 2025, QStringLiteral("resgate"),   30000);
        addInvestimento(6, 2025, QStringLiteral("dividendo"),  5000);

        QSqlDatabase db = DatabaseManager::instance().db();
        const FechamentoResumo r =
            FechamentoMensalService::simulate(db, QDate(2025, 6, 1));

        // O bug original retornaria 0 em TODOS estes — estes asserts o pegam.
        QCOMPARE(r.entradasCentavos,      qint64(420000)); // LIQUIDO, nao bruto
        QVERIFY(r.entradasCentavos != qint64(500000));     // nao e o bruto
        QCOMPARE(r.gastosCentavos,        qint64(120000));
        QCOMPARE(r.parcelasCentavos,      qint64(80000));
        QCOMPARE(r.investimentosCentavos, qint64(-70000)); // resgates - aportes
        // numLancamentos = entradas(1) + gastos(1) + parcelas(1) = 3.
        QCOMPARE(r.numLancamentos,        3);
        // saldoLiquido = 420000 - 120000 - 80000 = 220000.
        QCOMPARE(r.saldoLiquidoCentavos,  qint64(220000));

        // Garantia explicita anti-regressao: NADA disso pode ser zero com dados.
        QVERIFY(r.entradasCentavos != 0);
        QVERIFY(r.gastosCentavos   != 0);
        QVERIFY(r.parcelasCentavos != 0);
        QVERIFY(r.saldoLiquidoCentavos != 0);
    }

    // ESPEC 3: simulate e uma vista fina sobre ResumoService — deve bater
    // exatamente com resumoDoMes (fonte unica de verdade).
    void simulateBateComResumoService()
    {
        addEntrada(6, 2025, 700000, 40000, 20000); // liquida 640000
        addGasto(6, 2025, 90000);
        addCompraParcelada(6, 2025, 1, 50000);
        addInvestimento(6, 2025, QStringLiteral("aporte"), 200000);

        const ResumoMensal rm = ResumoService().resumoDoMes(6, 2025);
        QSqlDatabase db = DatabaseManager::instance().db();
        const FechamentoResumo r =
            FechamentoMensalService::simulate(db, QDate(2025, 6, 1));

        QCOMPARE(r.entradasCentavos,      rm.entradasLiquidas);
        QCOMPARE(r.gastosCentavos,        rm.gastos);
        QCOMPARE(r.parcelasCentavos,      rm.parcelas);
        QCOMPARE(r.investimentosCentavos, rm.investimentosNet);
        QCOMPARE(r.saldoLiquidoCentavos,  rm.saldoLiquido);
        QCOMPARE(r.numLancamentos,
                 rm.cntEntradas + rm.cntGastos + rm.cntParcelas);
    }

    // ESPEC 4: filtro estrito de mes/ano (herdado do ResumoService): dados de
    // outro mes/ano nao vazam para o periodo fechado.
    void filtroEstritoMesAno()
    {
        addEntrada(6, 2025, 100000, 0, 0); // alvo
        addEntrada(7, 2025, 999999, 0, 0); // outro mes
        addEntrada(6, 2024, 888888, 0, 0); // outro ano

        QSqlDatabase db = DatabaseManager::instance().db();
        const FechamentoResumo r =
            FechamentoMensalService::simulate(db, QDate(2025, 6, 1));
        QCOMPARE(r.entradasCentavos, qint64(100000));
        QCOMPARE(r.numLancamentos,   1);
    }

    // ─────────────────────────── close / findByPeriod ───────────────────────

    // ESPEC 5: close() persiste o snapshot com os valores corretos; o
    // dados_json carrega os mesmos centavos do simulate.
    void closePersisteSnapshotCorreto()
    {
        addEntrada(6, 2025, 500000, 50000, 30000); // liquida 420000
        addGasto(6, 2025, 120000);

        QSqlDatabase db = DatabaseManager::instance().db();
        const QDate periodo(2025, 6, 1);

        const auto snap = FechamentoMensalService::close(
            db, periodo, QStringLiteral("petrus"));
        QVERIFY2(snap.has_value(), "close deveria persistir o snapshot");
        QCOMPARE(snap->periodo, QStringLiteral("2025-06"));
        QCOMPARE(snap->fechadoPor, QStringLiteral("petrus"));

        // O JSON persistido deve refletir os valores corretos (nao-zero).
        QVERIFY(snap->dadosJson.contains(QStringLiteral("\"entradas_centavos\":420000")));
        QVERIFY(snap->dadosJson.contains(QStringLiteral("\"gastos_centavos\":120000")));
        QVERIFY(snap->dadosJson.contains(QStringLiteral("\"saldo_liquido_centavos\":300000")));

        // findByPeriod recupera o mesmo registro.
        const auto achado = FechamentoMensalService::findByPeriod(db, periodo);
        QVERIFY2(achado.has_value(), "findByPeriod deveria achar o snapshot");
        QCOMPARE(achado->periodo,   snap->periodo);
        QCOMPARE(achado->dadosJson, snap->dadosJson);
    }

    // ESPEC 6: close() nao sobrescreve periodo ja fechado; reopen() apaga o
    // snapshot e permite fechar de novo. findByPeriod some apos reopen.
    void closeNaoSobrescreveEReopenApaga()
    {
        addEntrada(6, 2025, 100000, 0, 0);
        QSqlDatabase db = DatabaseManager::instance().db();
        const QDate periodo(2025, 6, 1);

        const auto primeiro = FechamentoMensalService::close(
            db, periodo, QStringLiteral("a"));
        QVERIFY(primeiro.has_value());

        // Segundo close no mesmo periodo: nullopt (nao sobrescreve).
        const auto segundo = FechamentoMensalService::close(
            db, periodo, QStringLiteral("b"));
        QVERIFY2(!segundo.has_value(), "close nao deve sobrescrever periodo fechado");

        // reopen apaga; findByPeriod nao acha mais.
        QVERIFY(FechamentoMensalService::reopen(db, periodo));
        QVERIFY(!FechamentoMensalService::findByPeriod(db, periodo).has_value());

        // Apos reopen, fechar de novo volta a funcionar.
        const auto terceiro = FechamentoMensalService::close(
            db, periodo, QStringLiteral("c"));
        QVERIFY(terceiro.has_value());
        QCOMPARE(terceiro->fechadoPor, QStringLiteral("c"));
    }

    // ESPEC 7: findByPeriod em periodo nunca fechado -> nullopt.
    void findByPeriodInexistenteNullopt()
    {
        QSqlDatabase db = DatabaseManager::instance().db();
        const auto r = FechamentoMensalService::findByPeriod(
            db, QDate(2030, 1, 1));
        QVERIFY(!r.has_value());
    }
};

QTEST_MAIN(TestFechamento)
#include "test_fechamento.moc"
