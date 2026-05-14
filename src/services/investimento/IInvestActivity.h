// PeuBud - Activity pattern para operacoes de investimento (#89)
// Inspirado em kmymoney/views/investactivities.h e mymoneyenums.h.
//
// Cada operacao de investimento (Buy/Sell/Dividend/Yield/Split/...)
// implementa IInvestActivity, expondo:
//   - rawType()        : string persistida no DB (LancamentoInvestimento.tipoOperacao)
//   - type()           : enum forte
//   - label()          : texto humano (combo UI)
//   - camposVisiveis() : quais widgets do dialog mostrar
//   - consistencyCheck(): valida invariantes do tipo
//
// Substitui if/switch sobre QString por dispatch polimorfico (SRP + OCP).

#pragma once

#include <QString>
#include <QStringList>

#include "models/LancamentoInvestimento.h"

class IInvestActivity {
public:
    virtual ~IInvestActivity() = default;

    enum class Tipo {
        BuyShares,        // compra de cotas/acoes
        SellShares,       // venda
        Dividend,         // dividendo recebido em cash
        ReinvestDividend, // dividendo reinvestido (DRIP)
        Yield,            // rendimento (CDB, Tesouro, FII)
        AddShares,        // entrada sem custo (bonus, splits, heranca)
        RemoveShares,     // saida sem custo
        SplitShares,      // desdobramento
        InterestIncome,   // juros sobre capital proprio
        Bonificacao       // bonificacao em acoes
    };

    // Identificador persistido em DB ("buy" | "sell" | ...).
    [[nodiscard]] virtual QString rawType() const = 0;
    // Enum forte.
    [[nodiscard]] virtual Tipo    type()    const = 0;
    // Texto humano para combo UI.
    [[nodiscard]] virtual QString label()   const = 0;

    // Quais campos da UI o dialog deve exibir para este tipo.
    // O dialog espelha em show()/hide() de cada widget.
    struct CamposVisiveis {
        bool ticker        = true;
        bool quantidade    = true;
        bool precoUnitario = true;
        bool valor         = true;  // sempre visivel (calculado ou manual)
        bool jurosOuTaxa   = false;
        bool fatorSplit    = false;
    };
    [[nodiscard]] virtual CamposVisiveis camposVisiveis() const = 0;

    // Valida o lancamento contra invariantes do tipo. Lista vazia = OK.
    [[nodiscard]] virtual QStringList consistencyCheck(
        const LancamentoInvestimento &l) const = 0;
};
