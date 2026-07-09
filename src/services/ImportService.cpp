#include "ImportService.h"
#include "ParcelaService.h"
#include "services/import/ExistingTransactionMatchFinder.h"
#include "services/import/BankPDFParsers.h"

#ifdef PEUBUD_HAS_POPPLER
#include <poppler-qt6.h>
#include <memory>
#endif

#include <QFile>
#include <QTextStream>
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QDate>
#include <QDateTime>
#include <algorithm>
#include <cmath>

#ifdef QXLSX_LIBRARY
#include <xlsxdocument.h>
#endif

// ─── Helpers ──────────────────────────────────────────────────────────────────

ImportService::Result ImportService::stub(const QString &format)
{
    Result r;
    r.error = QStringLiteral(
        "Importação de %1 ainda não implementada nesta versão.\n"
        "Use CSV ou OFX para importar dados do seu banco.").arg(format);
    return r;
}

QChar ImportService::detectSeparator(const QString &firstLine)
{
    const int commas     = firstLine.count(QLatin1Char(','));
    const int semicolons = firstLine.count(QLatin1Char(';'));
    const int tabs       = firstLine.count(QLatin1Char('\t'));
    if (tabs > commas && tabs > semicolons) return QLatin1Char('\t');
    if (semicolons > commas)               return QLatin1Char(';');
    return QLatin1Char(',');
}

QChar ImportService::detectDecimal(const QString &sample)
{
    // If "," appears after digits and before 2 trailing digits, it's decimal
    const QRegularExpression rxCommaDecimal(QStringLiteral("\\d,\\d{2}(?:\\D|$)"));
    if (rxCommaDecimal.match(sample).hasMatch()) return QLatin1Char(',');
    return QLatin1Char('.');
}

qint64 ImportService::parseAmount(const QString &s, QChar decimalSep)
{
    QString clean = s.trimmed();
    // Remove currency symbols and spaces
    clean.remove(QRegularExpression(QStringLiteral("[R$€£¥\\s]")));
    if (decimalSep == QLatin1Char(',')) {
        clean.remove(QLatin1Char('.'));          // thousands sep
        clean.replace(QLatin1Char(','), QLatin1Char('.'));
    } else {
        clean.remove(QLatin1Char(','));          // thousands sep
    }
    bool ok = false;
    const double v = clean.toDouble(&ok);
    if (!ok) return 0;
    return static_cast<qint64>(std::round(std::abs(v) * 100.0));
}

QDate ImportService::parseDate(const QString &s)
{
    const QString t = s.trimmed();
    // Try common BR formats first
    static const QStringList formats = {
        QStringLiteral("dd/MM/yyyy"),
        QStringLiteral("d/M/yyyy"),
        QStringLiteral("dd/MM/yy"),
        QStringLiteral("yyyy-MM-dd"),
        QStringLiteral("yyyyMMdd"),    // OFX date
        QStringLiteral("MM/dd/yyyy"),
        QStringLiteral("d-M-yyyy"),
    };
    for (const auto &fmt : formats) {
        const QDate d = QDate::fromString(t.left(10), fmt);
        if (d.isValid()) return d;
    }
    // OFX: YYYYMMDDHHMMSS
    if (t.size() >= 8) {
        const QDate d = QDate::fromString(t.left(8), QStringLiteral("yyyyMMdd"));
        if (d.isValid()) return d;
    }
    return QDate::currentDate();
}

// ─── CSV ──────────────────────────────────────────────────────────────────────

