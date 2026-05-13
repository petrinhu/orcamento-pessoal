#pragma once
#include <QDialog>
#include <QStringList>

class QKeyEvent;
class QLineEdit;
class QLabel;
class QListWidget;
class QPushButton;
class QDateEdit;
class QTableWidget;
class QStackedWidget;
class QTabWidget;
class QTimer;

// Non-modal floating calculator with three tabs:
//   Tab 0 — Comum:     standard keypad + expression + clipboard history  [v1: functional]
//   Tab 1 — Financeira: HP-style TVM calculator (RPN/ALG)                [v2: functional]
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

    // Show/hide the "send to field" button based on whether a field invoked us.
    void setTemInvoker(bool has);

signals:
    void resultadoDisponivel(double valor);
    void transferirSolicitado(double valor);

    // Emitted when "Enviar para Cartões" is clicked in the Financeira tab.
    // pv = present value, n = number of periods, iMensal = rate %/month, pmt = payment.
    void tvmParaCartoes(double pv, int n, double iMensal, double pmt);

protected:
    void closeEvent(QCloseEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

private:
    void buildUi();
    void buildTabComum         (QWidget *tab);
    void buildTabFinanceira    (QWidget *tab);
    void buildTabWizards       (QWidget *tab);
    void buildTabBolsa         (QWidget *tab);  // #43
    void buildTabImoveis       (QWidget *tab);  // #54
    void buildTabRendaFixa     (QWidget *tab);  // #55
    void buildTabIrTributos    (QWidget *tab);  // #56
    void buildTabInflacaoCambio(QWidget *tab);  // #57
    QPushButton *makeBtnCalc(const QString &label, QWidget *parent);

    // ── Calculator logic (tab Comum) ──────────────────────────────────────
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

    // ── TVM logic (tab Financeira) ─────────────────────────────────────────
    // campo: 0=N 1=i 2=PV 3=PMT 4=FV
    void calcularTvm(int campo);
    void atualizarCet();
    void atualizarTabelaAmort();
    [[nodiscard]] double parseTvmField(QLineEdit *le) const;
    [[nodiscard]] QString formatTvmResult(double v) const;

    // ── UI — tab Comum ────────────────────────────────────────────────────
    QLineEdit   *m_display        = nullptr;
    QLabel      *m_exprLabel      = nullptr;
    QListWidget *m_historico      = nullptr;
    QPushButton *m_btnTransferir  = nullptr;

    // ── UI — tab Wizards ─────────────────────────────────────────────────
    QTabWidget      *m_tabs     = nullptr;
    QStackedWidget  *m_wizStack = nullptr;

    // ── UI — tab Financeira ───────────────────────────────────────────────
    QLineEdit    *m_tvmN          = nullptr;
    QLineEdit    *m_tvmI          = nullptr;
    QLineEdit    *m_tvmPV         = nullptr;
    QLineEdit    *m_tvmPMT        = nullptr;
    QLineEdit    *m_tvmFV         = nullptr;
    QLineEdit    *m_tvmCet        = nullptr;
    QDateEdit    *m_tvmData       = nullptr;
    QPushButton  *m_tvmBegin      = nullptr;
    QPushButton  *m_tvmEnd        = nullptr;
    QPushButton  *m_btnTabAmort   = nullptr;
    QWidget      *m_tabAmortPanel = nullptr;
    QTableWidget *m_tvmTabela     = nullptr;

    // ── State machine (tab Comum) ─────────────────────────────────────────
    double  m_reg      = 0.0;
    double  m_cur      = 0.0;
    QChar   m_op       = {};
    QString m_inputStr;
    bool    m_hasInput = false;
    bool    m_justDone = false;

    QStringList m_historyItems; // up to 20 recent entries

    // Debounce para recalculos pesados disparados por textChanged (#95)
    QTimer *m_debounceCet = nullptr;
};
