#include "ParcelaService.h"

#include <cmath>
#include <stdexcept>
#include <QLocale>
#include <QRegularExpression>

// ─── Date helpers ────────────────────────────────────────────────────────────

QDate ParcelaService::dataParaParcela(const Compra &c, int numero)
{
    // numero is absolute (1..N). parcelaInicial maps to (mesInicio, anoInicio).
    // offset = numero - parcelaInicial  (can be negative for past parcelas)
    int offset = numero - c.parcelaInicial;

    int mes = c.mesInicio + offset;
    int ano = c.anoInicio;

    // Normalise month (handles negative offsets too)
    while (mes < 1)  { mes += 12; ano--; }
    while (mes > 12) { mes -= 12; ano++; }

    return QDate(ano, mes, 1);
}

QList<QDate> ParcelaService::gerarDatas(const Compra &c)
{
    QList<QDate> datas;
    datas.reserve(c.numParcelas);
    for (int i = 1; i <= c.numParcelas; ++i)
        datas.append(dataParaParcela(c, i));
    return datas;
}

// ─── Mode 2.1/2.3: sem juros ─────────────────────────────────────────────────

ResultadoFinanceiro ParcelaService::semJuros(const Compra &c)
{
    ResultadoFinanceiro r;
    const QList<QDate> datas = gerarDatas(c);

    // valorParcela: if valorTotal given, divide; else use stored parcela_inicial value
    // Here we rely on c.valorTotal > 0 XOR parcelas stored separately.
    // For generation: use valorTotal / numParcelas (integer centavos, remainder to last)
    qint64 base      = (c.valorTotal > 0) ? c.valorTotal / c.numParcelas : 0;
    qint64 remainder = (c.valorTotal > 0) ? c.valorTotal % c.numParcelas : 0;

    for (int i = 1; i <= c.numParcelas; ++i) {
        Parcela p;
        p.numero   = i;
        p.valor    = base + (i == c.numParcelas ? remainder : 0);
        p.dataFatura = datas[i-1].toString(QStringLiteral("dd/MM/yyyy"));
        r.parcelas.append(p);
    }

    r.jurosMensaisPct = 0.0;
    r.jurosAnuaisPct  = 0.0;
    r.jurosTotais     = 0;
    r.totalPago       = c.valorTotal;
    return r;
}

// ─── Mode 2.4: com juros mensais (Price table) ───────────────────────────────

ResultadoFinanceiro ParcelaService::comJurosMensais(const Compra &c, double jurosPct)
{
    ResultadoFinanceiro r;
    if (c.valorTotal <= 0 || c.numParcelas <= 0)
        throw std::invalid_argument("valorTotal e numParcelas são obrigatórios");

    const double i = jurosPct / 100.0;
    const QList<QDate> datas = gerarDatas(c);

    qint64 pmtCentavos = 0;
    if (i < 1e-9) {
        // Zero interest
        pmtCentavos = c.valorTotal / c.numParcelas;
    } else {
        // Price formula: PMT = PV * i / [1 - (1+i)^-n]
        double pmt = static_cast<double>(c.valorTotal) * i
                     / (1.0 - std::pow(1.0 + i, -c.numParcelas));
        pmtCentavos = static_cast<qint64>(std::round(pmt));
    }

    r.totalPago = pmtCentavos * c.numParcelas;
    r.jurosTotais = r.totalPago - c.valorTotal;
    r.jurosMensaisPct = jurosPct;
    r.jurosAnuaisPct  = (std::pow(1.0 + i, 12) - 1.0) * 100.0;

    for (int k = 1; k <= c.numParcelas; ++k) {
        Parcela p;
        p.numero     = k;
        p.valor      = pmtCentavos;
        p.dataFatura = datas[k-1].toString(QStringLiteral("dd/MM/yyyy"));
        r.parcelas.append(p);
    }
    return r;
}

// ─── Mode 2.5: infer interest ────────────────────────────────────────────────

