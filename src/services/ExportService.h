#pragma once

#include "models/Compra.h"
#include "models/Parcela.h"
#include "models/Cartao.h"
#include "models/LancamentoEntrada.h"
#include "models/GastoAvulso.h"
#include "models/LancamentoInvestimento.h"
#include "models/ItemOrcamento.h"
#include "models/TransacaoRecorrente.h"
#include "models/Divida.h"
#include "models/Meta.h"
#include "models/Lembrete.h"

#include <QList>
#include <QString>
#include <QStringList>

// Exports the current dataset to various file formats.
// All methods are pure (no DB access) — caller passes snapshot data.
class ExportService {
public:
    // ── Cartões dataset (legacy, unchanged) ──────────────────────────────────
    struct Dataset {
        QList<Compra>         compras;
        QList<QList<Parcela>> parcelas; // index matches compras
        QList<Cartao>         cartoes;
    };

    // ── Shared table representation ──────────────────────────────────────────
    struct TableData {
        QStringList        headers;
        QList<QStringList> rows;    // last row = "TOTAL" where applicable
    };

    // ── Named section for multi-section export ───────────────────────────────
    struct Section {
        QString   name;
        TableData data;
    };

    // ── Financial transaction for OFX/QIF export ────────────────────────────
    struct OFXTx {
        QString dtPosted;   // yyyyMMdd
        qint64  valor;      // centavos, signed (positive=credit, negative=debit)
        QString fitid;      // unique transaction ID
        QString memo;
    };

    // ── Per-section table builders (pure, no DB) ─────────────────────────────
    static TableData buildTable             (const Dataset &d);
    static TableData buildTableEntradas     (const QList<LancamentoEntrada> &);
    static TableData buildTableGastos       (const QList<GastoAvulso> &);
    static TableData buildTableInvestimentos(const QList<LancamentoInvestimento> &);
    static TableData buildTablePlanejamento (const QList<ItemOrcamento> &);
    static TableData buildTableRecorrencias (const QList<TransacaoRecorrente> &);
    static TableData buildTableDividas      (const QList<Divida> &);
    static TableData buildTableMetas        (const QList<Meta> &);
    static TableData buildTableLembretes    (const QList<Lembrete> &);

    // ── OFX transaction builders (pure, no DB) ───────────────────────────────
    static QList<OFXTx> ofxFromEntradas(const QList<LancamentoEntrada> &);
    static QList<OFXTx> ofxFromGastos  (const QList<GastoAvulso> &);
    static QList<OFXTx> ofxFromDataset (const Dataset &);

    // ── Single-section (legacy, Cartões only) ────────────────────────────────
    static bool toCsv     (const QString &path, const Dataset &d);
    static bool toPdf     (const QString &path, const Dataset &d);
    static bool toXlsx    (const QString &path, const Dataset &d);
    static bool toOfx     (const QString &path, const Dataset &d);
    static bool toQif     (const QString &path, const Dataset &d);
    static bool toJson    (const QString &path, const Dataset &d);
    static bool toMarkdown(const QString &path, const Dataset &d);
    static bool toHtml    (const QString &path, const Dataset &d);
    static bool toTxt     (const QString &path, const Dataset &d);
    static bool toOds     (const QString &path, const Dataset &d);

    // ── Multi-section export ──────────────────────────────────────────────────
    static bool toMultiCsv     (const QString &path, const QList<Section> &);
    static bool toMultiXlsx    (const QString &path, const QList<Section> &);
    static bool toMultiJson    (const QString &path, const QList<Section> &);
    static bool toMultiMarkdown(const QString &path, const QList<Section> &);
    static bool toMultiHtml    (const QString &path, const QList<Section> &);
    static bool toMultiPdf     (const QString &path, const QList<Section> &);
    static bool toMultiTxt     (const QString &path, const QList<Section> &);
    static bool toMultiOfx     (const QString &path, const QList<OFXTx> &);
    static bool toMultiQif     (const QString &path, const QList<OFXTx> &);

private:
    static QString buildHtmlStr     (const Dataset &d, bool standalone = true);
    static QString buildHtmlMultiStr(const QList<Section> &, bool standalone = true);
    static QString cartaoNome(const Dataset &d, int cartaoId);
    static QString fmtBrl(qint64 centavos);
    static QString fmtPct(int pct10000);   // % × 10000 → "1,50"
};
