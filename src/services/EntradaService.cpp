#include "EntradaService.h"
#include "repositories/FonteRendaRepository.h"
#include "repositories/LancamentoEntradaRepository.h"

// ── Fontes ────────────────────────────────────────────────────────────────────

QList<FonteRenda> EntradaService::fontes() const
{
    return FonteRendaRepository().findAll();
}

bool EntradaService::inserirFonte(FonteRenda &f)
{
    return FonteRendaRepository().insert(f);
}

bool EntradaService::removerFonte(int id)
{
    return FonteRendaRepository().remove(id);
}

// ── Lançamentos ───────────────────────────────────────────────────────────────

QList<LancamentoEntrada> EntradaService::lancamentos() const
{
    return LancamentoEntradaRepository().findAll();
}

bool EntradaService::salvarComFonte(LancamentoEntrada &l)
{
    // Resolve fonteId if the caller supplied a name but no id.
    if (!l.fonte.trimmed().isEmpty() && l.fonteId == 0) {
        const FonteRenda f = FonteRendaRepository().findOrCreate(l.fonte.trimmed());
        l.fonteId = f.id;
    }

    LancamentoEntradaRepository repo;
    if (l.id == 0)
        return repo.insert(l);
    return repo.update(l);
}

bool EntradaService::remover(int id)
{
    return LancamentoEntradaRepository().remove(id);
}
