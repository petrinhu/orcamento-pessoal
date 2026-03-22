#pragma once

#include "core/interfaces/IRepository.h"
#include "models/Compra.h"
#include <QSqlQuery>

class CompraRepository final : public IRepository<Compra> {
public:
    bool insert(Compra &compra) override;
    bool update(const Compra &compra) override;
    bool remove(int id) override;
    std::optional<Compra> findById(int id) override;
    QList<Compra> findAll() override;

    // Find all with optional cartao filter (0 = all)
    QList<Compra> findByCartao(int cartaoId);

private:
    static Compra fromQuery(const QSqlQuery &q);
};
