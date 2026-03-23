#include "ExportService.h"
#include "ParcelaService.h"

#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDate>
#include <QDateTime>
#include <QPdfWriter>
#include <QPainter>
#include <QTextDocument>
#include <QPageSize>
#include <QPageLayout>
#include <QRegularExpression>
#include <QCoreApplication>

#ifdef QXLSX_LIBRARY
#include <xlsxdocument.h>
#include <xlsxformat.h>
#endif

// ─── Helpers ──────────────────────────────────────────────────────────────────

QString ExportService::cartaoNome(const Dataset &d, int cartaoId)
{
    for (const auto &ct : d.cartoes)
        if (ct.id == cartaoId) return ct.apelido;
    return {};
}

QString ExportService::fmtBrl(qint64 centavos)
{
    // "1234,56" — no symbol, BR decimal (for CSV/spreadsheet cells)
    const qint64 abs_c = centavos < 0 ? -centavos : centavos;
    return QStringLiteral("%1%2,%3")
        .arg(centavos < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

// ─── Table builder ────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTable(const Dataset &d)
{
    TableData td;

    // Compute month range
    QDate first = QDate(QDate::currentDate().year(), QDate::currentDate().month(), 1);
    QDate last  = first.addMonths(3);
    for (int i = 0; i < d.compras.size(); ++i) {
        const auto &parcelas = d.parcelas[i];
        if (parcelas.isEmpty()) continue;
        const QDate df = ParcelaService::dataParaParcela(d.compras[i], parcelas.first().numero);
        const QDate dl = ParcelaService::dataParaParcela(d.compras[i], parcelas.last().numero);
        if (df < first) first = df;
        if (dl > last)  last  = dl;
    }

    const int totalMonths = (last.year() - first.year()) * 12
                          + (last.month() - first.month()) + 1;

    // Headers
    td.headers << QCoreApplication::translate("ExportService", "Descrição")
               << QCoreApplication::translate("ExportService", "Cartão")
               << QCoreApplication::translate("ExportService", "Total");
    for (int k = 0; k < totalMonths; ++k)
        td.headers << first.addMonths(k).toString(QStringLiteral("MMM/yy"));

    // Data rows
    QList<qint64> monthTotals(totalMonths, 0LL);

    for (int i = 0; i < d.compras.size(); ++i) {
        const auto &c = d.compras[i];
        QStringList row;
        row << c.nome << cartaoNome(d, c.cartaoId);

        qint64 total = 0;
        QList<qint64> monthVals(totalMonths, 0LL);
        for (const auto &p : d.parcelas[i]) {
            const QDate dt = ParcelaService::dataParaParcela(c, p.numero);
            const int k = (dt.year() - first.year()) * 12 + (dt.month() - first.month());
            if (k >= 0 && k < totalMonths) {
                monthVals[k] += p.valor;
                monthTotals[k] += p.valor;
            }
            total += p.valor;
        }
        row << fmtBrl(total);
        for (int k = 0; k < totalMonths; ++k)
            row << (monthVals[k] > 0 ? fmtBrl(monthVals[k]) : QString());

        td.rows.append(row);
    }

    // Total row
    QStringList totRow;
    totRow << QCoreApplication::translate("ExportService", "TOTAL") << QString() << QString();
    qint64 grandTotal = 0;
    for (int k = 0; k < totalMonths; ++k) {
        totRow << (monthTotals[k] > 0 ? fmtBrl(monthTotals[k]) : QString());
        grandTotal += monthTotals[k];
    }
    totRow[2] = fmtBrl(grandTotal);
    td.rows.append(totRow);

    return td;
}

// ─── CSV ──────────────────────────────────────────────────────────────────────

bool ExportService::toCsv(const QString &path, const Dataset &d)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    // UTF-8 BOM for Excel on Windows
    out << "\xEF\xBB\xBF";

    const TableData td = buildTable(d);
    out << td.headers.join(QLatin1Char(';')) << QLatin1Char('\n');
    for (const auto &row : td.rows)
        out << row.join(QLatin1Char(';')) << QLatin1Char('\n');
    return true;
}

// ─── HTML helper ──────────────────────────────────────────────────────────────

QString ExportService::buildHtmlStr(const Dataset &d, bool standalone)
{
    const TableData td = buildTable(d);

    QString html;
    if (standalone) {
        html = QStringLiteral(
            "<!DOCTYPE html><html lang=\"pt-BR\"><head><meta charset=\"UTF-8\">"
            "<style>"
            "body{font-family:Arial,sans-serif;font-size:12px;}"
            "table{border-collapse:collapse;width:100%;}"
            "th{background:#1A7F37;color:white;padding:6px 8px;text-align:left;border:1px solid #ccc;}"
            "td{padding:5px 8px;border:1px solid #ddd;text-align:right;}"
            "td:first-child,td:nth-child(2){text-align:left;}"
            "tr:nth-child(even){background:#f6f8fa;}"
            "tr:last-child td{font-weight:bold;background:#f0f3f6;}"
            "</style></head><body><table>\n");
    } else {
        html = QStringLiteral("<table border=\"1\" cellpadding=\"5\" cellspacing=\"0\">\n");
    }

    // Header
    html += QStringLiteral("<tr>");
    for (const auto &h : td.headers)
        html += QStringLiteral("<th>%1</th>").arg(h.toHtmlEscaped());
    html += QStringLiteral("</tr>\n");

    // Rows
    for (int r = 0; r < td.rows.size(); ++r) {
        html += QStringLiteral("<tr>");
        for (const auto &cell : td.rows[r])
            html += QStringLiteral("<td>%1</td>").arg(cell.toHtmlEscaped());
        html += QStringLiteral("</tr>\n");
    }

    html += QStringLiteral("</table>");
    if (standalone) html += QStringLiteral("</body></html>");
    return html;
}

// ─── HTML ─────────────────────────────────────────────────────────────────────

bool ExportService::toHtml(const QString &path, const Dataset &d)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << buildHtmlStr(d, true);
    return true;
}

