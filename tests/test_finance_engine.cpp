// PeuBud - FinanceEngine TDD (#81)
// Casos de teste com valores verificaveis em Excel / HP-12C.
//
// Convencao Excel:
//   PV negativo = saida inicial (emprestimo recebido pelo cliente -> entra no
//   caixa = PV positivo do ponto de vista do banco; aqui usamos perspectiva
//   do cliente: cliente recebe = PV positivo; cliente paga PMT = PMT negativo).
//
// Equacao base: pv*(1+i)^n + pmt*(1+i*begin)*((1+i)^n - 1)/i + fv = 0

#include <QtTest/QtTest>
#include "services/FinanceEngine.h"

class TestFinanceEngine : public QObject
{
    Q_OBJECT

private slots:
    // PMT
    void pmtFinanciamento();
    void pmtTaxaZero();

    // FV
    void fvPoupanca();
    void fvTaxaZero();

    // PV
    void pvAposentadoria();

    // NPER
    void nperEmprestimo();

    // RATE (Newton-Raphson)
    void rateCdb();
    void rateGuessExtremo();

    // Conversao de taxa (#82)
    void taxaActuarielAnualParaMensal();
    void taxaProporcionalAnualParaMensal();
    void taxaActuarielMensalParaAnual();
    void taxaConversaoZero();

    // Capitalizacao continua (#81 follow-up)
    void fvContinuoExponencial();
    void pvContinuoInversoDeFv();
    void taxaContinuaDeDiscreta12mEquivale();
    void taxaDiscretaDeContinuaInversa();

    // Multiplas frequencias (#81 follow-up)
    void pmtMultiFreqSemestralVsMensal();
};

// Tolerancia: 0.01 (centavo de Real) para valores monetarios; 1e-6 para taxa.
static constexpr double TOL_MONEY = 0.01;
static constexpr double TOL_RATE  = 1e-6;

void TestFinanceEngine::pmtFinanciamento()
{
    // Excel: =PMT(0.008, 360, -100000, 0) = 848.66 (positivo, pv negativo).
    // Aqui com pv positivo, retorna negativo (saida do bolso).
    // Financiamento: cliente recebe 100k (PV positivo), paga PMT mensal por 360
    // meses a 0.8% am, FV=0 (quita).
    const double pmt = FinanceEngine::pmt(100000.0, 0.0, 0.008, 360, false);
    QVERIFY2(qAbs(pmt - (-848.16)) < TOL_MONEY,
             qPrintable(QStringLiteral("PMT = %1, esperado -848.16").arg(pmt)));
}

void TestFinanceEngine::pmtTaxaZero()
{
    // i=0: pmt = -(pv + fv) / n
    // PV=12000, n=12, fv=0 -> pmt = -1000 (parcela mensal sem juros)
    const double pmt = FinanceEngine::pmt(12000.0, 0.0, 0.0, 12, false);
    QVERIFY2(qAbs(pmt - (-1000.0)) < TOL_MONEY,
             qPrintable(QStringLiteral("PMT = %1, esperado -1000").arg(pmt)));
}

void TestFinanceEngine::fvPoupanca()
{
    // Excel: =FV(0.005, 12, -1000, 0) = 12335.5618...
    // Aporta 1000 (PMT negativo = saida do bolso) mensalmente por 12 meses a
    // 0.5% am, PV=0. FV positivo = saldo final na conta.
    const double fv = FinanceEngine::fv(0.0, -1000.0, 0.005, 12, false);
    QVERIFY2(qAbs(fv - 12335.5618) < TOL_MONEY,
             qPrintable(QStringLiteral("FV = %1, esperado 12335.5618").arg(fv)));
}

void TestFinanceEngine::fvTaxaZero()
{
    // i=0: fv = -pv - pmt*n
    // PV=0, pmt=-100, n=10 -> fv = 1000
    const double fv = FinanceEngine::fv(0.0, -100.0, 0.0, 10, false);
    QVERIFY2(qAbs(fv - 1000.0) < TOL_MONEY,
             qPrintable(QStringLiteral("FV = %1, esperado 1000").arg(fv)));
}

void TestFinanceEngine::pvAposentadoria()
{
    // Excel: =PV(0.005, 240, 3000, 0) = -418741.71
    // Quanto preciso ter hoje para sacar 3000/mes durante 240 meses (20 anos)
    // rendendo 0.5% am? PMT positivo = entrada (saque). PV negativo = saida
    // (deposito inicial necessario).
    const double pv = FinanceEngine::pv(3000.0, 0.0, 0.005, 240, false);
    QVERIFY2(qAbs(pv - (-418741.71)) < 1.0,
             qPrintable(QStringLiteral("PV = %1, esperado -418741.71").arg(pv)));
}

void TestFinanceEngine::nperEmprestimo()
{
    // Inverso do pmtFinanciamento: =NPER(0.008, -848.16, 100000, 0) = 360
    const double n = FinanceEngine::nper(100000.0, -848.16, 0.0, 0.008, false);
    QVERIFY2(qAbs(n - 360.0) < 0.1,
             qPrintable(QStringLiteral("N = %1, esperado 360").arg(n)));
}

