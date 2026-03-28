#pragma once

#include "models/Pagador.h"
#include <QList>
#include <QStringList>
#include <optional>

class PagadorRepository {
public:
    bool insert(Pagador &p);
    bool update(const Pagador &p);
    bool remove(int id);
    QList<Pagador> findAll() const;
    std::optional<Pagador> findById(int id) const;

    // Finds existing by nome (case-insensitive) or inserts a new one.
    // Returns the id of the found/created record, or 0 on failure.
    int findOrCreate(const QString &nome,
                     const QString &tipo = QStringLiteral("ambos"));

    // Sorted list of names for autocomplete.
    QStringList nomes() const;
};