// ─── PDF ──────────────────────────────────────────────────────────────────────

bool ExportService::toPdf(const QString &path, const Dataset &d)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setResolution(150);
    writer.setTitle(QStringLiteral("PeuBud — Parcelas"));

    QTextDocument doc;
    doc.setDefaultFont(QFont(QStringLiteral("Arial"), 9));
    doc.setHtml(buildHtmlStr(d, true));
    doc.setPageSize(QSizeF(writer.width(), writer.height()));

    QPainter painter(&writer);
    doc.drawContents(&painter);
    return true;
}

// ─── Markdown ─────────────────────────────────────────────────────────────────

bool ExportService::toMarkdown(const QString &path, const Dataset &d)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    const TableData td = buildTable(d);

    // Header row
    out << QStringLiteral("| ") << td.headers.join(QStringLiteral(" | ")) << QStringLiteral(" |\n");
    // Separator
    QStringList sep;
    for (const auto &h : td.headers)
        sep << QStringLiteral("---");
    out << QStringLiteral("| ") << sep.join(QStringLiteral(" | ")) << QStringLiteral(" |\n");
    // Data
    for (const auto &row : td.rows) {
        QStringList cells;
        for (const auto &c : row)
            cells << (c.isEmpty() ? QStringLiteral(" ") : c);
        out << QStringLiteral("| ") << cells.join(QStringLiteral(" | ")) << QStringLiteral(" |\n");
    }
    return true;
}

// ─── TXT ──────────────────────────────────────────────────────────────────────

