#pragma once

#include <QtTypes>

// Resumo agregado de um único mês (camada Mid).
// Todos os campos derivados são PRÉ-CALCULADOS por ResumoService — sem getters.
// Valores monetários em centavos (qint64). Convenção financeira do projeto.
struct ResumoMensal {
    int    mes = 0;   // 1–12
    int    ano = 0;

    // ── Entradas ───────────────────────────────────────────────────────────────
    qint64 entradasLiquidas = 0;  // Σ(valorBruto − irpf − inss)
    qint64 entradasBrutas   = 0;  // Σ valorBruto
    qint64 irpfRetido       = 0;  // Σ irpf
    qint64 inssRetido       = 0;  // Σ inss

    // ── Saídas ─────────────────────────────────────────────────────────────────
    qint64 gastos   = 0;          // Σ gastos avulsos do mês
    qint64 parcelas = 0;          // Σ parcelas que vencem no mês

    // ── Investimentos (D0: rendimento/dividendo/JCP NÃO entram aqui) ────────────
    qint64 aportes          = 0;  // Σ valor de operações "aporte"
    qint64 resgates         = 0;  // Σ valor de operações "resgate"
    qint64 investimentosNet = 0;  // = resgates − aportes

    // ── Contagens ──────────────────────────────────────────────────────────────
    int cntEntradas = 0;
    int cntGastos   = 0;
    int cntParcelas = 0;

    // ── Saldos derivados ───────────────────────────────────────────────────────
    qint64 saldoMes     = 0;      // = entradasLiquidas − gastos
    qint64 saldoLiquido = 0;      // = entradasLiquidas − gastos − parcelas
};
