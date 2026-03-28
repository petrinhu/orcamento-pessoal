#include "TvmService.h"

#include <cmath>
#include <stdexcept>

// ── TVM equation ──────────────────────────────────────────────────────────────
//
// END mode:   PV·(1+r)^n  +  PMT·[(1+r)^n − 1]/r  +  FV  =  0
// BEGIN mode: PV·(1+r)^n  +  PMT·(1+r)·[(1+r)^n − 1]/r  +  FV  =  0
//
// where r = i/100 (rate as fraction).
// For r == 0 (zero interest): PV + PMT·n + FV = 0.
//
// B(r) = 1 for END, (1+r) for BEGIN.

double TvmService::tvmF(double r, double n,
                        double pv, double pmt, double fv, bool begin)
{
    if (std::abs(r) < 1e-12) {
        // Zero-rate limit
        return pv + pmt * n + fv;
    }
    const double F = std::pow(1.0 + r, n);
    const double B = begin ? (1.0 + r) : 1.0;
    return pv * F + pmt * B * (F - 1.0) / r + fv;
}

double TvmService::tvmDF(double r, double n,
                         double pv, double pmt, double fv, bool begin)
{
    const double h = std::max(std::abs(r) * 1e-6, 1e-10);
    return (tvmF(r + h, n, pv, pmt, fv, begin) -
            tvmF(r - h, n, pv, pmt, fv, begin)) / (2.0 * h);
}

// ── Newton-Raphson for i ──────────────────────────────────────────────────────

double TvmService::resolverI(double n, double pv, double pmt, double fv, bool begin)
{
    // Try a set of initial guesses to escape local minima.
    // Rates are searched in the range [−99%, +1000%] per period.
    static const double kGuesses[] = {
        0.01, 0.001, 0.1, 0.05, 0.005, 0.2, 0.5
    };

    double best_r   = 0.0;
    double best_err = std::numeric_limits<double>::max();

    for (double r0 : kGuesses) {
        double r = r0;
        bool   converged = false;

        for (int iter = 0; iter < 200; ++iter) {
            const double f  = tvmF(r, n, pv, pmt, fv, begin);
            const double df = tvmDF(r, n, pv, pmt, fv, begin);

            if (std::abs(df) < 1e-20) break;

            double dr = f / df;
            // Dampen step to stay in valid range
            if (dr > 0.5) dr = 0.5;
            if (dr < -0.5) dr = -0.5;
            r -= dr;

            // Rate must stay > -1 (otherwise (1+r)^n is undefined for real n)
            if (r <= -1.0) { r = -0.99; }

            if (std::abs(f) < 1e-10) {
                converged = true;
                break;
            }
        }

        const double err = std::abs(tvmF(r, n, pv, pmt, fv, begin));
        if (err < best_err) {
            best_err = err;
            best_r   = r;
        }
        if (converged && err < 1e-8) break;
    }

    if (best_err > 1e-4)
        throw std::domain_error("TvmService: solução para i não convergiu");

    return best_r; // as fraction
}

// ── Public API ────────────────────────────────────────────────────────────────

TvmInput TvmService::resolver(TvmInput in, TvmSolve campo)
{
    const double r = in.i / 100.0;
    const double n = in.n;
    const double B = in.begin ? (1.0 + r) : 1.0;

    // Helper: (1+r)^n, with zero-rate guard
    auto F = [&]() -> double {
        return (std::abs(r) < 1e-12) ? 1.0 : std::pow(1.0 + r, n);
    };

    switch (campo) {
    case TvmSolve::N: {
        if (std::abs(r) < 1e-12) {
            if (std::abs(in.pmt) < 1e-12)
                throw std::domain_error("PMT = 0 e i = 0: N indeterminado");
            in.n = -(in.pv + in.fv) / in.pmt;
        } else {
            const double Br = B / r; // B/r factor
            const double num = in.pmt * Br - in.fv;
            const double den = in.pmt * Br + in.pv;
            if (den == 0.0 || num / den <= 0.0)
                throw std::domain_error("TvmService: sem solução real para N");
            in.n = std::log(num / den) / std::log(1.0 + r);
        }
        break;
    }
    case TvmSolve::I: {
        const double rSol = resolverI(in.n, in.pv, in.pmt, in.fv, in.begin);
        in.i = rSol * 100.0;
        break;
    }
    case TvmSolve::PV: {
        const double Fv = F();
        if (std::abs(r) < 1e-12) {
            in.pv = -(in.pmt * n + in.fv);
        } else {
            in.pv = -(in.pmt * B * (Fv - 1.0) / r + in.fv) / Fv;
        }
        break;
    }
    case TvmSolve::PMT: {
        const double Fv = F();
        if (std::abs(r) < 1e-12) {
            if (std::abs(n) < 1e-12)
                throw std::domain_error("N = 0: PMT indeterminado");
            in.pmt = -(in.pv + in.fv) / n;
        } else {
            const double denom = B * (Fv - 1.0) / r;
            if (std::abs(denom) < 1e-20)
                throw std::domain_error("TvmService: denominador zero ao resolver PMT");
            in.pmt = -(in.pv * Fv + in.fv) / denom;
        }
        break;
    }
    case TvmSolve::FV: {
        const double Fv = F();
        if (std::abs(r) < 1e-12) {
            in.fv = -(in.pv + in.pmt * n);
        } else {
            in.fv = -(in.pv * Fv + in.pmt * B * (Fv - 1.0) / r);
        }
        break;
    }
    }

    return in;
}

double TvmService::cetFromMensal(double iMensalPct)
{
    return ParcelaService::cetFromJurosMensais(iMensalPct);
}

double TvmService::mensalFromCet(double cetPct)
{
    return ParcelaService::jurosMensaisFromCet(cetPct);
}

QList<AmortizacaoRow> TvmService::amortizacao(const TvmInput &tvm, QDate dataInicio)
{
    // Price (SAF) schedule in END mode.
    // For BEGIN mode the first payment occurs at period 0; we model it as END
    // but shift dates back by one period — this matches the HP-12C convention.
    const int N = static_cast<int>(std::round(tvm.n));
    if (N <= 0) return {};

    const double r   = tvm.i / 100.0;
    const double pmt = tvm.pmt; // signed: negative for outflow
    double saldo      = tvm.pv;  // positive = amount owed

    QList<AmortizacaoRow> rows;
    rows.reserve(N);

    for (int k = 1; k <= N; ++k) {
        AmortizacaoRow row;
        row.numero = k;
        row.data   = dataInicio.addMonths(k - 1 + (tvm.begin ? -1 : 0));

        const double juros     = saldo * r;
        const double amort     = (-pmt) - juros; // pmt is negative outflow
        saldo                 -= amort;

        // Store as centavos (× 100), matching the existing AmortizacaoRow layout.
        row.saldoDevedor = static_cast<qint64>(std::round(saldo * 100.0));
        row.juros        = static_cast<qint64>(std::round(juros * 100.0));
        row.amortizacao  = static_cast<qint64>(std::round(amort * 100.0));
        row.parcela      = static_cast<qint64>(std::round((-pmt) * 100.0));

        rows.append(row);
    }

    return rows;
}
