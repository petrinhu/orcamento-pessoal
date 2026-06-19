#include "services/GastoService.h"

#include "repositories/CategoriaRepository.h"
#include "repositories/GastoAvulsoRepository.h"

#include <QHash>

namespace {

// Indice categoriaId -> TipoDespesa, montado uma vez via findAll() para evitar
// uma consulta ao banco por gasto. Categorias com id <= 0 sao ignoradas (nunca
// serao chave de gasto valido).
QHash<int, TipoDespesa> indiceTipoPorCategoria()
{
    QHash<int, TipoDespesa> idx;
    const auto cats = CategoriaRepository().findAll();
    for (const auto &c : cats)
        if (c.id > 0)
            idx.insert(c.id, c.tipoDespesa);
    return idx;
}

// Resolve o tipo de um gasto: TipoDespesa da categoria via categoriaId.
// categoriaId <= 0 (sem vinculo) ou orfa (id ausente no indice) -> Variavel.
TipoDespesa resolverTipo(const GastoAvulso &g, const QHash<int, TipoDespesa> &idx)
{
    if (g.categoriaId <= 0)
        return TipoDespesa::Variavel;
    return idx.value(g.categoriaId, TipoDespesa::Variavel);
}

} // namespace

ResumoGastos GastoService::resumoDoMes(int mes, int ano) const
{
    ResumoGastos r;

    const auto idx    = indiceTipoPorCategoria();
    const auto gastos = GastoAvulsoRepository().findByMesAno(mes, ano);

    for (const auto &g : gastos) {
        if (resolverTipo(g, idx) == TipoDespesa::Fixo) {
            r.totalFixos += g.valor;
            ++r.cntFixos;
        } else {
            r.totalVariaveis += g.valor;
            ++r.cntVariaveis;
        }
    }

    r.total = r.totalFixos + r.totalVariaveis;

    // Percentual de fixos em basis points (inteiro), sem float e sem divisao
    // por zero. fixos*10000/total; arredondamento por truncamento.
    if (r.total > 0)
        r.pctFixosBps = static_cast<int>((r.totalFixos * 10000) / r.total);

    return r;
}

QList<GastoAvulso> GastoService::porTipo(TipoDespesa tipo, int mes, int ano) const
{
    const auto idx    = indiceTipoPorCategoria();
    const auto gastos = GastoAvulsoRepository().findByMesAno(mes, ano);

    QList<GastoAvulso> out;
    for (const auto &g : gastos)
        if (resolverTipo(g, idx) == tipo)
            out.append(g);
    return out;
}

qint64 GastoService::totalPorTipo(TipoDespesa tipo, int mes, int ano) const
{
    const auto idx    = indiceTipoPorCategoria();
    const auto gastos = GastoAvulsoRepository().findByMesAno(mes, ano);

    qint64 soma = 0;
    for (const auto &g : gastos)
        if (resolverTipo(g, idx) == tipo)
            soma += g.valor;
    return soma;
}
