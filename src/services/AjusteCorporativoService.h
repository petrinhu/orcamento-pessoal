#pragma once

#include "models/AjusteCorporativo.h"

#include <QDate>
#include <QList>
#include <QString>
#include <optional>

// AjusteCorporativoService (#65) - regras de splits/bonificacoes.
//
// Stateless. Persistencia em ajustes_corporativos. Util principal eh
// `fatorCumulativo(ticker, dataAlvo, refData)` que calcula multiplicador
// para converter preco/qty historico em sequencia ajustada (split-adjusted),
// padrao usado em graficos de longo prazo.

class AjusteCorporativoService
{
public:
    static bool ensureSchema();

    // CRUD basico
    [[nodiscard]] static bool insert(AjusteCorporativo &a);
    [[nodiscard]] static bool remove(qint64 id);

    [[nodiscard]] static QList<AjusteCorporativo> findByTicker(const QString &ticker);
    [[nodiscard]] static QList<AjusteCorporativo> findAll();

    // Multiplicador para preco/qty de uma data passada. Mantem semantica:
    //   precoAjustado(dataAntiga) = precoOriginal / fatorCumulativo(...)
    //   qtyAjustada (dataAntiga)  = qtyOriginal  * fatorCumulativo(...)
    //
    // Retorna 1.0 se nao houver ajustes entre dataAlvo (exclusivo) e refData
    // (inclusivo). Calculo puro em memoria a partir de `eventos`.
    [[nodiscard]] static inline double fatorCumulativo(
        const QString &ticker,
        const QDate &dataAlvo,
        const QDate &refData,
        const QList<AjusteCorporativo> &eventos)
    {
        if (!dataAlvo.isValid() || !refData.isValid()) return 1.0;
        if (dataAlvo >= refData) return 1.0;
        double cumulativo = 1.0;
        const QString tk = ticker.toUpper();
        for (const auto &e : eventos) {
            if (e.ticker.toUpper() != tk) continue;
            if (e.dataEx > dataAlvo && e.dataEx <= refData) {
                cumulativo *= e.fator;
            }
        }
        return cumulativo;
    }
};
