#pragma once

#include "services/ParcelaService.h" // AmortizacaoRow

#include <QDate>
#include <QList>

// Time Value of Money input registers (HP-12C style).
// i is stored as percentage per period (e.g. 1.5 means 1.5 % per period).
// Monetary values are plain doubles (user-entered in the calculator UI).
struct TvmInput {
    double n     = 0.0;   // number of periods
    double i     = 0.0;   // rate per period in % (e.g. 1.5 = 1.5 % a.m.)
    double pv    = 0.0;   // present value  (positive = cash inflow)
    double pmt   = 0.0;   // payment per period
    double fv    = 0.0;   // future value
    bool   begin = false; // true = BEGIN mode (payment at start of each period)
};

enum class TvmSolve { N, I, PV, PMT, FV };

// Pure financial math — no DB access, no UI dependencies.
// Sign convention: cash outflows are negative (standard HP-12C).
class TvmService {
public:
    // Solve for the requested field; the other four must be set in `in`.
    // Returns updated TvmInput with the solved field filled in.
    // Throws std::domain_error if the equation has no real solution.
    static TvmInput resolver(TvmInput in, TvmSolve campo);

    // CET (annual effective rate) ↔ monthly rate conversions.
    static double cetFromMensal(double iMensalPct);
    static double mensalFromCet(double cetPct);

    // Price (SAF) amortization schedule derived from TvmInput.
    // dataInicio: date of the first period (period 0 for BEGIN, period 1 for END).
    // Returns one row per period (N rows total), matching the AmortizacaoRow layout
    // used by ParcelaService::tabelaAmortizacao.
    static QList<AmortizacaoRow> amortizacao(const TvmInput &tvm, QDate dataInicio);

private:
    // Evaluate the TVM function at rate fraction r (r = i/100).
    static double tvmF(double r, double n, double pv, double pmt, double fv, bool begin);

    // Numerical derivative of tvmF w.r.t. r.
    static double tvmDF(double r, double n, double pv, double pmt, double fv, bool begin);

    // Newton-Raphson solver for i; initial guess as fraction.
    static double resolverI(double n, double pv, double pmt, double fv, bool begin);
};
