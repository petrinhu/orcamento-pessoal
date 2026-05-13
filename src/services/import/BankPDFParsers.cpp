#include "BankPDFParsers.h"

#include <QDate>
#include <QDateTime>
#include <QRegularExpression>
#include <QString>
#include <cmath>

namespace {

// Helper: converte "1.234,56" (BR) ou "1234.56" (US) em centavos.
qint64 parseCentavos(const QString &raw)
{
    QString s = raw.trimmed();
    s.remove(QRegularExpression(QStringLiteral("[R$\\s+]")));
    bool neg = s.contains(QLatin1Char('-'));
    s.remove(QLatin1Char('-'));
    if (s.contains(QLatin1Char(','))) {
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    bool ok = false;
    const double v = s.toDouble(&ok);
    if (!ok) return 0;
    qint64 cents = static_cast<qint64>(std::round(std::abs(v) * 100.0));
    return neg ? -cents : cents;
}

QDate parseDate(const QString &s)
{
    static const QStringList fmts = {
        QStringLiteral("dd/MM/yyyy"),
        QStringLiteral("d/M/yyyy"),
        QStringLiteral("dd/MM/yy"),
        QStringLiteral("yyyy-MM-dd"),
    };
    for (const auto &f : fmts) {
        const QDate d = QDate::fromString(s.trimmed(), f);
        if (d.isValid()) return d;
    }
    return {};
}

// Cria Compra+Parcela a partir de tupla (data, descricao, valor centavos).
void appendTransacao(ImportService::Result &r,
                     const QDate &d, const QString &desc, qint64 cents)
{
    if (cents == 0 || !d.isValid()) return;
    Compra c;
    c.nome           = desc.trimmed().isEmpty() ? QStringLiteral("PDF Import") : desc.trimmed();
    c.valorTotal     = std::abs(cents);
    c.numParcelas    = 1;
    c.parcelaInicial = 1;
    c.mesInicio      = d.month();
    c.anoInicio      = d.year();
    c.moeda          = QStringLiteral("BRL");
    c.criadoEm       = QDateTime::currentDateTime().toString(Qt::ISODate);

    Parcela p;
    p.numero     = 1;
    p.valor      = std::abs(cents);
    p.dataFatura = d.toString(QStringLiteral("dd/MM/yyyy"));

    r.compras.append(c);
    r.parcelas.append({p});
}

// Linha generica: dd/MM/yyyy <desc> [+|-]?R$? 1.234,56
ImportService::Result parseGenericoLines(const QString &texto)
{
    ImportService::Result r;
    static const QRegularExpression rx(
        QStringLiteral("(\\d{2}/\\d{2}/\\d{2,4})\\s+(.+?)\\s+"
                       "(-?R?\\$?\\s*[\\d.]+,\\d{2})"),
        QRegularExpression::MultilineOption);
    auto it = rx.globalMatch(texto);
    while (it.hasNext()) {
        auto m = it.next();
        const QDate d  = parseDate(m.captured(1));
        const QString desc = m.captured(2).simplified();
        const qint64 cents = parseCentavos(m.captured(3));
        appendTransacao(r, d, desc, cents);
    }
    if (r.compras.isEmpty()) {
        r.error = QStringLiteral("Nenhuma transação reconhecida no PDF.");
    }
    return r;
}

} // namespace

// ── Nubank ──────────────────────────────────────────────────────────────────
// Sinal: "Nubank" + "Fatura" ou "Pagamento" no cabecalho.
bool NubankPDFParser::detectar(const QString &t) const
{
    return t.contains(QStringLiteral("Nubank"), Qt::CaseInsensitive)
        || t.contains(QStringLiteral("nu pagamentos"), Qt::CaseInsensitive);
}

ImportService::Result NubankPDFParser::parse(const QString &t) const
{
    // Layout fatura cartao Nubank:
    //   DD MMM <descricao>  R$ X.XXX,XX
    //   ex: "07 MAI  SUPERMERCADO XYZ  R$ 152,30"
    ImportService::Result r;
    static const QStringList meses = {
        QStringLiteral("jan"), QStringLiteral("fev"), QStringLiteral("mar"),
        QStringLiteral("abr"), QStringLiteral("mai"), QStringLiteral("jun"),
        QStringLiteral("jul"), QStringLiteral("ago"), QStringLiteral("set"),
        QStringLiteral("out"), QStringLiteral("nov"), QStringLiteral("dez"),
    };
    static const QRegularExpression rx(
        QStringLiteral("(\\d{1,2})\\s+(jan|fev|mar|abr|mai|jun|jul|ago|set|out|nov|dez)\\s+"
                       "(.+?)\\s+R?\\$?\\s*([\\d.]+,\\d{2})"),
        QRegularExpression::CaseInsensitiveOption);
    auto it = rx.globalMatch(t);
    const int anoAtual = QDate::currentDate().year();
    while (it.hasNext()) {
        auto m = it.next();
        const int dia  = m.captured(1).toInt();
        const int mes  = meses.indexOf(m.captured(2).toLower()) + 1;
        if (mes < 1) continue;
        const QDate d(anoAtual, mes, dia);
        const QString desc = m.captured(3).simplified();
        const qint64 cents = parseCentavos(m.captured(4));
        appendTransacao(r, d, desc, cents);
    }
    if (r.compras.isEmpty()) {
        // Cai para parser generico
        return parseGenericoLines(t);
    }
    return r;
}

// ── Itau ────────────────────────────────────────────────────────────────────
bool ItauPDFParser::detectar(const QString &t) const
{
    return t.contains(QStringLiteral("Itau"), Qt::CaseInsensitive)
        || t.contains(QStringLiteral("Itaú"), Qt::CaseInsensitive)
        || t.contains(QStringLiteral("BANCO ITAU"), Qt::CaseInsensitive);
}

ImportService::Result ItauPDFParser::parse(const QString &t) const
{
    // Layout extrato Itau: "dd/MM <descricao> <valor> [D|C]"
    ImportService::Result r;
    static const QRegularExpression rx(
        QStringLiteral("(\\d{2}/\\d{2})\\s+(.+?)\\s+([\\d.]+,\\d{2})\\s*([DC])?"),
        QRegularExpression::MultilineOption);
    auto it = rx.globalMatch(t);
    const int anoAtual = QDate::currentDate().year();
    while (it.hasNext()) {
        auto m = it.next();
        const QStringList dm = m.captured(1).split(QLatin1Char('/'));
        if (dm.size() != 2) continue;
        const QDate d(anoAtual, dm[1].toInt(), dm[0].toInt());
        const QString desc = m.captured(2).simplified();
        qint64 cents = parseCentavos(m.captured(3));
        if (m.captured(4) == QLatin1String("D")) cents = -cents;
        appendTransacao(r, d, desc, cents);
    }
    if (r.compras.isEmpty()) return parseGenericoLines(t);
    return r;
}

// ── Bradesco ────────────────────────────────────────────────────────────────
bool BradescoPDFParser::detectar(const QString &t) const
{
    return t.contains(QStringLiteral("Bradesco"), Qt::CaseInsensitive);
}

ImportService::Result BradescoPDFParser::parse(const QString &t) const
{
    return parseGenericoLines(t);
}

// ── BB ──────────────────────────────────────────────────────────────────────
bool BBPDFParser::detectar(const QString &t) const
{
    return t.contains(QStringLiteral("Banco do Brasil"), Qt::CaseInsensitive)
        || t.contains(QStringLiteral("BB.COM.BR"), Qt::CaseInsensitive);
}

ImportService::Result BBPDFParser::parse(const QString &t) const
{
    return parseGenericoLines(t);
}

// ── Caixa ───────────────────────────────────────────────────────────────────
bool CaixaPDFParser::detectar(const QString &t) const
{
    return t.contains(QStringLiteral("Caixa Econ"), Qt::CaseInsensitive)
        || t.contains(QStringLiteral("CEF "), Qt::CaseInsensitive);
}

ImportService::Result CaixaPDFParser::parse(const QString &t) const
{
    return parseGenericoLines(t);
}

// ── Inter ───────────────────────────────────────────────────────────────────
bool InterPDFParser::detectar(const QString &t) const
{
    return t.contains(QStringLiteral("Banco Inter"), Qt::CaseInsensitive)
        || t.contains(QStringLiteral("inter.co"), Qt::CaseInsensitive);
}

ImportService::Result InterPDFParser::parse(const QString &t) const
{
    return parseGenericoLines(t);
}

// ── Generico ────────────────────────────────────────────────────────────────
ImportService::Result GenericoPDFParser::parse(const QString &t) const
{
    return parseGenericoLines(t);
}
