#include "IMatchFinder.h"

#include <QString>
#include <cmath>

namespace {

// Tolerancia para considerar amount "aproximado": 1% relativo.
constexpr double AMOUNT_TOLERANCE_PCT = 0.01;

bool amountsEqual(qint64 a, qint64 b)
{
    return a == b;
}

bool amountsClose(qint64 a, qint64 b)
{
    if (a == 0 || b == 0) return a == b;
    const double ref  = std::abs(static_cast<double>(a));
    const double diff = std::abs(static_cast<double>(a - b));
    return diff / ref <= AMOUNT_TOLERANCE_PCT;
}

QString normalizePayee(const QString &s)
{
    return s.trimmed().toLower();
}

bool payeesMatch(const QString &a, const QString &b)
{
    const QString na = normalizePayee(a);
    const QString nb = normalizePayee(b);
    if (na.isEmpty() || nb.isEmpty()) return false;
    return na == nb;
}

bool payeesSimilar(const QString &a, const QString &b)
{
    const QString na = normalizePayee(a);
    const QString nb = normalizePayee(b);
    if (na.isEmpty() || nb.isEmpty()) return false;
    return na.contains(nb) || nb.contains(na);
}

} // namespace

MatchResult IMatchFinder::find(const MatchData &input) const
{
    // Passo 1: bankId match -> Duplicate (independe de janela)
    if (!input.bankId.isEmpty()) {
        for (const MatchData &c : m_candidates) {
            if (!c.bankId.isEmpty() && c.bankId == input.bankId)
                return MatchResult::Duplicate;
        }
    }

    // Passo 2-3: matching heuristico, restrito a janela de datas
    bool foundImprecise = false;
    for (const MatchData &c : m_candidates) {
        if (!c.date.isValid() || !input.date.isValid()) continue;
        const int diff = std::abs(c.date.daysTo(input.date));
        if (diff > m_matchWindow) continue;

        const bool valorIgual    = amountsEqual(c.amount, input.amount);
        const bool valorProximo  = amountsClose(c.amount, input.amount);
        const bool payeeIgual    = payeesMatch(c.payee, input.payee);
        const bool payeeProximo  = payeesSimilar(c.payee, input.payee);
        const bool contaIgual    = c.accountId.isEmpty() || input.accountId.isEmpty()
                                   || c.accountId == input.accountId;

        if (valorIgual && payeeIgual && contaIgual)
            return MatchResult::Precise;

        if (valorIgual && payeeIgual && !contaIgual)
            foundImprecise = true;
        else if (valorProximo || payeeProximo)
            foundImprecise = true;
    }

    return foundImprecise ? MatchResult::Imprecise : MatchResult::NotFound;
}
