#pragma once

#include "models/Compra.h"
#include "models/Parcela.h"

#include <QList>
#include <QDate>

struct ResultadoFinanceiro {
    QList<Parcela> parcelas;
    double         jurosMensaisPct  = 0.0; // % ao mês
    double         jurosAnuaisPct   = 0.0; // CET (% ao ano)
    qint64         jurosTotais      = 0;   // centavos
    qint64         totalPago        = 0;   // centavos (N × parcela)
};

// ── Financial detail panel data ───────────────────────────────────────────────

enum class SolveFor { Parcela, Total, Taxa, NumParcelas };

struct DadosFinanceiros {
    qint64 valorTotal      = 0;
    int    numParcelas     = 0;
    double jurosMensaisPct = 0.0;  // % ao mês  (ex: 1.5 = 1,5%)
    double cetPct          = 0.0;  // % ao ano
    qint64 valorParcela    = 0;    // PMT em centavos
    qint64 jurosTotais     = 0;    // centavos
};

// ── Amortization row ──────────────────────────────────────────────────────────

struct AmortizacaoRow {
    int    numero       = 0;
    QDate  data;
    qint64 saldoDevedor = 0;  // before this payment
    qint64 juros        = 0;
    qint64 amortizacao  = 0;
    qint64 parcela      = 0;  // = juros + amortizacao
};

// All financial calculations: pure logic, no DB access.
// Uses qint64 (centavos) throughout; floating-point only for rate solving (Newton-Raphson).
class ParcelaService {
public:
    // ── Generate parcelas ────────────────────────────────────────────────────

    // Mode 2.1/2.3: fixed value per parcela, no interest
    static ResultadoFinanceiro semJuros(const Compra &c);

    // Mode 2.4: given monthly rate (%), generate Price-table (SAC-like) parcelas
    static ResultadoFinanceiro comJurosMensais(const Compra &c, double jurosMensaisPct);

    // Mode 2.5: given valor_total, num_parcelas and valor_parcela → infer rate
    static ResultadoFinanceiro inferirJuros(qint64 valorTotal, int numParcelas,
                                            qint64 valorParcela);

    // ── Date helpers ─────────────────────────────────────────────────────────

    // Returns the QDate for parcela number `numero` given the compra's start point.
    // Handles month rollover / year wrap correctly.
    static QDate dataParaParcela(const Compra &c, int numero);

    // Generate N monthly dates starting from (mes, ano) for parcelaInicial.
    // #199 (QA-04): tornado publico p/ teste direto (era private, so testado
    // indiretamente via recalcularAposEdicao/tabelaAmortizacao).
    static QList<QDate> gerarDatas(const Compra &c);

    // ── Re-calculation after manual cell edit ────────────────────────────────

    enum class RecalcMode {
        SoEsta,          // change only this parcela
        RedistribuirDiff,// spread difference across remaining parcelas
        RecalcJuros,     // recompute interest from new values
        IgualarTodas     // set ALL parcelas to this value, recalculate valorTotal
    };

    static QList<Parcela> recalcularAposEdicao(
        const QList<Parcela> &parcelas,
        int  indiceParcela,
        qint64 novoValor,
        RecalcMode modo,
        qint64 valorTotal = 0
    );

    // ── Detail panel helpers ─────────────────────────────────────────────────

    // Build DadosFinanceiros from a Compra (computes PMT, CET, jurosTotais)
    static DadosFinanceiros      dadosFromCompra(const Compra &c);

    // Write back editable fields from DadosFinanceiros into a Compra
    static void                  aplicarDadosACompra(Compra &c, const DadosFinanceiros &d);

    // Recalculate the field indicated by solveFor; returns updated DadosFinanceiros
    static DadosFinanceiros      recalcular(const DadosFinanceiros &d, SolveFor solveFor);

    // Price (SAF) amortization schedule for a Compra
    static QList<AmortizacaoRow> tabelaAmortizacao(const Compra &c);

    // CET ↔ taxa mensal conversions
    static double cetFromJurosMensais(double jurosMensaisPct);
    static double jurosMensaisFromCet(double cetPct);

    // ── Utility ──────────────────────────────────────────────────────────────

    // Format centavos as a DISPLAY money value, locale-aware by
    // AppSettings::language() (pt-BR "R$ 1.234,56" / en-US "$1,234.56"),
    // explicit ISO currency `moeda` (default "BRL") -- integer arithmetic
    // only (#204, gêmeo de uifmt::formatCentavosBRL/#197 I18N-01).
    static QString formatarValor(qint64 centavos, const QString &moeda = QStringLiteral("BRL"));

    // Parse "1.234,56" → centavos. #199 (QA-03): "só ponto" ("1.234") é
    // tratado como milhar quando casa com grupos de 3 dígitos (dinheiro
    // nunca tem 3 casas decimais) — ver uifmt::parseCentavos (gêmeo).
    static qint64 parsearValor(const QString &texto);

    // Currency symbol for ISO code
    static QString simboloMoeda(const QString &moeda);

private:
    // Newton-Raphson solver: finds monthly rate i for VP = PMT * [1-(1+i)^-n]/i
    static double resolverTaxa(qint64 vp, qint64 pmt, int n);
};
