// PeuBud - AjusteCorporativoService TDD (#65)
// Testa apenas funcao pura fatorCumulativo (sem dependencia de DB).

#include <QtTest/QtTest>
#include "services/AjusteCorporativoService.h"

class TestAjustes : public QObject
{
    Q_OBJECT
private slots:
    void semEventosFator1();
    void splitTriploAumentaQty();
    void inplitDiminuiQty();
    void multiplosEventosCumulativo();
    void eventosForaDoIntervaloIgnorados();
    void tickerDiferenteIgnorado();
};

static AjusteCorporativo make(const QString &ticker, const QString &tipo,
                              const QDate &data, double fator)
{
    AjusteCorporativo a;
    a.ticker = ticker;
    a.tipo   = tipo;
    a.dataEx = data;
    a.fator  = fator;
    return a;
}

void TestAjustes::semEventosFator1()
{
    QCOMPARE(AjusteCorporativoService::fatorCumulativo(
        QStringLiteral("PETR4"),
        QDate(2020, 1, 1), QDate(2026, 1, 1), {}), 1.0);
}

void TestAjustes::splitTriploAumentaQty()
{
    QList<AjusteCorporativo> ev = {
        make(QStringLiteral("PETR4"), QStringLiteral("split"),
             QDate(2022, 5, 10), 3.0)
    };
    // Compra antes do split deve ser ajustada * 3
    QCOMPARE(AjusteCorporativoService::fatorCumulativo(
        QStringLiteral("PETR4"),
        QDate(2020, 1, 1), QDate(2026, 1, 1), ev), 3.0);
}

void TestAjustes::inplitDiminuiQty()
{
    QList<AjusteCorporativo> ev = {
        make(QStringLiteral("MGLU3"), QStringLiteral("inplit"),
             QDate(2023, 8, 15), 0.1)
    };
    QCOMPARE(AjusteCorporativoService::fatorCumulativo(
        QStringLiteral("MGLU3"),
        QDate(2020, 1, 1), QDate(2026, 1, 1), ev), 0.1);
}

void TestAjustes::multiplosEventosCumulativo()
{
    QList<AjusteCorporativo> ev = {
        make(QStringLiteral("VALE3"), QStringLiteral("split"),
             QDate(2021, 3, 1), 2.0),
        make(QStringLiteral("VALE3"), QStringLiteral("bonificacao"),
             QDate(2023, 6, 1), 1.1),
    };
    // 2 eventos: 2.0 * 1.1 = 2.2
    QCOMPARE(AjusteCorporativoService::fatorCumulativo(
        QStringLiteral("VALE3"),
        QDate(2020, 1, 1), QDate(2025, 1, 1), ev), 2.2);
}

void TestAjustes::eventosForaDoIntervaloIgnorados()
{
    QList<AjusteCorporativo> ev = {
        make(QStringLiteral("ITUB4"), QStringLiteral("split"),
             QDate(2019, 1, 1), 2.0),       // antes dataAlvo
        make(QStringLiteral("ITUB4"), QStringLiteral("split"),
             QDate(2027, 1, 1), 3.0),       // depois refData
    };
    QCOMPARE(AjusteCorporativoService::fatorCumulativo(
        QStringLiteral("ITUB4"),
        QDate(2020, 1, 1), QDate(2026, 1, 1), ev), 1.0);
}

void TestAjustes::tickerDiferenteIgnorado()
{
    QList<AjusteCorporativo> ev = {
        make(QStringLiteral("PETR4"), QStringLiteral("split"),
             QDate(2022, 1, 1), 2.0),
    };
    QCOMPARE(AjusteCorporativoService::fatorCumulativo(
        QStringLiteral("VALE3"),
        QDate(2020, 1, 1), QDate(2025, 1, 1), ev), 1.0);
}

QTEST_GUILESS_MAIN(TestAjustes)
#include "test_ajustes.moc"
