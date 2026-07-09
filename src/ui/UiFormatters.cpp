#include "UiFormatters.h"

#include <QLatin1Char>
#include <QRegularExpression>
#include <QStringLiteral>

#include <cmath>

namespace uifmt {

qint64 parseCentavos(const QString &txt)
{
    QString s = txt.trimmed();
    s.remove(QStringLiteral("R$")).remove(QLatin1Char(' '));
    // pt-BR: ponto = separador de milhar, vírgula = decimal.
    if (s.contains(QLatin1Char(',')) && s.contains(QLatin1Char('.'))) {
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    } else if (s.contains(QLatin1Char(','))) {
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    } else if (s.contains(QLatin1Char('.'))) {
        // #199 (QA-03): só ponto, sem vírgula. Dinheiro nunca tem 3 casas
        // decimais -- se o padrão é EXATAMENTE grupos de 3 dígitos após
        // ponto(s) ("1.234", "12.345.678"), é milhar, não decimal.
        static const QRegularExpression rxMilhar(
            QStringLiteral("^-?\\d{1,3}(\\.\\d{3})+$"));
        if (rxMilhar.match(s).hasMatch())
            s.remove(QLatin1Char('.'));
        // Senão (ex. "12.5", "1234.56"): mantém como decimal (comportamento
        // anterior, ponto já é o separador decimal esperado por toDouble).
    }
    bool ok = false;
    const double val = s.toDouble(&ok);
    return ok ? static_cast<qint64>(std::llround(val * 100.0)) : 0LL;
}

QString formatCentavos(qint64 cents)
{
    const qint64 abs_c = cents < 0 ? -cents : cents;
    return QStringLiteral("%1%2,%3")
        .arg(cents < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

QString formatCentavosBRL(qint64 cents)
{
    const qint64 abs_c = cents < 0 ? -cents : cents;
    return QStringLiteral("R$ %1%2,%3")
        .arg(cents < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

} // namespace uifmt
