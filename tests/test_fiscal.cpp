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
    // INSS - ancoras de precisao (aritmetica inteira #113):
    // valores EXATOS conferidos contra oraculo Decimal half-up da
    // tabela oficial 2024 (Portaria MTP 2/2024).
    void inssAncoraExataPorFaixa_data();
    void inssAncoraExataPorFaixa();

    // IRPF
    void irpfIsento();
    void irpfPrimeiraFaixa();
    void irpfFaixaMaxima();
    // IRPF - ancoras de precisao (#113): valores EXATOS vs tabela
    // oficial 2024 (Lei 14.663/2023, vigencia 02/2024).
    void irpfAncoraExataPorFaixa_data();
    void irpfAncoraExataPorFaixa();

    // Simples Nacional
    void simplesAnexoIPrimeiraFaixa();
    void simplesAnexoIIIIntermediaria();
    void simplesForaDoSimples();
    // Simples - ancoras de precisao (#113): aliquota efetiva por
    // anexo/RBT12 vs formula oficial LC 123/2006.
    void simplesAncoraExataPorAnexo_data();
    void simplesAncoraExataPorAnexo();

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

// ── Ancoras de precisao (#113) ───────────────────────────────────────────────
// Estas tabelas fixam o resultado CORRETO esperado (centavos qint64),
// calculado com aritmetica exata (Decimal half-up) sobre as tabelas
// oficiais 2024. Comparacao EXATA (QCOMPARE), sem tolerancia, para
// blindar a refatoracao integer e impedir regressao de +-1 centavo.

void TestFiscal::inssAncoraExataPorFaixa_data()
{
    QTest::addColumn<qint64>("bruto");
    QTest::addColumn<qint64>("esperado");

    // bordas e interior de cada faixa (Portaria MTP 2/2024)
    QTest::newRow("zero")                << qint64(0)       << qint64(0);
    QTest::newRow("abaixo R$1 -> 0,08")  << qint64(100)     << qint64(8);
    QTest::newRow("1k 1a faixa 7,5%")    << qint64(100000)  << qint64(7500);
    QTest::newRow("borda F1 1412,00")    << qint64(141200)  << qint64(10590);
    QTest::newRow("acima F1 (2a faixa)") << qint64(141201)  << qint64(10590);
    QTest::newRow("2k 2a faixa")         << qint64(200000)  << qint64(15882);
    QTest::newRow("borda F2 2666,68")    << qint64(266668)  << qint64(21882);
    QTest::newRow("3k 3a faixa")         << qint64(300000)  << qint64(25882);
    QTest::newRow("borda F3 4000,03")    << qint64(400003)  << qint64(37882);
    QTest::newRow("5k 4a faixa")         << qint64(500000)  << qint64(51882);
    QTest::newRow("borda F4 7786,02")    << qint64(778602)  << qint64(90885);
    QTest::newRow("acima F4 (teto)")     << qint64(778603)  << qint64(90885);
    QTest::newRow("10k teto fixo")       << qint64(1000000) << qint64(90885);
}

void TestFiscal::inssAncoraExataPorFaixa()
{
    QFETCH(qint64, bruto);
    QFETCH(qint64, esperado);
    QCOMPARE(FiscalService::inssMensal2024(bruto), esperado);
}

void TestFiscal::irpfAncoraExataPorFaixa_data()
{
    QTest::addColumn<qint64>("base");
    QTest::addColumn<qint64>("esperado");

    // Lei 14.663/2023 (vigencia 02/2024): faixas com parcela a deduzir
    QTest::newRow("abaixo isencao")      << qint64(200000)  << qint64(0);
    QTest::newRow("borda isencao 2259,20") << qint64(225920) << qint64(0);
    QTest::newRow("logo acima isencao")  << qint64(225921)  << qint64(0);
    QTest::newRow("2,5k 1a faixa 7,5%")  << qint64(250000)  << qint64(1806);
    QTest::newRow("borda f1/f2 2826,65") << qint64(282665)  << qint64(4256);
    QTest::newRow("3k 2a faixa 15%")     << qint64(300000)  << qint64(6856);
    QTest::newRow("borda f2/f3 3751,05") << qint64(375105)  << qint64(18122);
    QTest::newRow("4k 3a faixa 22,5%")   << qint64(400000)  << qint64(23723);
    QTest::newRow("borda f3/f4 4664,68") << qint64(466468)  << qint64(38678);
    QTest::newRow("5k 4a faixa 27,5%")   << qint64(500000)  << qint64(47900);
    QTest::newRow("10k faixa maxima")    << qint64(1000000) << qint64(185400);
}

void TestFiscal::irpfAncoraExataPorFaixa()
{
    QFETCH(qint64, base);
    QFETCH(qint64, esperado);
    QCOMPARE(FiscalService::irpfMensal2024(base), esperado);
}

void TestFiscal::simplesAncoraExataPorAnexo_data()
{
    QTest::addColumn<int>("anexo");
    QTest::addColumn<qint64>("rbt12");
    QTest::addColumn<double>("esperado");

    // Efetiva = (RBT12 * aliqNominal - PD) / RBT12  (LC 123/2006)
    // Anexo I 1a faixa (ate 180k): 4% nominal, PD=0 -> 4%
    QTest::newRow("AnexoI 100k -> 4%")
        << 1 << qint64(10000000) << 0.040;
    // Anexo III 2a faixa (180k-360k): 11,2% nom, PD 9.360,00
    // (300000*0,112 - 9360)/300000 = 0,0808
    QTest::newRow("AnexoIII 300k -> 8,08%")
        << 3 << qint64(30000000) << 0.0808;
    // Anexo V 4a faixa (720k-1,92M): 20,5% nom, PD 17.190,00
    // (1.000.000*0,205 - 17.190)/1.000.000 = 0,18781
    QTest::newRow("AnexoV 1M -> 18,781%")
        << 5 << qint64(100000000) << 0.18781;
    // Anexo I 6a faixa (3,6M-4,8M): 19% nom, PD 378.568,00
    // (4.000.000*0,19 - 378.568)/4.000.000 = 0,095358
    QTest::newRow("AnexoI 4M -> 9,5358%")
        << 1 << qint64(400000000) << 0.095358;
}

void TestFiscal::simplesAncoraExataPorAnexo()
{
    QFETCH(int, anexo);
    QFETCH(qint64, rbt12);
    QFETCH(double, esperado);
    const double aliq = FiscalService::simplesAliquotaEfetiva(anexo, rbt12);
    QVERIFY2(qAbs(aliq - esperado) < 1e-9,
        qPrintable(QStringLiteral("aliq=%1 esperado=%2")
                       .arg(aliq, 0, 'g', 12).arg(esperado, 0, 'g', 12)));
}

QTEST_GUILESS_MAIN(TestFiscal)
#include "test_fiscal.moc"
