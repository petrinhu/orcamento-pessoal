#pragma once

#include <QtTypes>

// Resumo agregado de um ano inteiro (camada Mid).
// Campos derivados PRÉ-CALCULADOS por ResumoService. Centavos (qint64).
struct ResumoAnual {
    int    ano = 0;

    qint64 entradasLiquidas = 0;  // Σ(valorBruto − irpf − inss) no ano
    qint64 gastos           = 0;  // Σ gastos avulsos no ano
    qint64 saldo            = 0;  // = entradasLiquidas − gastos

    qint64 proventosLiquidos = 0; // total líquido de proventos no ano
    int    cntProventos      = 0; // nº de eventos de provento no ano
};
