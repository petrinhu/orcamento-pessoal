#pragma once

// FinanceEngine (#81) - motor financeiro centralizado.
//
// Funcoes estaticas para os 5 TVM basicos (PMT/PV/FV/NPER/RATE).
//
// Convencao Excel / HP-12C:
//   - Cashflow positivo  = entrada de dinheiro para o sujeito.
//   - Cashflow negativo  = saida.
//   - i = fracao decimal por periodo (0.008 = 0.8% por periodo).
//   - n = numero de periodos.
//   - begin = true: pagamento no inicio do periodo; false: no fim (padrao).
//
// Equacao base (Excel/HP-12C):
//   pv*(1+i)^n + pmt*(1+i*begin)*((1+i)^n - 1)/i + fv = 0
//
// Casos especiais:
//   - i = 0: equacao degenerada; tratamento separado em cada funcao.
//   - n <= 0 ou nao-inteiro: tolerado em rate()/nper() mas com convencoes
//     documentadas no cpp.

class FinanceEngine
{
public:
    // PMT: parcela periodica. Dado pv, fv, i, n -> retorna pmt.
    [[nodiscard]] static double pmt(double pv, double fv, double i, int n,
                                    bool begin = false);

    // PV: valor presente. Dado pmt, fv, i, n -> retorna pv.
    [[nodiscard]] static double pv(double pmt, double fv, double i, int n,
                                   bool begin = false);

    // FV: valor futuro. Dado pv, pmt, i, n -> retorna fv.
    [[nodiscard]] static double fv(double pv, double pmt, double i, int n,
                                   bool begin = false);

    // NPER: numero de periodos. Dado pv, pmt, fv, i -> retorna n (double).
    // Pode nao ser inteiro - usuario arredonda para o caso de uso.
    [[nodiscard]] static double nper(double pv, double pmt, double fv, double i,
                                     bool begin = false);

    // RATE: taxa por periodo. Newton-Raphson. Dado pv, pmt, fv, n -> retorna i.
    // Algoritmo: gnucash/libgnucash/app-utils/calculation/fin.c:1304-1366.
    [[nodiscard]] static double rate(double pv, double pmt, double fv, int n,
                                     bool begin = false);

    // ── Conversao de taxa entre periodicidades (#82) ─────────────────────────
    //
    // periodosDe / periodosPara: numero de periodos por unidade comum (ano).
    // Ex.: anual->mensal usa de=1, para=12. Mensal->anual usa de=12, para=1.

    // Actuariel (regime composto): preserva equivalencia financeira.
    // Formula: (1 + i)^(periodosDe / periodosPara) - 1
    // Ex.: 12% a.a. -> a.m. = (1.12)^(1/12) - 1 = 0.9489%
    // Padrao BR para: Tesouro Direto, financiamento Price/SAC, CDB.
    [[nodiscard]] static double taxaEquivalente(double i, int periodosDe,
                                                int periodosPara);

    // Proporcional (regime linear/simples): divisao direta.
    // Formula: i * (periodosDe / periodosPara)
    // Ex.: 12% a.a. -> a.m. = 12% / 12 = 1%
    // Padrao BR para: cartao de credito, juros de mora, multas.
    [[nodiscard]] static double taxaProporcional(double i, int periodosDe,
                                                 int periodosPara);

    // ── Capitalizacao continua (#81 follow-up) ───────────────────────────────
    //
    // Regime continuo: limite de cap composta com periodos -> infinito.
    // Util para opcoes (Black-Scholes), forex offshore, modelos teoricos.

    // FV continuo: fv = pv * e^(i*t), t em unidades da taxa (anos).
    [[nodiscard]] static double fvContinuo(double pv, double iAnual,
                                            double anos);

    // PV continuo: pv = fv * e^(-i*t).
    [[nodiscard]] static double pvContinuo(double fv, double iAnual,
                                            double anos);

    // Taxa equivalente discreta -> continua. (1+i)^n - 1 = e^(r*n) - 1
    //   r_cont = n * ln(1 + i / n)
    // i: taxa por periodo discreto; nPeriodos: periodos por ano.
    [[nodiscard]] static double taxaContinuaDeDiscreta(double i, int nPeriodos);

    // Taxa contínua anual -> discreta com nPeriodos por ano.
    //   i_discreta = e^(r / nPeriodos) - 1
    [[nodiscard]] static double taxaDiscretaDeContinua(double rAnual,
                                                       int nPeriodos);

    // ── Multiplas frequencias capitalizacao (#81 follow-up) ──────────────────
    //
    // Modela cenario em que capitalizacao (rendimento) ocorre em frequencia
    // diferente da de pagamento. Exemplo: emprestimo HP-12C com cap semestral
    // e pgto mensal.
    //
    // pmtMultiFreq calcula PMT efetivo dado:
    //   pv: principal
    //   iCapital: taxa por periodo de capitalizacao
    //   nCap: numero de periodos de capitalizacao por ano
    //   nPgto: numero de pagamentos por ano
    //   anos: prazo total em anos
    //
    // Converte iCapital para taxa equivalente no periodo de pagamento, depois
    // delega para pmt() padrao.
    [[nodiscard]] static double pmtMultiFreq(double pv, double iCapital,
                                              int nCap, int nPgto, double anos);
};
