#pragma once

#include <QList>
#include <QPair>

// SuporteResistencia (#53) - calculos de niveis automaticos.
//
// Stateless e puro. Tres familias:
//   - Pivot Points classicos (PP/R1-R3/S1-S3)
//   - Camarilla (R1-R4/S1-S4, 1.1 / divisor)
//   - Fibonacci retracements (entre swing high e low)
//
// Detecta tambem swings highs/lows em series de close para passar
// para fibonacci ou para tracar linhas no grafico.

class SuporteResistencia
{
public:
    // OHLC de um periodo previo (dia/semana/mes) para gerar S/R do proximo.
    struct OHLC {
        double open  = 0.0;
        double high  = 0.0;
        double low   = 0.0;
        double close = 0.0;
    };

    // Pivot Points classicos:
    //   PP = (H + L + C) / 3
    //   R1 = 2*PP - L;        S1 = 2*PP - H
    //   R2 = PP + (H - L);    S2 = PP - (H - L)
    //   R3 = H + 2*(PP - L);  S3 = L - 2*(H - PP)
    struct PivotResult {
        double pp = 0.0;
        double r1 = 0.0;
        double r2 = 0.0;
        double r3 = 0.0;
        double s1 = 0.0;
        double s2 = 0.0;
        double s3 = 0.0;
    };
    [[nodiscard]] static PivotResult pivot(const OHLC &previo);

    // Camarilla:
    //   R4 = C + (H-L)*1.1/2;  R3 = C + (H-L)*1.1/4
    //   R2 = C + (H-L)*1.1/6;  R1 = C + (H-L)*1.1/12
    //   S1 = C - (H-L)*1.1/12; S2 = C - (H-L)*1.1/6
    //   S3 = C - (H-L)*1.1/4;  S4 = C - (H-L)*1.1/2
    struct CamarillaResult {
        double r1 = 0.0;
        double r2 = 0.0;
        double r3 = 0.0;
        double r4 = 0.0;
        double s1 = 0.0;
        double s2 = 0.0;
        double s3 = 0.0;
        double s4 = 0.0;
    };
    [[nodiscard]] static CamarillaResult camarilla(const OHLC &previo);

    // Fibonacci retracements entre swing high e swing low.
    // Niveis padrao: 0 / 23.6 / 38.2 / 50 / 61.8 / 78.6 / 100 (%)
    //   level = swingLow + (swingHigh - swingLow) * pct/100
    // Retorna lista de pares (pct, level).
    [[nodiscard]] static QList<QPair<double, double>> fibonacci(
        double swingHigh,
        double swingLow);

    // Swing high/low: pontos que sao maior/menor em janela de N periodos
    // a esquerda E a direita (janela total 2n+1). Retorna indices em series.
    // Se size < 2n+1 retorna lista vazia.
    [[nodiscard]] static QList<int> swingHighs(const QList<double> &series, int n = 5);
    [[nodiscard]] static QList<int> swingLows(const QList<double> &series, int n = 5);
};
