#pragma once

#include "core/interfaces/IRepository.h"
#include "models/Parcela.h"
#include <QSqlQuery>

class ParcelaRepository final : public IRepository<Parcela> {
public:
    bool insert(Parcela &parcela) override;
    bool update(const Parcela &parcela) override;
    bool remove(int id) override;
    std::optional<Parcela> findById(int id) override;
    QList<Parcela> findAll() override;

    // All parcelas of a given compra, ordered by numero
    QList<Parcela> findByCompra(int compraId);

    // Replace all parcelas of a compra (used when regenerating)
    bool replaceAll(int compraId, const QList<Parcela> &parcelas);

private:
    static Parcela fromQuery(const QSqlQuery &q);
};
