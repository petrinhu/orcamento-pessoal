#pragma once

#include "models/Divida.h"
#include <QList>
#include <optional>

class DividaRepository {
public:
    bool insert(Divida &d);
    bool update(const Divida &d);
    bool remove(int id);

    QList<Divida> findAll() const;
    QList<Divida> findByStatus(const QString &status) const;
    std::optional<Divida> findById(int id) const;
};
