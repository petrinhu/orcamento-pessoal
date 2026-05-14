// PeuBud - IndicadoresTecnicos TDD (#52)
// Testa SMA/EMA/RSI/MACD/BBands puros sem dependencia de DB.

#include <QtTest/QtTest>
#include <cmath>
#include "services/IndicadoresTecnicos.h"

class TestIndicadores : public QObject
{
    Q_OBJECT
private slots:
    void smaJanelaCincoSerieMonotonica();
    void emaPrimeiroIgualClose();
    void rsiMonotonicaCrescenteAproximaCem();
    void rsiMonotonicaDecrescenteAproximaZero();
    void macdConsistenciaInternal();
    void bbandsMiddleEhSma();
    void serieVaziaSemCrash();
};

static bool nearly(double a, double b, double tol)
{
    return std::fabs(a - b) <= tol;
}

void TestIndicadores::smaJanelaCincoSerieMonotonica()
{
    // [1..10], SMA(5)
    QList<double> close = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto out = IndicadoresTecnicos::sma(close, 5);
    QCOMPARE(out.size(), 10);
    // Primeiros 4 NaN
    for (int i = 0; i < 4; ++i) {
        QVERIFY(std::isnan(out[i]));
    }
    QVERIFY(nearly(out[4], 3.0, 1e-6));   // (1+2+3+4+5)/5
    QVERIFY(nearly(out[9], 8.0, 1e-6));   // (6+7+8+9+10)/5
}

void TestIndicadores::emaPrimeiroIgualClose()
{
    QList<double> close = {10.0, 12.0, 14.0, 16.0, 18.0};
    auto out = IndicadoresTecnicos::ema(close, 3);
    QCOMPARE(out.size(), 5);
    QVERIFY(nearly(out[0], 10.0, 1e-12));

    // k = 2/(3+1) = 0.5
    // ema[1] = 12 * 0.5 + 10 * 0.5 = 11
    QVERIFY(nearly(out[1], 11.0, 1e-12));
    // ema[2] = 14 * 0.5 + 11 * 0.5 = 12.5
    QVERIFY(nearly(out[2], 12.5, 1e-12));
    // ema[3] = 16 * 0.5 + 12.5 * 0.5 = 14.25
    QVERIFY(nearly(out[3], 14.25, 1e-12));

    // Tendencia ascendente: monotonicidade
    for (int i = 1; i < 5; ++i) {
        QVERIFY(out[i] > out[i - 1]);
    }
}

void TestIndicadores::rsiMonotonicaCrescenteAproximaCem()
{
    // 30 pontos sempre crescentes -> avgLoss = 0 -> RSI = 100
    QList<double> close;
    for (int i = 0; i < 30; ++i) {
        close.append(100.0 + i);
    }
    auto r = IndicadoresTecnicos::rsi(close, 14);
    QCOMPARE(r.size(), 30);
    // Primeiros 14 sao NaN
    for (int i = 0; i < 14; ++i) {
        QVERIFY(std::isnan(r[i]));
    }
    // A partir de 14, RSI proximo de 100
    for (int i = 14; i < 30; ++i) {
        QVERIFY(nearly(r[i], 100.0, 1e-4));
    }
}

void TestIndicadores::rsiMonotonicaDecrescenteAproximaZero()
{
    QList<double> close;
    for (int i = 0; i < 30; ++i) {
        close.append(200.0 - i);
    }
    auto r = IndicadoresTecnicos::rsi(close, 14);
    QCOMPARE(r.size(), 30);
    for (int i = 14; i < 30; ++i) {
        QVERIFY(nearly(r[i], 0.0, 1e-4));
    }
}

void TestIndicadores::macdConsistenciaInternal()
{
    // Para qualquer serie nao-trivial: macd[i] == ema(fast)[i] - ema(slow)[i]
    // e histogram[i] == macd[i] - signal[i].
    QList<double> close;
    for (int i = 0; i < 60; ++i) {
        close.append(50.0 + std::sin(i * 0.3) * 5.0 + i * 0.1);
    }
    auto m = IndicadoresTecnicos::macd(close, 12, 26, 9);
    QCOMPARE(m.macd.size(), 60);
    QCOMPARE(m.signal.size(), 60);
    QCOMPARE(m.histogram.size(), 60);

    auto eFast = IndicadoresTecnicos::ema(close, 12);
    auto eSlow = IndicadoresTecnicos::ema(close, 26);
    for (int i = 0; i < 60; ++i) {
        QVERIFY(nearly(m.macd[i], eFast[i] - eSlow[i], 1e-9));
        QVERIFY(nearly(m.histogram[i], m.macd[i] - m.signal[i], 1e-9));
    }
}

void TestIndicadores::bbandsMiddleEhSma()
{
    QList<double> close;
    for (int i = 0; i < 30; ++i) {
        close.append(10.0 + (i % 5));
    }
    auto bb = IndicadoresTecnicos::bbands(close, 20, 2.0);
    QCOMPARE(bb.middle.size(), 30);
    QCOMPARE(bb.upper.size(), 30);
    QCOMPARE(bb.lower.size(), 30);

    auto sma20 = IndicadoresTecnicos::sma(close, 20);
    for (int i = 0; i < 30; ++i) {
        if (i < 19) {
            QVERIFY(std::isnan(bb.middle[i]));
            QVERIFY(std::isnan(bb.upper[i]));
            QVERIFY(std::isnan(bb.lower[i]));
        } else {
            QVERIFY(nearly(bb.middle[i], sma20[i], 1e-9));
            // upper > middle > lower
            QVERIFY(bb.upper[i] > bb.middle[i]);
            QVERIFY(bb.lower[i] < bb.middle[i]);
            // upper - middle == middle - lower (simetria)
            const double dUp = bb.upper[i] - bb.middle[i];
            const double dDn = bb.middle[i] - bb.lower[i];
            QVERIFY(nearly(dUp, dDn, 1e-9));
        }
    }
}

void TestIndicadores::serieVaziaSemCrash()
{
    QList<double> empty;
    QCOMPARE(IndicadoresTecnicos::sma(empty, 5).size(), 0);
    QCOMPARE(IndicadoresTecnicos::ema(empty, 5).size(), 0);
    QCOMPARE(IndicadoresTecnicos::rsi(empty, 14).size(), 0);
    auto m = IndicadoresTecnicos::macd(empty);
    QCOMPARE(m.macd.size(), 0);
    QCOMPARE(m.signal.size(), 0);
    QCOMPARE(m.histogram.size(), 0);
    auto bb = IndicadoresTecnicos::bbands(empty);
    QCOMPARE(bb.middle.size(), 0);
    QCOMPARE(bb.upper.size(), 0);
    QCOMPARE(bb.lower.size(), 0);
}

QTEST_GUILESS_MAIN(TestIndicadores)
#include "test_indicadores.moc"
