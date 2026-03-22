#pragma once

#include <QString>
#include <QMap>

// Interface for currency exchange rate providers (DIP).
class ICurrencyProvider {
public:
    virtual ~ICurrencyProvider() = default;

    // Returns rates relative to baseCurrency (e.g., {"USD": 5.10, "EUR": 5.55})
    // Implementations should cache results to avoid repeated HTTP calls.
    virtual QMap<QString, double> fetchRates(const QString &baseCurrency = QStringLiteral("BRL")) = 0;
    virtual bool isAvailable() const = 0;
};
