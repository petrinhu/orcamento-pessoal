#pragma once
#include <QDialog>
#include <QStringList>

class QKeyEvent;
class QLineEdit;
class QLabel;
class QListWidget;
class QPushButton;

// Non-modal floating calculator with three tabs:
//   Tab 0 — Comum:     standard keypad + expression + clipboard history  [v1: functional]
//   Tab 1 — Financeira: HP-style TVM calculator (RPN/ALG)                [v2: stub]
//   Tab 2 — Wizards:    quick-calc buttons (CET, juros, parcelas…)       [v3: stub]
//
// Emits resultadoDisponivel(double) every time = is pressed.
// Window position is persisted via QSettings ("calculadora/geometry").
class CalculadoraDialog final : public QDialog {
    Q_OBJECT
public:
    explicit CalculadoraDialog(QWidget *parent = nullptr);

    // Pre-fill the display with a value (called when invoked from a field).
    void setValorInicial(double valor);

signals:
    void resultadoDisponivel(double valor);

protected:
    void closeEvent(QCloseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    void buildUi();
    void buildTabComum(QWidget *tab);
    QPushButton *makeBtnCalc(const QString &label, QWidget *parent);

    // ── Calculator logic ──────────────────────────────────────────────────
    void pressDigito(int d);
    void pressDecimal();
    void pressOperador(QChar op);
    void pressIgual();
    void pressPorcentagem();
    void pressCE();
    void pressC();
    void pressPlusMinus();
    void pressBackspace();

    void adicionarHistorico(const QString &entry);
    void refreshDisplay();
    [[nodiscard]] QString formatDisplay(double v) const;
    [[nodiscard]] double  aplicarOp(double a, double b, QChar op) const;

    // ── UI ────────────────────────────────────────────────────────────────
    QLineEdit   *m_display   = nullptr;
    QLabel      *m_exprLabel = nullptr;
    QListWidget *m_historico = nullptr;
    QPushButton *m_btnPin    = nullptr; // "Sempre visível" toggle

    // ── State machine ─────────────────────────────────────────────────────
    double  m_reg      = 0.0;   // stored operand (left side)
    double  m_cur      = 0.0;   // current display value
    QChar   m_op       = {};    // pending operator ('\0' = none)
    QString m_inputStr;         // digits being typed (uses '.' as decimal internally)
    bool    m_hasInput = false; // true while user is entering digits
    bool    m_justDone = false; // true after = or CE/C

    QStringList m_historyItems; // up to 20 recent entries
};
