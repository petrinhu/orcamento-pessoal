// PeuBud - DripSimulator TDD (#63)
// Service stateless puro - sem dependencia de DB.

#include <QtTest/QtTest>
#include "services/DripSimulator.h"

#include <cmath>

class TestDrip : public QObject
{
    Q_OBJECT
private slots:
    void zeroYieldZeroValorizacao();
    void yield6PorCentoAumentaQty();
    void semDripAcumulaCashSeparado();
    void aporteSemYieldNemValorizacao();
    void comparativoCdi13PorCento();
    void mesesZeroNaoCrash();
    void cagrPatrimonialBasico();
};

void TestDrip::zeroYieldZeroValorizacao()
{
    DripSimulator::Input in;
    in.precoInicial      = 100.0;
    in.quantidadeInicial = 10.0;
    in.aporteMensal      = 0.0;
    in.dividendYieldAnual = 0.0;
    in.valorizacaoAnual  = 0.0;
    in.meses             = 12;
    in.reinvestirDividendos = true;

    auto r = DripSimulator::simular(in);
    // Sem dividendos e sem valorizacao: patrimonio constante = qty * preco
    QCOMPARE(r.quantidadeFinal, 10.0);
    QCOMPARE(r.precoFinal, 100.0);
    QCOMPARE(r.patrimonioFinal, 1000.0);
    QCOMPARE(r.dividendosTotais, 0.0);
    QCOMPARE(r.serieMensal.size(), 12);
}

void TestDrip::yield6PorCentoAumentaQty()
{
    DripSimulator::Input in;
    in.precoInicial      = 100.0;
    in.quantidadeInicial = 100.0;  // patrimonio inicial 10000
    in.aporteMensal      = 0.0;
    in.dividendYieldAnual = 0.06;  // 6% a.a.
    in.valorizacaoAnual  = 0.0;
    in.meses             = 12;
    in.reinvestirDividendos = true;

    auto r = DripSimulator::simular(in);
    // DRIP com 0.5%/mes: qty cresce ~ (1.005)^12 ≈ 1.0617
    const double esperadoQty = 100.0 * std::pow(1.005, 12);
    QVERIFY2(qAbs(r.quantidadeFinal - esperadoQty) < esperadoQty * 0.01,
             qPrintable(QStringLiteral("qtyFinal=%1 esperado≈%2")
                .arg(r.quantidadeFinal).arg(esperadoQty)));
    // Patrimonio cresceu ~6% (sem valorizacao de preco)
    QVERIFY(r.patrimonioFinal > 10500.0);
    QVERIFY(r.patrimonioFinal < 10700.0);
}

void TestDrip::semDripAcumulaCashSeparado()
{
    DripSimulator::Input in;
    in.precoInicial      = 100.0;
    in.quantidadeInicial = 100.0;
    in.aporteMensal      = 0.0;
    in.dividendYieldAnual = 0.06;
    in.valorizacaoAnual  = 0.0;
    in.meses             = 12;
    in.reinvestirDividendos = false;  // cash mode

    auto r = DripSimulator::simular(in);
    // Sem DRIP: qty fica em 100, dividendos somam separado
    QCOMPARE(r.quantidadeFinal, 100.0);
    QCOMPARE(r.precoFinal, 100.0);
    QCOMPARE(r.patrimonioFinal, 10000.0);
    // 12 meses * (10000 * 0.06/12) = 12 * 50 = 600
    QVERIFY(qAbs(r.dividendosTotais - 600.0) < 1.0);
}

void TestDrip::aporteSemYieldNemValorizacao()
{
    DripSimulator::Input in;
    in.precoInicial      = 100.0;
    in.quantidadeInicial = 0.0;
    in.aporteMensal      = 1000.0;
    in.dividendYieldAnual = 0.0;
    in.valorizacaoAnual  = 0.0;
    in.meses             = 12;

    auto r = DripSimulator::simular(in);
    // 12 aportes de 1000 -> 12000 acumulado, preco constante 100
    QCOMPARE(r.aportesTotais, 12000.0);
    QVERIFY(qAbs(r.patrimonioFinal - 12000.0) < 0.01);
    QVERIFY(qAbs(r.quantidadeFinal - 120.0) < 0.0001);
}

void TestDrip::comparativoCdi13PorCento()
{
    DripSimulator::ComparativoInput in;
    in.aporteMensal     = 1000.0;
    in.meses            = 12;
    in.taxaCdiAnual     = 0.13;
    in.taxaIpcaAnual    = 0.045;
    in.retornoIbovAnual = 0.09;

    auto r = DripSimulator::comparativo(in);
    // FV anuidade mensal: PMT * ((1+rm)^n - 1) / rm
    // rm_cdi = (1.13)^(1/12) - 1 ≈ 0.01024
    // FV ≈ 1000 * ((1.01024)^12 - 1) / 0.01024 ≈ 12704
    QVERIFY2(r.cdi > 12500 && r.cdi < 12900,
             qPrintable(QStringLiteral("CDI 12m esperado ~12704, got %1").arg(r.cdi)));
    // IPCA 4.5%: rm ≈ 0.003675; FV ≈ 1000 * 12.246 ≈ 12246
    QVERIFY(r.ipca > 12100 && r.ipca < 12350);
    // IBOV 9%: FV ≈ 12463
    QVERIFY(r.ibov > 12300 && r.ibov < 12600);
    // Coerencia: maiores taxas -> maior FV
    QVERIFY(r.cdi > r.ibov);
    QVERIFY(r.ibov > r.ipca);
}

void TestDrip::mesesZeroNaoCrash()
{
    DripSimulator::Input in;
    in.precoInicial      = 100.0;
    in.quantidadeInicial = 10.0;
    in.meses             = 0;
    auto r = DripSimulator::simular(in);
    QCOMPARE(r.quantidadeFinal, 10.0);
    QCOMPARE(r.patrimonioFinal, 1000.0);
    QCOMPARE(r.serieMensal.size(), 0);

    DripSimulator::ComparativoInput ci;
    ci.aporteMensal = 1000.0;
    ci.meses = 0;
    auto cr = DripSimulator::comparativo(ci);
    QCOMPARE(cr.cdi, 0.0);
    QCOMPARE(cr.ipca, 0.0);
    QCOMPARE(cr.ibov, 0.0);
}

void TestDrip::cagrPatrimonialBasico()
{
    DripSimulator::Input in;
    in.precoInicial      = 100.0;
    in.quantidadeInicial = 10.0;
    in.aporteMensal      = 0.0;
    in.dividendYieldAnual = 0.0;
    in.valorizacaoAnual  = 0.10;  // 10% a.a. preco
    in.meses             = 24;
    in.reinvestirDividendos = false;

    auto r = DripSimulator::simular(in);
    // Sem yield/aporte: patrimonio = qty * preco com preco cresce 10% a.a.
    // FV/PV ratio = (1.1)^2 = 1.21
    // CAGR = (1.21)^(12/24) - 1 = (1.21)^0.5 - 1 ≈ 0.10 (10%)
    QVERIFY(qAbs(r.cagrPatrimonial - 0.10) < 0.01);
}

QTEST_GUILESS_MAIN(TestDrip)
#include "test_drip.moc"
