#pragma once

#include "IMatchFinder.h"

// ExistingTransactionMatchFinder - busca matches entre transacoes ja
// persistidas (compras/lancamentos_entradas/lancamentos_gastos) e o input
// importado. Detecta duplicatas em re-imports e variantes proximas que
// merecem revisao manual.
//
// O caller monta a lista de candidatos via repositorios (delegando filtros
// por data/conta) e injeta no construtor. Mantem o algoritmo desacoplado
// do banco para testabilidade.
class ExistingTransactionMatchFinder : public IMatchFinder {
public:
    using IMatchFinder::IMatchFinder;
};
