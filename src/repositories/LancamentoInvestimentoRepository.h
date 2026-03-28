#pragma once

#include "models/LancamentoInvestimento.h"
#include <QList>
#include <optional>

class LancamentoInvestimentoRepository {
public:
    bool insert(LancamentoInvestimento &l);
    bool update(const LancamentoInvestimento &l);
    bool remove(int id);
    QList<LancamentoInvestimento> findAll() const;
    QList<LancamentoInvestimento> findByMesAno(int mes, int ano) const;
    std::optional<LancamentoInvestimento> findById(int id) const;
};
