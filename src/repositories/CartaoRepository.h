#pragma once

#include "models/Cartao.h"
#include <QList>
#include <optional>

class CartaoRepository {
public:
    bool insert(Cartao &c);
    bool update(const Cartao &c);
    bool remove(int id);
    QList<Cartao> findAll() const;
    std::optional<Cartao> findById(int id) const;
};
