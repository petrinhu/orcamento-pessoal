#include "CalculadoraDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QTabWidget>
#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QSettings>
#include <QCloseEvent>
#include <QKeyEvent>
#include <cmath>

// ─── construction ────────────────────────────────────────────────────────────

CalculadoraDialog::CalculadoraDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Calculadora"));
    setWindowFlags(Qt::Window | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, false); // reuse across open/close cycles

    buildUi();

    QSettings s;
    const QByteArray geo = s.value(QStringLiteral("calculadora/geometry")).toByteArray();
    if (!geo.isEmpty())
        restoreGeometry(geo);
}

void CalculadoraDialog::closeEvent(QCloseEvent *e)
{
    QSettings s;
    s.setValue(QStringLiteral("calculadora/geometry"), saveGeometry());
    QDialog::closeEvent(e);
}

void CalculadoraDialog::keyPressEvent(QKeyEvent *e)
{
    switch (e->key()) {
    case Qt::Key_0: case Qt::Key_1: case Qt::Key_2: case Qt::Key_3: case Qt::Key_4:
    case Qt::Key_5: case Qt::Key_6: case Qt::Key_7: case Qt::Key_8: case Qt::Key_9:
        pressDigito(e->key() - Qt::Key_0);
        return;
    case Qt::Key_Period:
    case Qt::Key_Comma:
        pressDecimal();
        return;
    case Qt::Key_Plus:
        pressOperador(QLatin1Char('+'));
        return;
    case Qt::Key_Minus:
        pressOperador(QLatin1Char('-'));
        return;
    case Qt::Key_Asterisk:
        pressOperador(QLatin1Char('*'));
        return;
    case Qt::Key_Slash:
        pressOperador(QLatin1Char('/'));
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Equal:
        pressIgual();
        return;
    case Qt::Key_Backspace:
        pressBackspace();
        return;
    case Qt::Key_Delete:
        pressCE();
        return;
    case Qt::Key_Escape:
        pressC();
        return;
    case Qt::Key_Percent:
        pressPorcentagem();
        return;
    default:
        break;
    }
    QDialog::keyPressEvent(e);
}

// ─── UI build ────────────────────────────────────────────────────────────────

void CalculadoraDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(4);

    auto *tabs = new QTabWidget(this);

    auto *tabComum = new QWidget;
    buildTabComum(tabComum);
    tabs->addTab(tabComum, tr("Comum"));

    // Stubs: disabled until v2/v3
    auto *tabFin = new QWidget;
    tabs->addTab(tabFin, tr("Financeira"));
    tabs->setTabEnabled(1, false);
    tabs->setTabToolTip(1, tr("Em breve — calculadora financeira HP-style (TVM, RPN/ALG)"));

    auto *tabWiz = new QWidget;
    tabs->addTab(tabWiz, tr("Wizards"));
    tabs->setTabEnabled(2, false);
    tabs->setTabToolTip(2, tr("Em breve — cálculos rápidos (CET, juros, parcelas, ROI…)"));

    root->addWidget(tabs);
    setMinimumWidth(300);
    adjustSize();
}

