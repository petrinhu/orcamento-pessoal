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
    sep.reserve(td.headers.size());
    for (int i = 0; i < td.headers.size(); ++i)
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

// ═══════════════════════════════════════════════════════════════════════════════
// Per-section table builders
// ═══════════════════════════════════════════════════════════════════════════════

// ── helpers ───────────────────────────────────────────────────────────────────

QString ExportService::fmtPct(int pct10000)
{
    // e.g. 15000 → "1,50"
    if (pct10000 == 0) return QStringLiteral("0");
    QString s = QString::number(pct10000 / 10000.0, 'f', 4);
    while (s.endsWith(QLatin1Char('0'))) s.chop(1);
    if (s.endsWith(QLatin1Char('.')))    s.chop(1);
    return s.replace(QLatin1Char('.'), QLatin1Char(','));
}

// ── Entradas ──────────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTableEntradas(
    const QList<LancamentoEntrada> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Fonte"),
        QCoreApplication::translate("ExportService", "Categoria"),
        QCoreApplication::translate("ExportService", "Bruto (R$)"),
        QCoreApplication::translate("ExportService", "IRPF (R$)"),
        QCoreApplication::translate("ExportService", "INSS (R$)"),
        QCoreApplication::translate("ExportService", "Líquido (R$)"),
        QCoreApplication::translate("ExportService", "Data"),
        QCoreApplication::translate("ExportService", "Obs"),
    };

    qint64 totBruto = 0, totIrpf = 0, totInss = 0, totLiq = 0;
    for (const auto &l : lista) {
        const qint64 liq = l.valorBruto - l.irpf - l.inss;
        td.rows.append({
            l.fonte,
            l.categoria,
            fmtBrl(l.valorBruto),
            fmtBrl(l.irpf),
            fmtBrl(l.inss),
            fmtBrl(liq),
            QDate(l.ano, l.mes, l.dia).toString(QStringLiteral("dd/MM/yyyy")),
            l.obs,
        });
        totBruto += l.valorBruto;
        totIrpf  += l.irpf;
        totInss  += l.inss;
        totLiq   += liq;
    }
    td.rows.append({
        QCoreApplication::translate("ExportService", "TOTAL"),
        {}, fmtBrl(totBruto), fmtBrl(totIrpf), fmtBrl(totInss), fmtBrl(totLiq), {}, {}
    });
    return td;
}

// ── Gastos ────────────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTableGastos(
    const QList<GastoAvulso> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Descrição"),
        QCoreApplication::translate("ExportService", "Categoria"),
        QCoreApplication::translate("ExportService", "Tipo"),
        QCoreApplication::translate("ExportService", "Valor (R$)"),
        QCoreApplication::translate("ExportService", "Data"),
        QCoreApplication::translate("ExportService", "Obs"),
        QCoreApplication::translate("ExportService", "Beneficiário"),
    };

    qint64 total = 0;
    for (const auto &g : lista) {
        td.rows.append({
            g.descricao,
            g.categoria,
            tipoPgtoLabel(g.tipoPgto),
            fmtBrl(g.valor),
            QDate(g.ano, g.mes, g.dia).toString(QStringLiteral("dd/MM/yyyy")),
            g.obs,
            g.pagador,
        });
        total += g.valor;
    }
    td.rows.append({
        QCoreApplication::translate("ExportService", "TOTAL"),
        {}, {}, fmtBrl(total), {}, {}, {}
    });
    return td;
}

// ── Investimentos ─────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTableInvestimentos(
    const QList<LancamentoInvestimento> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Nome"),
        QCoreApplication::translate("ExportService", "Categoria"),
        QCoreApplication::translate("ExportService", "Tipo Ativo"),
        QCoreApplication::translate("ExportService", "Operação"),
        QCoreApplication::translate("ExportService", "Valor (R$)"),
        QCoreApplication::translate("ExportService", "Data"),
        QCoreApplication::translate("ExportService", "Obs"),
    };

    qint64 netTotal = 0;
    for (const auto &l : lista) {
        td.rows.append({
            l.nome,
            l.categoria,
            tipoAtivoLabel(l.tipoAtivo),
            tipoOpInvLabel(l.tipoOperacao),
            fmtBrl(l.valor),
            QDate(l.ano, l.mes, l.dia).toString(QStringLiteral("dd/MM/yyyy")),
            l.obs,
        });
        netTotal += valorSignadoInv(l);
    }
    td.rows.append({
        QCoreApplication::translate("ExportService", "TOTAL LÍQUIDO"),
        {}, {}, {}, fmtBrl(netTotal), {}, {}
    });
    return td;
}

