#pragma once

#include <QList>

// DripSimulator (#63) - Simulador de Rentabilidade e Dividendos.
//
// Stateless puro. DRIP (Dividend Reinvestment Plan): reinveste
// dividendos automaticamente comprando mais acoes ao preco do mes.
// Tambem oferece comparativo com benchmarks CDI/IPCA/IBOV usando
// taxas anuais informadas (no futuro plugavel ao BrapiProvider #58).

class DripSimulator
{
public:
    struct Input {
        double precoInicial      = 0.0;  // preco de compra (1a aquisicao)
        double quantidadeInicial = 0.0;  // qty comprada inicialmente
        double aporteMensal      = 0.0;  // aporte adicional fixo R$/mes
        double dividendYieldAnual = 0.0; // ex: 0.06 = 6% a.a.
        double valorizacaoAnual  = 0.0;  // ex: 0.10 = 10% a.a. preco sobe
        int    meses             = 12;   // horizonte da simulacao
        bool   reinvestirDividendos = true; // true = DRIP; false = cash
    };

    struct Result {
        double quantidadeFinal    = 0.0;
        double precoFinal         = 0.0;
        double patrimonioFinal    = 0.0;
        double dividendosTotais   = 0.0;  // pagos (cash) ou reinvestidos
        double aportesTotais      = 0.0;  // somatorio aporteMensal*meses
        double yieldOnCostFinal   = 0.0;  // div anual / patrimonio inicial
        double cagrPatrimonial    = 0.0;  // (FV/PV)^(12/n) - 1
        QList<double> serieMensal;        // patrimonio mes a mes (para chart)
    };

    // Simula DRIP mes-a-mes.
    //   precoMes      = precoInicial * (1 + valorizacaoAnual)^(i/12)
    //   dividendoMes  = patrimonio * dividendYieldAnual / 12
    //   if reinvestir: quantidade += dividendoMes / precoMes
    //   if aporteMensal > 0: quantidade += aporteMensal / precoMes
    //   patrimonio    = quantidade * precoMes
    [[nodiscard]] static Result simular(const Input &in);

    // Comparativo CDI/IPCA/IBOV com aportes mensais fixos.
    // Cada cenario usa taxa anual informada (manual; futuramente integra #58).
    struct ComparativoInput {
        double aporteMensal     = 0.0;
        int    meses            = 12;
        double taxaCdiAnual     = 0.0;  // ex: 0.13
        double taxaIpcaAnual    = 0.0;  // ex: 0.045
        double retornoIbovAnual = 0.0;  // ex: 0.09
    };
    struct ComparativoResult {
        double cdi  = 0.0;
        double ipca = 0.0;
        double ibov = 0.0;
    };
    // FV de anuidade mensal: PMT * ((1+r_mensal)^n - 1) / r_mensal
    // r_mensal derivado de (1+anual)^(1/12) - 1.
    [[nodiscard]] static ComparativoResult comparativo(const ComparativoInput &in);
};
