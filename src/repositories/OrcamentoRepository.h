#pragma once

#include "models/ItemOrcamento.h"
#include <QList>
#include <optional>

class OrcamentoRepository {
public:
    bool insert(ItemOrcamento &item);
    bool update(const ItemOrcamento &item);
    bool remove(int id);
    // Returns all items ordered by tipo (receita first) then nome.
    QList<ItemOrcamento> findAll() const;
    std::optional<ItemOrcamento> findById(int id) const;
};
