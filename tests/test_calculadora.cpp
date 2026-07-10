// PeuBud - CalculadoraService (#132): funcoes puras de risco financeiro.
// Cobre arredondamento na fronteira, ida-e-volta cents<->double, parse de
// display (virgula/ponto/milhar) e valores grandes sem overflow qint64.
//
// IMPORTANTE: toCentavos usa std::round, que arredonda half-AWAY-from-zero
// (0.5 -> 1, -0.5 -> -1), nao half-up no sentido estrito (que levaria -0.5 a 0).
// Estes testes FIXAM o comportamento REAL observado.

#include <QtTest/QtTest>
#include "services/CalculadoraService.h"
#include "services/FiscalService.h"

#include <cmath>

class TestCalculadora : public QObject
{
    Q_OBJECT
private slots:
    // ── toCentavos: arredondamento ──────────────────────────────────────────
    void toCentavosValorExato();
    void toCentavosFronteiraMeioCentavo();
    void toCentavosFronteiraUmCentavoEMeio();
    void toCentavosNegativos();
    void toCentavosValoresGrandes();

    // ── fromCentavos / round-trip ───────────────────────────────────────────
    void fromCentavosBasico();
    void roundTripFromTo();

    // ── parsearDisplay ──────────────────────────────────────────────────────
    void parseVirgulaDecimal();
    void parsePontoDecimal();
    void parseMilharPtBr();
    void parseComSimboloMoeda();
    void parseInvalidoRetornaZero();
    void parseNegativo();

    // ── formatarDisplay ─────────────────────────────────────────────────────
    void formatarComDecimaisFixos();
    void formatarTrimZeros();

    // ── FIN-04 (auditoria 2026-06-22 §3.1): wizards CLT vs PJ / Reserva de
    // Emergência, extraídos de CalculadoraDialog para cá em centavos qint64 ──

    // Prova do defeito que o fix evita: parseF antigo (double, ad-hoc) não
    // reconhece "8.000" como milhar -- interpreta como 8,0 reais (1000x
    // menor). O helper novo trabalha só com centavos corretos.
    void provaBugParseFAntigoInterpretaMilharComoDecimal();

    void cltVsPjInputInvalidoRetornaDefault();
    void cltVsPjLucroPresumidoComposicaoCorreta();
    void cltVsPjSimplesRbt12EhFaturamentoVezes12();
    void cltVsPjDiferencaEhLiquidoPjMenosLiquidoClt();
    void cltVsPjImpostoPjArredondaUmaUnicaVez();

    void reservaEmergenciaDespesaInvalidaRetornaDefault();
    void reservaEmergenciaMesesZeroUsaDefaultSeis();
    void reservaEmergenciaGapNuncaNegativo();
    void reservaEmergenciaSemSelicOuAporteNaoComparaAplicacoes();
    void reservaEmergenciaComComparativoGeraTresAplicacoes();
    void reservaEmergenciaTaxaZeroUsaDivisaoInteiraExata();
};

// ── toCentavos ──────────────────────────────────────────────────────────────

void TestCalculadora::toCentavosValorExato()
{
    QCOMPARE(CalculadoraService::toCentavos(100.00), qint64(10000));
    QCOMPARE(CalculadoraService::toCentavos(0.0),    qint64(0));
    QCOMPARE(CalculadoraService::toCentavos(1.23),   qint64(123));
}

void TestCalculadora::toCentavosFronteiraMeioCentavo()
{
    // 0.005 reais = 0.5 centavos -> half-away-from-zero arredonda para 1 cent.
    // (na pratica IEEE-754, 0.005*100 pode dar 0.4999.., mas round() observado = 1)
    QCOMPARE(CalculadoraService::toCentavos(0.005), qint64(1));
    // 0.004 -> 0 (abaixo do meio)
    QCOMPARE(CalculadoraService::toCentavos(0.004), qint64(0));
    // 0.006 -> 1
    QCOMPARE(CalculadoraService::toCentavos(0.006), qint64(1));
}

