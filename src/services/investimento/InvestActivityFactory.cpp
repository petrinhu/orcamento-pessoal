// PeuBud - InvestActivityFactory (#89)

#include "services/investimento/InvestActivityFactory.h"

#include "services/investimento/InvestActivities.h"

namespace {

// Tabela canonica (Tipo -> rawType -> construtor). Mantida em um unico ponto
// para garantir DRY entre create(QString), create(Tipo) e listAll().
struct Entry {
    IInvestActivity::Tipo tipo;
    const char *rawType;
    std::unique_ptr<IInvestActivity> (*ctor)();
};

template <typename T>
std::unique_ptr<IInvestActivity> make() { return std::make_unique<T>(); }

const QList<Entry> &table()
{
    using T = IInvestActivity::Tipo;
    static const QList<Entry> kTable = {
        { T::BuyShares,        "buy",      &make<invest::BuySharesActivity> },
        { T::SellShares,       "sell",     &make<invest::SellSharesActivity> },
        { T::Dividend,         "dividend", &make<invest::DividendActivity> },
        { T::ReinvestDividend, "reinvest", &make<invest::ReinvestDividendActivity> },
        { T::Yield,            "yield",    &make<invest::YieldActivity> },
        { T::AddShares,        "add",      &make<invest::AddSharesActivity> },
        { T::RemoveShares,     "remove",   &make<invest::RemoveSharesActivity> },
        { T::SplitShares,      "split",    &make<invest::SplitSharesActivity> },
        { T::InterestIncome,   "interest", &make<invest::InterestIncomeActivity> },
        { T::Bonificacao,      "bonus",    &make<invest::BonificacaoActivity> },
    };
    return kTable;
}

} // namespace

std::unique_ptr<IInvestActivity> InvestActivityFactory::create(const QString &rawType)
{
    for (const auto &e : table()) {
        if (rawType == QLatin1String(e.rawType))
            return e.ctor();
    }
    return nullptr;
}

std::unique_ptr<IInvestActivity> InvestActivityFactory::create(IInvestActivity::Tipo t)
{
    for (const auto &e : table()) {
        if (e.tipo == t)
            return e.ctor();
    }
    return nullptr; // inalcancavel se enum coberto
}

QList<QPair<QString, QString>> InvestActivityFactory::listAll()
{
    QList<QPair<QString, QString>> out;
    out.reserve(table().size());
    for (const auto &e : table()) {
        auto inst = e.ctor();
        out.append({ inst->rawType(), inst->label() });
    }
    return out;
}
