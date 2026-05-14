// PeuBud - 10 implementacoes concretas de IInvestActivity (#89)
//
// Cada subclass eh um POD comportamental (sem estado). Construir via
// InvestActivityFactory (preferencial) ou instanciar diretamente.

#pragma once

#include "services/investimento/IInvestActivity.h"

namespace invest {

// ── BuyShares ────────────────────────────────────────────────────────────────
class BuySharesActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── SellShares ───────────────────────────────────────────────────────────────
class SellSharesActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── Dividend ─────────────────────────────────────────────────────────────────
class DividendActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── ReinvestDividend (DRIP) ──────────────────────────────────────────────────
class ReinvestDividendActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── Yield (CDB/Tesouro/FII) ──────────────────────────────────────────────────
class YieldActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── AddShares ────────────────────────────────────────────────────────────────
class AddSharesActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── RemoveShares ─────────────────────────────────────────────────────────────
class RemoveSharesActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── SplitShares ──────────────────────────────────────────────────────────────
class SplitSharesActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── InterestIncome (JCP) ─────────────────────────────────────────────────────
class InterestIncomeActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

// ── Bonificacao ──────────────────────────────────────────────────────────────
class BonificacaoActivity final : public IInvestActivity {
public:
    [[nodiscard]] QString rawType() const override;
    [[nodiscard]] Tipo    type()    const override;
    [[nodiscard]] QString label()   const override;
    [[nodiscard]] CamposVisiveis camposVisiveis() const override;
    [[nodiscard]] QStringList consistencyCheck(
        const LancamentoInvestimento &l) const override;
};

} // namespace invest
