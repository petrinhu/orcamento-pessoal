// PeuBud - CurrencyService TDD (#18 expandida).
// Usa ICurrencyProvider mock para nao depender de HTTP.

#include <QtTest/QtTest>
#include <stdexcept>
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
    // ── Bordas (#132) ───────────────────────────────────────────────────────
    void moedaDestinoAusenteUsaFallbackUm();
    void moedaOrigemAusenteLancaExcecao();
    void taxaOrigemZeroLancaExcecao();
    void idaVoltaPreservaComArredondamento();
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

// ── Bordas (#132): protecao contra divisao por zero / moeda ausente ──────────

void TestCurrencyService::moedaDestinoAusenteUsaFallbackUm()
{
    // Moeda destino (JPY) NAO esta no provider. taxa() usa value(para, 1.0),
    // entao retorna rPara=1.0 / rDe. FIXA o comportamento REAL: nao lanca,
    // nao divide por zero — usa fallback 1.0 para o destino.
    // ACHADO documentado: destino ausente vira taxa "silenciosa" (=1/rDe),
    // sem sinalizar erro ao chamador. Ver relatorio.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);   // 1 BRL = 1 BRL
    // JPY ausente de proposito.
    CurrencyService svc(std::move(mock));

    const double t = svc.taxa(QStringLiteral("BRL"), QStringLiteral("JPY"));
    QVERIFY2(qAbs(t - 1.0) < 1e-9,
        qPrintable(QStringLiteral("destino ausente: taxa=%1, esperado fallback 1.0").arg(t)));
}

void TestCurrencyService::moedaOrigemAusenteLancaExcecao()
{
    // Moeda origem ausente -> value(de, 1.0) = 1.0, NAO dispara o guard
    // (guard so trava se rDe < 1e-12). Logo origem ausente tambem cai no
    // fallback 1.0 e NAO lanca. FIXA o comportamento REAL.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.20);
    CurrencyService svc(std::move(mock));

    // origem "XAU" ausente -> rDe=1.0; destino USD=0.20 -> taxa=0.20.
    const double t = svc.taxa(QStringLiteral("XAU"), QStringLiteral("USD"));
    QVERIFY2(qAbs(t - 0.20) < 1e-9,
        qPrintable(QStringLiteral("origem ausente: taxa=%1, esperado 0.20 (fallback)").arg(t)));
}

void TestCurrencyService::taxaOrigemZeroLancaExcecao()
{
    // Se o provider devolve a moeda origem com taxa exatamente 0, o guard
    // rDe < 1e-12 dispara e lanca std::runtime_error (evita divisao por zero).
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    mock->ratesFixed.insert(QStringLiteral("ZZZ"), 0.0);   // taxa zero explicita
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.20);
    CurrencyService svc(std::move(mock));

    bool throwed = false;
    try {
        svc.taxa(QStringLiteral("ZZZ"), QStringLiteral("USD"));
    } catch (const std::runtime_error &) {
        throwed = true;
    }
    QVERIFY2(throwed, "taxa com origem=0 deveria lancar runtime_error (sem divisao por zero)");
}

void TestCurrencyService::idaVoltaPreservaComArredondamento()
{
    // Conversao ida-volta A->B->A: por arredondamento de centavos, o valor
    // pode oscilar +-1 centavo. Tolerancia DOCUMENTADA: |delta| <= 1 cent.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.193); // taxa "feia" de proposito
    CurrencyService svc(std::move(mock));

    const qint64 original = 123456; // R$ 1.234,56
    const qint64 emUsd = svc.converter(original, QStringLiteral("BRL"), QStringLiteral("USD"));
    const qint64 deVolta = svc.converter(emUsd, QStringLiteral("USD"), QStringLiteral("BRL"));

    QVERIFY2(qAbs(deVolta - original) <= 1,
        qPrintable(QStringLiteral("ida-volta: original=%1, voltou=%2 (delta=%3, tol=1 cent)")
            .arg(original).arg(deVolta).arg(deVolta - original)));
}

QTEST_GUILESS_MAIN(TestCurrencyService)
#include "test_currency_service.moc"
