#include "CompraService.h"
#include "repositories/CartaoRepository.h"
#include "repositories/CompraRepository.h"
#include "repositories/ParcelaRepository.h"

// ── Cartões ───────────────────────────────────────────────────────────────────

QList<Cartao> CompraService::cartoes() const
{
    return CartaoRepository().findAll();
}

// ── Compras ───────────────────────────────────────────────────────────────────

QList<Compra> CompraService::compras() const
{
    return CompraRepository().findAll();
}

QList<Parcela> CompraService::parcelasDeCompra(int compraId) const
{
    return ParcelaRepository().findByCompra(compraId);
}

bool CompraService::inserir(Compra &compra, QList<Parcela> &parcelas)
{
    CompraRepository cr;
    if (!cr.insert(compra))
        return false;

    ParcelaRepository pr;
    for (Parcela &p : parcelas) {
        p.compraId = compra.id;
        if (!pr.insert(p))
            return false;
    }
    return true;
}

bool CompraService::atualizar(const Compra &compra)
{
    return CompraRepository().update(compra);
}

bool CompraService::atualizarComParcelas(const Compra &compra, const QList<Parcela> &parcelas)
{
    if (!CompraRepository().update(compra))
        return false;
    return ParcelaRepository().replaceAll(compra.id, parcelas);
}

bool CompraService::remover(int compraId)
{
    return CompraRepository().remove(compraId);
}

bool CompraService::substituirParcelas(int compraId, const QList<Parcela> &parcelas)
{
    return ParcelaRepository().replaceAll(compraId, parcelas);
}
