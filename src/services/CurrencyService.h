#pragma once

#include "core/currency/ICurrencyProvider.h"

#include <QMap>
#include <QString>
#include <memory>

// Converts values between currencies using a provider (DIP).
// Caches rates; refreshes at most once per calendar day.
class CurrencyService {
public:
    // Singleton initialized with FrankfurterProvider (lazy, safe to call after QApplication).
    static CurrencyService &instance();

    explicit CurrencyService(std::unique_ptr<ICurrencyProvider> provider);

    // Convert centavos from one currency to another.
    // Returns converted centavos (rounded).
    qint64 converter(qint64 centavos, const QString &de, const QString &para);

    // Rate de → para (e.g., 1 BRL = X USD)
    double taxa(const QString &de, const QString &para);

    // All supported currencies (from last fetch)
    QList<QString> moedas();

    bool disponivel() const;

private:
    std::unique_ptr<ICurrencyProvider> m_provider;
    QMap<QString, double>              m_rates;   // relative to BRL
    void ensureRates();
};
