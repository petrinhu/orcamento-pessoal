#include "services/ResumoService.h"

#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoEntradaRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "repositories/ParcelaRepository.h"
#include "repositories/ProventoRepository.h"
#include "services/CompraService.h"
#include "services/EntradaService.h"
#include "services/ParcelaService.h"

#include <QDate>
#include <QHash>

// Lógica extraída de DashboardWidget::atualizarDados() (#115). A correção D0
// (rendimento/dividendo/JCP fora de aportes/resgates) é aplicada aqui.

namespace {

// Parcela já com a data de vencimento resolvida (ano/mês), calculada via
// ParcelaService::dataParaParcela. Junta compras (1 query) + parcelas
// (1 query) em memória, via QHash<compraId, Compra> — evita o N+1 de
// CompraService::parcelasDeCompra(c.id) por compra (#195/PERF-03: antes
// custava 1 + C queries; agora custa 2, independente de C).
struct ParcelaComData {
    int    ano   = 0;
    int    mes   = 0;
    qint64 valor = 0;
};

QList<ParcelaComData> parcelasComData()
{
    const auto compras = CompraService().compras();
    QHash<int, Compra> compraPorId;
    compraPorId.reserve(compras.size());
    for (const auto &c : compras)
        compraPorId.insert(c.id, c);

    const auto parcelas = ParcelaRepository().findAll();
    QList<ParcelaComData> out;
    out.reserve(parcelas.size());
    for (const auto &p : parcelas) {
        const auto it = compraPorId.constFind(p.compraId);
        if (it == compraPorId.constEnd())
            continue; // parcela órfã (compra removida); ON DELETE CASCADE previne isso na prática
        const QDate d = ParcelaService::dataParaParcela(it.value(), p.numero);
        out.append({d.year(), d.month(), p.valor});
    }
    return out;
}

// Agrega uma lista JÁ FILTRADA/pré-selecionada para (mes,ano) num ResumoMensal.
// Usado tanto por resumoDoMes (listas filtradas via SQL/hash lookup para 1
// mês) quanto por serie() (sub-listas agrupadas em memória) — mesma lógica de
// soma em ambos os caminhos, garantindo paridade numérica entre os dois.
ResumoMensal agregar(int mes, int ano,
                      const QList<LancamentoEntrada> &entradas,
                      const QList<GastoAvulso> &gastos,
                      const QList<ParcelaComData> &parcelas,
                      const QList<LancamentoInvestimento> &invs)
{
    ResumoMensal r;
    r.mes = mes;
    r.ano = ano;

    for (const auto &l : entradas) {
        r.entradasBrutas   += l.valorBruto;
        r.irpfRetido       += l.irpf;
        r.inssRetido       += l.inss;
        r.entradasLiquidas += l.valorBruto - l.irpf - l.inss;
        ++r.cntEntradas;
    }

    for (const auto &g : gastos) {
        r.gastos += g.valor;
        ++r.cntGastos;
    }

    for (const auto &p : parcelas) {
        r.parcelas += p.valor;
        ++r.cntParcelas;
    }

    // Correção D0: apenas "aporte" e "resgate" entram no fluxo de caixa de
    // investimentos. "rendimento"/"dividendo"/JCP vivem só no card Proventos
    // — evita dupla contagem. Sinal de investimentosNet: resgates − aportes
    // (entrada de caixa positiva), coerente com valorSignadoInv() do model.
    for (const auto &inv : invs) {
        if (inv.tipoOperacao == QStringLiteral("aporte"))
            r.aportes += inv.valor;
        else if (inv.tipoOperacao == QStringLiteral("resgate"))
            r.resgates += inv.valor;
        // rendimento/dividendo: ignorados aqui (D0).
    }
    r.investimentosNet = r.resgates - r.aportes;

    r.saldoMes     = r.entradasLiquidas - r.gastos;
    r.saldoLiquido = r.entradasLiquidas - r.gastos - r.parcelas;

    return r;
}

} // namespace

ResumoMensal ResumoService::resumoDoMes(int mes, int ano) const
{
    // Cada busca já filtrada por (mes,ano) no SQL (exceto parcelas, cuja data
    // de vencimento é derivada — precisa do join compra+parcela em memória).
    const auto entradas = LancamentoEntradaRepository().findByMesAno(mes, ano);
    const auto gastos   = GastoAvulsoRepository().findByMesAno(mes, ano);
    const auto invs     = LancamentoInvestimentoRepository().findByMesAno(mes, ano);

    QList<ParcelaComData> parcelasDoMes;
    for (const auto &p : parcelasComData())
        if (p.ano == ano && p.mes == mes)
            parcelasDoMes.append(p);

    return agregar(mes, ano, entradas, gastos, parcelasDoMes, invs);
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

    // #195/PERF-03: busca cada fonte 1× fora do laço de meses (5 queries
    // fixas, independentes de M meses e C compras) e agrupa em memória por
    // chave (ano*100+mes). Antes: resumoDoMes(mes,ano) reconsultava tudo a
    // cada iteração — custo M×(4+C) (M=6,C=50 → 324 queries); agora M×O(1)
    // lookup sobre dados já em RAM.
    const auto todasEntradas = LancamentoEntradaRepository().findAll();
    const auto todosGastos   = GastoAvulsoRepository().findAll();
    const auto todasParcelas = parcelasComData();
    const auto todosInvs     = LancamentoInvestimentoRepository().findAll();

    auto chave = [](int ano, int mes) { return ano * 100 + mes; };

    QHash<int, QList<LancamentoEntrada>> entradasPorMes;
    for (const auto &l : todasEntradas)
        entradasPorMes[chave(l.ano, l.mes)].append(l);

    QHash<int, QList<GastoAvulso>> gastosPorMes;
    for (const auto &g : todosGastos)
        gastosPorMes[chave(g.ano, g.mes)].append(g);

    QHash<int, QList<ParcelaComData>> parcelasPorMes;
    for (const auto &p : todasParcelas)
        parcelasPorMes[chave(p.ano, p.mes)].append(p);

    QHash<int, QList<LancamentoInvestimento>> invsPorMes;
    for (const auto &inv : todosInvs)
        invsPorMes[chave(inv.ano, inv.mes)].append(inv);

    while (cur <= last) {
        const int k = chave(cur.year(), cur.month());
        out.append(agregar(cur.month(), cur.year(),
                            entradasPorMes.value(k),
                            gastosPorMes.value(k),
                            parcelasPorMes.value(k),
                            invsPorMes.value(k)));
        cur = cur.addMonths(1);
    }
    return out;
}