// ── Planejamento ──────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTablePlanejamento(
    const QList<ItemOrcamento> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Nome"),
        QCoreApplication::translate("ExportService", "Tipo"),
        QCoreApplication::translate("ExportService", "Valor Mensal (R$)"),
        QCoreApplication::translate("ExportService", "Ref. Categoria/Fonte"),
    };

    for (const auto &item : lista) {
        const QString tipoLabel = (item.tipo == QStringLiteral("receita"))
            ? QCoreApplication::translate("ExportService", "Receita")
            : QCoreApplication::translate("ExportService", "Despesa");
        td.rows.append({
            item.nome,
            tipoLabel,
            fmtBrl(item.valorMensal),
            item.categoriaRef,
        });
    }
    return td;
}

// ── Recorrências ──────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTableRecorrencias(
    const QList<TransacaoRecorrente> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Descrição"),
        QCoreApplication::translate("ExportService", "Tipo"),
        QCoreApplication::translate("ExportService", "Valor (R$)"),
        QCoreApplication::translate("ExportService", "Frequência"),
        QCoreApplication::translate("ExportService", "Início"),
        QCoreApplication::translate("ExportService", "Fim"),
        QCoreApplication::translate("ExportService", "Ativo"),
        QCoreApplication::translate("ExportService", "Último Lançamento"),
        QCoreApplication::translate("ExportService", "Obs"),
    };

    for (const auto &t : lista) {
        const QString tipoLabel = (t.tipo == QStringLiteral("entrada"))
            ? QCoreApplication::translate("ExportService", "Entrada")
            : QCoreApplication::translate("ExportService", "Gasto");
        // Capitalise frequencia (raw text: "mensal", "semanal", …)
        QString freqLabel = t.frequencia;
        if (!freqLabel.isEmpty())
            freqLabel[0] = freqLabel[0].toUpper();

        td.rows.append({
            t.descricao,
            tipoLabel,
            fmtBrl(t.valor),
            freqLabel,
            t.dataInicio.isValid()        ? t.dataInicio.toString(QStringLiteral("dd/MM/yyyy"))       : QString(),
            (!t.dataFim.isNull() && t.dataFim.isValid()) ? t.dataFim.toString(QStringLiteral("dd/MM/yyyy")) : QString(),
            t.ativo
                ? QCoreApplication::translate("ExportService", "Sim")
                : QCoreApplication::translate("ExportService", "Não"),
            (!t.ultimoLancamento.isNull() && t.ultimoLancamento.isValid())
                ? t.ultimoLancamento.toString(QStringLiteral("dd/MM/yyyy"))
                : QString(),
            t.obs,
        });
    }
    return td;
}