ImportService::Result ImportService::fromCsv(const QString &path)
{
    Result r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        r.error = QStringLiteral("Não foi possível abrir o arquivo: %1").arg(path);
        return r;
    }
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);

    // Read all lines, skip BOM if present
    QStringList lines;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (!line.isEmpty()) lines.append(line);
    }
    f.close();

    if (lines.isEmpty()) { r.error = QStringLiteral("Arquivo CSV vazio."); return r; }

    const QChar sep = detectSeparator(lines.first());

    // Sample a few lines to detect decimal separator
    const QString sample = lines.mid(1, qMin(5, lines.size()-1)).join(QStringLiteral(","));
    const QChar dec = detectDecimal(sample);

    // Parse header row
    const QStringList headers = lines.first().split(sep);

    // Try to identify column indices (case-insensitive, partial match)
    int colDate = -1, colDesc = -1, colAmt = -1;
    for (int i = 0; i < headers.size(); ++i) {
        const QString h = headers[i].trimmed().toLower();
        if (colDate < 0 && (h.contains(QStringLiteral("data")) ||
                            h.contains(QStringLiteral("date")) ||
                            h.contains(QStringLiteral("dt"))))     colDate = i;
        if (colDesc < 0 && (h.contains(QStringLiteral("descri")) ||
                            h.contains(QStringLiteral("desc")) ||
                            h.contains(QStringLiteral("memo"))   ||
                            h.contains(QStringLiteral("histor")) ||
                            h.contains(QStringLiteral("title"))  ||
                            h.contains(QStringLiteral("t\u00edtulo"))))   colDesc = i;
        if (colAmt  < 0 && (h.contains(QStringLiteral("valor"))  ||
                            h.contains(QStringLiteral("amount")) ||
                            h.contains(QStringLiteral("montant"))||
                            h.contains(QStringLiteral("vl_"))))   colAmt  = i;
    }
    // Fallback: first col = date, second = desc, last numeric = amount
    if (colDate < 0) colDate = 0;
    if (colDesc < 0) colDesc = qMin(1, headers.size()-1);
    if (colAmt  < 0) colAmt  = headers.size() - 1;

    const QDate hoje = QDate::currentDate();

    for (int li = 1; li < lines.size(); ++li) {
        const QStringList fields = lines[li].split(sep);
        if (fields.size() <= std::max({colDate, colDesc, colAmt})) continue;

        const QString rawDate = colDate < fields.size() ? fields[colDate].trimmed() : QString();
        const QString rawDesc = colDesc < fields.size() ? fields[colDesc].trimmed() : QString();
        const QString rawAmt  = colAmt  < fields.size() ? fields[colAmt ].trimmed() : QString();

        // Remove surrounding quotes
        auto unquote = [](QString s) {
            if (s.startsWith(QLatin1Char('"')) && s.endsWith(QLatin1Char('"')))
                s = s.mid(1, s.size()-2);
            return s;
        };

        const QDate   dt  = parseDate(unquote(rawDate));
        const QString nom = unquote(rawDesc).isEmpty()
                            ? QStringLiteral("Importado %1").arg(li) : unquote(rawDesc);
        const qint64  val = parseAmount(unquote(rawAmt), dec);
        if (val <= 0) continue;

        Compra c;
        c.nome           = nom;
        c.valorTotal     = val;
        c.numParcelas    = 1;
        c.parcelaInicial = 1;
        c.mesInicio      = dt.isValid() ? dt.month() : hoje.month();
        c.anoInicio      = dt.isValid() ? dt.year()  : hoje.year();
        c.moeda          = QStringLiteral("BRL");
        c.criadoEm       = QDateTime::currentDateTime().toString(Qt::ISODate);

        Parcela p;
        p.numero     = 1;
        p.valor      = val;
        p.dataFatura = dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy")) : QString();

        r.compras.append(c);
        r.parcelas.append({p});
    }

    if (r.compras.isEmpty())
        r.error = QStringLiteral("Nenhuma transação encontrada no arquivo CSV.");
    return r;
}

// ─── OFX ──────────────────────────────────────────────────────────────────────
// Handles OFX 2.x (XML) and OFX 1.x (SGML-like, line-by-line).

static QString extractSgmlTag(const QStringList &lines, int &idx, const QString &tag)
{
    // Searches forward from idx for <TAG>value and returns value
    const QString openTag  = QStringLiteral("<%1>").arg(tag.toUpper());
    for (int i = idx; i < lines.size(); ++i) {
        const QString line = lines[i].trimmed().toUpper();
        const int pos = line.indexOf(openTag);
        if (pos >= 0) {
            const QString after = lines[i].trimmed().mid(pos + openTag.size());
            // Value might be on same line: <TAG>value</TAG> or <TAG>value\n
            const QString closeTag = QStringLiteral("</%1>").arg(tag.toUpper());
            const int cpos = after.toUpper().indexOf(closeTag);
            idx = i;
            return (cpos >= 0) ? after.left(cpos).trimmed() : after.trimmed();
        }
    }
    return {};
}

