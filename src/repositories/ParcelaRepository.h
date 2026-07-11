#pragma once

#include "models/Parcela.h"
#include <QList>
#include <QSqlQuery>
#include <optional>

class ParcelaRepository final {
public:
    bool insert(Parcela &parcela);
    bool update(const Parcela &parcela);
    bool remove(int id);
    std::optional<Parcela> findById(int id);
    QList<Parcela> findAll();

    // All parcelas of a given compra, ordered by numero
    QList<Parcela> findByCompra(int compraId);

    // Replace all parcelas of a compra (used when regenerating)
    bool replaceAll(int compraId, const QList<Parcela> &parcelas);

private:
    static Parcela fromQuery(const QSqlQuery &q);
};
