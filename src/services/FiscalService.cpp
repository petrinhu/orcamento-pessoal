#include "FiscalService.h"

#include <QtTypes>
#include <algorithm>
#include <cmath>

// ── INSS mensal 2024 (Portaria MTP 2/2024) ───────────────────────────────────
//
// Faixas (R$):
//   ate 1.412,00          -> 7,5%
//   1.412,01 a 2.666,68   -> 9%
//   2.666,69 a 4.000,03   -> 12%
//   4.000,04 a 7.786,02   -> 14%
//   acima de 7.786,02     -> teto R$ 908,85
qint64 FiscalService::inssMensal2024(qint64 brutoCentavos)
{
    if (brutoCentavos <= 0) { return 0; }

    constexpr qint64 F1 = 141200;   // R$ 1.412,00
    constexpr qint64 F2 = 266668;   // R$ 2.666,68
    constexpr qint64 F3 = 400003;   // R$ 4.000,03
    constexpr qint64 F4 = 778602;   // R$ 7.786,02
    constexpr qint64 TETO = 90885;  // R$ 908,85

    const qint64 b = std::min<qint64>(brutoCentavos, F4);

    // Aritmetica inteira em centavos (convencao nº1: NUNCA double em
    // caminho monetario). Aliquotas expressas em permilagem (base 1000):
    //   7,5% -> 75 ; 9% -> 90 ; 12% -> 120 ; 14% -> 140.
    // Acumula o tributo de cada faixa em milesimos de centavo e arredonda
    // half-up uma unica vez no fim: (num + 500) / 1000.
    // Numerador maximo ~ F4*140 = 1,09e8, folga enorme em qint64.
    qint64 num = std::min<qint64>(b, F1) * 75;
    if (b > F1) { num += (std::min<qint64>(b, F2) - F1) * 90; }
    if (b > F2) { num += (std::min<qint64>(b, F3) - F2) * 120; }
    if (b > F3) { num += (std::min<qint64>(b, F4) - F3) * 140; }

    qint64 result = (num + 500) / 1000;
    if (brutoCentavos > F4) { result = TETO; }
    return std::min<qint64>(result, TETO);
}

// ── IRPF mensal 2024 ─────────────────────────────────────────────────────────
//
// Lei 14.663/2023, vigencia 02/2024:
//   ate 2.259,20            -> isento
//   2.259,21 a 2.826,65     -> 7,5%  - 169,44
//   2.826,66 a 3.751,05     -> 15%   - 381,44
//   3.751,06 a 4.664,68     -> 22,5% - 662,77
//   acima de 4.664,68       -> 27,5% - 896,00
qint64 FiscalService::irpfMensal2024(qint64 baseCentavos)
{
    if (baseCentavos <= 225920) { return 0; }

    // Aritmetica inteira em centavos (convencao nº1). Aliquota em
    // permilagem (base 1000) e parcela a deduzir (PD) ja em centavos
    // inteiros, multiplicada por 1000 para o mesmo denominador.
    //   numerador = base * aliqPermil - PD * 1000  (milesimos de centavo)
    //   resultado = (numerador + 500) / 1000        (half-up)
    // Numerador maximo p/ base R$1M ~ 2,75e11, folga enorme em qint64.
    qint64 num = 0;
    if      (baseCentavos > 466468) { num = (baseCentavos * 275) - (89600 * 1000); }
    else if (baseCentavos > 375105) { num = (baseCentavos * 225) - (66277 * 1000); }
    else if (baseCentavos > 282665) { num = (baseCentavos * 150) - (38144 * 1000); }
    else                            { num = (baseCentavos * 75)  - (16944 * 1000); }

    if (num <= 0) { return 0; }
    return (num + 500) / 1000;
}

