// PeuBud - CalculadoraService (#132): funcoes puras de risco financeiro.
// Cobre arredondamento na fronteira, ida-e-volta cents<->double, parse de
// display (virgula/ponto/milhar) e valores grandes sem overflow qint64.
//
// IMPORTANTE: toCentavos usa std::round, que arredonda half-AWAY-from-zero
// (0.5 -> 1, -0.5 -> -1), nao half-up no sentido estrito (que levaria -0.5 a 0).
// Estes testes FIXAM o comportamento REAL observado.

#include <QtTest/QtTest>
#include "services/CalculadoraService.h"

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

QTEST_GUILESS_MAIN(TestCalculadora)
#include "test_calculadora.moc"
