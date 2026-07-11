#pragma once

#include "models/Compra.h"
#include <QList>
#include <QSqlQuery>
#include <optional>

class CompraRepository final {
public:
    bool insert(Compra &compra);
    bool update(const Compra &compra);
    bool remove(int id);
    std::optional<Compra> findById(int id);
    QList<Compra> findAll();

    // Find all with optional cartao filter (0 = all)
    QList<Compra> findByCartao(int cartaoId);

private:
    static Compra fromQuery(const QSqlQuery &q);
};
