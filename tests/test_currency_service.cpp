// PeuBud - CurrencyService TDD (#18 expandida).
// Usa ICurrencyProvider mock para nao depender de HTTP.

#include <QtTest/QtTest>
#include "services/CurrencyService.h"
#include "core/currency/ICurrencyProvider.h"

// Mock estatico: retorna QMap fixo.
class MockProvider : public ICurrencyProvider {
public:
    QMap<QString, double> ratesFixed;
    bool available = true;

    QMap<QString, double> fetchRates(const QString &) override
    {
        return ratesFixed;
    }
    bool isAvailable() const override { return available; }
};

class TestCurrencyService : public QObject
{
    Q_OBJECT
private slots:
    void taxaBrlParaUsd();
    void taxaUsdParaEur();
    void converterCentavos();
    void moedasListadas();
    void provedorIndisponivel();
};

void TestCurrencyService::taxaBrlParaUsd()
{
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.20); // 1 BRL = 0.20 USD
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    CurrencyService svc(std::move(mock));

    const double t = svc.taxa(QStringLiteral("BRL"), QStringLiteral("USD"));
    QVERIFY2(qAbs(t - 0.20) < 1e-6,
        qPrintable(QStringLiteral("taxa = %1, esperado 0.20").arg(t)));
}

void TestCurrencyService::taxaUsdParaEur()
{
    // BRL -> USD = 0.20; BRL -> EUR = 0.18
    // USD -> EUR = 0.18 / 0.20 = 0.90
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.20);
    mock->ratesFixed.insert(QStringLiteral("EUR"), 0.18);
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    CurrencyService svc(std::move(mock));

    const double t = svc.taxa(QStringLiteral("USD"), QStringLiteral("EUR"));
    QVERIFY2(qAbs(t - 0.90) < 1e-6,
        qPrintable(QStringLiteral("taxa = %1, esperado 0.90").arg(t)));
}

void TestCurrencyService::converterCentavos()
{
    // 10000 cents BRL (R$ 100) -> USD com taxa 0.20 -> 2000 cents (\$ 20)
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.20);
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    CurrencyService svc(std::move(mock));

    QCOMPARE(svc.converter(10000, QStringLiteral("BRL"),
                            QStringLiteral("USD")), qint64(2000));
}

void TestCurrencyService::moedasListadas()
{
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.20);
    mock->ratesFixed.insert(QStringLiteral("EUR"), 0.18);
    mock->ratesFixed.insert(QStringLiteral("GBP"), 0.16);
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    CurrencyService svc(std::move(mock));

    const auto m = svc.moedas();
    QVERIFY(m.contains(QStringLiteral("USD")));
    QVERIFY(m.contains(QStringLiteral("EUR")));
    QVERIFY(m.contains(QStringLiteral("GBP")));
    QVERIFY(m.size() >= 3);
}

void TestCurrencyService::provedorIndisponivel()
{
    auto mock = std::make_unique<MockProvider>();
    mock->available = false;
    CurrencyService svc(std::move(mock));
    QVERIFY(!svc.disponivel());
}

QTEST_GUILESS_MAIN(TestCurrencyService)
#include "test_currency_service.moc"
