#include "services/ResumoService.h"

#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "repositories/ProventoRepository.h"
#include "services/CompraService.h"
#include "services/EntradaService.h"
#include "services/ParcelaService.h"

#include <QDate>

// Lógica extraída de DashboardWidget::atualizarDados() (#115). A correção D0
// (rendimento/dividendo/JCP fora de aportes/resgates) é aplicada aqui.

ResumoMensal ResumoService::resumoDoMes(int mes, int ano) const
{
    ResumoMensal r;
    r.mes = mes;
    r.ano = ano;

    // ── Entradas do mês ────────────────────────────────────────────────────────
    {
        const auto todos = EntradaService().lancamentos();
        for (const auto &l : todos) {
            if (l.mes == mes && l.ano == ano) {
                r.entradasBrutas   += l.valorBruto;
                r.irpfRetido       += l.irpf;
                r.inssRetido       += l.inss;
                r.entradasLiquidas += l.valorBruto - l.irpf - l.inss;
                ++r.cntEntradas;
            }
        }
    }

    // ── Gastos do mês ──────────────────────────────────────────────────────────
    {
        const auto gastos = GastoAvulsoRepository().findByMesAno(mes, ano);
        for (const auto &g : gastos) {
            r.gastos += g.valor;
            ++r.cntGastos;
        }
    }

    // ── Parcelas do mês ────────────────────────────────────────────────────────
    {
        CompraService svc;
        const auto compras = svc.compras();
        for (const auto &c : compras) {
            for (const auto &p : svc.parcelasDeCompra(c.id)) {
                const QDate d = ParcelaService::dataParaParcela(c, p.numero);
                if (d.year() == ano && d.month() == mes) {
                    r.parcelas += p.valor;
                    ++r.cntParcelas;
                }
            }
        }
    }

    // ── Investimentos do mês (correção D0) ─────────────────────────────────────
    // Apenas "aporte" e "resgate" entram no fluxo de caixa de investimentos.
    // "rendimento"/"dividendo"/JCP vivem só no card Proventos — evita dupla
    // contagem. Sinal de investimentosNet: resgates − aportes (entrada de caixa
    // positiva), coerente com valorSignadoInv() do model.
    {
        const auto invs = LancamentoInvestimentoRepository().findByMesAno(mes, ano);
        for (const auto &inv : invs) {
            if (inv.tipoOperacao == QStringLiteral("aporte"))
                r.aportes += inv.valor;
            else if (inv.tipoOperacao == QStringLiteral("resgate"))
                r.resgates += inv.valor;
            // rendimento/dividendo: ignorados aqui (D0).
        }
    }
    r.investimentosNet = r.resgates - r.aportes;

    // ── Saldos derivados ───────────────────────────────────────────────────────
    r.saldoMes     = r.entradasLiquidas - r.gastos;
    r.saldoLiquido = r.entradasLiquidas - r.gastos - r.parcelas;

    return r;
}

ResumoAnual ResumoService::resumoDoAno(int ano) const
{
    ResumoAnual r;
    r.ano = ano;

    {
        const auto todos = EntradaService().lancamentos();
        for (const auto &l : todos)
            if (l.ano == ano)
                r.entradasLiquidas += l.valorBruto - l.irpf - l.inss;

        const auto gastos = GastoAvulsoRepository().findAll();
        for (const auto &g : gastos)
            if (g.ano == ano)
                r.gastos += g.valor;
    }

    r.saldo = r.entradasLiquidas - r.gastos;

    // ── Proventos do ano ───────────────────────────────────────────────────────
    {
        ProventoRepository repoProv;
        r.proventosLiquidos = repoProv.totalLiquidoAno(ano);
        r.cntProventos = repoProv.findByPeriod(QDate(ano, 1, 1),
                                               QDate(ano, 12, 31)).size();
    }

    return r;
}

QVector<ResumoMensal> ResumoService::serie(const QDate &ini, const QDate &fim) const
{
    QVector<ResumoMensal> out;
    if (!ini.isValid() || !fim.isValid() || ini > fim)
        return out;

    // Itera mês a mês de forma inclusiva. Normaliza para o dia 1 para que
    // addMonths não pule meses (ex.: 31/jan + 1 mês).
    QDate cur(ini.year(), ini.month(), 1);
    const QDate last(fim.year(), fim.month(), 1);
    while (cur <= last) {
        out.append(resumoDoMes(cur.month(), cur.year()));
        cur = cur.addMonths(1);
    }
    return out;
}
