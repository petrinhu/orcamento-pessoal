// PeuBud - TvmService caracterizacao (#114)
// Rede de protecao para o motor financeiro ANTES da refatoracao fiscal (#113).
// Caracteriza o comportamento ATUAL de:
//   - TvmService::resolver  (caminho feliz + std::domain_error)
//   - TvmService::amortizacao (Price/SAF schedule: saldo->0, soma==total,
//                              juros decrescentes, bordas n=1 e i=0)
//
// Valores conferidos contra Excel / HP-12C. Convencao de sinal HP-12C:
//   PV positivo (cliente recebe), PMT negativo (cliente paga).
// Tolerancia: TOL_MONEY (1 centavo) em valores monetarios; TOL_RATE em taxas.

#include <QtTest/QtTest>
#include "services/TvmService.h"

#include <stdexcept>

class TestTvm : public QObject
{
    Q_OBJECT
private slots:
    // resolver - caminho feliz (inverso mutuo dos 5 TVM)
    void resolverPmtFinanciamento();
    void resolverPvDeAposentadoria();
    void resolverNperDeEmprestimo();
    void resolverFvDePoupanca();
    void resolverTaxaDeFinanciamento();

    // resolver - excecoes documentadas (std::domain_error)
    void resolverLancaDomainErrorQuandoNInvalidoPmt();
    void resolverLancaDomainErrorQuandoNInvalidoPv();
    void resolverLancaDomainErrorQuandoNInvalidoFv();
    void resolverLancaDomainErrorQuandoNInvalidoI();
    void resolverNperSemSolucaoReal();

    // amortizacao - invariantes Price/SAF
    void amortizacaoSaldoFinalZera();
    void amortizacaoSomaParcelasIgualTotalPago();
    void amortizacaoJurosDecrescentes();
    void amortizacaoPrimeiraParcelaMaisJurosQueUltima();

    // amortizacao - bordas
    void amortizacaoParcelaUnica();
    void amortizacaoTaxaZero();
    void amortizacaoNZeroRetornaVazio();
};

static constexpr double TOL_MONEY = 0.01;
static constexpr double TOL_RATE  = 1e-4;

// ── resolver: caminho feliz ───────────────────────────────────────────────────

void TestTvm::resolverPmtFinanciamento()
{
    // arrange: financiamento 100k, 360 meses a 0.8% a.m., FV=0.
    // Excel: =PMT(0.008;360;-100000;0) = 848.16. PV positivo -> PMT negativo.
    TvmInput in;
    in.pv  = 100000.0;
    in.fv  = 0.0;
    in.i   = 0.8;   // % por periodo (TvmInput usa percentagem)
    in.n   = 360.0;
    in.begin = false;

    // act
    const TvmInput out = TvmService::resolver(in, TvmSolve::PMT);

    // assert
    QVERIFY2(qAbs(out.pmt - (-848.16)) < TOL_MONEY,
        qPrintable(QStringLiteral("PMT = %1, esperado -848.16").arg(out.pmt)));
}

void TestTvm::resolverPvDeAposentadoria()
{
    // arrange: sacar 3000/mes por 240 meses a 0.5% a.m.
    // Excel: =PV(0.005;240;3000;0) = -418741.71
    TvmInput in;
    in.pmt = 3000.0;
    in.fv  = 0.0;
    in.i   = 0.5;
    in.n   = 240.0;

    // act
    const TvmInput out = TvmService::resolver(in, TvmSolve::PV);

    // assert (1 real de tolerancia por acumulo de 240 periodos)
    QVERIFY2(qAbs(out.pv - (-418741.71)) < 1.0,
        qPrintable(QStringLiteral("PV = %1, esperado -418741.71").arg(out.pv)));
}

void TestTvm::resolverNperDeEmprestimo()
{
    // arrange: inverso do financiamento -> n = 360
    // Excel: =NPER(0.008;-848.16;100000;0) = 360
    TvmInput in;
    in.pv  = 100000.0;
    in.pmt = -848.16;
    in.fv  = 0.0;
    in.i   = 0.8;

    // act
    const TvmInput out = TvmService::resolver(in, TvmSolve::N);

    // assert
    QVERIFY2(qAbs(out.n - 360.0) < 0.1,
        qPrintable(QStringLiteral("N = %1, esperado 360").arg(out.n)));
}

void TestTvm::resolverFvDePoupanca()
{
    // arrange: aporta 1000/mes (saida = PMT negativo) por 12 meses a 0.5% a.m.
    // Excel: =FV(0.005;12;-1000;0) = 12335.5618
    TvmInput in;
    in.pv  = 0.0;
    in.pmt = -1000.0;
    in.i   = 0.5;
    in.n   = 12.0;

    // act
    const TvmInput out = TvmService::resolver(in, TvmSolve::FV);

    // assert
    QVERIFY2(qAbs(out.fv - 12335.5618) < TOL_MONEY,
        qPrintable(QStringLiteral("FV = %1, esperado 12335.5618").arg(out.fv)));
}

