#pragma once

#include "IMatchFinder.h"

// ScheduledTransactionMatchFinder - busca matches entre transacoes recorrentes
// agendadas (`transacoes_recorrentes`) e o input importado. Usado para marcar
// que um lancamento OFX recem-chegado satisfaz uma recorrencia, evitando que
// o ImportService crie linha redundante.
//
// Compartilha o algoritmo de IMatchFinder; a diferenca eh quem monta a lista
// de candidatos (caller deve filtrar TransacaoRecorrente.ativo + janela de
// data via TransacaoRecorrenteRepository).
class ScheduledTransactionMatchFinder : public IMatchFinder {
public:
    using IMatchFinder::IMatchFinder;
};
