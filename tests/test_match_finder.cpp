// PeuBud - IMatchFinder TDD (#84)
// Refactor de ImportService: introduz pipeline de matching para evitar
// duplicatas ao importar extratos OFX/CSV/XLSX. Padrao de kmymoney
// (transactionmatchfinder.h:22-149) adaptado para nosso dominio.
//
// Quatro resultados possiveis:
//   NotFound  - sem candidato compativel
//   Imprecise - amount aproximado (+/- 1%) OU payee parcial, dentro da janela
//   Precise   - amount e payee exatos, conta igual, dentro da janela
//   Duplicate - bankId (FITID) ja registrado: importacao redundante

#include <QtTest/QtTest>
#include "services/import/ExistingTransactionMatchFinder.h"
#include "services/import/ScheduledTransactionMatchFinder.h"
#include "services/import/MatchData.h"

class TestMatchFinder : public QObject
{
    Q_OBJECT

private slots:
    // Janela de datas
    void janelaDefault4Dias();
    void janelaConfiguravel();

    // ExistingTransactionMatchFinder
    void existingSemCandidatosNotFound();
    void existingBankIdIgualDuplicate();
    void existingValorEPayeeIguaisPrecise();
    void existingValorAproximadoImprecise();
    void existingForaDaJanelaNotFound();
    void existingContaDiferenteImprecise();

    // ScheduledTransactionMatchFinder
    void scheduledExatoPrecise();
    void scheduledPayeeDiferenteImprecise();
    void scheduledForaJanelaNotFound();
};

static MatchData makeCandidate(const QString &bankId, qint64 amount,
                               const QString &payee, const QDate &date,
                               const QString &acct = QStringLiteral("ACC1"))
{
    MatchData d;
    d.bankId   = bankId;
    d.amount   = amount;
    d.payee    = payee;
    d.date     = date;
    d.accountId = acct;
    return d;
}

void TestMatchFinder::janelaDefault4Dias()
{
    ExistingTransactionMatchFinder f(QList<MatchData>{});
    QCOMPARE(f.matchWindow(), 4);
}

void TestMatchFinder::janelaConfiguravel()
{
    ExistingTransactionMatchFinder f(QList<MatchData>{});
    f.setMatchWindow(7);
    QCOMPARE(f.matchWindow(), 7);
}

void TestMatchFinder::existingSemCandidatosNotFound()
{
    ExistingTransactionMatchFinder f(QList<MatchData>{});
    const MatchData input = makeCandidate(QStringLiteral("FITID-1"), 12345,
                                          QStringLiteral("Supermercado"),
                                          QDate(2026, 5, 10));
    QCOMPARE(f.find(input), MatchResult::NotFound);
}

void TestMatchFinder::existingBankIdIgualDuplicate()
{
    QList<MatchData> existing = {
        makeCandidate(QStringLiteral("FITID-1"), 12345,
                      QStringLiteral("Supermercado"), QDate(2026, 5, 10))
    };
    ExistingTransactionMatchFinder f(existing);
    const MatchData input = makeCandidate(QStringLiteral("FITID-1"), 99999,
                                          QStringLiteral("Outra desc"),
                                          QDate(2026, 6, 1));
    QCOMPARE(f.find(input), MatchResult::Duplicate);
}

void TestMatchFinder::existingValorEPayeeIguaisPrecise()
{
    QList<MatchData> existing = {
        makeCandidate(QString(), 12345, QStringLiteral("Supermercado XYZ"),
                      QDate(2026, 5, 10))
    };
    ExistingTransactionMatchFinder f(existing);
    const MatchData input = makeCandidate(QStringLiteral("NEW-FITID"), 12345,
                                          QStringLiteral("Supermercado XYZ"),
                                          QDate(2026, 5, 11));
    QCOMPARE(f.find(input), MatchResult::Precise);
}

void TestMatchFinder::existingValorAproximadoImprecise()
{
    QList<MatchData> existing = {
        makeCandidate(QString(), 12345, QStringLiteral("Supermercado"),
                      QDate(2026, 5, 10))
    };
    ExistingTransactionMatchFinder f(existing);
    // 12400 ~= 12345 (+0,44% < 1%)
    const MatchData input = makeCandidate(QString(), 12400,
                                          QStringLiteral("Supermercado"),
                                          QDate(2026, 5, 11));
    QCOMPARE(f.find(input), MatchResult::Imprecise);
}

void TestMatchFinder::existingForaDaJanelaNotFound()
{
    QList<MatchData> existing = {
        makeCandidate(QString(), 12345, QStringLiteral("Supermercado"),
                      QDate(2026, 5, 1))
    };
    ExistingTransactionMatchFinder f(existing);
    // Janela default = 4 dias; diff = 10 dias
    const MatchData input = makeCandidate(QString(), 12345,
                                          QStringLiteral("Supermercado"),
                                          QDate(2026, 5, 11));
    QCOMPARE(f.find(input), MatchResult::NotFound);
}

void TestMatchFinder::existingContaDiferenteImprecise()
{
    QList<MatchData> existing = {
        makeCandidate(QString(), 12345, QStringLiteral("Supermercado"),
                      QDate(2026, 5, 10), QStringLiteral("ACC1"))
    };
    ExistingTransactionMatchFinder f(existing);
    // valor + payee iguais, mas conta diferente -> Imprecise
    const MatchData input = makeCandidate(QString(), 12345,
                                          QStringLiteral("Supermercado"),
                                          QDate(2026, 5, 11),
                                          QStringLiteral("ACC2"));
    QCOMPARE(f.find(input), MatchResult::Imprecise);
}

void TestMatchFinder::scheduledExatoPrecise()
{
    QList<MatchData> sched = {
        makeCandidate(QString(), 50000, QStringLiteral("Aluguel"),
                      QDate(2026, 5, 5))
    };
    ScheduledTransactionMatchFinder f(sched);
    const MatchData input = makeCandidate(QString(), 50000,
                                          QStringLiteral("Aluguel"),
                                          QDate(2026, 5, 6));
    QCOMPARE(f.find(input), MatchResult::Precise);
}

void TestMatchFinder::scheduledPayeeDiferenteImprecise()
{
    QList<MatchData> sched = {
        makeCandidate(QString(), 50000, QStringLiteral("Aluguel"),
                      QDate(2026, 5, 5))
    };
    ScheduledTransactionMatchFinder f(sched);
    const MatchData input = makeCandidate(QString(), 50000,
                                          QStringLiteral("Imobiliaria"),
                                          QDate(2026, 5, 5));
    QCOMPARE(f.find(input), MatchResult::Imprecise);
}

void TestMatchFinder::scheduledForaJanelaNotFound()
{
    QList<MatchData> sched = {
        makeCandidate(QString(), 50000, QStringLiteral("Aluguel"),
                      QDate(2026, 5, 1))
    };
    ScheduledTransactionMatchFinder f(sched);
    const MatchData input = makeCandidate(QString(), 50000,
                                          QStringLiteral("Aluguel"),
                                          QDate(2026, 6, 1));
    QCOMPARE(f.find(input), MatchResult::NotFound);
}

QTEST_GUILESS_MAIN(TestMatchFinder)
#include "test_match_finder.moc"
