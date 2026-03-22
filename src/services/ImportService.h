#pragma once

#include "models/Compra.h"
#include "models/Parcela.h"

#include <QDate>
#include <QList>
#include <QString>

// Parses external file formats into Compra/Parcela lists.
// Each method creates one Compra (numParcelas=1) per transaction.
// The caller inserts results into the DB via repositories.
class ImportService {
public:
    struct Result {
        QList<Compra>         compras;
        QList<QList<Parcela>> parcelas; // index matches compras
        QString               error;
        bool isValid() const { return error.isEmpty(); }
    };

    // ── Implemented ─────────────────────────────────────────────────────────
    static Result fromCsv (const QString &path); // auto-detect separator & decimal
    static Result fromOfx (const QString &path); // OFX 1.x SGML + OFX 2.x XML
    static Result fromXlsx(const QString &path); // requires QXlsx

    // ── Stubs (em desenvolvimento) ───────────────────────────────────────────
    static Result fromQif (const QString &path);
    static Result fromJson(const QString &path);
    static Result fromOds (const QString &path);
    static Result fromTxt (const QString &path);
    static Result fromMd  (const QString &path);
    static Result fromHtml(const QString &path);
    static Result fromPdf (const QString &path); // always returns "complexo" message

private:
    static QChar   detectSeparator(const QString &firstLine);
    static QChar   detectDecimal  (const QString &sample);
    static qint64  parseAmount    (const QString &s, QChar decimalSep);
    static QDate   parseDate      (const QString &s);
    static Result  stub           (const QString &format);
};
