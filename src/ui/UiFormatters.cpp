#include "UiFormatters.h"

#include "core/settings/AppSettings.h"
#include "services/ParcelaService.h"

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
    // #197 (I18N-01) originalmente tinha a lógica inteira/locale-aware
    // duplicada aqui. #204 achou o mesmo padrão hardcoded 2ª vez em
    // ParcelaService::formatarValor (que já precisa aceitar moeda EXPLÍCITA
    // por chamador -- uma Compra pode estar em moeda != moeda padrão do
    // app). Para não duplicar a aritmética/switch(language()) duas vezes,
    // este vira um wrapper fino: a moeda ativa é só mais um caso particular
    // de "moeda explícita" (a moeda padrão do app).
    return ParcelaService::formatarValor(cents, AppSettings::instance().defaultCurrency());
}

QString simboloMoedaAtiva()
{
    return ParcelaService::simboloMoeda(AppSettings::instance().defaultCurrency());
}

QString dataDisplayFormat()
{
    return AppSettings::instance().language() == AppSettings::Language::EnIntl
        ? QStringLiteral("MM/dd/yyyy")
        : QStringLiteral("dd/MM/yyyy");
}

QString dataDisplayFormatCurta()
{
    return AppSettings::instance().language() == AppSettings::Language::EnIntl
        ? QStringLiteral("MM/dd")
        : QStringLiteral("dd/MM");
}

QString dataDisplayFormatAnoCurto()
{
    return AppSettings::instance().language() == AppSettings::Language::EnIntl
        ? QStringLiteral("MM/dd/yy")
        : QStringLiteral("dd/MM/yy");
}

QString mesAnoDisplayFormat()
{
    // Ordem mês/ano não é ambígua entre locales (ao contrário de dia/mês) --
    // centralizado mesmo assim para ter 1 único ponto de verdade.
    return QStringLiteral("MM/yyyy");
}

} // namespace uifmt