ImportService::Result ImportService::fromOfx(const QString &path)
{
    Result r;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        r.error = QStringLiteral("Não foi possível abrir: %1").arg(path);
        return r;
    }
    const QByteArray raw = f.readAll();
    f.close();

    const QString content = QString::fromUtf8(raw);
    const bool isXml = content.trimmed().startsWith(QLatin1Char('<'));

    const QDate hoje = QDate::currentDate();

    if (isXml) {
        // OFX 2.x — valid XML
        QXmlStreamReader xml(content);
        QString dtPosted, trnAmt, memo, name;
        bool inStmtTrn = false;

        while (!xml.atEnd() && !xml.hasError()) {
            const QXmlStreamReader::TokenType token = xml.readNext();
            if (token == QXmlStreamReader::StartElement) {
                const QString elem = xml.name().toString().toUpper();
                if (elem == QStringLiteral("STMTTRN"))  { inStmtTrn = true; dtPosted.clear(); trnAmt.clear(); memo.clear(); name.clear(); }
                else if (inStmtTrn && elem == QStringLiteral("DTPOSTED")) dtPosted = xml.readElementText().trimmed();
                else if (inStmtTrn && elem == QStringLiteral("TRNAMT"))   trnAmt   = xml.readElementText().trimmed();
                else if (inStmtTrn && elem == QStringLiteral("MEMO"))     memo     = xml.readElementText().trimmed();
                else if (inStmtTrn && elem == QStringLiteral("NAME"))     name     = xml.readElementText().trimmed();
            } else if (token == QXmlStreamReader::EndElement) {
                if (xml.name().toString().toUpper() == QStringLiteral("STMTTRN") && inStmtTrn) {
                    inStmtTrn = false;
                    const QDate dt  = parseDate(dtPosted);
                    const qint64 val = parseAmount(trnAmt, QLatin1Char('.'));
                    if (val <= 0) continue;
                    const QString nom = (!memo.isEmpty() ? memo : name);

                    Compra c;
                    c.nome = nom.isEmpty() ? QStringLiteral("OFX Import") : nom;
                    c.valorTotal = val; c.numParcelas = 1; c.parcelaInicial = 1;
                    c.mesInicio = dt.isValid() ? dt.month() : hoje.month();
                    c.anoInicio = dt.isValid() ? dt.year()  : hoje.year();
                    c.moeda = QStringLiteral("BRL");
                    c.criadoEm = QDateTime::currentDateTime().toString(Qt::ISODate);
                    Parcela p; p.numero = 1; p.valor = val;
                    p.dataFatura = dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy")) : QString();
                    r.compras.append(c); r.parcelas.append({p});
                }
            }
        }
    } else {
        // OFX 1.x SGML — line-by-line
        const QStringList lines = content.split(QLatin1Char('\n'));
        for (int i = 0; i < lines.size(); ++i) {
            if (!lines[i].trimmed().toUpper().contains(QStringLiteral("<STMTTRN>"))) continue;
            int j = i + 1;
            const QString dtPosted = extractSgmlTag(lines, j, QStringLiteral("DTPOSTED"));
            j = i + 1;
            const QString trnAmt   = extractSgmlTag(lines, j, QStringLiteral("TRNAMT"));
            j = i + 1;
            QString memo           = extractSgmlTag(lines, j, QStringLiteral("MEMO"));
            if (memo.isEmpty()) { j = i + 1; memo = extractSgmlTag(lines, j, QStringLiteral("NAME")); }

            const QDate  dt  = parseDate(dtPosted);
            const qint64 val = parseAmount(trnAmt, QLatin1Char('.'));
            if (val <= 0) continue;

            Compra c;
            c.nome = memo.isEmpty() ? QStringLiteral("OFX Import") : memo;
            c.valorTotal = val; c.numParcelas = 1; c.parcelaInicial = 1;
            c.mesInicio = dt.isValid() ? dt.month() : hoje.month();
            c.anoInicio = dt.isValid() ? dt.year()  : hoje.year();
            c.moeda = QStringLiteral("BRL");
            c.criadoEm = QDateTime::currentDateTime().toString(Qt::ISODate);
            Parcela p; p.numero = 1; p.valor = val;
            p.dataFatura = dt.isValid() ? dt.toString(QStringLiteral("dd/MM/yyyy")) : QString();
            r.compras.append(c); r.parcelas.append({p});
        }
    }

    if (r.compras.isEmpty())
        r.error = QStringLiteral("Nenhuma transação OFX encontrada no arquivo.");
    return r;
}