bool ExportService::toTxt(const QString &path, const Dataset &d)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    const TableData td = buildTable(d);
    const int cols = td.headers.size();

    // Compute column widths
    QList<int> widths(cols, 0);
    for (int c = 0; c < cols; ++c)
        widths[c] = td.headers[c].size();
    for (const auto &row : td.rows)
        for (int c = 0; c < qMin(row.size(), cols); ++c)
            widths[c] = qMax(widths[c], row[c].size());

    auto hline = [&]() {
        QString line = QStringLiteral("+");
        for (int c = 0; c < cols; ++c)
            line += QString(widths[c] + 2, QLatin1Char('-')) + QStringLiteral("+");
        out << line << QLatin1Char('\n');
    };

    auto printRow = [&](const QStringList &row) {
        out << QStringLiteral("| ");
        for (int c = 0; c < cols; ++c) {
            const QString cell = c < row.size() ? row[c] : QString();
            out << cell.leftJustified(widths[c]);
            out << QStringLiteral(" | ");
        }
        out << QLatin1Char('\n');
    };

    hline();
    printRow(td.headers);
    hline();
    for (const auto &row : td.rows)
        printRow(row);
    hline();
    return true;
}

// ─── JSON ─────────────────────────────────────────────────────────────────────

bool ExportService::toJson(const QString &path, const Dataset &d)
{
    QJsonArray arr;
    for (int i = 0; i < d.compras.size(); ++i) {
        const auto &c = d.compras[i];
        QJsonObject obj;
        obj[QStringLiteral("id")]         = c.id;
        obj[QStringLiteral("nome")]       = c.nome;
        obj[QStringLiteral("valorTotal")] = c.valorTotal;
        obj[QStringLiteral("numParcelas")]= c.numParcelas;
        obj[QStringLiteral("moeda")]      = c.moeda;
        obj[QStringLiteral("cartao")]     = cartaoNome(d, c.cartaoId);
        obj[QStringLiteral("criadoEm")]   = c.criadoEm;

        QJsonArray parcsArr;
        for (const auto &p : d.parcelas[i]) {
            QJsonObject pobj;
            pobj[QStringLiteral("numero")]     = p.numero;
            pobj[QStringLiteral("valor")]      = p.valor;
            pobj[QStringLiteral("dataFatura")] = p.dataFatura;
            pobj[QStringLiteral("modificada")] = p.modificada;
            parcsArr.append(pobj);
        }
        obj[QStringLiteral("parcelas")] = parcsArr;
        arr.append(obj);
    }

    QJsonDocument doc(arr);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

// ─── OFX ──────────────────────────────────────────────────────────────────────

bool ExportService::toOfx(const QString &path, const Dataset &d)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    const QString now = QDateTime::currentDateTimeUtc()
        .toString(QStringLiteral("yyyyMMddHHmmss"));

    out << QStringLiteral(
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<?OFX OFXHEADER=\"200\" VERSION=\"211\" SECURITY=\"NONE\""
        " OLDFILEUID=\"NONE\" NEWFILEUID=\"NONE\"?>\n"
        "<OFX>\n"
        "  <BANKMSGSRSV1>\n"
        "    <STMTTRNRS>\n"
        "      <TRNUID>1</TRNUID>\n"
        "      <STATUS><CODE>0</CODE><SEVERITY>INFO</SEVERITY></STATUS>\n"
        "      <STMTRS>\n"
        "        <CURDEF>BRL</CURDEF>\n"
        "        <BANKACCTFROM><BANKID>0</BANKID><ACCTID>PEUBUD</ACCTID>"
        "<ACCTTYPE>CREDITLINE</ACCTTYPE></BANKACCTFROM>\n"
        "        <BANKTRANLIST>\n"
        "          <DTSTART>%1</DTSTART>\n"
        "          <DTEND>%1</DTEND>\n").arg(now);

    for (int i = 0; i < d.compras.size(); ++i) {
        const auto &c = d.compras[i];
        for (const auto &p : d.parcelas[i]) {
            const QDate dt = ParcelaService::dataParaParcela(c, p.numero);
            const QString dtStr = dt.toString(QStringLiteral("yyyyMMdd"));
            const QString amtStr = QStringLiteral("-%1.%2")
                .arg(p.valor / 100).arg(p.valor % 100, 2, 10, QLatin1Char('0'));
            out << QStringLiteral(
                "          <STMTTRN>\n"
                "            <TRNTYPE>DEBIT</TRNTYPE>\n"
                "            <DTPOSTED>%1</DTPOSTED>\n"
                "            <TRNAMT>%2</TRNAMT>\n"
                "            <FITID>PEUBUD-%3-%4</FITID>\n"
                "            <MEMO>%5 (%6/%7)</MEMO>\n"
                "          </STMTTRN>\n")
                .arg(dtStr, amtStr)
                .arg(c.id).arg(p.numero)
                .arg(c.nome.toHtmlEscaped())
                .arg(p.numero).arg(c.numParcelas);
        }
    }

    out << QStringLiteral(
        "        </BANKTRANLIST>\n"
        "      </STMTRS>\n"
        "    </STMTTRNRS>\n"
        "  </BANKMSGSRSV1>\n"
        "</OFX>\n");
    return true;
}

