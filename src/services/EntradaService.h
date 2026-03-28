#pragma once

#include "models/FonteRenda.h"
#include "models/LancamentoEntrada.h"
#include <QList>

// Service that coordinates LancamentoEntradaRepository + FonteRendaRepository.
// Widgets interact exclusively with this class — no direct repository access.
class EntradaService {
public:
    // ── Fontes ────────────────────────────────────────────────────────────────
    QList<FonteRenda> fontes() const;
    bool inserirFonte(FonteRenda &f);
    bool removerFonte(int id);

    // ── Lançamentos ───────────────────────────────────────────────────────────
    QList<LancamentoEntrada> lancamentos() const;

    // If l.fonte is non-empty and l.fonteId == 0, resolves fonteId via findOrCreate.
    // Inserts when l.id == 0, updates otherwise.
    bool salvarComFonte(LancamentoEntrada &l);

    bool remover(int id);
};
