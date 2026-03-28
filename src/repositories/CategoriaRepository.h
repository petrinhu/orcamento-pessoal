#pragma once

#include "models/Categoria.h"
#include <QList>
#include <QString>

class CategoriaRepository {
public:
    // CRUD
    int  insert(const Categoria &c);
    bool update(const Categoria &c);
    bool remove(int id);

    // Queries
    QList<Categoria> findAll() const;
    QList<Categoria> findByTipo(const QString &tipo) const;
    Categoria        findById(int id) const;

    // Retorna id (existente ou recém-criado); case-insensitive por nome+tipo
    int findOrCreate(const QString &nome, const QString &tipo, int paiId = 0);

    // Lista plana de nomes para autocomplete: "Pai > Filho" ou "Raiz"
    QStringList nomes(const QString &tipo) const;
};