void CalculadoraDialog::buildTabComum(QWidget *tab)
{
    auto *vbox = new QVBoxLayout(tab);
    vbox->setContentsMargins(4, 8, 4, 4);
    vbox->setSpacing(4);

    // ── expression label + "Sempre visível" checkbox ──────────────────────
    {
        auto *row = new QHBoxLayout;
        row->setSpacing(8);

        m_exprLabel = new QLabel(QString(), tab);
        m_exprLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        m_exprLabel->setObjectName(QStringLiteral("subtitle"));
        m_exprLabel->setMinimumHeight(18);

        m_chkAlwaysOnTop = new QCheckBox(tr("Sempre visível"), tab);
        m_chkAlwaysOnTop->setToolTip(tr("Manter calculadora sobre todas as outras janelas"));
        connect(m_chkAlwaysOnTop, &QCheckBox::toggled, this, [this](bool on) {
            setWindowFlag(Qt::WindowStaysOnTopHint, on);
            show(); // required to apply the flag change
        });

        row->addWidget(m_exprLabel, 1);
        row->addWidget(m_chkAlwaysOnTop);
        vbox->addLayout(row);
    }

    // ── display + copy button ──────────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;
        row->setSpacing(4);

        m_display = new QLineEdit(QStringLiteral("0"), tab);
        m_display->setReadOnly(true);
        m_display->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_display->setObjectName(QStringLiteral("calcDisplay"));
        QFont f = m_display->font();
        f.setPointSize(f.pointSize() + 6);
        f.setBold(true);
        m_display->setFont(f);
        m_display->setMinimumHeight(48);

        auto *btnCopy = new QToolButton(tab);
        btnCopy->setText(QStringLiteral("📋"));
        btnCopy->setToolTip(tr("Copiar para área de transferência"));
        btnCopy->setFixedSize(32, 48);
        connect(btnCopy, &QToolButton::clicked, this, [this]() {
            QApplication::clipboard()->setText(m_display->text());
        });

        row->addWidget(m_display, 1);
        row->addWidget(btnCopy);
        vbox->addLayout(row);
    }

    // ── button grid ────────────────────────────────────────────────────────
    //   Row 0: CE   C   ±   ÷
    //   Row 1:  7   8   9   ×
    //   Row 2:  4   5   6   −
    //   Row 3:  1   2   3   +
    //   Row 4:  0   ,   ⌫   =
    //   Row 5:  %  (single)

    auto *grid = new QGridLayout;
    grid->setSpacing(4);

    struct BtnDef { QString lbl; int row; int col; int rs; int cs; QString obj; };
    const QList<BtnDef> defs = {
        {tr("CE"),               0, 0, 1, 1, QStringLiteral("utilityBtn")},
        {tr("C"),                0, 1, 1, 1, QStringLiteral("utilityBtn")},
        {QStringLiteral("±"),    0, 2, 1, 1, QStringLiteral("utilityBtn")},
        {QStringLiteral("÷"),    0, 3, 1, 1, QStringLiteral("operatorBtn")},
        {QStringLiteral("7"),    1, 0, 1, 1, {}},
        {QStringLiteral("8"),    1, 1, 1, 1, {}},
        {QStringLiteral("9"),    1, 2, 1, 1, {}},
        {QStringLiteral("×"),    1, 3, 1, 1, QStringLiteral("operatorBtn")},
        {QStringLiteral("4"),    2, 0, 1, 1, {}},
        {QStringLiteral("5"),    2, 1, 1, 1, {}},
        {QStringLiteral("6"),    2, 2, 1, 1, {}},
        {QStringLiteral("−"),    2, 3, 1, 1, QStringLiteral("operatorBtn")},
        {QStringLiteral("1"),    3, 0, 1, 1, {}},
        {QStringLiteral("2"),    3, 1, 1, 1, {}},
        {QStringLiteral("3"),    3, 2, 1, 1, {}},
        {QStringLiteral("+"),    3, 3, 1, 1, QStringLiteral("operatorBtn")},
        {QStringLiteral("0"),    4, 0, 1, 1, {}},
        {QStringLiteral(","),    4, 1, 1, 1, {}},
        {QStringLiteral("⌫"),    4, 2, 1, 1, QStringLiteral("utilityBtn")},
        {QStringLiteral("="),    4, 3, 1, 1, QStringLiteral("primaryBtn")},
        {QStringLiteral("%"),    5, 0, 1, 1, QStringLiteral("utilityBtn")},
    };

    for (const auto &d : defs) {
        auto *btn = makeBtnCalc(d.lbl, tab);
        if (!d.obj.isEmpty()) btn->setObjectName(d.obj);
        grid->addWidget(btn, d.row, d.col, d.rs, d.cs);

        const QString lbl = d.lbl;
        connect(btn, &QPushButton::clicked, this, [this, lbl]() {
            if      (lbl >= QStringLiteral("0") && lbl <= QStringLiteral("9"))
                pressDigito(lbl.at(0).digitValue());
            else if (lbl == QStringLiteral(","))  pressDecimal();
            else if (lbl == QStringLiteral("="))  pressIgual();
            else if (lbl == QStringLiteral("÷"))  pressOperador(QLatin1Char('/'));
            else if (lbl == QStringLiteral("×"))  pressOperador(QLatin1Char('*'));
            else if (lbl == QStringLiteral("−"))  pressOperador(QLatin1Char('-'));
            else if (lbl == QStringLiteral("+"))  pressOperador(QLatin1Char('+'));
            else if (lbl == QStringLiteral("%"))  pressPorcentagem();
            else if (lbl == tr("CE"))             pressCE();
            else if (lbl == tr("C"))              pressC();
            else if (lbl == QStringLiteral("±"))  pressPlusMinus();
            else if (lbl == QStringLiteral("⌫"))  pressBackspace();
        });
    }
    vbox->addLayout(grid);

    // ── history ────────────────────────────────────────────────────────────
    auto *histLabel = new QLabel(tr("Histórico"), tab);
    histLabel->setObjectName(QStringLiteral("subtitle"));
    vbox->addWidget(histLabel);

    m_historico = new QListWidget(tab);
    m_historico->setMaximumHeight(100);
    m_historico->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historico->setToolTip(tr("Duplo-clique numa linha para restaurar o resultado"));
    connect(m_historico, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem *item) {
        const QString text = item->text();
        const int eq = text.lastIndexOf(QStringLiteral(" = "));
        if (eq < 0) return;
        const QString resStr = text.mid(eq + 3)
            .replace(QLatin1Char(','), QLatin1Char('.'));
        bool ok = false;
        const double v = resStr.toDouble(&ok);
        if (!ok) return;
        m_cur      = v;
        m_hasInput = false;
        m_justDone = true;
        refreshDisplay();
    });
    vbox->addWidget(m_historico);
}

