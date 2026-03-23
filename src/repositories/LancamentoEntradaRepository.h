#pragma once

#include "models/LancamentoEntrada.h"
#include <QList>
#include <optional>

class LancamentoEntradaRepository {
public:
    bool insert(LancamentoEntrada &l);
    bool update(const LancamentoEntrada &l);
    bool remove(int id);
    QList<LancamentoEntrada> findAll() const;
    QList<LancamentoEntrada> findByMesAno(int mes, int ano) const;
    std::optional<LancamentoEntrada> findById(int id) const;
};