// ── Dívidas ───────────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTableDividas(
    const QList<Divida> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Tipo"),
        QCoreApplication::translate("ExportService", "Contraparte"),
        QCoreApplication::translate("ExportService", "Descrição"),
        QCoreApplication::translate("ExportService", "Total (R$)"),
        QCoreApplication::translate("ExportService", "Pago (R$)"),
        QCoreApplication::translate("ExportService", "Saldo (R$)"),
        QCoreApplication::translate("ExportService", "Juros a.m. (%)"),
        QCoreApplication::translate("ExportService", "Vencimento"),
        QCoreApplication::translate("ExportService", "Status"),
        QCoreApplication::translate("ExportService", "Obs"),
    };

    qint64 totTotal = 0, totPago = 0, totSaldo = 0;
    for (const auto &d : lista) {
        const QDate venc = QDate::fromString(d.dataVenc, Qt::ISODate);
        td.rows.append({
            tipoDividaLabel(tipoDividaDeTexto(d.tipo)),
            d.contraparte,
            d.descricao,
            fmtBrl(d.valorTotal),
            fmtBrl(d.valorPago),
            fmtBrl(d.saldo()),
            fmtPct(d.jurosMensal),
            venc.isValid() ? venc.toString(QStringLiteral("dd/MM/yyyy")) : QString(),
            statusDividaLabel(statusDividaDeTexto(d.status)),
            d.obs,
        });
        totTotal += d.valorTotal;
        totPago  += d.valorPago;
        totSaldo += d.saldo();
    }
    td.rows.append({
        QCoreApplication::translate("ExportService", "TOTAL"),
        {}, {}, fmtBrl(totTotal), fmtBrl(totPago), fmtBrl(totSaldo), {}, {}, {}, {}
    });
    return td;
}

// ── Metas ─────────────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTableMetas(
    const QList<Meta> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Nome"),
        QCoreApplication::translate("ExportService", "Categoria"),
        QCoreApplication::translate("ExportService", "Meta (R$)"),
        QCoreApplication::translate("ExportService", "Acumulado (R$)"),
        QCoreApplication::translate("ExportService", "Progresso (%)"),
        QCoreApplication::translate("ExportService", "Prazo"),
        QCoreApplication::translate("ExportService", "Restam (dias)"),
        QCoreApplication::translate("ExportService", "Status"),
        QCoreApplication::translate("ExportService", "Obs"),
    };

    for (const auto &m : lista) {
        const QDate prazo = QDate::fromString(m.dataPrazo, Qt::ISODate);
        const int   dias  = m.diasRestantes();
        td.rows.append({
            m.nome,
            categoriaMetaLabel(categoriaMetaDeTexto(m.categoria)),
            fmtBrl(m.valorMeta),
            fmtBrl(m.valorAtual),
            QString::number(m.progresso()) + QStringLiteral("%"),
            prazo.isValid() ? prazo.toString(QStringLiteral("dd/MM/yyyy")) : QString(),
            (prazo.isValid() && dias != 0) ? QString::number(dias) : QString(),
            statusMetaLabel(statusMetaDeTexto(m.status)),
            m.obs,
        });
    }
    return td;
}

// ── Lembretes ─────────────────────────────────────────────────────────────────

ExportService::TableData ExportService::buildTableLembretes(
    const QList<Lembrete> &lista)
{
    TableData td;
    td.headers = {
        QCoreApplication::translate("ExportService", "Descrição"),
        QCoreApplication::translate("ExportService", "Valor (R$)"),
        QCoreApplication::translate("ExportService", "Vencimento"),
        QCoreApplication::translate("ExportService", "Frequência"),
        QCoreApplication::translate("ExportService", "Débito Auto"),
        QCoreApplication::translate("ExportService", "Status"),
        QCoreApplication::translate("ExportService", "Obs"),
    };

    for (const auto &l : lista) {
        const QDate venc = QDate::fromString(l.dataVenc, Qt::ISODate);
        td.rows.append({
            l.descricao,
            l.valor > 0 ? fmtBrl(l.valor) : QString(),
            venc.isValid() ? venc.toString(QStringLiteral("dd/MM/yyyy")) : QString(),
            frequenciaLembreteLabel(frequenciaLembreteDeTexto(l.frequencia)),
            l.debitoAutomatico
                ? QCoreApplication::translate("ExportService", "Sim")
                : QCoreApplication::translate("ExportService", "Não"),
            statusLembreteLabel(statusLembreteDeTexto(l.status)),
            l.obs,
        });
    }
    return td;
}

// ═══════════════════════════════════════════════════════════════════════════════
// OFX transaction builders
// ═══════════════════════════════════════════════════════════════════════════════

