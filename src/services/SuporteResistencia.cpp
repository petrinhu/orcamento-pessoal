#include "services/SuporteResistencia.h"

SuporteResistencia::PivotResult SuporteResistencia::pivot(const OHLC &previo)
{
    PivotResult r;
    const double h = previo.high;
    const double l = previo.low;
    const double c = previo.close;
    r.pp = (h + l + c) / 3.0;
    r.r1 = 2.0 * r.pp - l;
    r.s1 = 2.0 * r.pp - h;
    r.r2 = r.pp + (h - l);
    r.s2 = r.pp - (h - l);
    r.r3 = h + 2.0 * (r.pp - l);
    r.s3 = l - 2.0 * (h - r.pp);
    return r;
}

SuporteResistencia::CamarillaResult SuporteResistencia::camarilla(const OHLC &previo)
{
    CamarillaResult r;
    const double h = previo.high;
    const double l = previo.low;
    const double c = previo.close;
    const double range11 = (h - l) * 1.1;
    r.r4 = c + range11 / 2.0;
    r.r3 = c + range11 / 4.0;
    r.r2 = c + range11 / 6.0;
    r.r1 = c + range11 / 12.0;
    r.s1 = c - range11 / 12.0;
    r.s2 = c - range11 / 6.0;
    r.s3 = c - range11 / 4.0;
    r.s4 = c - range11 / 2.0;
    return r;
}

QList<QPair<double, double>> SuporteResistencia::fibonacci(double swingHigh, double swingLow)
{
    static const double pcts[] = {0.0, 23.6, 38.2, 50.0, 61.8, 78.6, 100.0};
    QList<QPair<double, double>> out;
    const double range = swingHigh - swingLow;
    for (double p : pcts) {
        const double level = swingLow + range * (p / 100.0);
        out.append({p, level});
    }
    return out;
}

QList<int> SuporteResistencia::swingHighs(const QList<double> &series, int n)
{
    QList<int> out;
    if (n < 1) return out;
    const int sz = static_cast<int>(series.size());
    if (sz < 2 * n + 1) return out;
    for (int i = n; i < sz - n; ++i) {
        const double v = series[i];
        bool isHigh = true;
        for (int k = 1; k <= n && isHigh; ++k) {
            if (series[i - k] >= v || series[i + k] >= v) {
                isHigh = false;
            }
        }
        if (isHigh) {
            out.append(i);
        }
    }
    return out;
}

QList<int> SuporteResistencia::swingLows(const QList<double> &series, int n)
{
    QList<int> out;
    if (n < 1) return out;
    const int sz = static_cast<int>(series.size());
    if (sz < 2 * n + 1) return out;
    for (int i = n; i < sz - n; ++i) {
        const double v = series[i];
        bool isLow = true;
        for (int k = 1; k <= n && isLow; ++k) {
            if (series[i - k] <= v || series[i + k] <= v) {
                isLow = false;
            }
        }
        if (isLow) {
            out.append(i);
        }
    }
    return out;
}