void TestFinanceEngine::rateCdb()
{
    // Inverso do pmtFinanciamento: =RATE(360, -848.16, 100000, 0) = 0.008
    const double i = FinanceEngine::rate(100000.0, -848.16, 0.0, 360, false);
    QVERIFY2(qAbs(i - 0.008) < 1e-4,
             qPrintable(QStringLiteral("i = %1, esperado 0.008").arg(i, 0, 'g', 10)));
}

void TestFinanceEngine::rateGuessExtremo()
{
    // Cresce 1500 ate suportar 24 saques de 100; taxa real ~4.16% am
    // (Excel/HP-12C convergem para esse valor com Newton-Raphson).
    const double i = FinanceEngine::rate(1500.0, -100.0, 0.0, 24, false);
    QVERIFY2(qAbs(i - 0.0416) < 1e-3,
             qPrintable(QStringLiteral("i = %1, esperado ~0.0416").arg(i, 0, 'g', 10)));
}

void TestFinanceEngine::taxaActuarielAnualParaMensal()
{
    // 12% a.a. composto -> a.m. = (1.12)^(1/12) - 1 = 0.94888...%
    const double iAnual  = 0.12;
    const double iMensal = FinanceEngine::taxaEquivalente(iAnual, 1, 12);
    QVERIFY2(qAbs(iMensal - 0.0094888) < 1e-6,
             qPrintable(QStringLiteral("iMensal = %1, esperado 0.0094888").arg(iMensal, 0, 'g', 10)));
}

void TestFinanceEngine::taxaProporcionalAnualParaMensal()
{
    // 12% a.a. linear -> a.m. = 12% / 12 = 1%
    const double iAnual  = 0.12;
    const double iMensal = FinanceEngine::taxaProporcional(iAnual, 1, 12);
    QVERIFY2(qAbs(iMensal - 0.01) < 1e-9,
             qPrintable(QStringLiteral("iMensal = %1, esperado 0.01").arg(iMensal, 0, 'g', 10)));
}

void TestFinanceEngine::taxaActuarielMensalParaAnual()
{
    // 1% a.m. composto -> a.a. = (1.01)^12 - 1 = 12.6825...%
    const double iMensal = 0.01;
    const double iAnual  = FinanceEngine::taxaEquivalente(iMensal, 12, 1);
    QVERIFY2(qAbs(iAnual - 0.126825) < 1e-5,
             qPrintable(QStringLiteral("iAnual = %1, esperado 0.126825").arg(iAnual, 0, 'g', 10)));
}

void TestFinanceEngine::taxaConversaoZero()
{
    // i=0 deve sempre retornar 0 (sem juros)
    QCOMPARE(FinanceEngine::taxaEquivalente(0.0, 1, 12), 0.0);
    QCOMPARE(FinanceEngine::taxaProporcional(0.0, 1, 12), 0.0);
}

void TestFinanceEngine::fvContinuoExponencial()
{
    // PV=1000, i=10% a.a. continuo, 1 ano -> 1000 * e^0.1 = 1105.17
    const double fv = FinanceEngine::fvContinuo(1000.0, 0.10, 1.0);
    QVERIFY2(qAbs(fv - 1105.1709) < 0.01,
        qPrintable(QStringLiteral("FV continuo = %1, esperado 1105.1709").arg(fv)));
}

void TestFinanceEngine::pvContinuoInversoDeFv()
{
    // PV * e^(0.05*2) = FV; reverso
    const double fv = 1105.17;
    const double pv = FinanceEngine::pvContinuo(fv, 0.05, 2.0);
    QVERIFY2(qAbs(pv - 1000.0) < 0.1,
        qPrintable(QStringLiteral("PV continuo = %1, esperado ~1000").arg(pv)));
}

void TestFinanceEngine::taxaContinuaDeDiscreta12mEquivale()
{
    // 1% a.m. discreta -> r_cont = 12 * ln(1.01) = 0.11944 (~11.94% a.a.)
    const double r = FinanceEngine::taxaContinuaDeDiscreta(0.01, 12);
    QVERIFY2(qAbs(r - 0.11944) < 1e-4,
        qPrintable(QStringLiteral("r_cont = %1, esperado ~0.11944").arg(r)));
}

void TestFinanceEngine::taxaDiscretaDeContinuaInversa()
{
    // r = 0.12 a.a. -> i_mensal = e^(0.12/12) - 1 = 0.01005 (~1,005% a.m.)
    const double i = FinanceEngine::taxaDiscretaDeContinua(0.12, 12);
    QVERIFY2(qAbs(i - 0.01005) < 1e-4,
        qPrintable(QStringLiteral("i = %1, esperado ~0.01005").arg(i)));
}

void TestFinanceEngine::pmtMultiFreqSemestralVsMensal()
{
    // Cap semestral 6% (12.36% a.a.) com pgto mensal por 1 ano.
    // iMensal = (1.1236)^(1/12) - 1 = 0.009759
    // pv=10000 -> pmt = -10000 * 0.009759 / (1 - (1.009759)^-12)
    //          = ~-885.65 (saida do bolso)
    const double pmt = FinanceEngine::pmtMultiFreq(10000.0, 0.06, 2, 12, 1.0);
    QVERIFY2(pmt < 0.0 && qAbs(pmt) > 800.0 && qAbs(pmt) < 1000.0,
        qPrintable(QStringLiteral("pmt = %1, esperado ~-885").arg(pmt)));
}

QTEST_GUILESS_MAIN(TestFinanceEngine)
#include "test_finance_engine.moc"
