// PeuBud - Factory de IInvestActivity por enum/rawType (#89)

#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <memory>

#include "services/investimento/IInvestActivity.h"

class InvestActivityFactory {
public:
    // Cria activity pelo rawType ("buy" | "sell" | "dividend" | ...).
    // Retorna nullptr se rawType desconhecido.
    [[nodiscard]] static std::unique_ptr<IInvestActivity> create(const QString &rawType);

    // Cria activity pelo enum forte (sempre retorna instancia valida).
    [[nodiscard]] static std::unique_ptr<IInvestActivity> create(IInvestActivity::Tipo t);

    // Lista todos os pares (rawType, label) suportados, na ordem canonica
    // (Buy, Sell, Dividend, Reinvest, Yield, Add, Remove, Split, Interest, Bonus).
    // UI usa para popular combo.
    [[nodiscard]] static QList<QPair<QString, QString>> listAll();
};
