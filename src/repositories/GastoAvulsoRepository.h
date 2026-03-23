#pragma once

#include "models/GastoAvulso.h"
#include <QList>
#include <QStringList>
#include <optional>

class GastoAvulsoRepository {
public:
    bool insert(GastoAvulso &g);
    bool update(const GastoAvulso &g);
    bool remove(int id);
    QList<GastoAvulso> findAll() const;
    QList<GastoAvulso> findByMesAno(int mes, int ano) const;
    std::optional<GastoAvulso> findById(int id) const;

    // Distinct categoria values already in the database (for autocomplete).
    QStringList categorias() const;
};
