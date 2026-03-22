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

    // ── Utility ──────────────────────────────────────────────────────────────

    // Format centavos as "R$ 1.234,56" (or other currency symbol)
    static QString formatarValor(qint64 centavos, const QString &moeda = QStringLiteral("BRL"));

    // Parse "1.234,56" → centavos
    static qint64 parsearValor(const QString &texto);

    // Currency symbol for ISO code
    static QString simboloMoeda(const QString &moeda);

private:
    // Newton-Raphson solver: finds monthly rate i for VP = PMT * [1-(1+i)^-n]/i
    static double resolverTaxa(qint64 vp, qint64 pmt, int n);

    // Generate N monthly dates starting from (mes, ano) for parcelaInicial
    static QList<QDate> gerarDatas(const Compra &c);
};