QList<ExportService::OFXTx> ExportService::ofxFromEntradas(
    const QList<LancamentoEntrada> &lista)
{
    QList<OFXTx> txs;
    for (const auto &l : lista) {
        const qint64 liq = l.valorBruto - l.irpf - l.inss;
        OFXTx tx;
        tx.dtPosted = QDate(l.ano, l.mes, l.dia).toString(QStringLiteral("yyyyMMdd"));
        tx.valor    = liq;   // positive = CREDIT
        tx.fitid    = QStringLiteral("ENT-%1").arg(l.id);
        tx.memo     = l.fonte.isEmpty() ? l.categoria : l.fonte;
        txs.append(tx);
    }
    return txs;
}

QList<ExportService::OFXTx> ExportService::ofxFromGastos(
    const QList<GastoAvulso> &lista)
{
    QList<OFXTx> txs;
    for (const auto &g : lista) {
        OFXTx tx;
        tx.dtPosted = QDate(g.ano, g.mes, g.dia).toString(QStringLiteral("yyyyMMdd"));
        tx.valor    = -g.valor;  // negative = DEBIT
        tx.fitid    = QStringLiteral("GST-%1").arg(g.id);
        tx.memo     = g.descricao;
        txs.append(tx);
    }
    return txs;
}

QList<ExportService::OFXTx> ExportService::ofxFromDataset(const Dataset &d)
{
    QList<OFXTx> txs;
    for (int i = 0; i < d.compras.size(); ++i) {
        const auto &c = d.compras[i];
        for (const auto &p : d.parcelas[i]) {
            const QDate dt = ParcelaService::dataParaParcela(c, p.numero);
            OFXTx tx;
            tx.dtPosted = dt.toString(QStringLiteral("yyyyMMdd"));
            tx.valor    = -p.valor;  // negative = DEBIT
            tx.fitid    = QStringLiteral("CRT-%1-%2").arg(c.id).arg(p.numero);
            tx.memo     = QStringLiteral("%1 (%2/%3)").arg(c.nome).arg(p.numero).arg(c.numParcelas);
            txs.append(tx);
        }
    }
    return txs;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Multi-section HTML helper
// ═══════════════════════════════════════════════════════════════════════════════

QString ExportService::buildHtmlMultiStr(const QList<Section> &sections, bool standalone)
{
    QString html;
    if (standalone) {
        html = QStringLiteral(
            "<!DOCTYPE html><html lang=\"pt-BR\"><head><meta charset=\"UTF-8\">"
            "<style>"
            "body{font-family:Arial,sans-serif;font-size:12px;}"
            "h2{color:#166F4A;margin-top:28px;margin-bottom:6px;}"
            "table{border-collapse:collapse;width:100%;margin-bottom:8px;}"
            "th{background:#166F4A;color:white;padding:6px 8px;text-align:left;border:1px solid #ccc;}"
            "td{padding:5px 8px;border:1px solid #ddd;text-align:right;}"
            "td:first-child,td:nth-child(2){text-align:left;}"
            "tr:nth-child(even){background:#f6f8fa;}"
            "tr:last-child td{font-weight:bold;background:#f0f3f6;}"
            "</style></head><body>\n");
    }

    for (const auto &s : sections) {
        html += QStringLiteral("<h2>%1</h2>\n<table>\n<tr>").arg(s.name.toHtmlEscaped());
        for (const auto &h : s.data.headers)
            html += QStringLiteral("<th>%1</th>").arg(h.toHtmlEscaped());
        html += QStringLiteral("</tr>\n");
        for (const auto &row : s.data.rows) {
            html += QStringLiteral("<tr>");
            for (const auto &cell : row)
                html += QStringLiteral("<td>%1</td>").arg(cell.toHtmlEscaped());
            html += QStringLiteral("</tr>\n");
        }
        html += QStringLiteral("</table>\n");
    }

    if (standalone) html += QStringLiteral("</body></html>");
    return html;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Multi-section export methods
// ═══════════════════════════════════════════════════════════════════════════════

bool ExportService::toMultiCsv(const QString &path, const QList<Section> &sections)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << "\xEF\xBB\xBF";  // UTF-8 BOM for Excel

    bool first = true;
    for (const auto &s : sections) {
        if (!first) out << QStringLiteral("\n\n");
        first = false;
        out << QStringLiteral("## %1\n").arg(s.name);
        out << s.data.headers.join(QLatin1Char(';')) << QLatin1Char('\n');
        for (const auto &row : s.data.rows)
            out << row.join(QLatin1Char(';')) << QLatin1Char('\n');
    }
    return true;
}

bool ExportService::toMultiMarkdown(const QString &path, const QList<Section> &sections)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    bool first = true;
    for (const auto &s : sections) {
        if (!first) out << QStringLiteral("\n");
        first = false;
        out << QStringLiteral("## %1\n\n").arg(s.name);

        // Header row
        out << QStringLiteral("| ") << s.data.headers.join(QStringLiteral(" | "))
            << QStringLiteral(" |\n");
        // Separator
        QStringList sep;
        sep.reserve(s.data.headers.size());
        for (int i = 0; i < s.data.headers.size(); ++i)
            sep << QStringLiteral("---");
        out << QStringLiteral("| ") << sep.join(QStringLiteral(" | ")) << QStringLiteral(" |\n");
        // Data
        for (const auto &row : s.data.rows) {
            QStringList cells;
            cells.reserve(row.size());
            for (const auto &c : row)
                cells << (c.isEmpty() ? QStringLiteral(" ") : c);
            out << QStringLiteral("| ") << cells.join(QStringLiteral(" | "))
                << QStringLiteral(" |\n");
        }
        out << QLatin1Char('\n');
    }
    return true;
}

