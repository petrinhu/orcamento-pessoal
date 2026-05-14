// PeuBud - ParcelaService TDD (#18 suite).
// Foca em funcoes puras: CET, conversao moeda, parse/format, datas.
// Funcoes que dependem de DB (recalcularAposEdicao) ficam para integracao.

#include <QtTest/QtTest>
#include "services/ParcelaService.h"

class TestParcelaService : public QObject
{
    Q_OBJECT
private slots:
    void cetFromJurosMensaisCompose();
    void jurosMensaisFromCetInverso();
    void formatarValorBRL();
    void parsearValorBRComDecimal();
    void simboloMoedaConhecidas();
};

void TestParcelaService::cetFromJurosMensaisCompose()
{
    // 1% a.m. -> CET a.a. = (1.01)^12 - 1 = 12.6825%
    const double cet = ParcelaService::cetFromJurosMensais(1.0);
    QVERIFY2(qAbs(cet - 12.6825) < 1e-3,
        qPrintable(QStringLiteral("CET = %1, esperado ~12.6825").arg(cet)));
}

void TestParcelaService::jurosMensaisFromCetInverso()
{
    // CET 12.6825% -> mensal 1%
    const double mensal = ParcelaService::jurosMensaisFromCet(12.6825);
    QVERIFY2(qAbs(mensal - 1.0) < 1e-3,
        qPrintable(QStringLiteral("mensal = %1, esperado ~1.0").arg(mensal)));
}

void TestParcelaService::formatarValorBRL()
{
    // 123456 cents = R$ 1.234,56 (locale BR)
    const QString s = ParcelaService::formatarValor(123456,
                                                     QStringLiteral("BRL"));
    QVERIFY2(s.contains(QStringLiteral("1.234,56")) ||
             s.contains(QStringLiteral("1234,56")),
        qPrintable(QStringLiteral("formatado = %1").arg(s)));
}

void TestParcelaService::parsearValorBRComDecimal()
{
    // "R$ 1.234,56" -> 123456 cents
    const qint64 c = ParcelaService::parsearValor(
        QStringLiteral("R$ 1.234,56"));
    QCOMPARE(c, qint64(123456));

    // Sem prefixo
    QCOMPARE(ParcelaService::parsearValor(QStringLiteral("99,90")),
             qint64(9990));
}

void TestParcelaService::simboloMoedaConhecidas()
{
    QCOMPARE(ParcelaService::simboloMoeda(QStringLiteral("BRL")),
             QStringLiteral("R$"));
    QCOMPARE(ParcelaService::simboloMoeda(QStringLiteral("USD")),
             QStringLiteral("$"));
    // EUR pode ser "€" ou simbolo similar
    const QString eur = ParcelaService::simboloMoeda(QStringLiteral("EUR"));
    QVERIFY(!eur.isEmpty());
}

QTEST_GUILESS_MAIN(TestParcelaService)
#include "test_parcela_service.moc"
