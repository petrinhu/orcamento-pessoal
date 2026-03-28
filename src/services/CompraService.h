#pragma once

#include "models/Cartao.h"
#include "models/Compra.h"
#include "models/Parcela.h"
#include <QList>

// Service that coordinates CompraRepository + ParcelaRepository + CartaoRepository.
// ParcelasWidget interacts exclusively with this class.
class CompraService {
public:
    // ── Cartões ───────────────────────────────────────────────────────────────
    QList<Cartao> cartoes() const;

    // ── Compras ───────────────────────────────────────────────────────────────
    QList<Compra> compras() const;
    QList<Parcela> parcelasDeCompra(int compraId) const;

    // Insert compra + its parcelas (sets compra.id on success).
    bool inserir(Compra &compra, QList<Parcela> &parcelas);

    // Update compra fields only (no parcela changes).
    bool atualizar(const Compra &compra);

    // Update compra fields + replace all its parcelas.
    bool atualizarComParcelas(const Compra &compra, const QList<Parcela> &parcelas);

    // Delete compra (cascades to parcelas via FK).
    bool remover(int compraId);

    // Replace all parcelas of a compra (used by CompraDetailPanel).
    bool substituirParcelas(int compraId, const QList<Parcela> &parcelas);
};
