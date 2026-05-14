#include "IndicadoresTecnicos.h"

#include <QtMath>
#include <cmath>

namespace
{
constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
}

QList<double> IndicadoresTecnicos::sma(const QList<double> &close, int n)
{
    QList<double> out;
    const int sz = close.size();
    out.reserve(sz);
    if (n <= 0 || sz == 0) {
        for (int i = 0; i < sz; ++i) {
            out.append(kNaN);
        }
        return out;
    }

    // Soma deslizante O(n)
    double soma = 0.0;
    for (int i = 0; i < sz; ++i) {
        soma += close[i];
        if (i >= n) {
            soma -= close[i - n];
        }
        if (i < n - 1) {
            out.append(kNaN);
        } else {
            out.append(soma / static_cast<double>(n));
        }
    }
    return out;
}

QList<double> IndicadoresTecnicos::ema(const QList<double> &close, int n)
{
    QList<double> out;
    const int sz = close.size();
    out.reserve(sz);
    if (sz == 0) {
        return out;
    }
    if (n <= 1) {
        // Sem suavizacao: retorna serie original.
        for (int i = 0; i < sz; ++i) {
            out.append(close[i]);
        }
        return out;
    }
    const double k = 2.0 / (static_cast<double>(n) + 1.0);
    double ema_prev = close[0];
    out.append(ema_prev);
    for (int i = 1; i < sz; ++i) {
        const double v = close[i] * k + ema_prev * (1.0 - k);
        out.append(v);
        ema_prev = v;
    }
    return out;
}

QList<double> IndicadoresTecnicos::rsi(const QList<double> &close, int n)
{
    QList<double> out;
    const int sz = close.size();
    out.reserve(sz);
    if (n <= 0 || sz == 0) {
        for (int i = 0; i < sz; ++i) {
            out.append(kNaN);
        }
        return out;
    }

    // Precisamos de >= n+1 elementos para 1 RSI valido (n diffs).
    if (sz < n + 1) {
        for (int i = 0; i < sz; ++i) {
            out.append(kNaN);
        }
        return out;
    }

    // Primeiros n elementos: nao definidos (RSI valido a partir do indice n).
    for (int i = 0; i < n; ++i) {
        out.append(kNaN);
    }

    // Medias iniciais (Wilder usa SMA dos primeiros n diffs)
    double sumGain = 0.0;
    double sumLoss = 0.0;
    for (int i = 1; i <= n; ++i) {
        const double d = close[i] - close[i - 1];
        if (d > 0.0) {
            sumGain += d;
        } else {
            sumLoss += -d;
        }
    }
    double avgGain = sumGain / static_cast<double>(n);
    double avgLoss = sumLoss / static_cast<double>(n);

    auto rsiFrom = [](double g, double l) -> double {
        if (l == 0.0) {
            return 100.0;
        }
        const double rs = g / l;
        return 100.0 - (100.0 / (1.0 + rs));
    };

    out.append(rsiFrom(avgGain, avgLoss));

    // Suavizacao Wilder para indices n+1..sz-1
    for (int i = n + 1; i < sz; ++i) {
        const double d = close[i] - close[i - 1];
        const double g = d > 0.0 ? d : 0.0;
        const double l = d < 0.0 ? -d : 0.0;
        avgGain = (avgGain * (n - 1) + g) / static_cast<double>(n);
        avgLoss = (avgLoss * (n - 1) + l) / static_cast<double>(n);
        out.append(rsiFrom(avgGain, avgLoss));
    }
    return out;
}

IndicadoresTecnicos::MACDResult IndicadoresTecnicos::macd(const QList<double> &close,
                                                          int fast, int slow, int signal)
{
    MACDResult out;
    if (close.isEmpty()) {
        return out;
    }
    const QList<double> emaFast = ema(close, fast);
    const QList<double> emaSlow = ema(close, slow);

    QList<double> macdLine;
    macdLine.reserve(close.size());
    for (int i = 0; i < close.size(); ++i) {
        macdLine.append(emaFast[i] - emaSlow[i]);
    }

    const QList<double> sig = ema(macdLine, signal);

    QList<double> hist;
    hist.reserve(close.size());
    for (int i = 0; i < close.size(); ++i) {
        hist.append(macdLine[i] - sig[i]);
    }

    out.macd      = macdLine;
    out.signal    = sig;
    out.histogram = hist;
    return out;
}

IndicadoresTecnicos::BBandsResult IndicadoresTecnicos::bbands(const QList<double> &close,
                                                              int n, double k)
{
    BBandsResult out;
    const int sz = close.size();
    if (sz == 0 || n <= 0) {
        return out;
    }

    out.middle = sma(close, n);
    out.upper.reserve(sz);
    out.lower.reserve(sz);

    for (int i = 0; i < sz; ++i) {
        if (i < n - 1) {
            out.upper.append(kNaN);
            out.lower.append(kNaN);
            continue;
        }
        const double mean = out.middle[i];
        // Stddev populacional na janela [i-n+1, i].
        double acc = 0.0;
        for (int j = i - n + 1; j <= i; ++j) {
            const double d = close[j] - mean;
            acc += d * d;
        }
        const double sd = std::sqrt(acc / static_cast<double>(n));
        out.upper.append(mean + k * sd);
        out.lower.append(mean - k * sd);
    }
    return out;
}