void TestTvm::resolverTaxaDeFinanciamento()
{
    // arrange: inverso do financiamento -> i = 0.8% a.m.
    // Excel: =RATE(360;-848.16;100000;0) = 0.008 -> 0.8 em percentagem.
    TvmInput in;
    in.pv  = 100000.0;
    in.pmt = -848.16;
    in.fv  = 0.0;
    in.n   = 360.0;

    // act
    const TvmInput out = TvmService::resolver(in, TvmSolve::I);

    // assert (out.i em %)
    QVERIFY2(qAbs(out.i - 0.8) < TOL_RATE * 100.0,
        qPrintable(QStringLiteral("i = %1%%, esperado 0.8%%").arg(out.i, 0, 'g', 10)));
}

// ── resolver: excecoes documentadas ──────────────────────────────────────────
// O header documenta: "Throws std::domain_error if the equation has no real
// solution." Em pratica, o cpp lanca domain_error para n<=0 ao resolver
// I/PV/PMT/FV. N (nper) nao valida n previamente -> ver caso separado.

void TestTvm::resolverLancaDomainErrorQuandoNInvalidoPmt()
{
    TvmInput in;
    in.pv = 1000.0; in.fv = 0.0; in.i = 1.0; in.n = 0.0;
    QVERIFY_THROWS_EXCEPTION(std::domain_error,
                             TvmService::resolver(in, TvmSolve::PMT));
}

void TestTvm::resolverLancaDomainErrorQuandoNInvalidoPv()
{
    TvmInput in;
    in.pmt = -100.0; in.fv = 0.0; in.i = 1.0; in.n = 0.0;
    QVERIFY_THROWS_EXCEPTION(std::domain_error,
                             TvmService::resolver(in, TvmSolve::PV));
}

void TestTvm::resolverLancaDomainErrorQuandoNInvalidoFv()
{
    TvmInput in;
    in.pv = 1000.0; in.pmt = -100.0; in.i = 1.0; in.n = -3.0;
    QVERIFY_THROWS_EXCEPTION(std::domain_error,
                             TvmService::resolver(in, TvmSolve::FV));
}

void TestTvm::resolverLancaDomainErrorQuandoNInvalidoI()
{
    TvmInput in;
    in.pv = 1000.0; in.pmt = -100.0; in.fv = 0.0; in.n = 0.0;
    QVERIFY_THROWS_EXCEPTION(std::domain_error,
                             TvmService::resolver(in, TvmSolve::I));
}

void TestTvm::resolverNperSemSolucaoReal()
{
    // Cenario sem solucao real: PV>0 e PMT no mesmo sentido (tambem entrada),
    // i>0 -> a divida nunca quita; NPER nao converge para n finito real.
    // Caracteriza o que a implementacao ATUAL faz (FinanceEngine::nper):
    // documentamos o comportamento observado (nan/inf ou excecao).
    TvmInput in;
    in.pv  = 1000.0;
    in.pmt = 100.0;   // mesmo sinal de PV: nunca amortiza
    in.fv  = 0.0;
    in.i   = 1.0;

    bool lancou = false;
    double nObtido = 0.0;
    try {
        const TvmInput out = TvmService::resolver(in, TvmSolve::N);
        nObtido = out.n;
    } catch (const std::exception &) {
        lancou = true;
    }
    // Aceitavel: lancar excecao OU retornar valor nao-finito (nan/inf).
    // Falha real seria retornar um n finito positivo plausivel (mascarando
    // um cenario impossivel). Documenta-se a borda aqui.
    const bool naoFinito = std::isnan(nObtido) || std::isinf(nObtido);
    QVERIFY2(lancou || naoFinito || nObtido <= 0.0,
        qPrintable(QStringLiteral("NPER sem solucao retornou n=%1 (esperado "
                                  "excecao, nao-finito ou <=0)").arg(nObtido)));
}

// ── amortizacao: invariantes Price/SAF ───────────────────────────────────────
// Convencao HP-12C: pv positivo, pmt negativo. saldo inicia em pv e deve
// convergir a ~0 na ultima linha (TvmService::amortizacao NAO absorve residuo
// na ultima parcela; saldo final pode ter alguns centavos de residuo - isto e
// caracterizado, nao corrigido).

static TvmInput makeAmort(double pv, double iPct, int n)
{
    TvmInput in;
    in.pv = pv;
    in.i  = iPct;
    in.n  = static_cast<double>(n);
    in.fv = 0.0;
    in.begin = false;
    // pmt resolvido pela propria engine (caminho de producao real)
    in.pmt = TvmService::resolver(in, TvmSolve::PMT).pmt;
    return in;
}

