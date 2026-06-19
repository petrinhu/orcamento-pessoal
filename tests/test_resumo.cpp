// PeuBud - suite de testes do ResumoService (#116, Onda 1).
//
// Camada Mid: ResumoService agrega Entradas, Gastos, Parcelas, Investimentos
// e Proventos em ResumoMensal / ResumoAnual / serie() para o Dashboard.
//
// Estes testes sao escritos a partir da ESPEC de comportamento (#116), NAO da
// leitura da implementacao. Provam o comportamento esperado; uma divergencia
// e reportada como bug do backend, nao "consertada" aqui.
//
// Setup de DB: abre o singleton DatabaseManager num diretorio temporario
// (cwd trocado em initTestCase). createSchema() cria todas as tabelas. Cada
// teste limpa as tabelas em init() para isolamento real. Valores em centavos
// (qint64); comparacoes exatas (QCOMPARE), sem float nos asserts.

#include <QtTest/QtTest>

#include <QDate>
#include <QDir>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "core/database/DatabaseManager.h"
#include "models/Compra.h"
#include "models/GastoAvulso.h"
#include "models/LancamentoEntrada.h"
#include "models/LancamentoInvestimento.h"
#include "models/Parcela.h"
#include "models/Provento.h"
#include "models/ResumoAnual.h"
#include "models/ResumoMensal.h"
#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "repositories/ProventoRepository.h"
#include "services/CompraService.h"
#include "services/EntradaService.h"
#include "services/ParcelaService.h"
#include "services/ResumoService.h"

class TestResumo : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_tmp;          // diretorio de trabalho isolado
    QString       m_prevCwd;      // cwd original (restaurado no fim)

    // ── Remove triggers de auditoria (workaround do bug de produto) ──────────
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

    // ── Limpa todas as tabelas que o ResumoService consulta ──────────────────
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
            QStringLiteral("proventos_recebidos"),
        };
        for (const QString &t : tabelas) {
            QSqlQuery q(db);
            q.exec(QStringLiteral("DELETE FROM %1").arg(t));
        }
    }

    // ── Helpers de insercao (via API publica de produção) ────────────────────
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
        l.obs        = QString::fromLatin1("");  // coluna NOT NULL
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

    // Cria compra de N parcelas de valor fixo, primeira parcela em (mes,ano).
    // Retorna a compra (com id setado) para inspecao de datas.
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

    void addProvento(const QString &ticker, const QString &tipo,
                     const QDate &data, qint64 liquido)
    {
        Provento p;
        p.ticker           = ticker;
        p.tipo             = tipo;
        p.dataEvento       = data;
        p.valorBrutoCent   = liquido;
        p.valorLiquidoCent = liquido;
        p.observacao       = QString::fromLatin1("");
        ProventoRepository repo;
        QVERIFY2(repo.insert(p), "falha ao inserir Provento");
    }

