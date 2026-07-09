#pragma once

#include <QtTypes>

// FiscalService (#61 #62) - regras fiscais BR stateless, usadas por wizards
// de Reserva de Emergencia (comparativo de aplicacoes liquidas de IR) e
// CLT vs PJ (folha, INSS, IRPF, Simples Nacional anexos I-V, Lucro Presumido).
//
// Convencao: todos os retornos sao em centavos quando indicado, ou fracao
// decimal (0.12 = 12%) para aliquotas. Tabelas refletem legislacao 2024:
//   - INSS escalonado: Lei 13.846/2019 + Portaria MTP 2/2024
//   - IRPF mensal: tabela vigente 2024 (Lei 14.663/2023)
//   - Simples Nacional: LC 123/2006 anexos I-V (faixas RBT12)
//   - Lucro Presumido: aliquotas efetivas tipicas servicos
//   - Poupanca: Lei 12.703/2012 (70% Selic se Selic > 8,5% a.a.)
//   - IR CDB/Tesouro: regressivo IN RFB 1.585/2015

class FiscalService
{
public:
    // ── Folha CLT ────────────────────────────────────────────────────────────

    // INSS mensal escalonado 2024 (teto R$ 908,85 sobre R$ 7.786,02).
    // brutoCentavos: salario bruto mensal em centavos.
    [[nodiscard]] static qint64 inssMensal2024(qint64 brutoCentavos);

    // IRPF mensal 2024 (sobre base = bruto - inss - deducoes opcionais).
    // baseCentavos: base de calculo em centavos.
    [[nodiscard]] static qint64 irpfMensal2024(qint64 baseCentavos);

    // ── Simples Nacional / Lucro Presumido ──────────────────────────────────

    // Aliquota efetiva do Simples Nacional anexo (I..V) para Receita Bruta
    // dos ultimos 12 meses (RBT12, em centavos). Retorna fracao decimal.
    // anexo: 1..5. Anexo IV trata desonerados separadamente.
    [[nodiscard]] static double simplesAliquotaEfetiva(int anexo,
                                                       qint64 rbt12Centavos);

    // Aliquota efetiva de PJ no Lucro Presumido para servicos (tipica):
    // PIS 0,65 + COFINS 3 + IR 4,8 + CSLL 2,88 + ISS 5 = 16,33%.
    [[nodiscard]] static double lucroPresumidoServicosAliquota();

    // ── Renda fixa: rendimento liquido mensal ───────────────────────────────

    // Taxa de juros mensal da Poupanca dado Selic anual (fracao).
    // Regra: Selic > 8,5% a.a. -> 0,5% a.m. + TR; senao 70% Selic + TR.
    // TR ignorada (tipicamente proxima de zero em 2024).
    [[nodiscard]] static double poupancaMensal(double selicAnual);

    // Aliquota de IR sobre rendimentos de CDB/Tesouro Selic por dias
    // mantidos (regressiva): >720 -> 15%, 361-720 -> 17,5%, 181-360 ->
    // 20%, ate 180 -> 22,5%.
    [[nodiscard]] static double irRendaFixaPorDias(int dias);

    // Aliquota de IR sobre rendimentos de CDB/Tesouro dado o PRAZO EM MESES
    // de calendario (ex.: 6, 12, 24 - o formato que o usuario tipicamente
    // informa). Converte para dias corridos reais com o fator medio
    // 365,25/12 (~30,4375 dias/mes) e delega a irRendaFixaPorDias.
    //
    // NAO usar 30 dias fixos por mes: prazos redondos de calendario tem
    // MAIS dias corridos que 30*meses (ex.: 6 meses = 182-183 dias, nao
    // 180), o que cruzaria a fronteira regressiva errada e cobraria IR a
    // mais (bug #190: wizard reimplementava a tabela em "meses" e chegava
    // a 22,5% aos 6 meses quando o correto, por dias, e 20%).
    [[nodiscard]] static double irRendaFixaPorMeses(double meses);

    // Aliquota de IOF regressivo sobre rendimentos para resgates < 30 dias.
    // dia 1 = 96%, dia 30 = 0%; lookup por dia mantido.
    [[nodiscard]] static double iofRendaFixaPorDias(int dias);
};