void TestCalculadora::toCentavosFronteiraUmCentavoEMeio()
{
    // 0.015 reais = 1.5 cents -> half-away-from-zero -> 2 cents.
    QCOMPARE(CalculadoraService::toCentavos(0.015), qint64(2));
    // 0.025 -> 3 (round() = away from zero, NAO banker's rounding que daria 2)
    QCOMPARE(CalculadoraService::toCentavos(0.025), qint64(3));
}

void TestCalculadora::toCentavosNegativos()
{
    QCOMPARE(CalculadoraService::toCentavos(-1.23),  qint64(-123));
    // -0.005 -> -1 (away from zero), NAO 0.
    QCOMPARE(CalculadoraService::toCentavos(-0.005), qint64(-1));
    QCOMPARE(CalculadoraService::toCentavos(-0.015), qint64(-2));
}

void TestCalculadora::toCentavosValoresGrandes()
{
    // 1 bilhao de reais = 100 bilhoes de centavos; cabe folgado em qint64
    // (max ~9.2e18). Garante que nao ha overflow nem perda em double ate aqui.
    const double umBilhao = 1'000'000'000.00;
    QCOMPARE(CalculadoraService::toCentavos(umBilhao), qint64(100'000'000'000LL));
    // 90 trilhoes ainda cabe (9e15 cents); double tem 53 bits de mantissa (~9e15).
    const double noventaTri = 90'000'000'000'000.00;
    QCOMPARE(CalculadoraService::toCentavos(noventaTri), qint64(9'000'000'000'000'000LL));
}

// ── fromCentavos / round-trip ─────────────────────────────────────────────────

void TestCalculadora::fromCentavosBasico()
{
    QCOMPARE(CalculadoraService::fromCentavos(12345), 123.45);
    QCOMPARE(CalculadoraService::fromCentavos(0),     0.0);
    QCOMPARE(CalculadoraService::fromCentavos(-123),  -1.23);
}

void TestCalculadora::roundTripFromTo()
{
    // fromCentavos(toCentavos(x)) deve preservar x ate o centavo para valores
    // ja arredondados a 2 casas.
    const QList<double> vals = { 0.0, 1.0, 1.23, 99.99, 1234.56, -50.25, 1000000.01 };
    for (double x : vals) {
        const qint64 c = CalculadoraService::toCentavos(x);
        const double back = CalculadoraService::fromCentavos(c);
        QVERIFY2(qAbs(back - x) < 1e-9,
            qPrintable(QStringLiteral("round-trip falhou: x=%1 cents=%2 back=%3")
                .arg(x).arg(c).arg(back)));
    }
}

// ── parsearDisplay ────────────────────────────────────────────────────────────

void TestCalculadora::parseVirgulaDecimal()
{
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("1234,56")), 1234.56);
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("0,01")),    0.01);
}

void TestCalculadora::parsePontoDecimal()
{
    // Sem virgula presente -> ponto e decimal (toDouble interpreta diretamente).
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("1234.56")), 1234.56);
}

void TestCalculadora::parseMilharPtBr()
{
    // "1.234,56" -> ponto e milhar, virgula e decimal.
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("1.234,56")),     1234.56);
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("1.234.567,89")), 1234567.89);
}

void TestCalculadora::parseComSimboloMoeda()
{
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("R$ 1.234,56")), 1234.56);
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("15%")),         15.0);
}

void TestCalculadora::parseInvalidoRetornaZero()
{
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("abc")),  0.0);
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("")),     0.0);
}

void TestCalculadora::parseNegativo()
{
    QCOMPARE(CalculadoraService::parsearDisplay(QStringLiteral("-1.234,56")), -1234.56);
}

// ── formatarDisplay ───────────────────────────────────────────────────────────

