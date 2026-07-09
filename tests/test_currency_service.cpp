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
    void moedaDestinoAusenteLancaExcecao();
    void moedaOrigemAusenteLancaExcecao();
    void taxaOrigemZeroLancaExcecao();
    void idaVoltaPreservaComArredondamento();
    // ── QA-02 / #189: aritmetica inteira, sem perda de centavo em double ────
    void converterEmpateExatoDeCentavoArrendondaHalfUp();
    void converterValorGrandeUsaWideningSemOverflow();
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

// ── Bordas (#132 / #156): protecao contra divisao por zero / moeda ausente ───

void TestCurrencyService::moedaDestinoAusenteLancaExcecao()
{
    // Moeda destino (JPY) NAO esta no provider. Antes do fix (#156),
    // taxa() usava value(para, 1.0) e fabricava uma taxa 1.0 falsa,
    // silenciosamente. Comportamento correto: lancar, nunca fabricar.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);   // 1 BRL = 1 BRL
    // JPY ausente de proposito.
    CurrencyService svc(std::move(mock));

    bool throwed = false;
    try {
        svc.taxa(QStringLiteral("BRL"), QStringLiteral("JPY"));
    } catch (const std::runtime_error &) {
        throwed = true;
    }
    QVERIFY2(throwed, "moeda destino ausente deveria lancar runtime_error (#156)");
}

void TestCurrencyService::moedaOrigemAusenteLancaExcecao()
{
    // Moeda origem ausente do mapa do provider. Antes do fix (#156),
    // value(de, 1.0) devolvia 1.0 e nao disparava o guard (que so travava
    // rDe < 1e-12). Comportamento correto: lancar, nunca fabricar taxa 1.0.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.20);
    CurrencyService svc(std::move(mock));

    bool throwed = false;
    try {
        svc.taxa(QStringLiteral("XAU"), QStringLiteral("USD"));
    } catch (const std::runtime_error &) {
        throwed = true;
    }
    QVERIFY2(throwed, "moeda origem ausente deveria lancar runtime_error (#156)");
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
    // Conversao ida-volta A->B->A com aritmetica inteira (QA-02/#189):
    // half-up half-up cancelam exatamente para este par de valores/taxa
    // (provado a mao: BRL->USD arredonda 23827.008 p/ baixo=23827;
    // USD->BRL arredonda 123455.9586 p/ cima=123456). Asserção EXATA —
    // a tolerancia +-1 antiga MASCARAVA a perda de centavo do double.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    mock->ratesFixed.insert(QStringLiteral("USD"), 0.193); // taxa "feia" de proposito
    CurrencyService svc(std::move(mock));

    const qint64 original = 123456; // R$ 1.234,56
    const qint64 emUsd = svc.converter(original, QStringLiteral("BRL"), QStringLiteral("USD"));
    const qint64 deVolta = svc.converter(emUsd, QStringLiteral("USD"), QStringLiteral("BRL"));

    QCOMPARE(deVolta, original);
}

void TestCurrencyService::converterEmpateExatoDeCentavoArrendondaHalfUp()
{
    // QA-02: 38075 centavos x taxa 2.78 = 105848.50 EXATO (empate de meio
    // centavo). O bug antigo (std::round sobre double) perdia esse centavo
    // em ~32% dos empates por imprecisao binaria (2.78 nao e representavel
    // exatamente em double). Com aritmetica inteira (micro-unidades +
    // __int128 + half-up explicito) o resultado e' deterministico: 105849.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    mock->ratesFixed.insert(QStringLiteral("XXX"), 2.78);
    CurrencyService svc(std::move(mock));

    QCOMPARE(svc.converter(38075, QStringLiteral("BRL"), QStringLiteral("XXX")),
              qint64(105849));
}

void TestCurrencyService::converterValorGrandeUsaWideningSemOverflow()
{
    // Prova a necessidade do widening p/ __int128: centavos * taxaMicro
    // excede o range de int64 (9.2e18) para valores grandes combinados com
    // taxa de moeda fraca. R$ 90 bilhoes (9e12 centavos) x taxa 5000 ->
    // produto intermediario ~4.5e19 (> INT64_MAX) mas resultado final
    // (4.5e16) cabe em qint64 — a conta so funciona se o produto
    // intermediario nao estourar antes da divisao.
    auto mock = std::make_unique<MockProvider>();
    mock->ratesFixed.insert(QStringLiteral("BRL"), 1.0);
    mock->ratesFixed.insert(QStringLiteral("WEAK"), 5000.0);
    CurrencyService svc(std::move(mock));

    const qint64 centavos = 9'000'000'000'000LL; // R$ 90 bilhoes
    QCOMPARE(svc.converter(centavos, QStringLiteral("BRL"), QStringLiteral("WEAK")),
              qint64(45'000'000'000'000'000LL));
}

QTEST_GUILESS_MAIN(TestCurrencyService)
#include "test_currency_service.moc"
