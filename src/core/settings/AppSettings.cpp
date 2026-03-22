#include "AppSettings.h"
#include "services/ParcelaService.h"

AppSettings &AppSettings::instance()
{
    static AppSettings s;
    return s;
}

AppSettings::AppSettings()
    : QObject(nullptr)
    , m_s(QStringLiteral("petrinhu"), QStringLiteral("PeuBud"))
{}

AppSettings::Language AppSettings::language() const
{
    return m_s.value(QStringLiteral("language"), QStringLiteral("pt-BR")).toString() == QStringLiteral("pt-BR")
           ? Language::PtBr : Language::EnIntl;
}

void AppSettings::setLanguage(Language lang)
{
    const QString code = (lang == Language::PtBr)
                         ? QStringLiteral("pt-BR") : QStringLiteral("en-US");
    if (m_s.value(QStringLiteral("language")).toString() == code) return;

    m_s.setValue(QStringLiteral("language"), code);

    // Apply currency default only if the user never explicitly chose one
    if (!m_s.contains(QStringLiteral("currency_manual"))) {
        const QString cur = (lang == Language::PtBr)
                            ? QStringLiteral("BRL") : QStringLiteral("USD");
        m_s.setValue(QStringLiteral("currency"), cur);
        emit currencyChanged(cur);
    }

    emit languageChanged(lang);
}

QString AppSettings::defaultCurrency() const
{
    return m_s.value(QStringLiteral("currency"), QStringLiteral("BRL")).toString();
}

void AppSettings::setDefaultCurrency(const QString &code)
{
    if (m_s.value(QStringLiteral("currency")).toString() == code) return;
    m_s.setValue(QStringLiteral("currency"), code);
    m_s.setValue(QStringLiteral("currency_manual"), true);
    emit currencyChanged(code);
}

QString AppSettings::currencySymbol() const
{
    return ParcelaService::simboloMoeda(defaultCurrency());
}