// ─── XLSX ─────────────────────────────────────────────────────────────────────

ImportService::Result ImportService::fromXlsx(const QString &path)
{
    // Delegate to CSV if QXlsx not available at runtime, but QXlsx IS linked.
    // Use QXlsx to read and convert to in-memory CSV-like structure.
    Result r;

#ifdef QXLSX_LIBRARY
    QXlsx::Document xlsx(path);
    if (!xlsx.load()) {
        r.error = QStringLiteral("Não foi possível abrir XLSX: %1").arg(path);
        return r;
    }

    QXlsx::Worksheet *ws = xlsx.currentWorksheet();
    if (!ws) { r.error = QStringLiteral("Planilha vazia."); return r; }

    const int maxRow = xlsx.dimension().lastRow();
    const int maxCol = xlsx.dimension().lastColumn();
    if (maxRow < 2) { r.error = QStringLiteral("XLSX sem dados."); return r; }

    // Read headers
    QStringList headers;
    for (int c = 1; c <= maxCol; ++c) {
        const QVariant v = xlsx.read(1, c);
        headers.append(v.isValid() ? v.toString().trimmed() : QString());
    }

    int colDate = -1, colDesc = -1, colAmt = -1;
    for (int i = 0; i < headers.size(); ++i) {
        const QString h = headers[i].toLower();
        if (colDate < 0 && (h.contains(QStringLiteral("data")) || h.contains(QStringLiteral("date")))) colDate = i;
        if (colDesc < 0 && (h.contains(QStringLiteral("descri")) || h.contains(QStringLiteral("memo")) || h.contains(QStringLiteral("histor")))) colDesc = i;
        if (colAmt  < 0 && (h.contains(QStringLiteral("valor")) || h.contains(QStringLiteral("amount")))) colAmt = i;
    }
    if (colDate < 0) colDate = 0;
    if (colDesc < 0) colDesc = 1;
    if (colAmt  < 0) colAmt  = maxCol - 1;

    const QDate hoje = QDate::currentDate();

    for (int row = 2; row <= maxRow; ++row) {
        const QVariant vDate = xlsx.read(row, colDate + 1);
        const QVariant vDesc = xlsx.read(row, colDesc + 1);
        const QVariant vAmt  = xlsx.read(row, colAmt  + 1);

        QDate dt = hoje;
        if (vDate.typeId() == QMetaType::QDate)
            dt = vDate.toDate();
        else if (vDate.typeId() == QMetaType::QDateTime)
            dt = vDate.toDateTime().date();
        else
            dt = parseDate(vDate.toString());

        const QString nom = vDesc.toString().trimmed();
        qint64 val = 0;
        if (vAmt.typeId() == QMetaType::Double || vAmt.typeId() == QMetaType::LongLong || vAmt.typeId() == QMetaType::Int)
            val = static_cast<qint64>(std::round(std::abs(vAmt.toDouble()) * 100.0));
        else {
            // #199 (QA-06): decimalSep NAO pode ser fixo em '.' — celulas de
            // planilha em pt-BR (ex.: ExportService::fmtBrl -> "1234,56") usam
            // virgula decimal. Com '.' fixo, parseAmount tratava a virgula
            // como separador de milhar e removia -> "123456" interpretado
            // como 123456,00, inflando o valor em 100x no round-trip.
            // detectDecimal (mesma heuristica do fromCsv) decide por celula.
            const QString raw = vAmt.toString();
            val = parseAmount(raw, detectDecimal(raw));
        }
        if (val <= 0) continue;

        Compra c;
        c.nome = nom.isEmpty() ? QStringLiteral("XLSX Import") : nom;
        c.valorTotal = val; c.numParcelas = 1; c.parcelaInicial = 1;
        c.mesInicio = dt.month(); c.anoInicio = dt.year();
        c.moeda = QStringLiteral("BRL");
        c.criadoEm = QDateTime::currentDateTime().toString(Qt::ISODate);
        Parcela p; p.numero = 1; p.valor = val;
        p.dataFatura = dt.toString(QStringLiteral("dd/MM/yyyy"));
        r.compras.append(c); r.parcelas.append({p});
    }
    if (r.compras.isEmpty())
        r.error = QStringLiteral("Nenhuma transação encontrada no XLSX.");
#else
    r.error = QStringLiteral("Suporte a XLSX não compilado nesta versão.");
#endif
    return r;
}

