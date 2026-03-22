#pragma once

#include "ICurrencyProvider.h"
#include <QDateTime>
#include <QNetworkAccessManager>

// Fetches ECB exchange rates from api.frankfurter.app (free, no API key).
// Rates are cached for the calendar day.
class FrankfurterProvider final : public QObject, public ICurrencyProvider {
    Q_OBJECT
public:
    explicit FrankfurterProvider(QObject *parent = nullptr);

    QMap<QString, double> fetchRates(const QString &baseCurrency = QStringLiteral("BRL")) override;
    bool isAvailable() const override;

private:
    QNetworkAccessManager  m_nam;
    QMap<QString, double>  m_cache;
    QString                m_cachedBase;
    QDate                  m_cacheDate;

    QMap<QString, double> fetchSync(const QString &base);
};