ResultadoFinanceiro ParcelaService::inferirJuros(qint64 valorTotal,
                                                  int    numParcelas,
                                                  qint64 valorParcela)
{
    ResultadoFinanceiro r;
    const double i = resolverTaxa(valorTotal, valorParcela, numParcelas);

    r.jurosMensaisPct = i * 100.0;
    r.jurosAnuaisPct  = (std::pow(1.0 + i, 12) - 1.0) * 100.0;
    r.totalPago       = valorParcela * numParcelas;
    r.jurosTotais     = r.totalPago - valorTotal;

    // Parcelas not generated here — caller uses them for display only
    for (int k = 1; k <= numParcelas; ++k) {
        Parcela p;
        p.numero = k;
        p.valor  = valorParcela;
        r.parcelas.append(p);
    }
    return r;
}

// ─── Newton-Raphson ───────────────────────────────────────────────────────────

double ParcelaService::resolverTaxa(qint64 vp, qint64 pmt, int n)
{
    // Solves: vp = pmt * [1-(1+i)^-n]/i
    // Equivalent: f(i) = pmt * [1-(1+i)^-n]/i - vp = 0
    if (vp <= 0 || pmt <= 0 || n <= 0) return 0.0;

    const double VP  = static_cast<double>(vp);
    const double PMT = static_cast<double>(pmt);

    // Zero interest check
    if (std::abs(PMT * n - VP) < 0.01) return 0.0;

    double i = 0.01; // initial guess 1% /month
    for (int iter = 0; iter < 1000; ++iter) {
        const double factor = std::pow(1.0 + i, -n);
        const double f  = PMT * (1.0 - factor) / i - VP;
        const double df = PMT * (
            (1.0 - factor) / (-i * i)
            + static_cast<double>(n) * factor / (i * (1.0 + i))
        );

        if (std::abs(df) < 1e-12) break;
        const double di = f / df;
        i -= di;
        if (i <= 0) i = 1e-6;
        if (std::abs(di) < 1e-10) break;
    }
    return i;
}

// ─── Recalc after cell edit ───────────────────────────────────────────────────

QList<Parcela> ParcelaService::recalcularAposEdicao(
    const QList<Parcela> &parcelas,
    int indexParcela,
    qint64 novoValor,
    RecalcMode modo,
    qint64 valorTotal)
{
    QList<Parcela> result = parcelas;
    if (indexParcela < 0 || indexParcela >= result.size()) return result;

    const qint64 delta = novoValor - result[indexParcela].valor;
    result[indexParcela].valor     = novoValor;
    result[indexParcela].modificada = true;

    switch (modo) {
    case RecalcMode::SoEsta:
        break;

    case RecalcMode::RedistribuirDiff: {
        // Spread delta across non-modified parcelas after this one
        QList<int> targets;
        for (int k = indexParcela + 1; k < result.size(); ++k)
            if (!result[k].modificada) targets.append(k);
        if (!targets.isEmpty()) {
            const qint64 share     = delta / targets.size();
            const qint64 remainder = delta % targets.size();
            for (int k = 0; k < targets.size(); ++k)
                result[targets[k]].valor -= share + (k == 0 ? remainder : 0);
        }
        break;
    }

    case RecalcMode::IgualarTodas: {
        // Set ALL other parcelas to the same value, mark them modified,
        // and derive the new valorTotal as N × novoValor.
        for (int k = 0; k < result.size(); ++k) {
            result[k].valor     = novoValor;
            result[k].modificada = true;
        }
        break;
    }

    case RecalcMode::RecalcJuros: {
        if (valorTotal <= 0) break;
        // Infer new rate from modified parcela and redistribute
        const int remaining = result.size() - indexParcela - 1;
        if (remaining <= 0) break;
        qint64 paidSoFar = 0;
        for (int k = 0; k <= indexParcela; ++k)
            paidSoFar += result[k].valor;
        const qint64 vp = valorTotal - (paidSoFar - result[indexParcela].valor);
        if (vp <= 0) break;
        const double i = resolverTaxa(vp, novoValor, remaining + 1);
        if (i > 1e-9) {
            double pmt = static_cast<double>(vp) * i
                         / (1.0 - std::pow(1.0 + i, -(remaining + 1)));
            const qint64 pmtC = static_cast<qint64>(std::round(pmt));
            for (int k = indexParcela + 1; k < result.size(); ++k)
                result[k].valor = pmtC;
        }
        break;
    }
    }

    return result;
}