// ─── Match annotation (#84) ──────────────────────────────────────────────────

ImportService::AnnotationCounts ImportService::annotateMatches(
    Result &r, const QList<MatchData> &existing, int matchWindow)
{
    AnnotationCounts c;
    r.matches.clear();
    r.matches.reserve(r.compras.size());

    ExistingTransactionMatchFinder finder(existing);
    finder.setMatchWindow(matchWindow);

    for (int i = 0; i < r.compras.size(); ++i) {
        const Compra &cm = r.compras[i];
        MatchData input;
        input.amount = cm.valorTotal;
        input.payee  = cm.nome;
        input.date   = QDate(cm.anoInicio, cm.mesInicio, 1);
        // bankId/accountId nao sao preenchidos pelos parsers atuais; podem
        // entrar em #93 (parsers PDF por banco) e gerar Duplicate confiavel.

        const MatchResult mr = finder.find(input);
        r.matches.append(mr);
        switch (mr) {
        case MatchResult::NotFound:  ++c.notFound;  break;
        case MatchResult::Imprecise: ++c.imprecise; break;
        case MatchResult::Precise:   ++c.precise;   break;
        case MatchResult::Duplicate: ++c.duplicate; break;
        }
    }
    return c;
}

// ─── Stubs ────────────────────────────────────────────────────────────────────

ImportService::Result ImportService::fromQif (const QString &) { return stub(QStringLiteral("QIF"));      }
ImportService::Result ImportService::fromJson (const QString &) { return stub(QStringLiteral("JSON"));     }
ImportService::Result ImportService::fromOds  (const QString &) { return stub(QStringLiteral("ODS"));      }
ImportService::Result ImportService::fromTxt  (const QString &) { return stub(QStringLiteral("TXT/CNAB")); }
ImportService::Result ImportService::fromMd   (const QString &) { return stub(QStringLiteral("Markdown")); }
ImportService::Result ImportService::fromHtml (const QString &) { return stub(QStringLiteral("HTML"));     }
ImportService::Result ImportService::fromPdf(const QString &path)
{
    Result r;

#ifdef PEUBUD_HAS_POPPLER
    // Extracao de texto via Poppler-qt6
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
    if (!doc || doc->isLocked()) {
        r.error = QStringLiteral("Não foi possível abrir o PDF: %1").arg(path);
        return r;
    }
    QString texto;
    for (int i = 0; i < doc->numPages(); ++i) {
        std::unique_ptr<Poppler::Page> page(doc->page(i));
        if (page) texto += page->text(QRectF()) + QLatin1Char('\n');
    }
    if (texto.trimmed().isEmpty()) {
        r.error = QStringLiteral("PDF sem texto extraível (possivelmente escaneado).");
        return r;
    }

    // Pipeline de parsers em ordem de prioridade
    NubankPDFParser nu;   if (nu.detectar(texto))  return nu.parse(texto);
    ItauPDFParser   it;   if (it.detectar(texto))  return it.parse(texto);
    BradescoPDFParser br; if (br.detectar(texto))  return br.parse(texto);
    BBPDFParser     bb;   if (bb.detectar(texto))  return bb.parse(texto);
    CaixaPDFParser  cx;   if (cx.detectar(texto))  return cx.parse(texto);
    InterPDFParser  in;   if (in.detectar(texto))  return in.parse(texto);

    GenericoPDFParser ge;
    return ge.parse(texto);
#else
    Q_UNUSED(path)
    r.error = QStringLiteral(
        "Importação de PDF requer Poppler-qt6 (pacote poppler-qt6-devel).\n"
        "Esta build foi compilada sem suporte.\n"
        "Alternativa: exporte o extrato em OFX ou CSV.");
    return r;
#endif
}
