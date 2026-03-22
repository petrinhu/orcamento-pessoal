#pragma once

#include "models/Compra.h"
#include "models/Parcela.h"
#include "models/Cartao.h"

#include <QList>
#include <QString>
#include <QStringList>

// Exports the current dataset to various file formats.
// All methods are pure (no DB access) — caller passes snapshot data.
class ExportService {
public:
    struct Dataset {
        QList<Compra>         compras;
        QList<QList<Parcela>> parcelas; // index matches compras
        QList<Cartao>         cartoes;
    };

    // ── Implemented ─────────────────────────────────────────────────────────
    static bool toCsv     (const QString &path, const Dataset &d); // BR: ; decimal ,
    static bool toPdf     (const QString &path, const Dataset &d); // QPdfWriter
    static bool toXlsx    (const QString &path, const Dataset &d); // QXlsx
    static bool toOfx     (const QString &path, const Dataset &d); // OFX 2.0 XML
    static bool toQif     (const QString &path, const Dataset &d); // Quicken QIF
    static bool toJson    (const QString &path, const Dataset &d); // QJsonDocument
    static bool toMarkdown(const QString &path, const Dataset &d); // GFM table
    static bool toHtml    (const QString &path, const Dataset &d); // HTML table
    static bool toTxt     (const QString &path, const Dataset &d); // fixed-width ASCII
    static bool toOds     (const QString &path, const Dataset &d); // stub

    // ── Shared helpers (used by MainWindow to build table for display) ───────
    struct TableData {
        QStringList        headers;
        QList<QStringList> rows;    // last row = "TOTAL"
    };
    static TableData buildTable(const Dataset &d);

private:
    static QString buildHtmlStr(const Dataset &d, bool standalone = true);
    static QString cartaoNome(const Dataset &d, int cartaoId);
    static QString fmtBrl(qint64 centavos);
};
