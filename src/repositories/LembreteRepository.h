#pragma once

#include "models/Lembrete.h"
#include <QList>
#include <optional>

class LembreteRepository {
public:
    bool insert(Lembrete &l);
    bool update(const Lembrete &l);
    bool remove(int id);

    QList<Lembrete> findAll() const;
    // Retorna lembretes pendentes ou não pagos ordenados por vencimento.
    QList<Lembrete> findAtivos() const;
    std::optional<Lembrete> findById(int id) const;
};
