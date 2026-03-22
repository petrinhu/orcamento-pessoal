#include "CurrencyService.h"

#include <cmath>
#include <stdexcept>

CurrencyService::CurrencyService(std::unique_ptr<ICurrencyProvider> provider)
    : m_provider(std::move(provider))
{}

void CurrencyService::ensureRates()
{
    m_rates = m_provider->fetchRates(QStringLiteral("BRL"));
}

bool CurrencyService::disponivel() const
{
    return m_provider->isAvailable();
}

double CurrencyService::taxa(const QString &de, const QString &para)
{
    ensureRates();
    if (de == para) return 1.0;

    // rates are relative to BRL, so:
    //   rate[X] = how many X per 1 BRL
    //   to convert de→para: value_para = value_de * rate[para] / rate[de]
    const double rDe  = m_rates.value(de,   1.0);
    const double rPara = m_rates.value(para, 1.0);
    if (rDe < 1e-12) throw std::runtime_error("Unknown source currency");
    return rPara / rDe;
}

qint64 CurrencyService::converter(qint64 centavos, const QString &de, const QString &para)
{
    if (de == para) return centavos;
    return static_cast<qint64>(std::round(static_cast<double>(centavos) * taxa(de, para)));
}

QList<QString> CurrencyService::moedas()
{
    ensureRates();
    return m_rates.keys();
}