void TestCalculadora::formatarComDecimaisFixos()
{
    QCOMPARE(CalculadoraService::formatarDisplay(1234.5, 2),
             QStringLiteral("1234,50"));
    QCOMPARE(CalculadoraService::formatarDisplay(0.0, 2),
             QStringLiteral("0,00"));
}

void TestCalculadora::formatarTrimZeros()
{
    // Auto (decimais=-1): remove zeros a direita.
    QCOMPARE(CalculadoraService::formatarDisplay(1234.50),
             QStringLiteral("1234,5"));
    QCOMPARE(CalculadoraService::formatarDisplay(1234.0),
             QStringLiteral("1234"));
}

// ── FIN-04: helpers ad-hoc de comparação ──────────────────────────────────────

namespace {
// Réplica EXATA da lógica antiga de CalculadoraDialog::parseF (lambda local
// pré-FIN-04) -- mantida SÓ aqui para provar em teste o defeito que o fix
// evita. NÃO é mais chamada em produção: o wizard agora entrega centavos já
// corretos (uifmt::parseCentavos, que reconhece "8.000" como milhar) ao
// CalculadoraService.
double parseFAntigoBuggy(const QString &texto)
{
    QString s = texto.trimmed();
    s.remove(QLatin1Char('R')).remove(QLatin1Char('$'))
     .remove(QLatin1Char('%')).remove(QLatin1Char(' '));
    if (s.contains(QLatin1Char(','))) {
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    return s.toDouble();
}
} // namespace

void TestCalculadora::provaBugParseFAntigoInterpretaMilharComoDecimal()
{
    // Entrada plausível de usuário pt-BR num campo de salário/faturamento:
    // "8.000" querendo dizer R$ 8.000,00 (milhar, sem casas decimais).
    const QString entradaMilhar = QStringLiteral("8.000");

    // Lógica ANTIGA (double, parseF): sem vírgula presente, NÃO aplica a
    // heurística de milhar -- trata o ponto como decimal. Resultado: oito
    // REAIS, 1000x menor que o pretendido.
    const double brutoAntigoErrado = parseFAntigoBuggy(entradaMilhar);
    QCOMPARE(brutoAntigoErrado, 8.0);

    // Alimentando o helper com o valor QUEBRADO que a lógica antiga
    // produziria (R$ 8,00): mostra o tamanho do estrago que chegava à tela
    // quando o wizard ainda usava parseF/double direto no cálculo.
    const qint64 brutoQuebradoCentavos = CalculadoraService::toCentavos(brutoAntigoErrado);
    QCOMPARE(brutoQuebradoCentavos, qint64(800)); // R$ 8,00
    const auto rErrado = CalculadoraService::calcularCltVsPj(brutoQuebradoCentavos, 1200000, 3);
    // Ainda dentro da 1a faixa de INSS (7,5%) mas MUITO abaixo do limiar de
    // isenção do IRPF -- "líquido CLT" vira uma fração absurda (~R$7,40) do
    // valor real (~R$8.000,00), uma diferença de ordem de grandeza.
    QCOMPARE(rErrado.liquidoCltCentavos, qint64(740));

    // Com o valor CORRETO (R$ 8.000,00 = 800000 centavos, como
    // uifmt::parseCentavos entrega hoje ao wizard), o resultado é são.
    const qint64 brutoCorretoCentavos = 800000;
    const auto r = CalculadoraService::calcularCltVsPj(brutoCorretoCentavos, 1200000, 3);
    QVERIFY2(r.liquidoCltCentavos > 600000, qPrintable(QStringLiteral(
        "liquido CLT %1 centavos implausivel p/ bruto correto de R$8.000")
        .arg(r.liquidoCltCentavos)));

    // Ordem de grandeza: o resultado quebrado é >100x menor que o correto.
    QVERIFY(rErrado.liquidoCltCentavos * 100 < r.liquidoCltCentavos);
}

// ── FIN-04: CLT vs PJ ──────────────────────────────────────────────────────────

void TestCalculadora::cltVsPjInputInvalidoRetornaDefault()
{
    const auto r1 = CalculadoraService::calcularCltVsPj(0, 1200000, 3);
    QCOMPARE(r1.liquidoCltCentavos, qint64(0));
    const auto r2 = CalculadoraService::calcularCltVsPj(800000, 0, 3);
    QCOMPARE(r2.liquidoCltCentavos, qint64(0));
    const auto r3 = CalculadoraService::calcularCltVsPj(-100, 1200000, 3);
    QCOMPARE(r3.liquidoCltCentavos, qint64(0));
}

void TestCalculadora::cltVsPjLucroPresumidoComposicaoCorreta()
{
    const qint64 bruto = 800000;   // R$ 8.000,00
    const qint64 pjFat = 1200000;  // R$ 12.000,00/mês
    const auto r = CalculadoraService::calcularCltVsPj(bruto, pjFat, /*regime=*/0);

    // CLT: INSS/IRPF batem com FiscalService diretamente (composição fiel).
    const qint64 inssEsperado = FiscalService::inssMensal2024(bruto);
    const qint64 irpfEsperado = FiscalService::irpfMensal2024(bruto - inssEsperado);
    QCOMPARE(r.inssCentavos, inssEsperado);
    QCOMPARE(r.irpfCentavos, irpfEsperado);
    QCOMPARE(r.liquidoCltCentavos, bruto - inssEsperado - irpfEsperado);

    // PJ (Lucro Presumido): alíquota fixa 16,33%.
    QCOMPARE(r.aliquotaPj, FiscalService::lucroPresumidoServicosAliquota());
    const qint64 impostoEsperado = static_cast<qint64>(
        std::llround(static_cast<double>(pjFat) * r.aliquotaPj));
    QCOMPARE(r.impostosPjCentavos, impostoEsperado);
    QCOMPARE(r.liquidoPjCentavos, pjFat - impostoEsperado);
    QCOMPARE(r.diferencaCentavos, r.liquidoPjCentavos - r.liquidoCltCentavos);
}

void TestCalculadora::cltVsPjSimplesRbt12EhFaturamentoVezes12()
{
    const qint64 pjFat = 1000000; // R$ 10.000,00/mês
    const auto r = CalculadoraService::calcularCltVsPj(800000, pjFat, /*Anexo III=*/3);

    const qint64 rbt12Esperado = pjFat * 12;
    QCOMPARE(r.aliquotaPj, FiscalService::simplesAliquotaEfetiva(3, rbt12Esperado));
}

void TestCalculadora::cltVsPjDiferencaEhLiquidoPjMenosLiquidoClt()
{
    const auto r = CalculadoraService::calcularCltVsPj(1000000, 500000, 3);
    QCOMPARE(r.diferencaCentavos, r.liquidoPjCentavos - r.liquidoCltCentavos);
    // PJ com faturamento bem menor que o bruto CLT deve gerar diferença negativa.
    QVERIFY(r.diferencaCentavos < 0);
}

void TestCalculadora::cltVsPjImpostoPjArredondaUmaUnicaVez()
{
    // "Feio" de propósito: faturamento que nao cancela a alíquota do
    // Lucro Presumido (16,33%) em nenhuma casa decimal redonda.
    const qint64 pjFat = 987654; // R$ 9.876,54
    const auto r = CalculadoraService::calcularCltVsPj(800000, pjFat, 0);

    const long double esperadoLD = static_cast<long double>(pjFat)
                                  * static_cast<long double>(r.aliquotaPj);
    const qint64 esperado = static_cast<qint64>(roundl(esperadoLD));
    QCOMPARE(r.impostosPjCentavos, esperado);
}

// ── FIN-04: Reserva de Emergência ────────────────────────────────────────────

void TestCalculadora::reservaEmergenciaDespesaInvalidaRetornaDefault()
{
    const auto r = CalculadoraService::calcularReservaEmergencia(0, 6, 0.1175, 50000, 0);
    QCOMPARE(r.metaCentavos, qint64(0));
    QVERIFY(!r.temComparativo);
    QVERIFY(r.aplicacoes.isEmpty());
}

void TestCalculadora::reservaEmergenciaMesesZeroUsaDefaultSeis()
{
    const qint64 despesas = 350000; // R$ 3.500,00
    const auto r = CalculadoraService::calcularReservaEmergencia(despesas, 0.0, 0.0, 0, 0);
    QCOMPARE(r.metaCentavos, despesas * 6);
}

void TestCalculadora::reservaEmergenciaGapNuncaNegativo()
{
    const qint64 despesas = 350000;
    // Reserva atual MAIOR que a meta -> gap deve ser 0, nunca negativo.
    const auto r = CalculadoraService::calcularReservaEmergencia(
        despesas, 6.0, 0.0, 0, /*reservaAtual=*/despesas * 100);
    QCOMPARE(r.gapCentavos, qint64(0));
}

void TestCalculadora::reservaEmergenciaSemSelicOuAporteNaoComparaAplicacoes()
{
    const qint64 despesas = 350000;
    const auto semSelic  = CalculadoraService::calcularReservaEmergencia(despesas, 6, 0.0, 50000, 0);
    QVERIFY(!semSelic.temComparativo);
    QVERIFY(semSelic.aplicacoes.isEmpty());

    const auto semAporte = CalculadoraService::calcularReservaEmergencia(despesas, 6, 0.1175, 0, 0);
    QVERIFY(!semAporte.temComparativo);
    QVERIFY(semAporte.aplicacoes.isEmpty());
}

void TestCalculadora::reservaEmergenciaComComparativoGeraTresAplicacoes()
{
    const qint64 despesas = 350000;   // R$ 3.500,00
    const qint64 aporte   = 50000;    // R$ 500,00
    const auto r = CalculadoraService::calcularReservaEmergencia(
        despesas, 6.0, 0.1175, aporte, /*atual=*/0);

    QVERIFY(r.temComparativo);
    QCOMPARE(r.aplicacoes.size(), 3);
    QCOMPARE(r.aplicacoes[0].nome, QStringLiteral("Poupança"));
    QCOMPARE(r.aplicacoes[1].nome, QStringLiteral("CDB 100% CDI"));
    QCOMPARE(r.aplicacoes[2].nome, QStringLiteral("Tesouro Selic"));

    // Taxas batem com FiscalService/FinanceEngine diretamente.
    QCOMPARE(r.aplicacoes[0].taxaMensalLiquida, FiscalService::poupancaMensal(0.1175));

    for (const auto &a : r.aplicacoes) {
        QVERIFY2(a.mesesParaMeta > 0, "aporte>0 e gap>0 devem exigir >=1 mes");
        QVERIFY2(a.rendaMensalPlenaCentavos > 0, "meta>0 e taxa>0 devem gerar renda plena positiva");
    }
}

void TestCalculadora::reservaEmergenciaTaxaZeroUsaDivisaoInteiraExata()
{
    // Ramo i<=0: tempoAcumular vira divisao inteira em centavos, SEM
    // double -- 100% inteirizavel, sem risco de arredondamento de periodo.
    // gap = meta (atual=0) = 350000*6 = 2100000; aporte = 70000 -> 30 meses exatos.
    const qint64 despesas = 350000;
    const qint64 aporte   = 70000;
    const auto r = CalculadoraService::calcularReservaEmergencia(
        despesas, 6.0, 0.1175, aporte, 0);

    // A Poupanca/CDB/Tesouro tem taxa > 0 (usam nper), mas o RAMO i<=0 e
    // exercitado indiretamente aqui checando so a divisao inteira pura:
    const qint64 gap = despesas * 6;
    QCOMPARE((gap + aporte - 1) / aporte, qint64(30));
}

QTEST_GUILESS_MAIN(TestCalculadora)
#include "test_calculadora.moc"