void TestTvm::amortizacaoSaldoFinalZera()
{
    // arrange: 10000 em 12x a 1% a.m.
    const TvmInput in = makeAmort(10000.0, 1.0, 12);
    const QDate inicio(2025, 1, 1);

    // act
    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, inicio);

    // assert: 12 linhas e saldo final ~0 (residuo de arredondamento de PMT).
    QCOMPARE(rows.size(), 12);
    const qint64 saldoFinal = rows.last().saldoDevedor; // centavos
    QVERIFY2(qAbs(saldoFinal) <= 100, // <= R$ 1,00 de residuo tolerado
        qPrintable(QStringLiteral("saldo final = %1 centavos, esperado ~0")
                       .arg(saldoFinal)));
}

void TestTvm::amortizacaoSomaParcelasIgualTotalPago()
{
    // arrange
    const TvmInput in = makeAmort(10000.0, 1.0, 12);
    const QDate inicio(2025, 1, 1);

    // act
    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, inicio);

    // assert: soma das parcelas == |pmt| * n (em centavos), sem centavo perdido.
    qint64 soma = 0;
    for (const auto &r : rows) soma += r.parcela;
    const qint64 esperado =
        static_cast<qint64>(std::llround(-in.pmt * 100.0)) * rows.size();
    QCOMPARE(soma, esperado);
}

void TestTvm::amortizacaoJurosDecrescentes()
{
    // arrange
    const TvmInput in = makeAmort(10000.0, 1.0, 12);
    const QDate inicio(2025, 1, 1);

    // act
    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, inicio);

    // assert: em Price, juros caem monotonicamente periodo a periodo.
    for (int k = 1; k < rows.size(); ++k) {
        QVERIFY2(rows[k].juros < rows[k-1].juros,
            qPrintable(QStringLiteral("juros[%1]=%2 nao < juros[%3]=%4")
                .arg(k).arg(rows[k].juros).arg(k-1).arg(rows[k-1].juros)));
    }
}

void TestTvm::amortizacaoPrimeiraParcelaMaisJurosQueUltima()
{
    // arrange
    const TvmInput in = makeAmort(10000.0, 1.0, 12);
    const QDate inicio(2025, 1, 1);

    // act
    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, inicio);

    // assert
    QVERIFY(rows.first().juros > rows.last().juros);
    // 1a parcela: juros sobre saldo cheio = 10000 * 1% = 100,00 = 10000 cents
    QVERIFY2(qAbs(rows.first().juros - 10000) <= 1,
        qPrintable(QStringLiteral("juros 1a parcela = %1, esperado ~10000")
                       .arg(rows.first().juros)));
}

// ── amortizacao: bordas ──────────────────────────────────────────────────────

void TestTvm::amortizacaoParcelaUnica()
{
    // arrange: n=1, 1000 a 2% a.m. -> pmt cobre principal + 1 juro.
    const TvmInput in = makeAmort(1000.0, 2.0, 1);
    const QDate inicio(2025, 6, 1);

    // act
    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, inicio);

    // assert
    QCOMPARE(rows.size(), 1);
    // juros = 1000 * 2% = 20,00 ; amortizacao = principal inteiro (1000,00)
    QVERIFY2(qAbs(rows.first().juros - 2000) <= 1,
        qPrintable(QStringLiteral("juros = %1, esperado ~2000").arg(rows.first().juros)));
    QVERIFY2(qAbs(rows.first().amortizacao - 100000) <= 1,
        qPrintable(QStringLiteral("amort = %1, esperado ~100000").arg(rows.first().amortizacao)));
    QVERIFY2(qAbs(rows.first().saldoDevedor) <= 1,
        qPrintable(QStringLiteral("saldo final = %1, esperado ~0").arg(rows.first().saldoDevedor)));
}

void TestTvm::amortizacaoTaxaZero()
{
    // arrange: sem juros, 1200 em 12x -> pmt = -100,00 ; juros sempre 0.
    TvmInput in;
    in.pv = 1200.0; in.i = 0.0; in.n = 12.0; in.fv = 0.0; in.begin = false;
    in.pmt = TvmService::resolver(in, TvmSolve::PMT).pmt; // -100
    const QDate inicio(2025, 1, 1);

    // act
    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, inicio);

    // assert
    QCOMPARE(rows.size(), 12);
    for (const auto &r : rows)
        QCOMPARE(r.juros, qint64(0));
    QVERIFY2(qAbs(rows.last().saldoDevedor) <= 1,
        qPrintable(QStringLiteral("saldo final = %1, esperado 0").arg(rows.last().saldoDevedor)));
}

void TestTvm::amortizacaoNZeroRetornaVazio()
{
    TvmInput in;
    in.pv = 1000.0; in.i = 1.0; in.n = 0.0; in.pmt = -100.0;
    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, QDate(2025, 1, 1));
    QVERIFY(rows.isEmpty());
}

QTEST_GUILESS_MAIN(TestTvm)
#include "test_tvm.moc"