bool ExportService::toMultiHtml(const QString &path, const QList<Section> &sections)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    out << buildHtmlMultiStr(sections, true);
    return true;
}

bool ExportService::toMultiPdf(const QString &path, const QList<Section> &sections)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Landscape);
    writer.setResolution(150);
    writer.setTitle(QStringLiteral("PeuBud — Exportação"));

    QTextDocument doc;
    doc.setDefaultFont(QFont(QStringLiteral("Arial"), 9));
    doc.setHtml(buildHtmlMultiStr(sections, true));
    doc.setPageSize(QSizeF(writer.width(), writer.height()));

    QPainter painter(&writer);
    doc.drawContents(&painter);
    return true;
}

bool ExportService::toMultiTxt(const QString &path, const QList<Section> &sections)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    for (const auto &s : sections) {
        // Section header
        const QString title = QStringLiteral("  %1  ").arg(s.name.toUpper());
        const QString bar(title.size(), QLatin1Char('='));
        out << bar << QLatin1Char('\n') << title << QLatin1Char('\n') << bar << QLatin1Char('\n');

        const auto &td  = s.data;
        const int   cols = td.headers.size();
        QList<int>  widths(cols, 0);
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
                out << cell.leftJustified(widths[c]) << QStringLiteral(" | ");
            }
            out << QLatin1Char('\n');
        };

        hline();
        printRow(td.headers);
        hline();
        for (const auto &row : td.rows)
            printRow(row);
        hline();
        out << QLatin1Char('\n');
    }
    return true;
}