QPushButton *CalculadoraDialog::makeBtnCalc(const QString &label, QWidget *parent)
{
    auto *btn = new QPushButton(label, parent);
    btn->setMinimumSize(54, 44);
    btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QFont f = btn->font();
    f.setPointSize(f.pointSize() + 1);
    btn->setFont(f);
    return btn;
}

// ─── public API ──────────────────────────────────────────────────────────────

void CalculadoraDialog::setValorInicial(double valor)
{
    m_cur      = valor;
    m_hasInput = false;
    m_justDone = true;
    m_op       = {};
    m_reg      = 0.0;
    m_exprLabel->clear();
    refreshDisplay();
}

// ─── calculator logic ─────────────────────────────────────────────────────────

void CalculadoraDialog::pressDigito(int d)
{
    if (m_justDone || !m_hasInput) {
        m_inputStr = (d == 0) ? QStringLiteral("0") : QString::number(d);
        m_hasInput = true;
        m_justDone = false;
    } else {
        if (m_inputStr == QStringLiteral("0") && d != 0)
            m_inputStr = QString::number(d);
        else if (m_inputStr.length() < 15)
            m_inputStr += QString::number(d);
    }
    m_cur = m_inputStr.toDouble();
    refreshDisplay();
}

void CalculadoraDialog::pressDecimal()
{
    if (m_justDone || !m_hasInput) {
        m_inputStr = QStringLiteral("0.");
        m_hasInput = true;
        m_justDone = false;
    } else if (!m_inputStr.contains(QLatin1Char('.'))) {
        m_inputStr += QLatin1Char('.');
    }
    refreshDisplay();
}

void CalculadoraDialog::pressOperador(QChar op)
{
    if (m_hasInput) {
        m_cur      = m_inputStr.toDouble();
        m_hasInput = false;
    }
    // Chain operation: evaluate pending op before storing new one
    if (!m_op.isNull() && !m_justDone)
        m_cur = aplicarOp(m_reg, m_cur, m_op);

    m_reg      = m_cur;
    m_op       = op;
    m_justDone = false;

    const QString opStr = (op == QLatin1Char('/')) ? QStringLiteral("÷")
                        : (op == QLatin1Char('*')) ? QStringLiteral("×")
                        : op;
    m_exprLabel->setText(QStringLiteral("%1 %2").arg(formatDisplay(m_reg), opStr));

    refreshDisplay();
}