// ─── Formatting ──────────────────────────────────────────────────────────────

QString ParcelaService::formatarValor(qint64 centavos, const QString &moeda)
{
    const qint64 abs_c = std::abs(centavos);
    const QString sign = centavos < 0 ? QStringLiteral("-") : QString();
    const QString sym  = simboloMoeda(moeda);
    return QStringLiteral("%1%2 %3,%4")
        .arg(sign)
        .arg(sym)
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

qint64 ParcelaService::parsearValor(const QString &texto)
{
    // Accept "1.234,56" or "1234,56" or "1234.56"
    QString s = texto;
    s.remove(QRegularExpression(QStringLiteral("[^\\d,\\.]")));
    // If comma is decimal separator
    if (s.contains(QLatin1Char(',')) && s.contains(QLatin1Char('.'))) {
        // "1.234,56" → remove dot, replace comma
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    } else if (s.contains(QLatin1Char(','))) {
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    bool ok = false;
    double v = s.toDouble(&ok);
    if (!ok) return 0;
    return static_cast<qint64>(std::round(v * 100.0));
}

QString ParcelaService::simboloMoeda(const QString &moeda)
{
    static const QMap<QString, QString> symbols = {
        { QStringLiteral("BRL"), QStringLiteral("R$")  },
        { QStringLiteral("USD"), QStringLiteral("$")   },
        { QStringLiteral("EUR"), QStringLiteral("€")   },
        { QStringLiteral("GBP"), QStringLiteral("£")   },
        { QStringLiteral("JPY"), QStringLiteral("¥")   },
        { QStringLiteral("CAD"), QStringLiteral("CA$") },
        { QStringLiteral("AUD"), QStringLiteral("A$")  },
        { QStringLiteral("CHF"), QStringLiteral("Fr")  },
        { QStringLiteral("CNY"), QStringLiteral("¥")   },
        { QStringLiteral("HKD"), QStringLiteral("HK$") },
        { QStringLiteral("SGD"), QStringLiteral("S$")  },
        { QStringLiteral("KRW"), QStringLiteral("₩")   },
        { QStringLiteral("INR"), QStringLiteral("₹")   },
        { QStringLiteral("MXN"), QStringLiteral("MX$") },
        { QStringLiteral("ARS"), QStringLiteral("$")   },
        { QStringLiteral("CLP"), QStringLiteral("$")   },
        { QStringLiteral("COP"), QStringLiteral("$")   },
        { QStringLiteral("PEN"), QStringLiteral("S/")  },
        { QStringLiteral("UYU"), QStringLiteral("$U")  },
        { QStringLiteral("BOB"), QStringLiteral("Bs")  },
        { QStringLiteral("PYG"), QStringLiteral("\u20b2") },
        { QStringLiteral("GTQ"), QStringLiteral("Q")   },
        { QStringLiteral("NOK"), QStringLiteral("kr")  },
        { QStringLiteral("SEK"), QStringLiteral("kr")  },
        { QStringLiteral("DKK"), QStringLiteral("kr")  },
        { QStringLiteral("PLN"), QStringLiteral("z\u0142") },
        { QStringLiteral("CZK"), QStringLiteral("K\u010d") },
        { QStringLiteral("TRY"), QStringLiteral("\u20ba") },
        { QStringLiteral("RUB"), QStringLiteral("\u20bd") },
        { QStringLiteral("ZAR"), QStringLiteral("R")   },
    };
    return symbols.value(moeda, moeda);
}
