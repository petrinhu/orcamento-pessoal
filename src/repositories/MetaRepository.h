#pragma once

#include "models/Meta.h"
#include <QList>
#include <optional>

class MetaRepository {
public:
    bool insert(Meta &m);
    bool update(const Meta &m);
    bool remove(int id);

    QList<Meta> findAll() const;
    std::optional<Meta> findById(int id) const;
};
