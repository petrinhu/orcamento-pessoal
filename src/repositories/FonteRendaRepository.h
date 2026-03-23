#pragma once

#include "models/FonteRenda.h"
#include <QList>
#include <optional>

class FonteRendaRepository {
public:
    bool insert(FonteRenda &f);
    bool update(const FonteRenda &f);
    bool remove(int id);
    QList<FonteRenda> findAll() const;
    std::optional<FonteRenda> findById(int id) const;
    FonteRenda findOrCreate(const QString &nome);
};
