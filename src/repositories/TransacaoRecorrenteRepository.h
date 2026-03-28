#pragma once

#include "models/TransacaoRecorrente.h"
#include <QList>
#include <optional>

class TransacaoRecorrenteRepository {
public:
    bool insert(TransacaoRecorrente &t);
    bool update(const TransacaoRecorrente &t);
    bool remove(int id);
    QList<TransacaoRecorrente> findAll() const;
    std::optional<TransacaoRecorrente> findById(int id) const;

    // Update only the ultimo_lancamento field (called by RecorrenciaService after each entry generated).
    bool updateUltimoLancamento(int id, const QDate &data);
};
