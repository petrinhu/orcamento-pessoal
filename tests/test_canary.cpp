// PeuBud - teste canário (#102)
// Valida que a infra de testes está viva: Qt6::Test linkado, ctest registra e
// executa. NÃO testa lógica de negócio - isso vem nos próximos testes (TDD).

#include <QtTest/QtTest>

class TestCanary : public QObject
{
    Q_OBJECT

private slots:
    void aritmeticaBasica();
    void centavosComoQint64();
};

void TestCanary::aritmeticaBasica()
{
    QCOMPARE(2 + 2, 4);
    QCOMPARE(10 - 3, 7);
    QCOMPARE(7 * 6, 42);
    QCOMPARE(20 / 4, 5);
}

void TestCanary::centavosComoQint64()
{
    // Convenção do projeto: dinheiro em centavos (qint64), nunca float.
    // Ver CONTRACT.md §12.1 + .claude/CLAUDE.md (Convenções).
    const qint64 reais   = 100;        // R$ 1,00
    const qint64 centavo = 1;
    QCOMPARE(reais + centavo, qint64{101});

    // Multiplicação por % (juros armazenados como % × 10000):
    // 1,5% × 10000 = 15000
    const qint64 jurosPctEscalado = 15000;
    const qint64 saldoCentavos    = 1000000;          // R$ 10.000,00
    // juros = saldo × jurosPctEscalado / (100 * 10000)
    const qint64 jurosCentavos = saldoCentavos * jurosPctEscalado / 1000000;
    QCOMPARE(jurosCentavos, qint64{15000}); // R$ 150,00
}

QTEST_GUILESS_MAIN(TestCanary)
#include "test_canary.moc"
