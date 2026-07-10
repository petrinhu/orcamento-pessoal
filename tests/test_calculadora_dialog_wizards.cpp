// PeuBud — ARQ-03 (auditoria 2026-06-22 §3.3): caracterização da aba
// "Wizards" da CalculadoraDialog, ANTES uma god-function
// (buildTabWizards, ~1.388 linhas) que virou registrador fino chamando 20
// métodos buildWizardX() próprios.
//
// Este teste prova, via instância REAL de CalculadoraDialog (offscreen):
//   1. os 20 painéis continuam registrados na ordem 1:1 com o combo seletor
//      (o índice do combo é quem seleciona a página do QStackedWidget — se
//      um wizard fosse perdido/duplicado na extração, count() divergiria);
//   2. o fluxo digitar → clicar "Calcular" → ler o QLabel de resultado
//      continua produzindo o MESMO texto formatado de antes da extração
//      (congela o comportamento — regressão para qualquer refactor futuro
//      dos wizards que ainda restam no formato antigo double/QLineEdit).
//
// Valores de entrada escolhidos para caírem em números redondos (sem
// depender de arredondamento de ponto flutuante no oráculo do teste).

#include <QtTest/QtTest>
#include <QApplication>
#include <QComboBox>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QSettings>
#include <QTemporaryDir>

#include "ui/widgets/CalculadoraDialog.h"

class TestCalculadoraDialogWizards : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();

    void vinteWizardsRegistradosNaOrdem();
    void wizard1_cetTaxaMensal();
    void wizard6_rendaPassiva();
    void wizard13_fireNumber();

private:
    QTemporaryDir m_settingsDir;
};

void TestCalculadoraDialogWizards::initTestCase()
{
    QVERIFY(m_settingsDir.isValid());
    QSettings::setPath(QSettings::IniFormat,    QSettings::UserScope, m_settingsDir.path());
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
}

// ── Helpers internos ─────────────────────────────────────────────────────────

namespace {

QWidget *wizTabOf(CalculadoraDialog *dlg)
{
    auto *tabs = dlg->findChild<QTabWidget *>();
    if (!tabs) return nullptr;
    return tabs->widget(2); // aba "Wizards" (índice 2: Comum, Financeira, Wizards, ...)
}

QStackedWidget *wizStackOf(CalculadoraDialog *dlg)
{
    QWidget *tab = wizTabOf(dlg);
    return tab ? tab->findChild<QStackedWidget *>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}

QComboBox *wizComboOf(CalculadoraDialog *dlg)
{
    QWidget *tab = wizTabOf(dlg);
    return tab ? tab->findChild<QComboBox *>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
}

QPushButton *findCalcButton(QWidget *panel)
{
    for (auto *b : panel->findChildren<QPushButton *>())
        if (b->text() == QStringLiteral("Calcular"))
            return b;
    return nullptr;
}

QLabel *findVisibleResultLabel(QWidget *panel)
{
    // A CalculadoraDialog não é .show()'n neste teste (offscreen, sem
    // necessidade de renderizar); isVisible() combinaria com a cadeia de
    // ancestrais e sempre daria falso. isHidden() reflete só o estado
    // explícito hide()/show() do próprio widget — é isso que o wizard usa
    // para revelar o QLabel de resultado após o cálculo.
    for (auto *l : panel->findChildren<QLabel *>())
        if (!l->isHidden() && l->textInteractionFlags().testFlag(Qt::TextSelectableByMouse))
            return l;
    return nullptr;
}

} // namespace

// ── Estrutura: 20 wizards, mesma ordem combo <-> stack ───────────────────────

void TestCalculadoraDialogWizards::vinteWizardsRegistradosNaOrdem()
{
    CalculadoraDialog dlg;
    auto *stack = wizStackOf(&dlg);
    auto *combo = wizComboOf(&dlg);
    QVERIFY(stack);
    QVERIFY(combo);
    QCOMPARE(stack->count(), 20);
    QCOMPARE(combo->count(), 20);
    QCOMPARE(combo->itemText(0),  QStringLiteral("1. CET ↔ Taxa mensal"));
    QCOMPARE(combo->itemText(18), QStringLiteral("19. Reserva de Emergência"));
    QCOMPARE(combo->itemText(19), QStringLiteral("20. CLT vs PJ"));
}

// ── Wizard 1: CET ↔ Taxa mensal (2% a.m. -> CET % a.a.) ──────────────────────

void TestCalculadoraDialogWizards::wizard1_cetTaxaMensal()
{
    CalculadoraDialog dlg;
    auto *stack = wizStackOf(&dlg);
    QVERIFY(stack);
    QWidget *panel = stack->widget(0);
    QVERIFY(panel);

    const auto edits = panel->findChildren<QLineEdit *>();
    QCOMPARE(edits.size(), 1);
    edits.first()->setText(QStringLiteral("2"));

    auto *btnCalc = findCalcButton(panel);
    QVERIFY(btnCalc);
    btnCalc->click();

    auto *lblRes = findVisibleResultLabel(panel);
    QVERIFY(lblRes);
    // (1,02)^12 - 1 = 26,824179...% -> wizFmtF(res, 4) = "26,8242"
    QCOMPARE(lblRes->text(), QStringLiteral("CET: <b>26,8242 % a.a.</b>"));
}

// ── Wizard 6: Renda passiva (capital 100000, taxa 0,5% a.m.) ─────────────────

void TestCalculadoraDialogWizards::wizard6_rendaPassiva()
{
    CalculadoraDialog dlg;
    auto *stack = wizStackOf(&dlg);
    QVERIFY(stack);
    QWidget *panel = stack->widget(5);
    QVERIFY(panel);

    const auto edits = panel->findChildren<QLineEdit *>();
    QCOMPARE(edits.size(), 2);
    edits.at(0)->setText(QStringLiteral("100000")); // Capital
    edits.at(1)->setText(QStringLiteral("0,5"));     // Taxa mensal %

    auto *btnCalc = findCalcButton(panel);
    QVERIFY(btnCalc);
    btnCalc->click();

    auto *lblRes = findVisibleResultLabel(panel);
    QVERIFY(lblRes);
    // pmt = 100000*0,005 = 500 ; anual = 500*12 = 6000 (números redondos)
    QCOMPARE(lblRes->text(),
              QStringLiteral("Renda mensal: <b>R$ 500</b><br>Renda anual: R$ 6000"));
}

// ── Wizard 13: FIRE Number, só despesas (taxa/inflação/anos = 0) ─────────────

void TestCalculadoraDialogWizards::wizard13_fireNumber()
{
    CalculadoraDialog dlg;
    auto *stack = wizStackOf(&dlg);
    QVERIFY(stack);
    QWidget *panel = stack->widget(12);
    QVERIFY(panel);

    const auto edits = panel->findChildren<QLineEdit *>();
    QCOMPARE(edits.size(), 4);
    edits.at(0)->setText(QStringLiteral("5000")); // Despesas mensais
    // Retorno/Inflação/Anos ficam vazios -> extra vazio, sem taxa real/futuro.

    auto *btnCalc = findCalcButton(panel);
    QVERIFY(btnCalc);
    btnCalc->click();

    auto *lblRes = findVisibleResultLabel(panel);
    QVERIFY(lblRes);
    // fireHoje = 5000*12*25 = 1.500.000 (número redondo)
    QCOMPARE(lblRes->text(),
              QStringLiteral("FIRE Number (hoje): <b>R$ 1500000</b><br>"
                              "= R$ 5000 × 12 × 25"));
}

QTEST_MAIN(TestCalculadoraDialogWizards)
#include "test_calculadora_dialog_wizards.moc"