// ── Simples Nacional - tabelas LC 123/2006 com Lei Complementar 155/2016 ────
//
// Aliquota efetiva = (RBT12 * aliquotaNominal - PD) / RBT12
//
// Anexo I  - comercio
// Anexo II - industria
// Anexo III - servicos em geral (escritorios, fisioterapia, agencias etc)
// Anexo IV - servicos com tributacao apartada (construcao, advocacia,
//             servicos de vigilancia, limpeza)
// Anexo V  - servicos intelectuais (TI, medicina, engenharia, jornalismo etc;
//             pode migrar para Anexo III via Fator R >= 28%)
double FiscalService::simplesAliquotaEfetiva(int anexo, qint64 rbt12Centavos)
{
    if (rbt12Centavos <= 0) { return 0.0; }
    const qint64 rbt12 = rbt12Centavos;

    // Caminho monetario em inteiros (convencao nº1): aliquota nominal em
    // permilagem (base 1000; nominais tem ate 1 casa de %, ex 11,2% -> 112)
    // e PD em centavos inteiros. A divisao final produz a ALIQUOTA EFETIVA,
    // que e uma fracao adimensional (0..1) consumida como multiplicador
    // pelos chamadores -> por isso o retorno permanece double (nao e
    // valor monetario em centavos).
    struct Faixa { qint64 limite; qint64 aliqPermil; qint64 pd; };

    // Tabelas LC 123 atualizadas (2018+):
    static const Faixa anexoI[] = {
        { 18000000,   40,         0 },
        { 36000000,   73,    597600 },
        { 72000000,   95,   1390800 },
        {192000000,  107,   2236800 },
        {360000000,  143,  13986800 },
        {480000000,  190,  37856800 },
    };
    static const Faixa anexoII[] = {
        { 18000000,   45,         0 },
        { 36000000,   78,    597600 },
        { 72000000,  100,   1390800 },
        {192000000,  112,   2236800 },
        {360000000,  147,  13986800 },
        {480000000,  300,  72000000 },
    };
    static const Faixa anexoIII[] = {
        { 18000000,   60,         0 },
        { 36000000,  112,    936000 },
        { 72000000,  135,   1764000 },
        {192000000,  160,   3564000 },
        {360000000,  210,  12564000 },
        {480000000,  330,  54000000 },
    };
    static const Faixa anexoIV[] = {
        { 18000000,   45,         0 },
        { 36000000,   90,    810000 },
        { 72000000,  102,   1242000 },
        {192000000,  140,   3978000 },
        {360000000,  220,  18378000 },
        {480000000,  330,  72780000 },
    };
    static const Faixa anexoV[] = {
        { 18000000,  155,         0 },
        { 36000000,  180,    450000 },
        { 72000000,  195,    990000 },
        {192000000,  205,   1719000 },
        {360000000,  230,   4860000 },
        {480000000,  305,  31530000 },
    };

    const Faixa *tab = nullptr;
    int len = 6;
    switch (anexo) {
    case 1: tab = anexoI;   break;
    case 2: tab = anexoII;  break;
    case 3: tab = anexoIII; break;
    case 4: tab = anexoIV;  break;
    case 5: tab = anexoV;   break;
    default: return 0.0;
    }

    for (int i = 0; i < len; ++i) {
        if (rbt12 <= tab[i].limite) {
            // tributo (centavos*1000) = rbt12*aliqPermil - pd*1000
            const qint64 num = (rbt12 * tab[i].aliqPermil) - (tab[i].pd * 1000);
            if (num <= 0) { return 0.0; }
            return static_cast<double>(num)
                 / (static_cast<double>(rbt12) * 1000.0);
        }
    }
    // Acima do teto: nao se enquadra mais no Simples.
    return static_cast<double>(tab[len - 1].aliqPermil) / 1000.0;
}

double FiscalService::lucroPresumidoServicosAliquota()
{
    // PIS 0,65 + COFINS 3 + IRPJ 4,8 + CSLL 2,88 + ISS 5 = 16,33%
    return 0.1633;
}

// ── Renda fixa ───────────────────────────────────────────────────────────────

double FiscalService::poupancaMensal(double selicAnual)
{
    if (selicAnual > 0.085) {
        return 0.005; // 0,5% a.m. + TR (TR ignorada, ~0 em 2024)
    }
    // 70% Selic a.a., convertido para mensal composto
    const double anualEfetiva = 0.70 * selicAnual;
    return std::pow(1.0 + anualEfetiva, 1.0 / 12.0) - 1.0;
}

double FiscalService::irRendaFixaPorDias(int dias)
{
    if (dias <= 180)  return 0.225;
    if (dias <= 360)  return 0.200;
    if (dias <= 720)  return 0.175;
    return 0.150;
}

double FiscalService::iofRendaFixaPorDias(int dias)
{
    if (dias >= 30) return 0.0;
    if (dias <= 0)  return 0.96;
    // Tabela regressiva: cada dia reduz ~3,33 pontos percentuais
    static const double iof[30] = {
        0.96, 0.93, 0.90, 0.86, 0.83, 0.80, 0.76, 0.73, 0.70, 0.66,
        0.63, 0.60, 0.56, 0.53, 0.50, 0.46, 0.43, 0.40, 0.36, 0.33,
        0.30, 0.26, 0.23, 0.20, 0.16, 0.13, 0.10, 0.06, 0.03, 0.00,
    };
    return iof[dias - 1];
}
