#include "UiFormatters.h"

#include <QLatin1Char>
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