void CalculadoraDialog::pressIgual()
{
    if (m_hasInput) {
        m_cur      = m_inputStr.toDouble();
        m_hasInput = false;
    }

    if (!m_op.isNull()) {
        const double b = m_cur;
        const double result = aplicarOp(m_reg, b, m_op);

        const QString opStr = (m_op == QLatin1Char('/')) ? QStringLiteral("÷")
                            : (m_op == QLatin1Char('*')) ? QStringLiteral("×")
                            : m_op;
        adicionarHistorico(QStringLiteral("%1 %2 %3 = %4")
            .arg(formatDisplay(m_reg), opStr, formatDisplay(b), formatDisplay(result)));

        m_cur = result;
        m_op  = {};
        m_reg = 0.0;
        m_exprLabel->clear();
    }

    m_justDone = true;
    refreshDisplay();
    QApplication::clipboard()->setText(m_display->text());
    emit resultadoDisponivel(m_cur);
}

void CalculadoraDialog::pressPorcentagem()
{
    if (m_hasInput) {
        m_cur      = m_inputStr.toDouble();
        m_hasInput = false;
    }
    m_cur      = m_op.isNull() ? (m_cur / 100.0)
                               : (m_reg * m_cur / 100.0);
    m_justDone = true;
    refreshDisplay();
}

void CalculadoraDialog::pressCE()
{
    m_cur      = 0.0;
    m_inputStr.clear();
    m_hasInput = false;
    m_justDone = true;
    refreshDisplay();
}

void CalculadoraDialog::pressC()
{
    m_cur      = 0.0;
    m_reg      = 0.0;
    m_op       = {};
    m_inputStr.clear();
    m_hasInput = false;
    m_justDone = false;
    m_exprLabel->clear();
    refreshDisplay();
}

void CalculadoraDialog::pressPlusMinus()
{
    if (m_hasInput) {
        if (m_inputStr.startsWith(QLatin1Char('-')))
            m_inputStr.remove(0, 1);
        else
            m_inputStr.prepend(QLatin1Char('-'));
        m_cur = m_inputStr.toDouble();
    } else {
        m_cur = -m_cur;
    }
    refreshDisplay();
}

void CalculadoraDialog::pressBackspace()
{
    if (!m_hasInput || m_inputStr.isEmpty()) return;
    m_inputStr.chop(1);
    if (m_inputStr.isEmpty() || m_inputStr == QStringLiteral("-"))
        m_inputStr = QStringLiteral("0");
    m_cur = m_inputStr.toDouble();
    refreshDisplay();
}

void CalculadoraDialog::adicionarHistorico(const QString &entry)
{
    m_historyItems.prepend(entry);
    if (m_historyItems.size() > 20) m_historyItems.removeLast();
    m_historico->clear();
    for (const auto &e : m_historyItems)
        m_historico->addItem(e);
}

void CalculadoraDialog::refreshDisplay()
{
    QString txt;
    if (m_hasInput) {
        if (m_inputStr.endsWith(QLatin1Char('.'))) {
            // Show trailing comma to confirm decimal was pressed
            txt = formatDisplay(m_inputStr.chopped(1).toDouble()) + QStringLiteral(",");
        } else {
            txt = formatDisplay(m_inputStr.toDouble());
        }
    } else {
        txt = formatDisplay(m_cur);
    }
    m_display->setText(txt);
}

QString CalculadoraDialog::formatDisplay(double v) const
{
    // Integers: no decimal part; fractional: up to 8 places, no trailing zeros
    if (v == std::floor(v) && std::abs(v) < 1e13)
        return QString::number(static_cast<qint64>(v));
    QString s = QString::number(v, 'f', 8);
    while (s.endsWith(QLatin1Char('0'))) s.chop(1);
    if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    return s.replace(QLatin1Char('.'), QLatin1Char(','));
}

double CalculadoraDialog::aplicarOp(double a, double b, QChar op) const
{
    if (op == QLatin1Char('+')) return a + b;
    if (op == QLatin1Char('-')) return a - b;
    if (op == QLatin1Char('*')) return a * b;
    if (op == QLatin1Char('/') && b != 0.0) return a / b;
    return b; // division by zero: return b unchanged
}
