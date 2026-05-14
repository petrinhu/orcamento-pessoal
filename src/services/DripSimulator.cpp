#include "services/DripSimulator.h"

#include <cmath>

DripSimulator::Result DripSimulator::simular(const Input &in)
{
    Result r;
    if (in.meses <= 0 || in.precoInicial <= 0.0) {
        r.precoFinal = in.precoInicial;
        r.quantidadeFinal = in.quantidadeInicial;
        r.patrimonioFinal = in.precoInicial * in.quantidadeInicial;
        return r;
    }

    double qty       = in.quantidadeInicial;
    const double pv  = in.precoInicial * in.quantidadeInicial; // base p/ YOC
    double patrimonio = pv;

    r.serieMensal.reserve(in.meses);

    for (int i = 1; i <= in.meses; ++i) {
        const double precoMes = in.precoInicial
                              * std::pow(1.0 + in.valorizacaoAnual, double(i) / 12.0);

        // patrimonio "antes" do reinvest/aporte deste mes -> base do dividendo
        const double patrimonioBase = qty * precoMes;
        const double dividendoMes   = patrimonioBase * (in.dividendYieldAnual / 12.0);
        r.dividendosTotais += dividendoMes;

        if (in.reinvestirDividendos && precoMes > 0.0) {
            qty += dividendoMes / precoMes;
        }
        if (in.aporteMensal > 0.0 && precoMes > 0.0) {
            qty += in.aporteMensal / precoMes;
            r.aportesTotais += in.aporteMensal;
        }

        patrimonio = qty * precoMes;
        r.serieMensal.append(patrimonio);
    }

    r.quantidadeFinal = qty;
    r.precoFinal      = in.precoInicial
                      * std::pow(1.0 + in.valorizacaoAnual, double(in.meses) / 12.0);
    r.patrimonioFinal = patrimonio;

    // Yield on cost final = (dividendo anual atual estimado) / patrimonio inicial
    // dividendo anual proximo = patrimonio_atual * dividendYieldAnual
    if (pv > 0.0) {
        r.yieldOnCostFinal = (patrimonio * in.dividendYieldAnual) / pv;
    }

    // CAGR patrimonial: (FV/PV)^(12/meses) - 1 ; soh faz sentido se pv>0
    if (pv > 0.0 && patrimonio > 0.0) {
        r.cagrPatrimonial = std::pow(patrimonio / pv, 12.0 / double(in.meses)) - 1.0;
    }

    return r;
}

DripSimulator::ComparativoResult DripSimulator::comparativo(const ComparativoInput &in)
{
    ComparativoResult r;
    if (in.meses <= 0 || in.aporteMensal <= 0.0) {
        return r;
    }

    const int n = in.meses;
    auto fvAnuidade = [&](double taxaAnual) -> double {
        if (taxaAnual <= 0.0) {
            // sem juros: apenas aportes cumulativos
            return in.aporteMensal * double(n);
        }
        const double rMensal = std::pow(1.0 + taxaAnual, 1.0 / 12.0) - 1.0;
        if (rMensal == 0.0) return in.aporteMensal * double(n);
        return in.aporteMensal * (std::pow(1.0 + rMensal, double(n)) - 1.0) / rMensal;
    };

    r.cdi  = fvAnuidade(in.taxaCdiAnual);
    r.ipca = fvAnuidade(in.taxaIpcaAnual);
    r.ibov = fvAnuidade(in.retornoIbovAnual);
    return r;
}
