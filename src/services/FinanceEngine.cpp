#include "FinanceEngine.h"
#include <cmath>
#include <stdexcept>

// ── Helpers ──────────────────────────────────────────────────────────────────

// f(i) = pv*(1+i)^n + pmt*(1+i*begin)*((1+i)^n - 1)/i + fv
// Retorna o residuo da equacao TVM em i.
static double tvmF(double pv, double pmt, double fv, double i, double n, bool begin)
{
    if (i == 0.0) {
        // Equacao degenerada: pv + pmt*n + fv = 0
        return pv + pmt * n + fv;
    }
    const double pot   = std::pow(1.0 + i, n);
    const double anuit = (pot - 1.0) / i;
    const double adj   = begin ? (1.0 + i) : 1.0;
    return pv * pot + pmt * adj * anuit + fv;
}

// df/di analitica para Newton-Raphson.
// d/di [ pv*(1+i)^n + pmt*adj*((1+i)^n - 1)/i + fv ]
//   = pv*n*(1+i)^(n-1)
//     + pmt*[ d(adj)/di * anuit + adj * d(anuit)/di ]
//
// Para BEGIN (adj=1+i): d(adj)/di = 1
// Para END   (adj=1)  : d(adj)/di = 0
//
// d(anuit)/di = d/di[(pot-1)/i] = (n*pot/(1+i)*i - (pot-1)) / i^2
//             = (n*pot/(1+i) - anuit) / i
static double tvmDF(double pv, double pmt, double i, double n, bool begin)
{
    if (i == 0.0) {
        // d/di na vizinhanca de 0 - expansao de Taylor; aproximacao numerica
        const double h = 1e-7;
        const double fmais  = tvmF(pv, pmt, 0.0, h,  n, begin);
        const double fmenos = tvmF(pv, pmt, 0.0, -h, n, begin);
        return (fmais - fmenos) / (2.0 * h);
    }
    const double pot       = std::pow(1.0 + i, n);
    const double anuit     = (pot - 1.0) / i;
    const double dAnuit    = (n * pot / (1.0 + i) - anuit) / i;
    const double adj       = begin ? (1.0 + i) : 1.0;
    const double dAdj      = begin ? 1.0 : 0.0;
    return pv * n * std::pow(1.0 + i, n - 1.0)
         + pmt * (dAdj * anuit + adj * dAnuit);
}

// ── PMT ──────────────────────────────────────────────────────────────────────
double FinanceEngine::pmt(double pv, double fv, double i, int n, bool begin)
{
    if (n == 0) throw std::domain_error("pmt: n must be > 0");
    if (i == 0.0) {
        return -(pv + fv) / static_cast<double>(n);
    }
    const double pot   = std::pow(1.0 + i, n);
    const double adj   = begin ? (1.0 + i) : 1.0;
    return -(pv * pot + fv) * i / (adj * (pot - 1.0));
}

// ── PV ───────────────────────────────────────────────────────────────────────
double FinanceEngine::pv(double pmt, double fv, double i, int n, bool begin)
{
    if (n == 0) throw std::domain_error("pv: n must be > 0");
    if (i == 0.0) {
        return -(pmt * static_cast<double>(n) + fv);
    }
    const double pot   = std::pow(1.0 + i, n);
    const double adj   = begin ? (1.0 + i) : 1.0;
    const double anuit = (pot - 1.0) / i;
    return -(pmt * adj * anuit + fv) / pot;
}

// ── FV ───────────────────────────────────────────────────────────────────────
double FinanceEngine::fv(double pv, double pmt, double i, int n, bool begin)
{
    if (n == 0) throw std::domain_error("fv: n must be > 0");
    if (i == 0.0) {
        return -(pv + pmt * static_cast<double>(n));
    }
    const double pot   = std::pow(1.0 + i, n);
    const double adj   = begin ? (1.0 + i) : 1.0;
    const double anuit = (pot - 1.0) / i;
    return -(pv * pot + pmt * adj * anuit);
}

// ── NPER ─────────────────────────────────────────────────────────────────────
// Formula fechada (deriva da equacao TVM):
//   pv*(1+i)^n + pmt*adj*((1+i)^n - 1)/i + fv = 0
//   (1+i)^n * (pv + pmt*adj/i) = pmt*adj/i - fv
//   n = ln( (pmt*adj/i - fv) / (pv + pmt*adj/i) ) / ln(1+i)
double FinanceEngine::nper(double pv, double pmt, double fv, double i, bool begin)
{
    if (i == 0.0) {
        if (pmt == 0.0) throw std::domain_error("nper: pmt=0 e i=0");
        return -(pv + fv) / pmt;
    }
    const double adj = begin ? (1.0 + i) : 1.0;
    const double a   = pmt * adj / i;
    const double num = a - fv;
    const double den = pv + a;
    if (den == 0.0 || num / den <= 0.0) {
        throw std::domain_error("nper: solucao inexistente (log de nao-positivo)");
    }
    return std::log(num / den) / std::log(1.0 + i);
}

// ── RATE (Newton-Raphson) ────────────────────────────────────────────────────
// Algoritmo derivado de gnucash fin.c:1304-1366.
// Guess inicial em 3 ramos:
//   - Se pmt e fv tem mesmo sinal: guess = sign-aware ratio
//   - Se pv e fv tem sinais opostos (caso comum): guess = (fv/pv)^(1/n) - 1
//   - Fallback: 0.01 (1% por periodo)
double FinanceEngine::rate(double pv, double pmt, double fv, int n, bool begin)
{
    if (n == 0) throw std::domain_error("rate: n must be > 0");

    // Guess inicial inteligente
    double i;
    if (pmt == 0.0 && pv != 0.0 && fv != 0.0
        && (pv * fv) < 0.0) {
        // Caso simples: PV cresce/diminui ate FV sem pagamentos
        i = std::pow(-fv / pv, 1.0 / n) - 1.0;
    } else if (pv != 0.0 && pmt != 0.0 && (pv * pmt) < 0.0) {
        // Emprestimo classico: estimativa baseada em razao pmt/pv
        i = std::abs(pmt) / std::abs(pv);
        if (i > 1.0) i = 0.01; // sanity
    } else {
        i = 0.01;
    }

    // Newton-Raphson com 50 iteracoes max, tolerancia 1e-10
    constexpr int    MAX_ITER = 50;
    constexpr double TOL_F    = 1e-10;
    constexpr double TOL_I    = 1e-12;

    for (int iter = 0; iter < MAX_ITER; ++iter) {
        const double f = tvmF(pv, pmt, fv, i, static_cast<double>(n), begin);
        if (std::abs(f) < TOL_F) return i;

        const double df = tvmDF(pv, pmt, i, static_cast<double>(n), begin);
        if (std::abs(df) < 1e-14) {
            throw std::domain_error("rate: derivada zero, sem convergencia");
        }

        const double delta = f / df;
        const double iNew  = i - delta;

        // Sanity: evita taxa <= -100% (singularidade em (1+i)^n)
        if (iNew <= -0.999999) {
            i = (i - 0.999999) / 2.0;
            continue;
        }
        if (std::abs(delta) < TOL_I) return iNew;
        i = iNew;
    }
    throw std::domain_error("rate: Newton-Raphson nao convergiu em 50 iteracoes");
}
