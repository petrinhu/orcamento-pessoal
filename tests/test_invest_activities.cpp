// PeuBud - TDD InvestActivities + Factory (#89)
// Testa Activity pattern para operacoes de investimento sem dependencia de DB.

#include <QtTest/QtTest>

#include "models/LancamentoInvestimento.h"
#include "services/investimento/IInvestActivity.h"
#include "services/investimento/InvestActivities.h"
#include "services/investimento/InvestActivityFactory.h"

class TestInvestActivities : public QObject
{
    Q_OBJECT
private slots:
    void factoryCriaTodasDezPorRawType();
    void factoryEnumBuyMapeiaBuy();
    void factoryRawDividendMapeiaTipoDividend();
    void buyCamposSemFatorSplit();
    void splitCamposComFatorSplit();
    void buyConsistencyTickerVazioFalha();
    void buyConsistencyQuantidadeZeroFalha();
    void buyValidoSemErros();
    void listAllRetornaPeloMenosDez();
    void factoryRawInvalidoRetornaNullptr();
    // Extras: cobertura mais completa.
    void sellValidoSemErros();
    void dividendoSemValorFalha();
    void yieldExibeJuros();
    void splitFatorZeroFalha();
    void addSharesAceitaPrecoZero();
};

static LancamentoInvestimento makeLanc(const QString &ticker,
                                       double qty,
                                       qint64 preco,
                                       qint64 valor)
{
    LancamentoInvestimento l;
    l.ticker        = ticker;
    l.quantidade    = qty;
    l.precoUnitario = preco;
    l.valor         = valor;
    return l;
}

void TestInvestActivities::factoryCriaTodasDezPorRawType()
{
    const QStringList raws = {
        QStringLiteral("buy"), QStringLiteral("sell"),
        QStringLiteral("dividend"), QStringLiteral("reinvest"),
        QStringLiteral("yield"), QStringLiteral("add"),
        QStringLiteral("remove"), QStringLiteral("split"),
        QStringLiteral("interest"), QStringLiteral("bonus")
    };
    QCOMPARE(raws.size(), 10);
    for (const QString &raw : raws) {
        auto act = InvestActivityFactory::create(raw);
        QVERIFY2(act != nullptr,
                 qPrintable(QStringLiteral("rawType nao mapeado: ") + raw));
        QCOMPARE(act->rawType(), raw);
    }
}

void TestInvestActivities::factoryEnumBuyMapeiaBuy()
{
    auto act = InvestActivityFactory::create(IInvestActivity::Tipo::BuyShares);
    QVERIFY(act != nullptr);
    QCOMPARE(act->rawType(), QStringLiteral("buy"));
    QCOMPARE(act->type(), IInvestActivity::Tipo::BuyShares);
}

void TestInvestActivities::factoryRawDividendMapeiaTipoDividend()
{
    auto act = InvestActivityFactory::create(QStringLiteral("dividend"));
    QVERIFY(act != nullptr);
    QCOMPARE(act->type(), IInvestActivity::Tipo::Dividend);
}

void TestInvestActivities::buyCamposSemFatorSplit()
{
    invest::BuySharesActivity buy;
    auto c = buy.camposVisiveis();
    QCOMPARE(c.fatorSplit, false);
    QCOMPARE(c.ticker, true);
    QCOMPARE(c.quantidade, true);
    QCOMPARE(c.precoUnitario, true);
}

void TestInvestActivities::splitCamposComFatorSplit()
{
    invest::SplitSharesActivity split;
    auto c = split.camposVisiveis();
    QCOMPARE(c.fatorSplit, true);
    QCOMPARE(c.precoUnitario, false);
    QCOMPARE(c.valor, false);
}

void TestInvestActivities::buyConsistencyTickerVazioFalha()
{
    invest::BuySharesActivity buy;
    auto l = makeLanc(QString(), 100.0, 2500, 250000);
    QStringList erros = buy.consistencyCheck(l);
    QVERIFY(!erros.isEmpty());
}

void TestInvestActivities::buyConsistencyQuantidadeZeroFalha()
{
    invest::BuySharesActivity buy;
    auto l = makeLanc(QStringLiteral("PETR4"), 0.0, 2500, 0);
    QStringList erros = buy.consistencyCheck(l);
    QVERIFY(!erros.isEmpty());
}

void TestInvestActivities::buyValidoSemErros()
{
    invest::BuySharesActivity buy;
    auto l = makeLanc(QStringLiteral("PETR4"), 100.0, 2500, 250000);
    QStringList erros = buy.consistencyCheck(l);
    QVERIFY2(erros.isEmpty(), qPrintable(erros.join(QStringLiteral("; "))));
}

void TestInvestActivities::listAllRetornaPeloMenosDez()
{
    auto pares = InvestActivityFactory::listAll();
    QVERIFY(pares.size() >= 10);
    // Cada entrada deve ter raw e label nao-vazios.
    for (const auto &p : pares) {
        QVERIFY(!p.first.isEmpty());
        QVERIFY(!p.second.isEmpty());
    }
}

void TestInvestActivities::factoryRawInvalidoRetornaNullptr()
{
    auto act = InvestActivityFactory::create(QStringLiteral("invalido"));
    QVERIFY(act == nullptr);
}

void TestInvestActivities::sellValidoSemErros()
{
    invest::SellSharesActivity sell;
    auto l = makeLanc(QStringLiteral("VALE3"), 50.0, 0, 0);
    QStringList erros = sell.consistencyCheck(l);
    QVERIFY2(erros.isEmpty(), qPrintable(erros.join(QStringLiteral("; "))));
}

void TestInvestActivities::dividendoSemValorFalha()
{
    invest::DividendActivity div;
    auto l = makeLanc(QStringLiteral("ITUB4"), 0.0, 0, 0);
    QStringList erros = div.consistencyCheck(l);
    QVERIFY(!erros.isEmpty());
}

void TestInvestActivities::yieldExibeJuros()
{
    invest::YieldActivity y;
    auto c = y.camposVisiveis();
    QCOMPARE(c.jurosOuTaxa, true);
    QCOMPARE(c.quantidade, false);
}

void TestInvestActivities::splitFatorZeroFalha()
{
    invest::SplitSharesActivity sp;
    auto l = makeLanc(QStringLiteral("MGLU3"), 0.0, 0, 0);
    QStringList erros = sp.consistencyCheck(l);
    QVERIFY(!erros.isEmpty());
}

void TestInvestActivities::addSharesAceitaPrecoZero()
{
    invest::AddSharesActivity add;
    auto l = makeLanc(QStringLiteral("BBSE3"), 10.0, 0, 0);
    QStringList erros = add.consistencyCheck(l);
    QVERIFY2(erros.isEmpty(), qPrintable(erros.join(QStringLiteral("; "))));
}

QTEST_APPLESS_MAIN(TestInvestActivities)
#include "test_invest_activities.moc"