bool ExportService::toMultiJson(const QString &path, const QList<Section> &sections)
{
    QJsonObject root;
    root[QStringLiteral("exportado_em")] =
        QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonObject secObj;
    for (const auto &s : sections) {
        QJsonArray rows;
        for (const auto &row : s.data.rows) {
            QJsonObject rowObj;
            for (int c = 0; c < qMin(row.size(), s.data.headers.size()); ++c)
                rowObj[s.data.headers[c]] = row[c];
            rows.append(rowObj);
        }
        secObj[s.name] = rows;
    }
    root[QStringLiteral("secoes")] = secObj;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool ExportService::toMultiXlsx(const QString &path, const QList<Section> &sections)
{
#ifdef QXLSX_LIBRARY
    QXlsx::Document xlsx;

    QXlsx::Format headerFmt;
    headerFmt.setFontBold(true);
    headerFmt.setFontColor(QColor(QStringLiteral("#FFFFFF")));
    headerFmt.setPatternBackgroundColor(QColor(QStringLiteral("#166F4A")));
    headerFmt.setBorderStyle(QXlsx::Format::BorderThin);

    QXlsx::Format totalFmt;
    totalFmt.setFontBold(true);
    totalFmt.setPatternBackgroundColor(QColor(QStringLiteral("#F0F3F6")));

    for (const auto &s : sections) {
        xlsx.addSheet(s.name);
        xlsx.selectSheet(s.name);

        for (int c = 0; c < s.data.headers.size(); ++c)
            xlsx.write(1, c + 1, s.data.headers[c], headerFmt);

        for (int r = 0; r < s.data.rows.size(); ++r) {
            const bool isTotal = (r == s.data.rows.size() - 1);
            const QXlsx::Format &fmt = isTotal ? totalFmt : QXlsx::Format();
            for (int c = 0; c < s.data.rows[r].size(); ++c) {
                if (!s.data.rows[r][c].isEmpty())
                    xlsx.write(r + 2, c + 1, s.data.rows[r][c], fmt);
            }
        }
        xlsx.setColumnWidth(1, 28);
        xlsx.setColumnWidth(2, 18);
    }

    // Remove default empty sheet created by QXlsx (if any sections were added)
    if (!sections.isEmpty())
        xlsx.deleteSheet(QStringLiteral("Sheet1"));

    return xlsx.saveAs(path);
#else
    Q_UNUSED(sections)
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    f.write("XLSX support not compiled.\n");
    return false;
#endif
}

// ── OFX / QIF (financial transactions) ───────────────────────────────────────

bool ExportService::toMultiOfx(const QString &path, const QList<OFXTx> &txs)
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
        "<ACCTTYPE>CHECKING</ACCTTYPE></BANKACCTFROM>\n"
        "        <BANKTRANLIST>\n"
        "          <DTSTART>%1</DTSTART>\n"
        "          <DTEND>%1</DTEND>\n").arg(now);

    for (const auto &tx : txs) {
        const QString type   = tx.valor >= 0 ? QStringLiteral("CREDIT") : QStringLiteral("DEBIT");
        const qint64  absVal = tx.valor >= 0 ? tx.valor : -tx.valor;
        const QString amtStr = tx.valor >= 0
            ? QStringLiteral("%1.%2").arg(absVal / 100).arg(absVal % 100, 2, 10, QLatin1Char('0'))
            : QStringLiteral("-%1.%2").arg(absVal / 100).arg(absVal % 100, 2, 10, QLatin1Char('0'));

        out << QStringLiteral(
            "          <STMTTRN>\n"
            "            <TRNTYPE>%1</TRNTYPE>\n"
            "            <DTPOSTED>%2</DTPOSTED>\n"
            "            <TRNAMT>%3</TRNAMT>\n"
            "            <FITID>%4</FITID>\n"
            "            <MEMO>%5</MEMO>\n"
            "          </STMTTRN>\n")
            .arg(type, tx.dtPosted, amtStr, tx.fitid, tx.memo.toHtmlEscaped());
    }

    out << QStringLiteral(
        "        </BANKTRANLIST>\n"
        "      </STMTRS>\n"
        "    </STMTTRNRS>\n"
        "  </BANKMSGSRSV1>\n"
        "</OFX>\n");
    return true;
}

bool ExportService::toMultiQif(const QString &path, const QList<OFXTx> &txs)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);

    out << QStringLiteral("!Type:Bank\n");
    for (const auto &tx : txs) {
        const QDate dt = QDate::fromString(tx.dtPosted, QStringLiteral("yyyyMMdd"));
        out << QStringLiteral("D%1\n").arg(dt.toString(QStringLiteral("MM/dd/yyyy")));
        const qint64  absVal = tx.valor >= 0 ? tx.valor : -tx.valor;
        const QString sign   = tx.valor >= 0 ? QString() : QStringLiteral("-");
        out << QStringLiteral("T%1%2.%3\n")
               .arg(sign)
               .arg(absVal / 100)
               .arg(absVal % 100, 2, 10, QLatin1Char('0'));
        out << QStringLiteral("P%1\n").arg(tx.memo);
        out << QStringLiteral("^\n");
    }
    return true;
}