private slots:
    void initTestCase()
    {
        QVERIFY(m_tmp.isValid());
        m_prevCwd = QDir::currentPath();
        // DatabaseManager usa caminhos relativos ("data/<slug>.db"); isolar cwd.
        QVERIFY(QDir::setCurrent(m_tmp.path()));

        const bool ok = DatabaseManager::instance().open(
            QStringLiteral("test-resumo"));
        QVERIFY2(ok, "DatabaseManager::open falhou");
        QVERIFY(DatabaseManager::instance().isOpen());

        // BUG DE PRODUTO conhecido (#116, fora de escopo): AuditLogService cria
        // triggers AFTER INSERT/UPDATE que referenciam colunas inexistentes
        // (ex.: NEW.valor em lancamentos_entradas, cuja coluna real e
        // valor_bruto). Isso faz QUALQUER insert nessas tabelas falhar com
        // "no such column". O ResumoService apenas LE dados, logo a auditoria e
        // irrelevante para esta suite — removemos os triggers audit_* no setup
        // para conseguir popular o banco. Ver "Achados" no relatorio da #116.
        dropTriggersAudit();
    }

    void cleanupTestCase()
    {
        // Nao chamamos close() para evitar criptografia em build release;
        // apenas restaura o cwd. O QTemporaryDir e removido no destrutor.
        if (!m_prevCwd.isEmpty())
            QDir::setCurrent(m_prevCwd);
    }

    void init()
    {
        limparTabelas();   // isolamento: cada teste comeca com banco limpo
    }

    // ───────────────────────────── resumoDoMes ──────────────────────────────

    // ESPEC 1: mes vazio -> todos os campos 0; contadores 0; saldoLiquido 0.
    void mesVazioTudoZero()
    {
        const ResumoMensal r = ResumoService().resumoDoMes(6, 2025);

        QCOMPARE(r.mes, 6);
        QCOMPARE(r.ano, 2025);
        QCOMPARE(r.entradasLiquidas, qint64(0));
        QCOMPARE(r.entradasBrutas,   qint64(0));
        QCOMPARE(r.irpfRetido,       qint64(0));
        QCOMPARE(r.inssRetido,       qint64(0));
        QCOMPARE(r.gastos,           qint64(0));
        QCOMPARE(r.parcelas,         qint64(0));
        QCOMPARE(r.aportes,          qint64(0));
        QCOMPARE(r.resgates,         qint64(0));
        QCOMPARE(r.investimentosNet, qint64(0));
        QCOMPARE(r.cntEntradas, 0);
        QCOMPARE(r.cntGastos,   0);
        QCOMPARE(r.cntParcelas, 0);
        QCOMPARE(r.saldoMes,     qint64(0));
        QCOMPARE(r.saldoLiquido, qint64(0));
    }

    // ESPEC 2: entrada liquida = bruto - irpf - inss; brutos/irpf/inss/cnt.
    void entradaLiquidaCalculada()
    {
        addEntrada(6, 2025, 500000, 50000, 30000);

        const ResumoMensal r = ResumoService().resumoDoMes(6, 2025);

        QCOMPARE(r.entradasBrutas,   qint64(500000));
        QCOMPARE(r.irpfRetido,       qint64(50000));
        QCOMPARE(r.inssRetido,       qint64(30000));
        QCOMPARE(r.entradasLiquidas, qint64(420000)); // 500000-50000-30000
        QCOMPARE(r.cntEntradas, 1);
    }

    // ESPEC 3 (CRITICO, D0): aporte e resgate entram; dividendo/rendimento NAO.
    // investimentosNet = resgates - aportes = 30000 - 100000 = -70000.
    void investimentosD0DividendoForaDoNet()
    {
        addInvestimento(6, 2025, QStringLiteral("aporte"),    100000);
        addInvestimento(6, 2025, QStringLiteral("resgate"),    30000);
        addInvestimento(6, 2025, QStringLiteral("dividendo"),   5000);

        const ResumoMensal r = ResumoService().resumoDoMes(6, 2025);

        QCOMPARE(r.aportes,          qint64(100000));
        QCOMPARE(r.resgates,         qint64(30000));
        // dividendo NAO pode vazar para aportes/resgates/net (fica em Proventos).
        QCOMPARE(r.investimentosNet, qint64(-70000)); // 30000 - 100000
    }

    // Reforco D0: "rendimento" tambem fica fora de aportes/resgates/net.
    void investimentosD0RendimentoForaDoNet()
    {
        addInvestimento(6, 2025, QStringLiteral("rendimento"), 8000);

        const ResumoMensal r = ResumoService().resumoDoMes(6, 2025);

        QCOMPARE(r.aportes,          qint64(0));
        QCOMPARE(r.resgates,         qint64(0));
        QCOMPARE(r.investimentosNet, qint64(0));
    }

    // ESPEC 4 (CRITICO, virada de mes): parcela em dez/2025 conta em (12,2025)
    // e NAO em (1,2026). dataParaParcela e a referencia de vencimento.
    void parcelaViradaDeMes()
    {
        // 3 parcelas comecando em dez/2025 -> dez/2025, jan/2026, fev/2026.
        const Compra c = addCompraParcelada(12, 2025, 3, 90000);

        // Sanidade: a parcela 1 vence em dez/2025 conforme dataParaParcela.
        const QDate dP1 = ParcelaService::dataParaParcela(c, 1);
        QCOMPARE(dP1.year(),  2025);
        QCOMPARE(dP1.month(), 12);
        const QDate dP2 = ParcelaService::dataParaParcela(c, 2);
        QCOMPARE(dP2.year(),  2026);
        QCOMPARE(dP2.month(), 1);

        const ResumoMensal dez = ResumoService().resumoDoMes(12, 2025);
        QCOMPARE(dez.parcelas,    qint64(90000)); // so a parcela 1
        QCOMPARE(dez.cntParcelas, 1);

        const ResumoMensal jan = ResumoService().resumoDoMes(1, 2026);
        QCOMPARE(jan.parcelas,    qint64(90000)); // so a parcela 2
        QCOMPARE(jan.cntParcelas, 1);

        // A parcela de dez NAO vaza para jan: ambos tem exatamente 1 parcela,
        // logo a contagem de dez nao "soma" em jan.
    }

    // ESPEC 5: saldoMes vs saldoLiquido. Com parcelas>0,
    // saldoLiquido == saldoMes - parcelas; saldoMes == liquidas - gastos.
    void saldoMesVsSaldoLiquido()
    {
        addEntrada(6, 2025, 500000, 0, 0);  // liquidas = 500000
        addGasto(6, 2025, 120000);          // gastos   = 120000
        addCompraParcelada(6, 2025, 1, 80000); // parcela jun/2025 = 80000

        const ResumoMensal r = ResumoService().resumoDoMes(6, 2025);

        QCOMPARE(r.parcelas, qint64(80000));
        QCOMPARE(r.saldoMes,     qint64(380000)); // 500000 - 120000
        QCOMPARE(r.saldoLiquido, qint64(300000)); // 380000 - 80000
        QCOMPARE(r.saldoLiquido, r.saldoMes - r.parcelas);
    }

    // ESPEC 6: filtro estrito mes/ano. Mesmo mes, ano diferente nao vaza.
    void filtroEstritoMesAno()
    {
        addEntrada(6, 2025, 100000, 0, 0); // alvo
        addEntrada(6, 2024, 999999, 0, 0); // mesmo mes, outro ano
        addEntrada(7, 2025, 888888, 0, 0); // outro mes, mesmo ano

        const ResumoMensal r = ResumoService().resumoDoMes(6, 2025);

        QCOMPARE(r.entradasLiquidas, qint64(100000));
        QCOMPARE(r.cntEntradas, 1);
    }

    // ───────────────────────────── resumoDoAno ──────────────────────────────

    // ESPEC 7: acumulacao no ano (jan + dez); ano vizinho nao conta.
    void anoAcumulaEntradas()
    {
        addEntrada(1,  2025, 200000, 0, 0);
        addEntrada(12, 2025, 300000, 0, 0);
        addEntrada(6,  2026, 999999, 0, 0); // ano vizinho — nao conta

        const ResumoAnual r = ResumoService().resumoDoAno(2025);

        QCOMPARE(r.ano, 2025);
        QCOMPARE(r.entradasLiquidas, qint64(500000)); // 200000 + 300000
    }

    // ESPEC 8: virada de ano. resumoDoAno(2025) nao inclui dez/2024 nem jan/2026.
    void anoViradaNaoVaza()
    {
        addEntrada(12, 2024, 111111, 0, 0); // ano anterior
        addEntrada(1,  2026, 222222, 0, 0); // ano seguinte
        addEntrada(6,  2025, 400000, 0, 0); // alvo

        const ResumoAnual r = ResumoService().resumoDoAno(2025);
        QCOMPARE(r.entradasLiquidas, qint64(400000));
    }

    // ESPEC 9: proventos do ano via totalLiquidoAno + cntProventos via findByPeriod.
    void proventosDoAno()
    {
        addProvento(QStringLiteral("PETR4"), QStringLiteral("dividendo"),
                    QDate(2025, 3, 10), 12000);
        addProvento(QStringLiteral("HGLG11"), QStringLiteral("rendimento"),
                    QDate(2025, 9, 5), 8000);
        addProvento(QStringLiteral("VALE3"), QStringLiteral("dividendo"),
                    QDate(2024, 12, 31), 50000); // fora do ano

        ProventoRepository repo;
        const qint64 esperadoLiquido = repo.totalLiquidoAno(2025);
        const int    esperadoCnt     = repo.findByPeriod(QDate(2025, 1, 1),
                                                         QDate(2025, 12, 31)).size();

        const ResumoAnual r = ResumoService().resumoDoAno(2025);

        QCOMPARE(r.proventosLiquidos, esperadoLiquido);
        QCOMPARE(r.cntProventos,      esperadoCnt);
        // Sanidade dos valores concretos esperados pela ESPEC.
        QCOMPARE(r.proventosLiquidos, qint64(20000)); // 12000 + 8000
        QCOMPARE(r.cntProventos,      2);
    }

    // ──────────────────────────────── serie ─────────────────────────────────

    // ESPEC 10: intervalo inclusivo [jan, mar] do mesmo ano -> 3 itens,
    // ordem cronologica, cada item igual ao resumoDoMes correspondente.
    void serieInclusivaMesmoAno()
    {
        addEntrada(1, 2025, 100000, 0, 0);
        addEntrada(2, 2025, 200000, 0, 0);
        addEntrada(3, 2025, 300000, 0, 0);

        const QVector<ResumoMensal> s = ResumoService().serie(
            QDate(2025, 1, 1), QDate(2025, 3, 31));

        QCOMPARE(s.size(), 3);
        QCOMPARE(s[0].mes, 1); QCOMPARE(s[0].ano, 2025);
        QCOMPARE(s[1].mes, 2); QCOMPARE(s[1].ano, 2025);
        QCOMPARE(s[2].mes, 3); QCOMPARE(s[2].ano, 2025);

        ResumoService svc;
        for (const ResumoMensal &item : s) {
            const ResumoMensal esperado = svc.resumoDoMes(item.mes, item.ano);
            QCOMPARE(item.entradasLiquidas, esperado.entradasLiquidas);
            QCOMPARE(item.saldoLiquido,     esperado.saldoLiquido);
        }
        QCOMPARE(s[0].entradasLiquidas, qint64(100000));
        QCOMPARE(s[2].entradasLiquidas, qint64(300000));
    }

    // ESPEC 11: intervalo cruzando ano (nov/2025 -> fev/2026) -> 4 itens.
    void serieCruzandoAno()
    {
        addEntrada(11, 2025, 110000, 0, 0);
        addEntrada(12, 2025, 120000, 0, 0);
        addEntrada(1,  2026, 130000, 0, 0);
        addEntrada(2,  2026, 140000, 0, 0);

        const QVector<ResumoMensal> s = ResumoService().serie(
            QDate(2025, 11, 1), QDate(2026, 2, 28));

        QCOMPARE(s.size(), 4);
        QCOMPARE(s[0].mes, 11); QCOMPARE(s[0].ano, 2025);
        QCOMPARE(s[1].mes, 12); QCOMPARE(s[1].ano, 2025);
        QCOMPARE(s[2].mes,  1); QCOMPARE(s[2].ano, 2026);
        QCOMPARE(s[3].mes,  2); QCOMPARE(s[3].ano, 2026);

        QCOMPARE(s[0].entradasLiquidas, qint64(110000));
        QCOMPARE(s[1].entradasLiquidas, qint64(120000));
        QCOMPARE(s[2].entradasLiquidas, qint64(130000));
        QCOMPARE(s[3].entradasLiquidas, qint64(140000));
    }

    // ESPEC 12: mes sem dado no meio aparece com zeros (nao pula);
    // ini invalido ou ini>fim -> vetor vazio.
    void serieMesVazioNoMeioEBordas()
    {
        addEntrada(1, 2025, 100000, 0, 0);
        // fev/2025 sem dado
        addEntrada(3, 2025, 300000, 0, 0);

        const QVector<ResumoMensal> s = ResumoService().serie(
            QDate(2025, 1, 1), QDate(2025, 3, 31));

        QCOMPARE(s.size(), 3);
        QCOMPARE(s[1].mes, 2);                       // fev presente
        QCOMPARE(s[1].entradasLiquidas, qint64(0));  // com zeros, nao pulado
        QCOMPARE(s[1].cntEntradas, 0);

        // ini > fim -> vazio
        const QVector<ResumoMensal> inv1 = ResumoService().serie(
            QDate(2025, 3, 1), QDate(2025, 1, 1));
        QCOMPARE(inv1.size(), 0);

        // ini invalido -> vazio
        const QVector<ResumoMensal> inv2 = ResumoService().serie(
            QDate(), QDate(2025, 3, 1));
        QCOMPARE(inv2.size(), 0);
    }
};

QTEST_MAIN(TestResumo)
#include "test_resumo.moc"
