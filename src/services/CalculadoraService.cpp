#include "CalculadoraService.h"
#include <cmath>

QString CalculadoraService::formatarDisplay(double valor, int decimais)
{
    if (decimais >= 0) {
        return QString::number(valor, 'f', decimais)
            .replace(QLatin1Char('.'), QLatin1Char(','));
    }
    // Auto: up to 10 decimal places, trim trailing zeros
    QString s = QString::number(valor, 'f', 10);
    if (s.contains(QLatin1Char('.'))) {
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    }
    return s.replace(QLatin1Char('.'), QLatin1Char(','));
}

double CalculadoraService::parsearDisplay(const QString &texto)
{
    QString s = texto.trimmed();
    // Remove currency symbols / percent signs
    s.remove(QLatin1Char('R')).remove(QLatin1Char('$')).remove(QLatin1Char('%'));
    s = s.trimmed();
    // If string contains comma: comma is decimal, dots are thousands separators
    if (s.contains(QLatin1Char(','))) {
        s.remove(QLatin1Char('.'));          // remove thousands separators
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    bool ok = false;
    const double v = s.toDouble(&ok);
    return ok ? v : 0.0;
}

qint64 CalculadoraService::toCentavos(double valor)
{
    return static_cast<qint64>(std::round(valor * 100.0));
}

double CalculadoraService::fromCentavos(qint64 centavos)
{
    return centavos / 100.0;
}
