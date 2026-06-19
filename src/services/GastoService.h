#pragma once

#include "models/Categoria.h"   // Categoria::TipoDespesa
#include "models/GastoAvulso.h"
#include <QList>
#include <QtTypes>

// Camada Mid: consolida gastos avulsos por classificacao fixo/variavel (#119).
//
// Regra de resolucao do tipo (decidida em #118/#119): o tipo de um gasto e o
// TipoDespesa da sua categoria, resolvido via categoriaId. Se categoriaId <= 0
// (sem vinculo) ou apontar para categoria inexistente/orfa -> Variavel (default
// seguro, nao infla a despesa fixa do orcamento). SEM heranca pai->filho: cada
// categoria responde por si.
//
// Stateless, sem interface: instancia repos direto a cada chamada (padrao do
// projeto, igual ResumoService). Sem Qt Widgets/Charts — apenas models e repos.
// Todos os valores monetarios em centavos (qint64).
struct ResumoGastos {
    qint64 totalFixos     = 0;   // soma dos gastos resolvidos como Fixo (centavos)
    qint64 totalVariaveis = 0;   // soma dos gastos resolvidos como Variavel (centavos)
    qint64 total          = 0;   // totalFixos + totalVariaveis (centavos)
    int    cntFixos       = 0;   // quantidade de gastos fixos
    int    cntVariaveis   = 0;   // quantidade de gastos variaveis
    // Percentual de fixos em basis points (fixos*10000/total), inteiro, para
    // evitar float — coerente com a convencao do projeto (juros ×10000).
    // 0 quando total == 0 (sem divisao por zero). Ex.: 50% -> 5000.
    int    pctFixosBps    = 0;
};

class GastoService {
public:
    GastoService() = default;

    // Consolida os gastos de um unico mes (1–12) de um ano em fixos/variaveis.
    // Monta um indice categoriaId -> TipoDespesa uma vez (CategoriaRepository::
    // findAll()) para nao consultar o banco por gasto.
    ResumoGastos resumoDoMes(int mes, int ano) const;

    // Lista os gastos do mes cujo tipo resolvido == `tipo`.
    QList<GastoAvulso> porTipo(TipoDespesa tipo, int mes, int ano) const;

    // Conveniencia: soma (centavos) dos gastos do mes do tipo indicado.
    qint64 totalPorTipo(TipoDespesa tipo, int mes, int ano) const;
};