// ─── QIF ──────────────────────────────────────────────────────────────────────

bool ExportService::toQif(const QString &path, const Dataset &d)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out << QStringLiteral("!Type:CCard\n");

    for (int i = 0; i < d.compras.size(); ++i) {
        const auto &c = d.compras[i];
        for (const auto &p : d.parcelas[i]) {
            const QDate dt = ParcelaService::dataParaParcela(c, p.numero);
            out << QStringLiteral("D%1\n").arg(dt.toString(QStringLiteral("MM/dd/yyyy")));
            out << QStringLiteral("T-%1.%2\n")
                    .arg(p.valor / 100).arg(p.valor % 100, 2, 10, QLatin1Char('0'));
            out << QStringLiteral("P%1\n").arg(c.nome);
            out << QStringLiteral("M%1 (%2/%3)\n").arg(c.nome).arg(p.numero).arg(c.numParcelas);
            out << QStringLiteral("^\n");
        }
    }
    return true;
}

// ─── XLSX ─────────────────────────────────────────────────────────────────────

bool ExportService::toXlsx(const QString &path, const Dataset &d)
{
#ifdef QXLSX_LIBRARY
    QXlsx::Document xlsx;

    const TableData td = buildTable(d);

    QXlsx::Format headerFmt;
    headerFmt.setFontBold(true);
    headerFmt.setFontColor(QColor(QStringLiteral("#FFFFFF")));
    headerFmt.setPatternBackgroundColor(QColor(QStringLiteral("#1A7F37")));
    headerFmt.setBorderStyle(QXlsx::Format::BorderThin);

    QXlsx::Format totalFmt;
    totalFmt.setFontBold(true);
    totalFmt.setPatternBackgroundColor(QColor(QStringLiteral("#F0F3F6")));

    // Header row
    for (int c = 0; c < td.headers.size(); ++c)
        xlsx.write(1, c + 1, td.headers[c], headerFmt);

    // Data rows
    for (int r = 0; r < td.rows.size(); ++r) {
        const bool isTotal = (r == td.rows.size() - 1);
        const QXlsx::Format &fmt = isTotal ? totalFmt : QXlsx::Format();
        for (int c = 0; c < td.rows[r].size(); ++c) {
            const QString &cell = td.rows[r][c];
            if (!cell.isEmpty())
                xlsx.write(r + 2, c + 1, cell, fmt);
        }
    }

    // Auto-width first 3 columns
    xlsx.setColumnWidth(1, 30);
    xlsx.setColumnWidth(2, 16);
    xlsx.setColumnWidth(3, 14);

    return xlsx.saveAs(path);
#else
    Q_UNUSED(d)
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write("XLSX support not compiled.\n");
    return false;
#endif
}

// ─── ODS (stub) ───────────────────────────────────────────────────────────────

bool ExportService::toOds(const QString &path, const Dataset &d)
{
    Q_UNUSED(path)
    Q_UNUSED(d)
    return false; // stub — caller should show "em desenvolvimento" message
}
