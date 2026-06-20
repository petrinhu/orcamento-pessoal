// PeuBud — teste de regressão #175
// Garante que o locale pt-BR produz siglas de mês em português,
// não em inglês (bug: QDate::toString("MMM") sem locale → locale C → inglês).
//
// Intencional: puro Qt::Core + Qt::Test, sem deps de UI/DB/crypto.

#include <QtTest/QtTest>
#include <QLocale>
#include <QDate>

class TestLocaleMeses : public QObject
{
    Q_OBJECT

private slots:
    void siglasEmPtBR();
    void siglasEmEnUS();
    void fevereiro_ptBR_naoEhFeb();
    void semPontoNaSigla();
};

// Helper que espelha o padrão usado nos widgets corrigidos (#175).
static QString fmtMesAbrev(int ano, int mes,
                            const QLocale &loc,
                            const QString &fmt = QStringLiteral("MMM/yy"))
{
    const QDate d(ano, mes, 1);
    QString s = loc.toString(d, fmt);
    s.remove(u'.');
    if (!s.isEmpty()) s[0] = s[0].toUpper();
    return s;
}

void TestLocaleMeses::siglasEmPtBR()
{
    const QLocale ptBR(QLocale::Portuguese, QLocale::Brazil);

    QCOMPARE(fmtMesAbrev(2025, 1,  ptBR), QStringLiteral("Jan/25"));
    QCOMPARE(fmtMesAbrev(2025, 2,  ptBR), QStringLiteral("Fev/25"));
    QCOMPARE(fmtMesAbrev(2025, 3,  ptBR), QStringLiteral("Mar/25"));
    QCOMPARE(fmtMesAbrev(2025, 4,  ptBR), QStringLiteral("Abr/25"));
    QCOMPARE(fmtMesAbrev(2025, 5,  ptBR), QStringLiteral("Mai/25"));
    QCOMPARE(fmtMesAbrev(2025, 6,  ptBR), QStringLiteral("Jun/25"));
    QCOMPARE(fmtMesAbrev(2025, 7,  ptBR), QStringLiteral("Jul/25"));
    QCOMPARE(fmtMesAbrev(2025, 8,  ptBR), QStringLiteral("Ago/25"));
    QCOMPARE(fmtMesAbrev(2025, 9,  ptBR), QStringLiteral("Set/25"));
    QCOMPARE(fmtMesAbrev(2025, 10, ptBR), QStringLiteral("Out/25"));
    QCOMPARE(fmtMesAbrev(2025, 11, ptBR), QStringLiteral("Nov/25"));
    QCOMPARE(fmtMesAbrev(2025, 12, ptBR), QStringLiteral("Dez/25"));
}

void TestLocaleMeses::siglasEmEnUS()
{
    const QLocale enUS(QLocale::English, QLocale::UnitedStates);

    QCOMPARE(fmtMesAbrev(2025, 1,  enUS), QStringLiteral("Jan/25"));
    QCOMPARE(fmtMesAbrev(2025, 2,  enUS), QStringLiteral("Feb/25"));
    QCOMPARE(fmtMesAbrev(2025, 4,  enUS), QStringLiteral("Apr/25"));
    QCOMPARE(fmtMesAbrev(2025, 5,  enUS), QStringLiteral("May/25"));
    QCOMPARE(fmtMesAbrev(2025, 8,  enUS), QStringLiteral("Aug/25"));
}

void TestLocaleMeses::fevereiro_ptBR_naoEhFeb()
{
    // Regressão core do #175: fevereiro DEVE ser "Fev", nunca "Feb".
    const QLocale ptBR(QLocale::Portuguese, QLocale::Brazil);
    const QString resultado = fmtMesAbrev(2025, 2, ptBR);
    QVERIFY2(resultado.startsWith(QStringLiteral("Fev")),
             qPrintable(QStringLiteral("Esperado 'Fev/25', obtido '%1'").arg(resultado)));
}

void TestLocaleMeses::semPontoNaSigla()
{
    // QLocale pt-BR pode retornar "fev." — o .remove(u'.') deve eliminar o ponto.
    const QLocale ptBR(QLocale::Portuguese, QLocale::Brazil);
    const QString resultado = fmtMesAbrev(2025, 2, ptBR);
    QVERIFY2(!resultado.contains(u'.'),
             qPrintable(QStringLiteral("Sigla não deve conter ponto: '%1'").arg(resultado)));
}

QTEST_GUILESS_MAIN(TestLocaleMeses)
#include "test_locale_meses.moc"
