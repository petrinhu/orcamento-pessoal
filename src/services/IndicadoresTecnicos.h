#pragma once

#include <QList>

// IndicadoresTecnicos (#52) - indicadores tecnicos classicos para serie de
// precos de fechamento. Stateless e puro: sem Qt SQL/Network/Widgets.
// Substitui dependencia de TA-Lib (nao disponivel em dnf/apt/pacman).
//
// Implementacoes referenciam definicoes classicas:
//   SMA      -> media aritmetica janela N
//   EMA      -> Wilder smoothing k = 2/(N+1); seed = close[0]
//   RSI      -> Wilder RSI N=14; metodo SMA inicial + suavizacao Wilder
//   MACD     -> EMA(fast) - EMA(slow); signal = EMA(macd, n)
//   BBands   -> SMA(N) +/- k * stddev populacional
//
// Convencao numerica:
//   - elementos nao definidos retornados como qQNaN() (NaN IEEE 754)
//   - serie vazia -> output vazio (nunca crash)

class IndicadoresTecnicos
{
public:
    // Moving Average simples (window N): output[i] = avg(close[i-N+1..i]).
    // Primeiros N-1 elementos = NaN. n <= 0 ou close.size() < n -> NaNs.
    [[nodiscard]] static QList<double> sma(const QList<double> &close, int n);

    // Exponential Moving Average. Smoothing k = 2/(n+1).
    // ema[0] = close[0]; ema[i] = close[i]*k + ema[i-1]*(1-k).
    // Serie vazia -> vazio.
    [[nodiscard]] static QList<double> ema(const QList<double> &close, int n);

    // RSI Wilder. Periodo padrao 14.
    // Calculo: ganhos/perdas iniciais = media simples primeiros N;
    // depois suavizacao Wilder: avgGain = (avgGain*(N-1)+ganho)/N.
    // RSI[i] = 100 - 100/(1+avgGain/avgLoss). Primeiros N-1 = NaN.
    [[nodiscard]] static QList<double> rsi(const QList<double> &close, int n = 14);

    // MACD (12, 26, 9): retorna { macdLine, signalLine, histogram }.
    // macd = EMA(close, fast) - EMA(close, slow).
    // signal = EMA(macd, signal).
    // histogram = macd - signal.
    struct MACDResult {
        QList<double> macd;
        QList<double> signal;
        QList<double> histogram;
    };
    [[nodiscard]] static MACDResult macd(const QList<double> &close,
                                         int fast = 12, int slow = 26, int signal = 9);

    // Bollinger Bands (N=20, k=2 desvios): { middle, upper, lower }.
    // middle = SMA(close, N).
    // stddev populacional sobre janela.
    // upper = middle + k*stddev; lower = middle - k*stddev.
    // Primeiros N-1 elementos = NaN.
    struct BBandsResult {
        QList<double> middle;
        QList<double> upper;
        QList<double> lower;
    };
    [[nodiscard]] static BBandsResult bbands(const QList<double> &close,
                                              int n = 20, double k = 2.0);
};
