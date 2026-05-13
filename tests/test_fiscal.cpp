// PeuBud - FiscalService TDD (#61 #62)
// Casos baseados em calculadoras fiscais publicas (Receita Federal, Sebrae,
// calculadorabasica.com.br) com valores verificaveis.

#include <QtTest/QtTest>
#include "services/FiscalService.h"

class TestFiscal : public QObject
{
    Q_OBJECT

private slots:
    // INSS
    void inssIsentoZero();
    void inssPrimeiraFaixa();
    void inssQuartaFaixa();
    void inssAcimaDoTeto();

    // IRPF
    void irpfIsento();
    void irpfPrimeiraFaixa();
    void irpfFaixaMaxima();

    // Simples Nacional
    void simplesAnexoIPrimeiraFaixa();
    void simplesAnexoIIIIntermediaria();
    void simplesForaDoSimples();

    // Lucro Presumido
    void lucroPresumido();

    // Poupanca / IR renda fixa
    void poupancaSelicAlta();
    void poupancaSelicBaixa();
    void irRendaFixaProgressao();
    void iofRegressivoAteJanela();
};

void TestFiscal::inssIsentoZero()
{
    QCOMPARE(FiscalService::inssMensal2024(0), qint64(0));
}

void TestFiscal::inssPrimeiraFaixa()
{
    // R$ 1.000,00 * 7,5% = R$ 75,00
    QCOMPARE(FiscalService::inssMensal2024(100000), qint64(7500));
}

void TestFiscal::inssQuartaFaixa()
{
    // R$ 5.000,00:
    //   1412,00 * 7,5%   = 105,90
    //   1254,68 * 9%     = 112,9212
    //   1333,35 * 12%    = 160,0020
    //   1000,00 * 14%    = 140,00 (faixa parcial ate 5000-4000,03)
    // Total approx R$ 518,82
    const qint64 inss = FiscalService::inssMensal2024(500000);
    QVERIFY2(qAbs(inss - 51882) <= 5,
        qPrintable(QStringLiteral("INSS = %1, esperado ~51882").arg(inss)));
}

void TestFiscal::inssAcimaDoTeto()
{
    // R$ 10.000,00 ultrapassa teto: deve fechar em R$ 908,85
    QCOMPARE(FiscalService::inssMensal2024(1000000), qint64(90885));
}

void TestFiscal::irpfIsento()
{
    QCOMPARE(FiscalService::irpfMensal2024(200000), qint64(0));
    QCOMPARE(FiscalService::irpfMensal2024(225920), qint64(0));
}

void TestFiscal::irpfPrimeiraFaixa()
{
    // Base R$ 2.500,00 -> 7,5% * 2500 - 169,44 = 187,50 - 169,44 = 18,06
    const qint64 irpf = FiscalService::irpfMensal2024(250000);
    QVERIFY2(qAbs(irpf - 1806) <= 5,
        qPrintable(QStringLiteral("IRPF = %1, esperado ~1806").arg(irpf)));
}

void TestFiscal::irpfFaixaMaxima()
{
    // Base R$ 10.000,00 -> 27,5% * 10000 - 896,00 = 2750 - 896 = 1854,00
    const qint64 irpf = FiscalService::irpfMensal2024(1000000);
    QCOMPARE(irpf, qint64(185400));
}

void TestFiscal::simplesAnexoIPrimeiraFaixa()
{
    // RBT12 = R$ 100.000,00 (1a faixa, ate 180k) -> 4% nominal, PD=0
    // Aliquota efetiva = 4%
    const double aliq = FiscalService::simplesAliquotaEfetiva(1, 10000000);
    QVERIFY2(qAbs(aliq - 0.040) < 1e-6,
        qPrintable(QStringLiteral("aliq = %1, esperado 0.040").arg(aliq)));
}

void TestFiscal::simplesAnexoIIIIntermediaria()
{
    // RBT12 = R$ 300.000,00 (2a faixa 180k-360k), anexo III:
    // aliquota nominal 11,2%, PD 9360,00
    // Efetiva = (300000 * 0.112 - 9360) / 300000 = (33600 - 9360) / 300000
    //        = 24240 / 300000 = 0.0808 (8,08%)
    const double aliq = FiscalService::simplesAliquotaEfetiva(3, 30000000);
    QVERIFY2(qAbs(aliq - 0.0808) < 1e-4,
        qPrintable(QStringLiteral("aliq = %1, esperado 0.0808").arg(aliq)));
}

void TestFiscal::simplesForaDoSimples()
{
    // Anexo invalido = 0%
    QCOMPARE(FiscalService::simplesAliquotaEfetiva(0,    50000000), 0.0);
    QCOMPARE(FiscalService::simplesAliquotaEfetiva(99,   50000000), 0.0);
}

void TestFiscal::lucroPresumido()
{
    QCOMPARE(FiscalService::lucroPresumidoServicosAliquota(), 0.1633);
}

void TestFiscal::poupancaSelicAlta()
{
    // Selic 11,75% > 8,5% -> 0,5% a.m.
    QCOMPARE(FiscalService::poupancaMensal(0.1175), 0.005);
}

void TestFiscal::poupancaSelicBaixa()
{
    // Selic 6% < 8,5% -> 70% * 6% = 4,2% a.a.
    // Mensal = (1,042)^(1/12) - 1 ~= 0,3437% a.m.
    const double m = FiscalService::poupancaMensal(0.06);
    QVERIFY2(qAbs(m - 0.003437) < 1e-5,
        qPrintable(QStringLiteral("m = %1, esperado ~0.003437").arg(m)));
}

void TestFiscal::irRendaFixaProgressao()
{
    QCOMPARE(FiscalService::irRendaFixaPorDias(30),   0.225);
    QCOMPARE(FiscalService::irRendaFixaPorDias(180),  0.225);
    QCOMPARE(FiscalService::irRendaFixaPorDias(181),  0.200);
    QCOMPARE(FiscalService::irRendaFixaPorDias(360),  0.200);
    QCOMPARE(FiscalService::irRendaFixaPorDias(361),  0.175);
    QCOMPARE(FiscalService::irRendaFixaPorDias(720),  0.175);
    QCOMPARE(FiscalService::irRendaFixaPorDias(721),  0.150);
    QCOMPARE(FiscalService::irRendaFixaPorDias(3650), 0.150);
}

void TestFiscal::iofRegressivoAteJanela()
{
    QCOMPARE(FiscalService::iofRendaFixaPorDias(1),  0.96);
    QCOMPARE(FiscalService::iofRendaFixaPorDias(29), 0.03);
    QCOMPARE(FiscalService::iofRendaFixaPorDias(30), 0.0);
    QCOMPARE(FiscalService::iofRendaFixaPorDias(60), 0.0);
}

QTEST_GUILESS_MAIN(TestFiscal)
#include "test_fiscal.moc"
