// PeuBud - RecorrenciaService::proximaApos (#132): funcao PURA de data.
// Alto risco de off-by-one em fronteiras de mes/ano. Fixa o comportamento REAL.
//
// proximaApos delega a QDate::addMonths/addDays/addYears:
//   semanal   -> +7 dias
//   quinzenal -> +14 dias
//   anual     -> +1 ano (QDate clampa 29/fev em ano nao-bissexto p/ 28/fev)
//   default   -> +1 mes  (QDate clampa o dia ao ultimo dia valido do mes alvo)

#include <QtTest/QtTest>
#include "services/RecorrenciaService.h"

class TestRecorrencia : public QObject
{
    Q_OBJECT
private slots:
    void mensalJaneiro31ParaFevereiro();   // 31/jan -> 28/fev (nao-bissexto)
    void mensalJaneiro31ParaFevereiroBissexto(); // 31/jan/2024 -> 29/fev/2024
    void mensalMarco31ParaAbril30();        // 31/mar -> 30/abr (30 dias)
    void mensalDiaNormalPreserva();         // 15/jan -> 15/fev
    void mensalViradaDeAno();               // 31/dez -> 31/jan ano seguinte
    void semanalMaisSete();
    void quinzenalMaisQuatorze();
    void semanalAtravessaMes();
    void anualMaisUmAno();
    void anual29FevParaNaoBissexto();       // 29/fev/2024 -> 28/fev/2025
    void frequenciaDesconhecidaCaiNoMensal();
};

void TestRecorrencia::mensalJaneiro31ParaFevereiro()
{
    // 31/jan/2025 + 1 mes. Fev/2025 tem 28 dias -> QDate clampa para 28/fev.
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 1, 31), QStringLiteral("mensal"));
    QCOMPARE(r, QDate(2025, 2, 28));
}

void TestRecorrencia::mensalJaneiro31ParaFevereiroBissexto()
{
    // 2024 e bissexto: fev tem 29 dias.
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2024, 1, 31), QStringLiteral("mensal"));
    QCOMPARE(r, QDate(2024, 2, 29));
}

void TestRecorrencia::mensalMarco31ParaAbril30()
{
    // 31/mar + 1 mes -> abr tem 30 dias -> 30/abr.
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 3, 31), QStringLiteral("mensal"));
    QCOMPARE(r, QDate(2025, 4, 30));
}

void TestRecorrencia::mensalDiaNormalPreserva()
{
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 1, 15), QStringLiteral("mensal"));
    QCOMPARE(r, QDate(2025, 2, 15));
}

void TestRecorrencia::mensalViradaDeAno()
{
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 12, 31), QStringLiteral("mensal"));
    QCOMPARE(r, QDate(2026, 1, 31));
}

void TestRecorrencia::semanalMaisSete()
{
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 6, 1), QStringLiteral("semanal"));
    QCOMPARE(r, QDate(2025, 6, 8));
}

void TestRecorrencia::quinzenalMaisQuatorze()
{
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 6, 1), QStringLiteral("quinzenal"));
    QCOMPARE(r, QDate(2025, 6, 15));
}

void TestRecorrencia::semanalAtravessaMes()
{
    // 28/fev/2025 + 7 dias -> 7/mar (fev tem 28 dias em 2025).
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 2, 28), QStringLiteral("semanal"));
    QCOMPARE(r, QDate(2025, 3, 7));
}

void TestRecorrencia::anualMaisUmAno()
{
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 6, 15), QStringLiteral("anual"));
    QCOMPARE(r, QDate(2026, 6, 15));
}

void TestRecorrencia::anual29FevParaNaoBissexto()
{
    // 29/fev/2024 + 1 ano -> 2025 nao e bissexto -> QDate clampa para 28/fev.
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2024, 2, 29), QStringLiteral("anual"));
    QCOMPARE(r, QDate(2025, 2, 28));
}

void TestRecorrencia::frequenciaDesconhecidaCaiNoMensal()
{
    // Default branch: qualquer string nao reconhecida -> mensal (+1 mes).
    const QDate r = RecorrenciaService::proximaApos(
        QDate(2025, 1, 15), QStringLiteral("trimestral"));
    QCOMPARE(r, QDate(2025, 2, 15));
}

QTEST_GUILESS_MAIN(TestRecorrencia)
#include "test_recorrencia.moc"
