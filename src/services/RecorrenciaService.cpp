#include "RecorrenciaService.h"
#include "repositories/TransacaoRecorrenteRepository.h"
#include "repositories/GastoAvulsoRepository.h"
#include "services/EntradaService.h"
#include "models/LancamentoEntrada.h"
#include "models/GastoAvulso.h"

#include <QDateTime>

// ── Static helper ─────────────────────────────────────────────────────────────

QDate RecorrenciaService::proximaApos(QDate d, const QString &frequencia)
{
    if (frequencia == QStringLiteral("semanal"))   return d.addDays(7);
    if (frequencia == QStringLiteral("quinzenal")) return d.addDays(14);
    if (frequencia == QStringLiteral("anual"))     return d.addYears(1);
    return d.addMonths(1); // mensal (default)
}

// ── Generation ────────────────────────────────────────────────────────────────

int RecorrenciaService::gerarPendentes(QDate ate)
{
    int count = 0;
    TransacaoRecorrenteRepository repo;
    const auto todas = repo.findAll();

    for (auto tr : todas) {
        if (!tr.ativo) continue;

        QDate next;
        if (!tr.ultimoLancamento.isValid())
            next = tr.dataInicio;
        else
            next = proximaApos(tr.ultimoLancamento, tr.frequencia);

        while (next <= ate && next >= tr.dataInicio) {
            if (tr.dataFim.isValid() && next > tr.dataFim) break;

            if (tr.tipo == QStringLiteral("entrada")) {
                LancamentoEntrada l;
                l.mes        = next.month();
                l.ano        = next.year();
                l.dia        = next.day();
                l.fonte      = tr.fonte;
                l.fonteId    = tr.fonteId;
                l.valorBruto = tr.valor;
                l.obs        = tr.obs;
                l.criadoEm   = QDateTime::currentDateTime().toString(Qt::ISODate);
                EntradaService().salvarComFonte(l);
            } else {
                GastoAvulso g;
                g.mes       = next.month();
                g.ano       = next.year();
                g.dia       = next.day();
                g.descricao = tr.descricao;
                g.categoria = tr.categoria;
                g.tipoPgto  = tr.tipoPgto;
                g.valor     = tr.valor;
                g.obs       = tr.obs;
                g.criadoEm  = QDateTime::currentDateTime().toString(Qt::ISODate);
                GastoAvulsoRepository().insert(g);
            }

            tr.ultimoLancamento = next;
            repo.updateUltimoLancamento(tr.id, next);
            ++count;

            next = proximaApos(next, tr.frequencia);
        }
    }
    return count;
}
