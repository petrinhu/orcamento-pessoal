#include "CalculadoraDialog.h"
#include "GlossaryLabel.h"
#include "ui/GlossarioDecorator.h"
#include "ui/Theme.h"
#include "ui/UiFormatters.h"
#include "services/TvmService.h"
#include "services/ParcelaService.h"
#include "services/FinanceEngine.h"
#include "services/FiscalService.h"
#include "services/CalculadoraService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QToolButton>
#include <QTabWidget>
#include <QDateEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QFrame>
#include <QApplication>
#include <QClipboard>
#include <QSettings>
#include <QCloseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QComboBox>
#include <QFormLayout>
#include <QRadioButton>
#include <QStackedWidget>
#include <QTimer>
#include <cmath>
#include <memory>
#include <stdexcept>

// ─── construction ────────────────────────────────────────────────────────────

CalculadoraDialog::CalculadoraDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Calculadora"));
    setWindowFlags(Qt::Window | Qt::CustomizeWindowHint
                   | Qt::WindowTitleHint
                   | Qt::WindowCloseButtonHint
                   | Qt::WindowMinimizeButtonHint);
    setAttribute(Qt::WA_DeleteOnClose, false); // reuse across open/close cycles

    // Debounce 300ms para recalculos pesados disparados por textChanged (#95)
    m_debounceCet = new QTimer(this);
    m_debounceCet->setSingleShot(true);
    m_debounceCet->setInterval(300);
    connect(m_debounceCet, &QTimer::timeout, this, &CalculadoraDialog::atualizarCet);

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

    m_tabs = new QTabWidget(this);

    auto *tabComum = new QWidget;
    buildTabComum(tabComum);
    m_tabs->addTab(tabComum, tr("Comum"));

    auto *tabFin = new QWidget;
    buildTabFinanceira(tabFin);
    m_tabs->addTab(tabFin, tr("Financeira"));

    auto *tabWiz = new QWidget;
    buildTabWizards(tabWiz);
    m_tabs->addTab(tabWiz, tr("Wizards"));

    auto *tabBolsa = new QWidget;
    buildTabBolsa(tabBolsa);
    m_tabs->addTab(tabBolsa, tr("Bolsa"));

    auto *tabImoveis = new QWidget;
    buildTabImoveis(tabImoveis);
    m_tabs->addTab(tabImoveis, tr("Imóveis"));

    auto *tabRF = new QWidget;
    buildTabRendaFixa(tabRF);
    m_tabs->addTab(tabRF, tr("Renda Fixa"));

    auto *tabIR = new QWidget;
    buildTabIrTributos(tabIR);
    m_tabs->addTab(tabIR, tr("IR & Tributos"));

    auto *tabIC = new QWidget;
    buildTabInflacaoCambio(tabIC);
    m_tabs->addTab(tabIC, tr("Inflação"));

    root->addWidget(m_tabs);
    setMinimumWidth(300);
    adjustSize();

    // Cue de glossário (#166 §9.6): amplia a cobertura a TODAS as abas da
    // calculadora (Bolsa, Renda Fixa, IR, etc.). Os 4 GlossaryLabel curados da
    // tab Financeira já se auto-marcam ("glossarioUnderlined") e são pulados —
    // sem pintura dupla. Dialog modal não é alcançado pela varredura do MainWindow.
    GlossarioDecorator::aplicarTooltips(this);
}

void CalculadoraDialog::buildTabComum(QWidget *tab)
{
    auto *vbox = new QVBoxLayout(tab);
    vbox->setContentsMargins(4, 8, 4, 4);
    vbox->setSpacing(4);

    // ── expression label ───────────────────────────────────────────────────
    m_exprLabel = new QLabel(QString(), tab);
    m_exprLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_exprLabel->setObjectName(QStringLiteral("subtitle"));
    m_exprLabel->setMinimumHeight(18);
    vbox->addWidget(m_exprLabel);

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
        btnCopy->setIcon(Theme::emojiIcon(QStringLiteral("📋")));
        btnCopy->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btnCopy->setToolTip(tr("Copiar para área de transferência"));
        btnCopy->setAccessibleName(tr("Copiar resultado para área de transferência")); // #198 (A11Y-01)
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
        {QStringLiteral("="),    4, 3, 1, 1, QStringLiteral("primary")},
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

    // ── send-to-field button ───────────────────────────────────────────────
    m_btnTransferir = new QPushButton(tr("Enviar ao campo"), tab);
    m_btnTransferir->setObjectName(QStringLiteral("primary"));
    m_btnTransferir->setVisible(false);
    connect(m_btnTransferir, &QPushButton::clicked, this, [this]() {
        if (!m_op.isNull()) pressIgual();
        const auto resp = QMessageBox::question(
            this, tr("Transferir"),
            tr("Enviar %1 para o campo?").arg(m_display->text()),
            QMessageBox::Ok | QMessageBox::Cancel,
            QMessageBox::Ok);
        if (resp != QMessageBox::Ok) return;
        emit transferirSolicitado(m_cur);
    });
    vbox->addWidget(m_btnTransferir);

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

// ─── tab Financeira ───────────────────────────────────────────────────────────

void CalculadoraDialog::buildTabFinanceira(QWidget *tab)
{
    auto *vbox = new QVBoxLayout(tab);
    vbox->setContentsMargins(8, 8, 8, 8);
    vbox->setSpacing(5);

    // ── BEGIN / END + ALG label ────────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;

        m_tvmEnd = new QPushButton(tr("END"), tab);
        m_tvmEnd->setCheckable(true);
        m_tvmEnd->setChecked(true);
        m_tvmEnd->setFixedWidth(56);

        m_tvmBegin = new QPushButton(tr("BEGIN"), tab);
        m_tvmBegin->setCheckable(true);
        m_tvmBegin->setFixedWidth(56);

        connect(m_tvmEnd, &QPushButton::toggled, this, [this](bool on) {
            if (on) m_tvmBegin->setChecked(false);
            else if (!m_tvmBegin->isChecked()) m_tvmEnd->setChecked(true);
        });
        connect(m_tvmBegin, &QPushButton::toggled, this, [this](bool on) {
            if (on) m_tvmEnd->setChecked(false);
            else if (!m_tvmEnd->isChecked()) m_tvmBegin->setChecked(true);
        });

        auto *algLbl = new QLabel(QStringLiteral("ALG"), tab);
        algLbl->setObjectName(QStringLiteral("subtitle"));
        algLbl->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        row->addWidget(m_tvmEnd);
        row->addWidget(m_tvmBegin);
        row->addStretch();
        row->addWidget(algLbl);
        vbox->addLayout(row);
    }

    // ── Separator ─────────────────────────────────────────────────────────
    auto makeSep = [&]() {
        auto *s = new QFrame(tab);
        s->setFrameShape(QFrame::HLine);
        s->setFrameShadow(QFrame::Sunken);
        return s;
    };
    vbox->addWidget(makeSep());

    // ── TVM registers: N / i / PV / PMT / FV ─────────────────────────────
    // Grid columns: label | input | ▶ | unit
    auto *grid = new QGridLayout;
    grid->setSpacing(4);
    grid->setColumnStretch(1, 1);

    m_tvmN   = new QLineEdit(tab);
    m_tvmI   = new QLineEdit(tab);
    m_tvmPV  = new QLineEdit(tab);
    m_tvmPMT = new QLineEdit(tab);
    m_tvmFV  = new QLineEdit(tab);

    struct TvmRow { QString lbl; QString unit; QLineEdit *fld; int idx; };
    const QList<TvmRow> tvmRows = {
        {tr("N"),      {},      m_tvmN,   0},
        {tr("i/mês"),  tr("%"), m_tvmI,   1},
        {tr("PV"),     tr("R$"),m_tvmPV,  2},
        {tr("PMT"),    tr("R$"),m_tvmPMT, 3},
        {tr("FV"),     tr("R$"),m_tvmFV,  4},
    };

    for (int r = 0; r < tvmRows.size(); ++r) {
        const auto &row = tvmRows.at(r);

        // GlossaryLabel (#166 §9.6): sublinhado pontilhado + tooltip do glossario
        // para siglas TVM (PV/PMT/FV/NPER/RATE). "N" e "i/mes" nao tem entrada
        // propria no glossario — usam QLabel comum (sem cue vazio).
        auto *lbl = new GlossaryLabel(row.lbl + QStringLiteral(":"), tab);
        lbl->setMinimumWidth(46);

        row.fld->setPlaceholderText(QStringLiteral("0"));
        row.fld->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // U+25B6 puro (sem VS16/FE0F) → símbolo tipográfico monocromático (#165 fix).
        auto *btn = new QPushButton(QStringLiteral("▶"), tab);
        btn->setFixedWidth(28);
        btn->setToolTip(tr("Calcular %1").arg(row.lbl));
        const int idx = row.idx;
        connect(btn, &QPushButton::clicked, this, [this, idx]() { calcularTvm(idx); });

        auto *unitLbl = new QLabel(row.unit, tab);
        unitLbl->setMinimumWidth(22);
        unitLbl->setObjectName(QStringLiteral("subtitle"));

        grid->addWidget(lbl,      r, 0);
        grid->addWidget(row.fld,  r, 1);
        grid->addWidget(btn,      r, 2);
        grid->addWidget(unitLbl,  r, 3);
    }
    vbox->addLayout(grid);

    vbox->addWidget(makeSep());

    // ── CET (auto, read-only) ──────────────────────────────────────────────
    {
        auto *row = new QHBoxLayout;
        // GlossaryLabel (#166 §9.6): CET e o termo mais importante pro leigo
        auto *lbl = new GlossaryLabel(tr("CET:"), tab);
        lbl->setMinimumWidth(46);

        m_tvmCet = new QLineEdit(tab);
        m_tvmCet->setReadOnly(true);
        m_tvmCet->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_tvmCet->setPlaceholderText(QStringLiteral("—"));

        auto *unitLbl = new QLabel(tr("% a.a."), tab);
        unitLbl->setObjectName(QStringLiteral("subtitle"));

        row->addWidget(lbl);
        row->addWidget(m_tvmCet, 1);
        row->addWidget(unitLbl);
        vbox->addLayout(row);
    }
    // Debounce 300ms: cada keystroke reinicia o timer; atualizarCet so dispara
    // depois de 300ms de inatividade. Evita recalculo a cada tecla (#95).
    connect(m_tvmI, &QLineEdit::textChanged,
            this, [this]() { m_debounceCet->start(); });

    // ── Data de início (for amortization table) ────────────────────────────
    {
        auto *row = new QHBoxLayout;
        auto *lbl = new QLabel(tr("Data início:"), tab);
        lbl->setMinimumWidth(46);

        m_tvmData = new QDateEdit(QDate::currentDate(), tab);
        m_tvmData->setDisplayFormat(uifmt::dataDisplayFormat());
        m_tvmData->setCalendarPopup(true);

        row->addWidget(lbl);
        row->addWidget(m_tvmData, 1);
        vbox->addLayout(row);
    }

    vbox->addWidget(makeSep());

    // ── Amortization toggle + table ───────────────────────────────────────
    m_btnTabAmort = new QPushButton(tr("▼ Tabela de amortização"), tab);
    m_btnTabAmort->setCheckable(true);
    connect(m_btnTabAmort, &QPushButton::toggled, this, [this](bool on) {
        m_tabAmortPanel->setVisible(on);
        m_btnTabAmort->setText(on ? tr("▲ Tabela de amortização")
                                  : tr("▼ Tabela de amortização"));
        if (on) atualizarTabelaAmort();
        adjustSize();
    });
    vbox->addWidget(m_btnTabAmort);

    m_tabAmortPanel = new QWidget(tab);
    {
        auto *pl = new QVBoxLayout(m_tabAmortPanel);
        pl->setContentsMargins(0, 0, 0, 0);

        m_tvmTabela = new QTableWidget(0, 6, m_tabAmortPanel);
        m_tvmTabela->setHorizontalHeaderLabels({
            tr("Nº"), tr("Data"), tr("Saldo"), tr("Juros"), tr("Amort."), tr("PMT")
        });
        m_tvmTabela->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_tvmTabela->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_tvmTabela->verticalHeader()->hide();
        m_tvmTabela->setMinimumHeight(160);
        m_tvmTabela->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        pl->addWidget(m_tvmTabela);
    }
    m_tabAmortPanel->setVisible(false);
    vbox->addWidget(m_tabAmortPanel);

    // ── Bottom buttons ─────────────────────────────────────────────────────
    auto *btnLimpar = new QPushButton(tr("Limpar registros"), tab);
    connect(btnLimpar, &QPushButton::clicked, this, [this]() {
        for (auto *f : {m_tvmN, m_tvmI, m_tvmPV, m_tvmPMT, m_tvmFV, m_tvmCet})
            f->clear();
        m_tvmTabela->setRowCount(0);
    });
    vbox->addWidget(btnLimpar);

    auto *btnCartoes = new QPushButton(tr("Enviar para Cartões"), tab);
    btnCartoes->setObjectName(QStringLiteral("primary"));
    connect(btnCartoes, &QPushButton::clicked, this, [this]() {
        const double pv  = parseTvmField(m_tvmPV);
        const int    n   = static_cast<int>(parseTvmField(m_tvmN));
        const double iM  = parseTvmField(m_tvmI);
        const double pmt = parseTvmField(m_tvmPMT);
        emit tvmParaCartoes(pv, n, iM, pmt);
    });
    vbox->addWidget(btnCartoes);

    vbox->addStretch();
}

double CalculadoraDialog::parseTvmField(QLineEdit *le) const
{
    QString s = le->text().trimmed();
    s.remove(QLatin1Char('R')).remove(QLatin1Char('$')).remove(QLatin1Char('%')).remove(QLatin1Char(' '));
    if (s.contains(QLatin1Char(','))) {
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    return s.toDouble();
}

QString CalculadoraDialog::formatTvmResult(double v) const
{
    if (v == std::floor(v) && std::abs(v) < 1e12)
        return QString::number(static_cast<qint64>(v));
    QString s = QString::number(v, 'f', 8);
    while (s.endsWith(QLatin1Char('0'))) s.chop(1);
    if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    return s.replace(QLatin1Char('.'), QLatin1Char(','));
}

void CalculadoraDialog::calcularTvm(int campo)
{
    TvmInput in;
    in.n     = parseTvmField(m_tvmN);
    in.i     = parseTvmField(m_tvmI);
    in.pv    = parseTvmField(m_tvmPV);
    in.pmt   = parseTvmField(m_tvmPMT);
    in.fv    = parseTvmField(m_tvmFV);
    in.begin = m_tvmBegin->isChecked();

    try {
        const auto solve = static_cast<TvmSolve>(campo);
        const TvmInput out = TvmService::resolver(in, solve);
        switch (solve) {
        case TvmSolve::N:   m_tvmN->setText(formatTvmResult(out.n));   break;
        case TvmSolve::I:   m_tvmI->setText(formatTvmResult(out.i));   break;
        case TvmSolve::PV:  m_tvmPV->setText(formatTvmResult(out.pv)); break;
        case TvmSolve::PMT: m_tvmPMT->setText(formatTvmResult(out.pmt)); break;
        case TvmSolve::FV:  m_tvmFV->setText(formatTvmResult(out.fv)); break;
        }
        if (m_btnTabAmort->isChecked()) atualizarTabelaAmort();
    } catch (const std::exception &e) {
        QMessageBox::warning(this, tr("Erro no cálculo"),
            tr("Não foi possível calcular: %1").arg(QString::fromUtf8(e.what())));
    }
}

void CalculadoraDialog::atualizarCet()
{
    const double iM = parseTvmField(m_tvmI);
    if (iM <= 0.0) { m_tvmCet->clear(); return; }
    const double cet = TvmService::cetFromMensal(iM);
    m_tvmCet->setText(formatTvmResult(cet));
}

void CalculadoraDialog::atualizarTabelaAmort()
{
    m_tvmTabela->setRowCount(0);

    TvmInput in;
    in.n     = parseTvmField(m_tvmN);
    in.i     = parseTvmField(m_tvmI);
    in.pv    = parseTvmField(m_tvmPV);
    in.pmt   = parseTvmField(m_tvmPMT);
    in.fv    = parseTvmField(m_tvmFV);
    in.begin = m_tvmBegin->isChecked();

    if (in.n <= 0.0 || in.pmt == 0.0) return;

    const QList<AmortizacaoRow> rows = TvmService::amortizacao(in, m_tvmData->date());
    m_tvmTabela->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        const auto &row = rows.at(r);
        auto *iNum  = new QTableWidgetItem(QString::number(row.numero));
        auto *iData = new QTableWidgetItem(row.data.toString(uifmt::dataDisplayFormat()));
        auto *iSaldo = new QTableWidgetItem(ParcelaService::formatarValor(row.saldoDevedor));
        auto *iJuros = new QTableWidgetItem(ParcelaService::formatarValor(row.juros));
        auto *iAmort = new QTableWidgetItem(ParcelaService::formatarValor(row.amortizacao));
        auto *iPmt   = new QTableWidgetItem(ParcelaService::formatarValor(row.parcela));

        for (auto *item : {iSaldo, iJuros, iAmort, iPmt})
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_tvmTabela->setItem(r, 0, iNum);
        m_tvmTabela->setItem(r, 1, iData);
        m_tvmTabela->setItem(r, 2, iSaldo);
        m_tvmTabela->setItem(r, 3, iJuros);
        m_tvmTabela->setItem(r, 4, iAmort);
        m_tvmTabela->setItem(r, 5, iPmt);
    }
}

// ─── public API ──────────────────────────────────────────────────────────────

void CalculadoraDialog::setTemInvoker(bool has)
{
    m_btnTransferir->setVisible(has);
}

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

// ─── tab Wizards ──────────────────────────────────────────────────────────────

// ── Wizard helpers (ARQ-03, auditoria 2026-06-22 §3.3: extraídos de
// buildTabWizards -- eram lambdas locais duplicadas em cada painel; viraram
// métodos privados reutilizados pelos 20 buildWizardX() abaixo) ────────────
double CalculadoraDialog::wizParseF(QLineEdit *le)
{
    QString s = le->text().trimmed();
    s.remove(QLatin1Char('R')).remove(QLatin1Char('$'))
     .remove(QLatin1Char('%')).remove(QLatin1Char(' '));
    if (s.contains(QLatin1Char(','))) {
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    return s.toDouble();
}

QString CalculadoraDialog::wizFmtF(double v, int dec)
{
    if (v == std::floor(v) && std::abs(v) < 1e12)
        return QString::number(static_cast<qint64>(v));
    QString s = QString::number(v, 'f', dec);
    while (s.endsWith(QLatin1Char('0'))) s.chop(1);
    if (s.endsWith(QLatin1Char('.'))) s.chop(1);
    return s.replace(QLatin1Char('.'), QLatin1Char(','));
}

// Creates a styled QLineEdit and QLabel pair for a form row
QLineEdit *CalculadoraDialog::wizMakeLE(QWidget *parent)
{
    auto *le = new QLineEdit(parent);
    le->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    le->setPlaceholderText(QStringLiteral("0"));
    return le;
}

// Creates a result label (hidden until first calculation)
QLabel *CalculadoraDialog::wizMakeResLbl(QWidget *parent)
{
    auto *lbl = new QLabel(parent);
    lbl->setWordWrap(true);
    lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lbl->hide();
    return lbl;
}

// Creates "↑ Usar na Comum" button (hidden until first calculation)
QPushButton *CalculadoraDialog::wizMakeUsarBtn(QWidget *parent, std::shared_ptr<double> val)
{
    auto *btn = new QPushButton(tr("↑ Usar na Comum"), parent);
    btn->hide();
    connect(btn, &QPushButton::clicked, this, [this, val]() {
        setValorInicial(*val);
        m_tabs->setCurrentIndex(0);
    });
    return btn;
}

void CalculadoraDialog::buildTabWizards(QWidget *tab)
{
    auto *vbox = new QVBoxLayout(tab);
    vbox->setContentsMargins(8, 8, 8, 8);
    vbox->setSpacing(6);

    // ── Selector ──────────────────────────────────────────────────────────────
    auto *combo = new QComboBox(tab);
    combo->addItems({
        tr("1. CET ↔ Taxa mensal"),
        tr("2. Equivalência de taxas"),
        tr("3. Parcelas — Price"),
        tr("4. Parcelas — SAC"),
        tr("5. Valor futuro"),
        tr("6. Renda passiva"),
        tr("7. ROI"),
        tr("8. Comprometimento de renda"),
        tr("9. Saldo devedor (quitação)"),
        tr("10. Meta financeira"),
        tr("11. Recálculo com amort. parcial"),
        tr("12. Comparador RF — CDB vs LCI/LCA"),
        tr("13. FIRE Number (Regra dos 25)"),
        tr("14. Lean FIRE"),
        tr("15. Fat FIRE"),
        tr("16. Coast FIRE"),
        tr("17. Barista FIRE"),
        tr("18. Geographic Arbitrage"),
        tr("19. Reserva de Emergência"),
        tr("20. CLT vs PJ"),
    });
    vbox->addWidget(combo);

    m_wizStack = new QStackedWidget(tab);
    vbox->addWidget(m_wizStack, 1);

    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            m_wizStack, &QStackedWidget::setCurrentIndex);

    // ── Painéis (ARQ-03: cada wizard extraído para método próprio; esta
    //    função ficou como registrador fino, não mais god-function) ────────
    m_wizStack->addWidget(buildWizardCetTaxaMensal());
    m_wizStack->addWidget(buildWizardEquivalenciaTaxas());
    m_wizStack->addWidget(buildWizardParcelasPrice());
    m_wizStack->addWidget(buildWizardParcelasSac());
    m_wizStack->addWidget(buildWizardValorFuturo());
    m_wizStack->addWidget(buildWizardRendaPassiva());
    m_wizStack->addWidget(buildWizardRoi());
    m_wizStack->addWidget(buildWizardComprometimentoRenda());
    m_wizStack->addWidget(buildWizardSaldoDevedor());
    m_wizStack->addWidget(buildWizardMetaFinanceira());
    m_wizStack->addWidget(buildWizardAmortizacaoParcial());
    m_wizStack->addWidget(buildWizardComparadorRf());
    m_wizStack->addWidget(buildWizardFireNumber());
    m_wizStack->addWidget(buildWizardLeanFire());
    m_wizStack->addWidget(buildWizardFatFire());
    m_wizStack->addWidget(buildWizardCoastFire());
    m_wizStack->addWidget(buildWizardBaristaFire());
    m_wizStack->addWidget(buildWizardGeographicArbitrage());
    m_wizStack->addWidget(buildWizardReservaEmergencia());
    m_wizStack->addWidget(buildWizardCltVsPj());
}

// ── Wizard 1: CET ↔ Taxa mensal ──────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardCetTaxaMensal()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *rbMensal = new QRadioButton(tr("Taxa mensal → CET (% a.a.)"), panel);
    auto *rbCet    = new QRadioButton(tr("CET (% a.a.) → taxa mensal"), panel);
    rbMensal->setChecked(true);
    lay->addWidget(rbMensal);
    lay->addWidget(rbCet);

    auto *form   = new QFormLayout;
    auto *edtIn  = wizMakeLE(panel);
    auto *lblUnit = new QLabel(QStringLiteral("% a.m."), panel);
    lblUnit->setObjectName(QStringLiteral("subtitle"));
    auto *rowIn = new QHBoxLayout;
    rowIn->addWidget(edtIn, 1);
    rowIn->addWidget(lblUnit);
    form->addRow(tr("Taxa:"), rowIn);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(rbMensal, &QRadioButton::toggled, this, [lblUnit](bool on) {
        lblUnit->setText(on ? QStringLiteral("% a.m.") : QStringLiteral("% a.a."));
    });
    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double v = wizParseF(edtIn);
        if (v <= 0.0) { lblRes->setText(tr("Informe um valor maior que zero.")); lblRes->show(); return; }
        double res;
        if (rbMensal->isChecked()) {
            res = TvmService::cetFromMensal(v);
            lblRes->setText(tr("CET: <b>%1 % a.a.</b>").arg(wizFmtF(res, 4)));
        } else {
            res = TvmService::mensalFromCet(v);
            lblRes->setText(tr("Taxa mensal: <b>%1 % a.m.</b>").arg(wizFmtF(res, 6)));
        }
        *resVal = res;
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 2: Equivalência de taxas ───────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardEquivalenciaTaxas()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    const QStringList periodos = {
        tr("Dia"), tr("Semana"), tr("Quinzena"), tr("Mês"),
        tr("Bimestre"), tr("Trimestre"), tr("Semestre"), tr("Ano"),
    };
    const QList<int> diasPer = {1, 7, 15, 30, 60, 90, 180, 360};

    auto *form   = new QFormLayout;
    auto *edtTxa = wizMakeLE(panel);
    auto *cbOrig = new QComboBox(panel);
    cbOrig->addItems(periodos);
    cbOrig->setCurrentIndex(3); // Mês
    auto *cbDest = new QComboBox(panel);
    cbDest->addItems(periodos);
    cbDest->setCurrentIndex(7); // Ano
    form->addRow(tr("Taxa (%):"), edtTxa);
    form->addRow(tr("Período origem:"), cbOrig);
    form->addRow(tr("Período destino:"), cbDest);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double taxa = wizParseF(edtTxa);
        const int    nOrig = diasPer.at(cbOrig->currentIndex());
        const int    nDest = diasPer.at(cbDest->currentIndex());
        // #69: dedup -> FinanceEngine::taxaEquivalente (regime composto)
        const double equiv = FinanceEngine::taxaEquivalente(
                                 taxa / 100.0, nDest, nOrig) * 100.0;
        *resVal = equiv;
        lblRes->setText(tr("Taxa equivalente: <b>%1 % por %2</b>")
            .arg(wizFmtF(equiv, 6), cbDest->currentText().toLower()));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 3: Parcelas — Price ────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardParcelasPrice()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form  = new QFormLayout;
    auto *edtPV = wizMakeLE(panel);
    auto *edtI  = wizMakeLE(panel);
    auto *edtN  = wizMakeLE(panel);
    form->addRow(tr("Valor financiado (PV):"), edtPV);
    form->addRow(tr("Taxa mensal (%):"), edtI);
    form->addRow(tr("Nº de parcelas (N):"), edtN);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double pv = wizParseF(edtPV);
        const double i  = wizParseF(edtI) / 100.0;
        const double n  = wizParseF(edtN);
        if (pv <= 0.0 || n <= 0.0) {
            lblRes->setText(tr("Informe PV e N maiores que zero.")); lblRes->show(); return;
        }
        // #69: dedup -> FinanceEngine::pmt (PV>0 -> retorna pmt<0, inverter)
        const double pmt   = -FinanceEngine::pmt(pv, 0.0, i, static_cast<int>(n));
        const double total  = pmt * n;
        const double juros  = total - pv;
        *resVal = pmt;
        lblRes->setText(
            tr("Parcela (PMT): <b>R$ %1</b><br>"
               "Total pago: R$ %2<br>"
               "Juros totais: R$ %3")
            .arg(wizFmtF(pmt, 2), wizFmtF(total, 2), wizFmtF(juros, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 4: Parcelas — SAC ──────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardParcelasSac()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form  = new QFormLayout;
    auto *edtPV = wizMakeLE(panel);
    auto *edtI  = wizMakeLE(panel);
    auto *edtN  = wizMakeLE(panel);
    form->addRow(tr("Valor financiado (PV):"), edtPV);
    form->addRow(tr("Taxa mensal (%):"), edtI);
    form->addRow(tr("Nº de parcelas (N):"), edtN);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double pv = wizParseF(edtPV);
        const double i  = wizParseF(edtI) / 100.0;
        const int    n  = static_cast<int>(wizParseF(edtN));
        if (pv <= 0.0 || n <= 0) {
            lblRes->setText(tr("Informe PV e N maiores que zero.")); lblRes->show(); return;
        }
        const double amort = pv / n;
        const double pmt1  = amort + pv * i;
        const double pmtN  = amort * (1.0 + i);
        // Juros totais SAC = i * PV * (N+1) / 2
        const double juros = i * pv * (n + 1) / 2.0;
        const double total = pv + juros;
        *resVal = pmt1;
        lblRes->setText(
            tr("1ª parcela: <b>R$ %1</b><br>"
               "Última parcela: R$ %2<br>"
               "Total pago: R$ %3<br>"
               "Juros totais: R$ %4")
            .arg(wizFmtF(pmt1, 2), wizFmtF(pmtN, 2), wizFmtF(total, 2), wizFmtF(juros, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 5: Valor futuro ────────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardValorFuturo()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form  = new QFormLayout;
    auto *edtPV = wizMakeLE(panel);
    auto *edtI  = wizMakeLE(panel);
    auto *edtN  = wizMakeLE(panel);
    form->addRow(tr("Capital inicial (PV):"), edtPV);
    form->addRow(tr("Taxa mensal (%):"), edtI);
    form->addRow(tr("Prazo (meses):"), edtN);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double pv = wizParseF(edtPV);
        const double i  = wizParseF(edtI) / 100.0;
        const double n  = wizParseF(edtN);
        if (pv <= 0.0 || n <= 0.0) {
            lblRes->setText(tr("Informe PV e N maiores que zero.")); lblRes->show(); return;
        }
        // #69: dedup -> FinanceEngine::fv (sinal: pv>0 retorna fv<0)
        const double fv    = -FinanceEngine::fv(pv, 0.0, i, static_cast<int>(n));
        const double ganho = fv - pv;
        *resVal = fv;
        lblRes->setText(
            tr("Valor futuro (FV): <b>R$ %1</b><br>"
               "Rendimento: R$ %2 (%3 %)")
            .arg(wizFmtF(fv, 2), wizFmtF(ganho, 2), wizFmtF(ganho / pv * 100.0, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 6: Renda passiva ───────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardRendaPassiva()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form   = new QFormLayout;
    auto *edtCap = wizMakeLE(panel);
    auto *edtI   = wizMakeLE(panel);
    form->addRow(tr("Capital (R$):"), edtCap);
    form->addRow(tr("Taxa mensal (%):"), edtI);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double cap = wizParseF(edtCap);
        const double i   = wizParseF(edtI) / 100.0;
        if (cap <= 0.0 || i <= 0.0) {
            lblRes->setText(tr("Informe capital e taxa maiores que zero.")); lblRes->show(); return;
        }
        const double pmt   = cap * i;
        const double anual = pmt * 12.0;
        *resVal = pmt;
        lblRes->setText(
            tr("Renda mensal: <b>R$ %1</b><br>"
               "Renda anual: R$ %2")
            .arg(wizFmtF(pmt, 2), wizFmtF(anual, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 7: ROI ─────────────────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardRoi()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form    = new QFormLayout;
    auto *edtInv  = wizMakeLE(panel);
    auto *edtRet  = wizMakeLE(panel);
    auto *edtMeses = wizMakeLE(panel);
    edtMeses->setPlaceholderText(tr("Opcional"));
    form->addRow(tr("Investimento (R$):"), edtInv);
    form->addRow(tr("Retorno total (R$):"), edtRet);
    form->addRow(tr("Prazo (meses, opcional):"), edtMeses);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double inv   = wizParseF(edtInv);
        const double ret   = wizParseF(edtRet);
        const double meses = wizParseF(edtMeses);
        if (inv <= 0.0) {
            lblRes->setText(tr("Investimento deve ser maior que zero.")); lblRes->show(); return;
        }
        const double ganho = ret - inv;
        const double roi   = ganho / inv * 100.0;
        *resVal = roi;
        QString txt = tr("Ganho/Perda: <b>R$ %1</b><br>ROI: <b>%2 %</b>")
            .arg(wizFmtF(ganho, 2), wizFmtF(roi, 4));
        if (meses > 0.0 && (1.0 + roi / 100.0) > 0.0) {
            // #69: dedup -> FinanceEngine::taxaEquivalente
            const double iMes = FinanceEngine::taxaEquivalente(
                                    roi / 100.0, 1, static_cast<int>(meses)) * 100.0;
            txt += tr("<br>Taxa mensal equiv.: %1 % a.m.").arg(wizFmtF(iMes, 4));
        }
        lblRes->setText(txt);
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 8: Comprometimento de renda ────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardComprometimentoRenda()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form    = new QFormLayout;
    auto *edtPMT  = wizMakeLE(panel);
    auto *edtRend = wizMakeLE(panel);
    form->addRow(tr("Parcela / dívida mensal (R$):"), edtPMT);
    form->addRow(tr("Renda mensal líquida (R$):"), edtRend);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double pmt  = wizParseF(edtPMT);
        const double rend = wizParseF(edtRend);
        if (rend <= 0.0) {
            lblRes->setText(tr("Renda deve ser maior que zero.")); lblRes->show(); return;
        }
        const double pct = pmt / rend * 100.0;
        *resVal = pct;
        const QString semaforo = (pct < 30.0) ? tr("✅ Saudável (< 30 %)")
                               : (pct < 40.0) ? tr("⚠️ Atenção (30–40 %)")
                                              : tr("🚫 Comprometido (> 40 %)");
        lblRes->setText(
            tr("Comprometimento: <b>%1 %</b><br>%2<br>Disponível: R$ %3")
            .arg(wizFmtF(pct, 2), semaforo, wizFmtF(rend - pmt, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 9: Saldo devedor (quitação antecipada) ─────────────────────────
QWidget *CalculadoraDialog::buildWizardSaldoDevedor()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form  = new QFormLayout;
    auto *edtPV = wizMakeLE(panel);
    auto *edtI  = wizMakeLE(panel);
    auto *edtN  = wizMakeLE(panel);
    auto *edtK  = wizMakeLE(panel);
    form->addRow(tr("Valor financiado (PV):"), edtPV);
    form->addRow(tr("Taxa mensal (%):"), edtI);
    form->addRow(tr("Prazo total (N):"), edtN);
    form->addRow(tr("Parcelas já pagas (k):"), edtK);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double pv = wizParseF(edtPV);
        const double i  = wizParseF(edtI) / 100.0;
        const double n  = wizParseF(edtN);
        const double k  = wizParseF(edtK);
        if (pv <= 0.0 || n <= 0.0 || k < 0.0 || k >= n) {
            lblRes->setText(tr("Verifique os valores (k deve ser < N).")); lblRes->show(); return;
        }
        // #69/#190: dedup -> FinanceEngine::pmt / saldoDevedor
        const double pmt = -FinanceEngine::pmt(pv, 0.0, i, static_cast<int>(n));
        const double sd  = FinanceEngine::saldoDevedor(pv, i, static_cast<int>(n), k);
        *resVal = sd;
        lblRes->setText(
            tr("Parcela (PMT): R$ %1<br>"
               "Saldo devedor após %2 parcelas: <b>R$ %3</b><br>"
               "Parcelas restantes: %4")
            .arg(wizFmtF(pmt, 2),
                 QString::number(static_cast<qint64>(k)),
                 wizFmtF(sd, 2),
                 QString::number(static_cast<qint64>(n - k))));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 10: Meta financeira ────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardMetaFinanceira()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form  = new QFormLayout;
    auto *edtPV = wizMakeLE(panel);
    auto *edtFV = wizMakeLE(panel);
    auto *edtI  = wizMakeLE(panel);
    form->addRow(tr("Capital atual (R$):"), edtPV);
    form->addRow(tr("Meta (R$):"), edtFV);
    form->addRow(tr("Taxa mensal (%):"), edtI);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double pv = wizParseF(edtPV);
        const double fv = wizParseF(edtFV);
        const double i  = wizParseF(edtI) / 100.0;
        if (pv <= 0.0 || fv <= pv || i <= 0.0) {
            lblRes->setText(tr("Meta deve ser maior que o capital, e taxa > 0.")); lblRes->show(); return;
        }
        // #190: dedup -> FinanceEngine::nper (pv investido negativo,
        // fv meta positiva, sem aportes -> equivale a log(fv/pv)/log(1+i))
        const double n    = FinanceEngine::nper(-pv, 0.0, fv, i);
        const int    nInt = static_cast<int>(std::ceil(n));
        const int    anos = nInt / 12;
        const int    mes  = nInt % 12;
        *resVal = static_cast<double>(nInt);
        lblRes->setText(
            tr("Prazo necessário: <b>%1 meses</b><br>(%2 ano(s) e %3 mês(es))")
            .arg(nInt).arg(anos).arg(mes));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 11: Recálculo com amortização parcial ──────────────────────────
QWidget *CalculadoraDialog::buildWizardAmortizacaoParcial()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form     = new QFormLayout;
    auto *edtSD    = wizMakeLE(panel);
    auto *edtI     = wizMakeLE(panel);
    auto *edtNR    = wizMakeLE(panel);
    auto *edtAmort = wizMakeLE(panel);
    form->addRow(tr("Saldo devedor atual (R$):"), edtSD);
    form->addRow(tr("Taxa mensal (%):"), edtI);
    form->addRow(tr("Parcelas restantes (N):"), edtNR);
    form->addRow(tr("Amortização extra (R$):"), edtAmort);
    lay->addLayout(form);

    auto *rbRedN   = new QRadioButton(tr("Reduzir número de parcelas"), panel);
    auto *rbRedPMT = new QRadioButton(tr("Reduzir valor da parcela"), panel);
    rbRedN->setChecked(true);
    lay->addWidget(rbRedN);
    lay->addWidget(rbRedPMT);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double sd    = wizParseF(edtSD);
        const double i     = wizParseF(edtI) / 100.0;
        const double nr    = wizParseF(edtNR);
        const double amort = wizParseF(edtAmort);
        if (sd <= 0.0 || i <= 0.0 || nr <= 0.0 || amort <= 0.0 || amort >= sd) {
            lblRes->setText(tr("Verifique os valores (amortização deve ser < saldo devedor)."));
            lblRes->show(); return;
        }
        // #69: dedup -> FinanceEngine::pmt (sinal invertido para Price BR)
        const double pmtOrig      = -FinanceEngine::pmt(sd, 0.0, i, static_cast<int>(nr));
        const double novoSD       = sd - amort;
        const double jurosOrigTotal = pmtOrig * nr - sd;

        if (rbRedN->isChecked()) {
            double nNovo = 0.0;
            try {
                // #190: dedup -> FinanceEngine::nper (equivale
                // algebricamente a -log(1 - novoSD*i/pmtOrig)/log(1+i);
                // o domain_error cobre o guard "amortizacao insuficiente")
                nNovo = FinanceEngine::nper(-novoSD, pmtOrig, 0.0, i);
            } catch (const std::domain_error &) {
                lblRes->setText(tr("Amortização insuficiente para reduzir parcelas."));
                lblRes->show(); return;
            }
            const int    nInt    = static_cast<int>(std::ceil(nNovo));
            const double jurosNovo = pmtOrig * nNovo - novoSD;
            const double economia  = jurosOrigTotal - jurosNovo;
            *resVal = pmtOrig;
            lblRes->setText(
                tr("PMT: R$ %1 (inalterada)<br>"
                   "Novo N: <b>%2 parcelas</b> (era %3)<br>"
                   "Economia de juros: <b>R$ %4</b>")
                .arg(wizFmtF(pmtOrig, 2),
                     QString::number(nInt),
                     QString::number(static_cast<qint64>(nr)),
                     wizFmtF(economia, 2)));
        } else {
            // #69: dedup -> FinanceEngine::pmt
            const double pmtNovo   = -FinanceEngine::pmt(novoSD, 0.0, i, static_cast<int>(nr));
            const double jurosNovo = pmtNovo * nr - novoSD;
            const double economia  = jurosOrigTotal - jurosNovo;
            *resVal = pmtNovo;
            lblRes->setText(
                tr("Nova parcela (PMT): <b>R$ %1</b> (era R$ %2)<br>"
                   "N: %3 parcelas (inalterado)<br>"
                   "Economia de juros: <b>R$ %4</b>")
                .arg(wizFmtF(pmtNovo, 2),
                     wizFmtF(pmtOrig, 2),
                     QString::number(static_cast<qint64>(nr)),
                     wizFmtF(economia, 2)));
        }
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 12: Comparador RF — CDB vs LCI/LCA ────────────────────────────
QWidget *CalculadoraDialog::buildWizardComparadorRf()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form      = new QFormLayout;
    auto *edtValor  = wizMakeLE(panel);
    auto *edtMeses  = wizMakeLE(panel);
    auto *edtCDB    = wizMakeLE(panel);
    auto *edtLCI    = wizMakeLE(panel);
    edtValor->setPlaceholderText(tr("ex: 10000"));
    edtMeses->setPlaceholderText(tr("ex: 24"));
    edtCDB->setPlaceholderText(tr("ex: 13,5"));
    edtLCI->setPlaceholderText(tr("ex: 10,0"));
    form->addRow(tr("Valor inicial (R$):"),      edtValor);
    form->addRow(tr("Prazo (meses):"),            edtMeses);
    form->addRow(tr("Taxa CDB (% a.a.):"),        edtCDB);
    form->addRow(tr("Taxa LCI/LCA (% a.a.):"),   edtLCI);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes = wizMakeResLbl(panel);
    auto  resVal = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double valor = wizParseF(edtValor);
        const double meses = wizParseF(edtMeses);
        const double taxaCDB = wizParseF(edtCDB);
        const double taxaLCI = wizParseF(edtLCI);
        const int    n        = static_cast<int>(std::round(meses));
        if (valor <= 0.0 || meses <= 0.0 || taxaCDB <= 0.0 || n <= 0) {
            lblRes->setText(tr("Verifique os valores."));
            lblRes->show(); return;
        }
        // #190 (ARQ-02, CRITICO): IR regressivo CDB via FiscalService,
        // por DIAS CORRIDOS REAIS (nao "meses" aproximados por 30). O
        // wizard reimplementava a tabela com meses<=N e cruzava a
        // fronteira errada (6/12/24 meses de calendario tem MAIS dias
        // corridos que 180/360/720) -> cobrava +2,5 p.p. de IR a mais.
        const double ir = FiscalService::irRendaFixaPorMeses(meses);

        // #69/#190: dedup -> FinanceEngine::taxaEquivalente / fv
        const double taxaCDBm = FinanceEngine::taxaEquivalente(taxaCDB / 100.0, 1, 12);
        const double taxaLCIm = FinanceEngine::taxaEquivalente(taxaLCI / 100.0, 1, 12);

        // FV = valor*(1+i)^n via FinanceEngine::fv (pv negativo = saida
        // do bolso na convencao Excel/HP-12C); rendimento bruto = FV - valor.
        const double brutosCDB  = FinanceEngine::fv(-valor, 0.0, taxaCDBm, n) - valor;
        const double liquidoCDB = brutosCDB * (1.0 - ir);
        const double liquidoLCI = FinanceEngine::fv(-valor, 0.0, taxaLCIm, n) - valor;

        // Break-even: taxa LCI/LCA que iguala CDB líquido
        const double breakEvenAA = (std::pow(1.0 + taxaCDB / 100.0, 1.0 - ir) - 1.0) * 100.0;

        const bool cdbMelhor = liquidoCDB >= liquidoLCI;
        *resVal = cdbMelhor ? liquidoCDB : liquidoLCI;

        lblRes->setText(
            tr("CDB bruto: R$ %1 | IR (%2%): R$ %3<br>"
               "CDB <b>líquido: R$ %4</b><br>"
               "LCI/LCA líquido: R$ %5<br>"
               "Vencedor: <b>%6</b><br>"
               "Break-even LCI/LCA: %7% a.a.")
            .arg(wizFmtF(brutosCDB, 2),
                 wizFmtF(ir * 100.0, 1),
                 wizFmtF(brutosCDB * ir, 2),
                 wizFmtF(liquidoCDB, 2),
                 wizFmtF(liquidoLCI, 2),
                 cdbMelhor ? tr("CDB") : tr("LCI/LCA"),
                 wizFmtF(breakEvenAA, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 13: FIRE Number (Regra dos 25) ────────────────────────────────
QWidget *CalculadoraDialog::buildWizardFireNumber()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form       = new QFormLayout;
    auto *edtDesp    = wizMakeLE(panel);
    auto *edtTaxa    = wizMakeLE(panel);
    auto *edtInfla   = wizMakeLE(panel);
    auto *edtAnos    = wizMakeLE(panel);
    edtDesp->setPlaceholderText(tr("ex: 5000"));
    edtTaxa->setPlaceholderText(tr("ex: 7,0"));
    edtInfla->setPlaceholderText(tr("ex: 4,5"));
    edtAnos->setPlaceholderText(tr("ex: 20"));
    form->addRow(tr("Despesas mensais (R$):"),    edtDesp);
    form->addRow(tr("Retorno nominal (% a.a.):"), edtTaxa);
    form->addRow(tr("Inflação (% a.a.):"),        edtInfla);
    form->addRow(tr("Anos até FIRE:"),             edtAnos);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes = wizMakeResLbl(panel);
    auto  resVal = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double desp   = wizParseF(edtDesp);
        const double taxa   = wizParseF(edtTaxa);
        const double infla  = wizParseF(edtInfla);
        const double anos   = wizParseF(edtAnos);
        if (desp <= 0.0) {
            lblRes->setText(tr("Informe as despesas mensais."));
            lblRes->show(); return;
        }
        const double fireHoje    = desp * 12.0 * 25.0;
        const double taxaReal    = taxa > 0.0 && infla > 0.0
                                 ? ((1.0 + taxa / 100.0) / (1.0 + infla / 100.0) - 1.0) * 100.0
                                 : 0.0;
        // FIRE em valores futuros (corrigido pela inflação)
        const double fireFuturo  = anos > 0.0 && infla > 0.0
                                 ? fireHoje * std::pow(1.0 + infla / 100.0, anos)
                                 : fireHoje;
        *resVal = fireHoje;

        QString extra;
        if (taxaReal > 0.0)
            extra = tr("<br>Taxa real: %1% a.a.").arg(wizFmtF(taxaReal, 2));
        if (anos > 0.0)
            extra += tr("<br>FIRE em %1 anos (inflação): <b>R$ %2</b>")
                     .arg(wizFmtF(anos, 0), wizFmtF(fireFuturo, 2));

        lblRes->setText(
            tr("FIRE Number (hoje): <b>R$ %1</b><br>"
               "= R$ %2 × 12 × 25%3")
            .arg(wizFmtF(fireHoje, 2), wizFmtF(desp, 2), extra));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 14: Lean FIRE ─────────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardLeanFire()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form     = new QFormLayout;
    auto *edtDesp  = wizMakeLE(panel);
    auto *edtLean  = wizMakeLE(panel);
    edtDesp->setPlaceholderText(tr("ex: 5000"));
    edtLean->setPlaceholderText(tr("ex: 3000"));
    form->addRow(tr("Despesas atuais (R$/mês):"), edtDesp);
    form->addRow(tr("Despesas enxutas (R$/mês):"), edtLean);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes = wizMakeResLbl(panel);
    auto  resVal = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double desp = wizParseF(edtDesp);
        const double lean = wizParseF(edtLean);
        if (lean <= 0.0) {
            lblRes->setText(tr("Informe as despesas enxutas."));
            lblRes->show(); return;
        }
        const double leanFire = lean * 12.0 * 25.0;
        const double fireNorm = desp > 0.0 ? desp * 12.0 * 25.0 : 0.0;
        const double economia = fireNorm > 0.0 ? fireNorm - leanFire : 0.0;
        *resVal = leanFire;

        lblRes->setText(
            tr("Lean FIRE: <b>R$ %1</b><br>"
               "= R$ %2 × 12 × 25%3")
            .arg(wizFmtF(leanFire, 2),
                 wizFmtF(lean, 2),
                 fireNorm > 0.0
                     ? tr("<br>Redução vs FIRE normal: <b>R$ %1</b>").arg(wizFmtF(economia, 2))
                     : QString()));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 15: Fat FIRE ───────────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardFatFire()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form     = new QFormLayout;
    auto *edtDesp  = wizMakeLE(panel);
    auto *edtMult  = wizMakeLE(panel);
    edtDesp->setPlaceholderText(tr("ex: 15000"));
    edtMult->setPlaceholderText(tr("33 a 40 (padrão: 33)"));
    form->addRow(tr("Despesas mensais (R$):"), edtDesp);
    form->addRow(tr("Multiplicador:"),          edtMult);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes = wizMakeResLbl(panel);
    auto  resVal = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double desp = wizParseF(edtDesp);
        double mult = wizParseF(edtMult);
        if (desp <= 0.0) {
            lblRes->setText(tr("Informe as despesas mensais."));
            lblRes->show(); return;
        }
        if (mult < 25.0) mult = 33.0;  // padrão Fat FIRE
        const double fatFire = desp * 12.0 * mult;
        const double fireNorm = desp * 12.0 * 25.0;
        const double extra   = fatFire - fireNorm;
        *resVal = fatFire;

        lblRes->setText(
            tr("Fat FIRE: <b>R$ %1</b><br>"
               "= R$ %2 × 12 × %3<br>"
               "Acima do FIRE padrão: R$ %4 (+%5%)")
            .arg(wizFmtF(fatFire, 2),
                 wizFmtF(desp, 2),
                 wizFmtF(mult, 0),
                 wizFmtF(extra, 2),
                 wizFmtF((mult / 25.0 - 1.0) * 100.0, 1)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 16: Coast FIRE ─────────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardCoastFire()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form      = new QFormLayout;
    auto *edtFire   = wizMakeLE(panel);
    auto *edtTaxa   = wizMakeLE(panel);
    auto *edtAnos   = wizMakeLE(panel);
    edtFire->setPlaceholderText(tr("ex: 2500000"));
    edtTaxa->setPlaceholderText(tr("ex: 7,0"));
    edtAnos->setPlaceholderText(tr("ex: 25"));
    form->addRow(tr("FIRE objetivo (R$):"),      edtFire);
    form->addRow(tr("Taxa real (% a.a.):"),       edtTaxa);
    form->addRow(tr("Anos até aposentadoria:"),   edtAnos);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes = wizMakeResLbl(panel);
    auto  resVal = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double fire = wizParseF(edtFire);
        const double taxa = wizParseF(edtTaxa);
        const double anos = wizParseF(edtAnos);
        if (fire <= 0.0 || taxa <= 0.0 || anos <= 0.0) {
            lblRes->setText(tr("Verifique os valores."));
            lblRes->show(); return;
        }
        const double r         = taxa / 100.0;
        const double coastFire = fire / std::pow(1.0 + r, anos);
        *resVal = coastFire;

        lblRes->setText(
            tr("Coast FIRE: <b>R$ %1</b><br>"
               "Com esse patrimônio hoje, sem aportes,<br>"
               "em %2 anos a %3% a.a. real você atinge R$ %4.")
            .arg(wizFmtF(coastFire, 2),
                 wizFmtF(anos, 0),
                 wizFmtF(taxa, 2),
                 wizFmtF(fire, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 17: Barista FIRE ───────────────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardBaristaFire()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form       = new QFormLayout;
    auto *edtDesp    = wizMakeLE(panel);
    auto *edtParcial = wizMakeLE(panel);
    edtDesp->setPlaceholderText(tr("ex: 8000"));
    edtParcial->setPlaceholderText(tr("ex: 2000"));
    form->addRow(tr("Despesas mensais totais (R$):"),   edtDesp);
    form->addRow(tr("Renda parcial futura (R$/mês):"),  edtParcial);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes = wizMakeResLbl(panel);
    auto  resVal = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double desp    = wizParseF(edtDesp);
        const double parcial = wizParseF(edtParcial);
        if (desp <= 0.0) {
            lblRes->setText(tr("Informe as despesas mensais."));
            lblRes->show(); return;
        }
        const double gap        = std::max(0.0, desp - parcial);
        const double baristaFire = gap * 12.0 * 25.0;
        const double fireNorm    = desp * 12.0 * 25.0;
        const double reducao     = fireNorm - baristaFire;
        *resVal = baristaFire;

        lblRes->setText(
            tr("Barista FIRE: <b>R$ %1</b><br>"
               "Gap mensal a cobrir: R$ %2 (R$ %3 − R$ %4)<br>"
               "Redução vs FIRE padrão: R$ %5 (-%6%)")
            .arg(wizFmtF(baristaFire, 2),
                 wizFmtF(gap, 2),
                 wizFmtF(desp, 2),
                 wizFmtF(parcial, 2),
                 wizFmtF(reducao, 2),
                 wizFmtF(fireNorm > 0.0 ? reducao / fireNorm * 100.0 : 0.0, 1)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 18: Geographic Arbitrage ──────────────────────────────────────
QWidget *CalculadoraDialog::buildWizardGeographicArbitrage()
{
    auto *panel = new QWidget(m_wizStack);
    auto *lay   = new QVBoxLayout(panel);
    lay->setContentsMargins(4, 8, 4, 4);
    lay->setSpacing(8);

    auto *form       = new QFormLayout;
    auto *edtRenda   = wizMakeLE(panel);
    auto *edtOrigem  = wizMakeLE(panel);
    auto *edtDestino = wizMakeLE(panel);
    edtRenda->setPlaceholderText(tr("ex: 15000"));
    edtOrigem->setPlaceholderText(tr("ex: 100 (São Paulo = 100)"));
    edtDestino->setPlaceholderText(tr("ex: 45 (Florianópolis = 45)"));
    form->addRow(tr("Renda mensal (R$):"),          edtRenda);
    form->addRow(tr("Índice COL origem:"),           edtOrigem);
    form->addRow(tr("Índice COL destino:"),          edtDestino);
    lay->addLayout(form);

    auto *lblInfo = new QLabel(
        tr("COL = Custo de Vida relativo. Defina SP=100 como base e estime outros."),
        panel);
    lblInfo->setWordWrap(true);
    lblInfo->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    lay->addWidget(lblInfo);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes = wizMakeResLbl(panel);
    auto  resVal = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const double renda   = wizParseF(edtRenda);
        const double origem  = wizParseF(edtOrigem);
        const double destino = wizParseF(edtDestino);
        if (renda <= 0.0 || origem <= 0.0 || destino <= 0.0) {
            lblRes->setText(tr("Verifique os valores."));
            lblRes->show(); return;
        }
        const double fator         = origem / destino;
        const double poderCompra   = renda * fator;
        const double equivalenteLD = renda / fator;  // renda equivalente no destino
        *resVal = poderCompra;

        lblRes->setText(
            tr("Fator de arbitragem: <b>%1×</b><br>"
               "Poder de compra no destino: <b>R$ %2</b><br>"
               "Equivale a ganhar R$ %3 na origem<br>"
               "Economia mensal potencial: R$ %4")
            .arg(wizFmtF(fator, 2),
                 wizFmtF(poderCompra, 2),
                 wizFmtF(equivalenteLD, 2),
                 wizFmtF(poderCompra - renda, 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 19 — Reserva de Emergência (#61) ──────────────────────────────
// Compara 3 aplicacoes para acumulo: Poupanca, CDB 100% CDI (IR
// regressivo) e Tesouro Selic (IR + IOF). Usa FiscalService.
QWidget *CalculadoraDialog::buildWizardReservaEmergencia()
{
    auto *panel  = new QWidget;
    auto *lay    = new QVBoxLayout(panel);
    lay->setContentsMargins(8, 8, 8, 4);
    lay->setSpacing(6);

    lay->addWidget(new QLabel(tr("<b>Reserva de Emergência</b>"), panel));

    auto *form      = new QFormLayout;
    auto *edtDesp   = wizMakeLE(panel);
    auto *edtMeses  = wizMakeLE(panel);
    auto *edtSelic  = wizMakeLE(panel);
    auto *edtGuarda = wizMakeLE(panel);
    auto *edtAtual  = wizMakeLE(panel);
    edtDesp->setPlaceholderText(tr("ex: 3500"));
    edtMeses->setPlaceholderText(tr("6"));
    edtSelic->setPlaceholderText(tr("ex: 11,75 (% a.a.)"));
    edtGuarda->setPlaceholderText(tr("ex: 500"));
    edtAtual->setPlaceholderText(tr("0"));
    form->addRow(tr("Despesas mensais (R$):"), edtDesp);
    form->addRow(tr("Meses de reserva:"),       edtMeses);
    form->addRow(tr("Selic anual (%):"),        edtSelic);
    form->addRow(tr("Aporte mensal (R$):"),     edtGuarda);
    form->addRow(tr("Reserva atual (R$):"),     edtAtual);
    lay->addLayout(form);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    // FIN-04 (auditoria 2026-06-22 §3.1): calculo migrado para
    // CalculadoraService::calcularReservaEmergencia (qint64 centavos).
    // Campos monetarios via uifmt::parseCentavos (nao mais parseF/double);
    // parseF/fmtF ficam so para os campos NAO-monetarios (meses, Selic%)
    // e para FORMATAR o resultado na exibicao (regra: virgula so na
    // formatacao final).
    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const qint64 despCent   = uifmt::parseCentavos(edtDesp->text());
        double       meses      = wizParseF(edtMeses);
        const double selic      = wizParseF(edtSelic) / 100.0;
        const qint64 guardaCent = uifmt::parseCentavos(edtGuarda->text());
        const qint64 atualCent  = uifmt::parseCentavos(edtAtual->text());

        if (despCent <= 0) {
            lblRes->setText(tr("Informe as despesas."));
            lblRes->show(); return;
        }

        const auto r = CalculadoraService::calcularReservaEmergencia(
            despCent, meses, selic, guardaCent, atualCent);
        if (meses <= 0.0) meses = 6.0;
        *resVal = CalculadoraService::fromCentavos(r.metaCentavos);

        QString txt = tr("Meta: <b>R$ %1</b> (%2× despesas mensais)<br>"
                         "Gap atual: <b>R$ %3</b><br><br>")
                          .arg(wizFmtF(CalculadoraService::fromCentavos(r.metaCentavos), 2),
                               wizFmtF(meses, 0),
                               wizFmtF(CalculadoraService::fromCentavos(r.gapCentavos), 2));

        if (!r.temComparativo) {
            txt += tr("<i>Informe Selic e aporte para comparativo de aplicações.</i>");
            lblRes->setText(txt); lblRes->show(); btnUsar->show(); return;
        }

        txt += tr("<table cellspacing='4'>"
                  "<tr><th align='left'>Aplicação</th>"
                  "<th align='right'>Taxa líq./mês</th>"
                  "<th align='right'>Meses p/ meta</th>"
                  "<th align='right'>Renda plena</th></tr>");
        for (const auto &a : r.aplicacoes) {
            txt += tr("<tr><td>%1</td><td align='right'>%2%</td>"
                      "<td align='right'>%3</td>"
                      "<td align='right'>R$ %4</td></tr>")
                      .arg(a.nome, wizFmtF(a.taxaMensalLiquida * 100.0, 4),
                           wizFmtF(static_cast<double>(a.mesesParaMeta), 0),
                           wizFmtF(CalculadoraService::fromCentavos(a.rendaMensalPlenaCentavos), 2));
        }
        txt += QStringLiteral("</table>");

        lblRes->setText(txt);
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Wizard 20 — CLT vs PJ (#62) ──────────────────────────────────────────
// CLT: INSS + IRPF mensais via FiscalService (tabelas 2024).
// PJ : Simples Nacional anexos I-V (calculado por RBT12) ou Lucro
//      Presumido (~16,33% efetivo para servicos).
// Bonus: estima break-even — faturamento PJ que iguala liquido CLT.
QWidget *CalculadoraDialog::buildWizardCltVsPj()
{
    auto *panel  = new QWidget;
    auto *lay    = new QVBoxLayout(panel);
    lay->setContentsMargins(8, 8, 8, 4);
    lay->setSpacing(6);

    lay->addWidget(new QLabel(tr("<b>CLT vs PJ</b>"), panel));

    auto *form    = new QFormLayout;
    auto *edtCLT  = wizMakeLE(panel);
    auto *edtPJ   = wizMakeLE(panel);
    auto *cbReg   = new QComboBox(panel);
    cbReg->addItem(tr("Simples — Anexo I (comércio)"),                       1);
    cbReg->addItem(tr("Simples — Anexo II (indústria)"),                     2);
    cbReg->addItem(tr("Simples — Anexo III (serviços em geral)"),            3);
    cbReg->addItem(tr("Simples — Anexo IV (construção/limpeza/advocacia)"),  4);
    cbReg->addItem(tr("Simples — Anexo V (serviços intelectuais)"),          5);
    cbReg->addItem(tr("Lucro Presumido (serviços)"),                         0);
    edtCLT->setPlaceholderText(tr("ex: 8000"));
    edtPJ->setPlaceholderText(tr("ex: 12000"));
    form->addRow(tr("Salário CLT bruto (R$):"),  edtCLT);
    form->addRow(tr("Faturamento PJ (R$/mês):"), edtPJ);
    form->addRow(tr("Regime PJ:"),                cbReg);
    lay->addLayout(form);

    auto *lblInfo = new QLabel(
        tr("INSS escalonado 7,5–14% (teto R$ 908,85). IRPF tabela 2024. "
           "Simples calcula RBT12 = faturamento × 12."), panel);
    lblInfo->setWordWrap(true);
    lblInfo->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    lay->addWidget(lblInfo);

    auto *btnCalc = new QPushButton(tr("Calcular"), panel);
    btnCalc->setObjectName(QStringLiteral("primary"));
    lay->addWidget(btnCalc);

    auto *lblRes  = wizMakeResLbl(panel);
    auto  resVal  = std::make_shared<double>(0.0);
    auto *btnUsar = wizMakeUsarBtn(panel, resVal);
    lay->addWidget(lblRes);
    lay->addWidget(btnUsar);
    lay->addStretch();

    // FIN-04 (auditoria 2026-06-22 §3.1): calculo migrado para
    // CalculadoraService::calcularCltVsPj (qint64 centavos). Antes:
    // `double impPJ = pjFat * aliqPJ` -- multiplicacao monetaria simples
    // (NAO transcendental) em double, 100% inteirizavel. Parse dos
    // campos monetarios via uifmt::parseCentavos (nao mais parseF/
    // QString::toDouble, que nao reconhece milhar sem virgula -- ver
    // test_calculadora.cpp::provaBugParseFAntigoInterpretaMilharComoDecimal).
    // parseF/fmtF ficam so para exibicao/campos nao-monetarios.
    connect(btnCalc, &QPushButton::clicked, this, [=]() {
        const qint64 brutoCent = uifmt::parseCentavos(edtCLT->text());
        const qint64 pjFatCent = uifmt::parseCentavos(edtPJ->text());
        const int    reg       = cbReg->currentData().toInt();
        if (brutoCent <= 0 || pjFatCent <= 0) {
            lblRes->setText(tr("Verifique os valores.")); lblRes->show(); return;
        }

        const auto r = CalculadoraService::calcularCltVsPj(brutoCent, pjFatCent, reg);
        const QString regNome = (reg == 0)
            ? tr("Lucro Presumido (serviços)")
            : cbReg->currentText();

        *resVal = CalculadoraService::fromCentavos(r.diferencaCentavos);

        const QString sinal = r.diferencaCentavos >= 0 ? QStringLiteral("+") : QString();
        lblRes->setText(
            tr("<b>CLT</b><br>"
               "&nbsp;&nbsp;INSS: R$ %1 · IRPF: R$ %2 · <b>Líquido: R$ %3</b><br><br>"
               "<b>PJ — %4</b><br>"
               "&nbsp;&nbsp;Alíquota efetiva: %5%<br>"
               "&nbsp;&nbsp;Impostos: R$ %6 · <b>Líquido: R$ %7</b><br><br>"
               "Diferença PJ − CLT: <b>%8R$ %9</b><br>"
               "Break-even PJ (≈ liq. CLT): <b>R$ %10/mês</b>")
            .arg(wizFmtF(CalculadoraService::fromCentavos(r.inssCentavos), 2),
                 wizFmtF(CalculadoraService::fromCentavos(r.irpfCentavos), 2),
                 wizFmtF(CalculadoraService::fromCentavos(r.liquidoCltCentavos), 2),
                 regNome,
                 wizFmtF(r.aliquotaPj * 100.0, 4),
                 wizFmtF(CalculadoraService::fromCentavos(r.impostosPjCentavos), 2),
                 wizFmtF(CalculadoraService::fromCentavos(r.liquidoPjCentavos), 2),
                 sinal,
                 wizFmtF(CalculadoraService::fromCentavos(qAbs(r.diferencaCentavos)), 2),
                 wizFmtF(CalculadoraService::fromCentavos(r.breakEvenCentavos), 2)));
        lblRes->show();
        btnUsar->show();
    });
    return panel;
}

// ── Helpers compartilhados (impl simples para todas as abas) ─────────────
namespace {
double parseDouble(QLineEdit *le) {
    QString s = le->text().trimmed();
    s.remove(QLatin1Char('R')).remove(QLatin1Char('$')).remove(QLatin1Char('%'))
     .remove(QLatin1Char(' '));
    if (s.contains(QLatin1Char(',')))
        s.remove(QLatin1Char('.')).replace(QLatin1Char(','), QLatin1Char('.'));
    return s.toDouble();
}
QString fmtBR(double v, int dec = 2) {
    QString s = QString::number(v, 'f', dec);
    return s.replace(QLatin1Char('.'), QLatin1Char(','));
}
QLineEdit *mkLE(QWidget *p, const QString &placeholder = QString()) {
    auto *le = new QLineEdit(p);
    le->setAlignment(Qt::AlignRight);
    if (!placeholder.isEmpty()) le->setPlaceholderText(placeholder);
    return le;
}
}

// ── #43 Bolsa de Valores ─────────────────────────────────────────────────
void CalculadoraDialog::buildTabBolsa(QWidget *tab)
{
    auto *v = new QVBoxLayout(tab);
    v->addWidget(new QLabel(tr("<b>Calculadora de Bolsa</b><br>"
        "Payback simples + ROI sobre dividendos."), tab));
    auto *form = new QFormLayout;
    auto *edtPreco = mkLE(tab, tr("ex: 28,50"));
    auto *edtDiv   = mkLE(tab, tr("ex: 2,40 (anual)"));
    auto *edtMeta  = mkLE(tab, tr("ex: 100 (%)"));
    form->addRow(tr("Preço atual (R$):"),        edtPreco);
    form->addRow(tr("Dividendo anual/papel:"),   edtDiv);
    form->addRow(tr("Meta de retorno (%):"),     edtMeta);
    v->addLayout(form);

    auto *btn = new QPushButton(tr("Calcular"), tab);
    btn->setObjectName(QStringLiteral("primary"));
    v->addWidget(btn);
    auto *lbl = new QLabel(tab); lbl->setWordWrap(true); v->addWidget(lbl);
    v->addStretch();

    connect(btn, &QPushButton::clicked, tab, [=]() {
        const double preco = parseDouble(edtPreco);
        const double div   = parseDouble(edtDiv);
        const double meta  = parseDouble(edtMeta) / 100.0;
        if (preco <= 0 || div <= 0) { lbl->setText(tr("Informe preço e dividendo.")); return; }
        const double dy        = div / preco * 100.0;
        const double payback   = preco / div;
        const double precoMeta = div / meta;
        lbl->setText(tr(
            "<b>DY:</b> %1 %<br>"
            "<b>Payback (anos via dividendos):</b> %2<br>"
            "<b>Preço-teto para meta %3 %:</b> R$ %4")
            .arg(fmtBR(dy, 4), fmtBR(payback, 2),
                 fmtBR(meta * 100.0, 2), fmtBR(precoMeta, 2)));
    });
}

// ── #54 Imóveis ──────────────────────────────────────────────────────────
void CalculadoraDialog::buildTabImoveis(QWidget *tab)
{
    auto *v = new QVBoxLayout(tab);
    v->addWidget(new QLabel(tr("<b>Calculadora Imóveis</b><br>"
        "ITBI + custos + comparativo SAC × Price."), tab));
    auto *form = new QFormLayout;
    auto *edtVal   = mkLE(tab, tr("ex: 400000"));
    auto *edtITBI  = mkLE(tab, tr("3,0 (% — varia por cidade)"));
    auto *edtEntr  = mkLE(tab, tr("ex: 100000"));
    auto *edtTaxa  = mkLE(tab, tr("0,8 (% a.m.)"));
    auto *edtPrazo = mkLE(tab, tr("360 (meses)"));
    form->addRow(tr("Valor do imóvel (R$):"),   edtVal);
    form->addRow(tr("ITBI (%):"),                edtITBI);
    form->addRow(tr("Entrada (R$):"),            edtEntr);
    form->addRow(tr("Juros mensais (%):"),       edtTaxa);
    form->addRow(tr("Prazo (meses):"),           edtPrazo);
    v->addLayout(form);

    auto *btn = new QPushButton(tr("Calcular"), tab);
    btn->setObjectName(QStringLiteral("primary"));
    v->addWidget(btn);
    auto *lbl = new QLabel(tab); lbl->setWordWrap(true); v->addWidget(lbl);
    v->addStretch();

    connect(btn, &QPushButton::clicked, tab, [=]() {
        const double val = parseDouble(edtVal);
        const double itbi = parseDouble(edtITBI) / 100.0;
        const double entr = parseDouble(edtEntr);
        const double i    = parseDouble(edtTaxa) / 100.0;
        const int    n    = static_cast<int>(parseDouble(edtPrazo));
        if (val <= 0 || n <= 0 || entr >= val) {
            lbl->setText(tr("Verifique os valores."));
            return;
        }
        const double custosITBI = val * itbi;
        const double fin        = val - entr;
        // Price: pmt fixa
        const double pmtPrice = (i > 0)
            ? -FinanceEngine::pmt(fin, 0.0, i, n)
            : fin / n;
        // SAC: amortização fixa, parcelas decrescentes; primeira parcela
        const double amortSAC = fin / n;
        const double pmtSAC1  = amortSAC + fin * i;
        const double pmtSACn  = amortSAC + amortSAC * i; // ultima
        lbl->setText(tr(
            "<b>Custos:</b> ITBI R$ %1<br>"
            "Valor financiado: R$ %2<br><br>"
            "<b>Price:</b> parcela fixa R$ %3 · total R$ %4<br>"
            "<b>SAC:</b> 1ª parcela R$ %5, última R$ %6 · total R$ %7")
            .arg(fmtBR(custosITBI, 2), fmtBR(fin, 2),
                 fmtBR(pmtPrice, 2), fmtBR(pmtPrice * n, 2),
                 fmtBR(pmtSAC1, 2), fmtBR(pmtSACn, 2),
                 fmtBR((pmtSAC1 + pmtSACn) / 2.0 * n, 2)));
    });
}

// ── #55 Renda Fixa ───────────────────────────────────────────────────────
void CalculadoraDialog::buildTabRendaFixa(QWidget *tab)
{
    auto *v = new QVBoxLayout(tab);
    v->addWidget(new QLabel(tr("<b>Calculadora Renda Fixa</b><br>"
        "Aporte único + prazo · comparativo bruto/líquido CDB/LCI/Tesouro."), tab));
    auto *form = new QFormLayout;
    auto *edtAporte = mkLE(tab, tr("ex: 10000"));
    auto *edtSelic  = mkLE(tab, tr("11,75"));
    auto *edtCDIPct = mkLE(tab, tr("100 (% CDI)"));
    auto *edtDias   = mkLE(tab, tr("ex: 1095 (3 anos)"));
    form->addRow(tr("Aporte (R$):"),       edtAporte);
    form->addRow(tr("Selic anual (%):"),    edtSelic);
    form->addRow(tr("% do CDI:"),           edtCDIPct);
    form->addRow(tr("Prazo (dias):"),       edtDias);
    v->addLayout(form);

    auto *btn = new QPushButton(tr("Calcular"), tab);
    btn->setObjectName(QStringLiteral("primary"));
    v->addWidget(btn);
    auto *lbl = new QLabel(tab); lbl->setWordWrap(true); v->addWidget(lbl);
    v->addStretch();

    connect(btn, &QPushButton::clicked, tab, [=]() {
        const double aporte = parseDouble(edtAporte);
        const double selic  = parseDouble(edtSelic) / 100.0;
        const double cdiPct = parseDouble(edtCDIPct) / 100.0;
        const int    dias   = static_cast<int>(parseDouble(edtDias));
        if (aporte <= 0 || selic <= 0 || dias <= 0) {
            lbl->setText(tr("Verifique os valores."));
            return;
        }
        const double anos    = dias / 365.0;
        const double cdiAA   = selic; // CDI ~ Selic
        const double txCDB   = cdiAA * cdiPct;
        const double brutoCDB = aporte * std::pow(1.0 + txCDB, anos);
        const double ganhoCDB = brutoCDB - aporte;
        const double aliqIR   = FiscalService::irRendaFixaPorDias(dias);
        const double liquidoCDB = aporte + ganhoCDB * (1.0 - aliqIR);
        // LCI/LCA: isento IR; tipico 90% CDI
        const double brutoLCI = aporte * std::pow(1.0 + cdiAA * 0.90, anos);
        // Poupanca
        const double mPoup    = FiscalService::poupancaMensal(selic);
        const double brutoPoup = aporte * std::pow(1.0 + mPoup, anos * 12.0);
        lbl->setText(tr(
            "<b>CDB %1% CDI</b><br>"
            "&nbsp;&nbsp;Bruto: R$ %2 · IR %3% · <b>Líquido: R$ %4</b><br><br>"
            "<b>LCI/LCA 90% CDI (isento IR):</b> R$ %5<br><br>"
            "<b>Poupança</b> (taxa Selic > 8,5% = 0,5% a.m.): R$ %6")
            .arg(fmtBR(cdiPct * 100.0, 0),
                 fmtBR(brutoCDB, 2),
                 fmtBR(aliqIR * 100.0, 1),
                 fmtBR(liquidoCDB, 2),
                 fmtBR(brutoLCI, 2),
                 fmtBR(brutoPoup, 2)));
    });
}

// ── #56 IR & Tributos ────────────────────────────────────────────────────
void CalculadoraDialog::buildTabIrTributos(QWidget *tab)
{
    auto *v = new QVBoxLayout(tab);
    v->addWidget(new QLabel(tr("<b>Calculadora IRPF Anual</b><br>"
        "Base anual + dedutíveis · imposto devido · simples vs completa."), tab));
    auto *form = new QFormLayout;
    auto *edtRend  = mkLE(tab, tr("ex: 120000 (anual)"));
    auto *edtINSS  = mkLE(tab, tr("ex: 9600"));
    auto *edtDep   = mkLE(tab, tr("ex: 2 (dependentes)"));
    auto *edtSaude = mkLE(tab, tr("ex: 4800 (despesas saúde)"));
    auto *edtEduc  = mkLE(tab, tr("ex: 3000 (educação, teto R$ 3.561,50)"));
    auto *edtPrev  = mkLE(tab, tr("ex: 12000 (PGBL ate 12% renda)"));
    form->addRow(tr("Rendimento tributável anual (R$):"), edtRend);
    form->addRow(tr("INSS anual (R$):"),                  edtINSS);
    form->addRow(tr("Dependentes:"),                       edtDep);
    form->addRow(tr("Despesas saúde (R$):"),               edtSaude);
    form->addRow(tr("Despesas educação (R$):"),            edtEduc);
    form->addRow(tr("PGBL (R$):"),                          edtPrev);
    v->addLayout(form);

    auto *btn = new QPushButton(tr("Calcular"), tab);
    btn->setObjectName(QStringLiteral("primary"));
    v->addWidget(btn);
    auto *lbl = new QLabel(tab); lbl->setWordWrap(true); v->addWidget(lbl);
    v->addStretch();

    connect(btn, &QPushButton::clicked, tab, [=]() {
        const double rend  = parseDouble(edtRend);
        const double inss  = parseDouble(edtINSS);
        const int    dep   = static_cast<int>(parseDouble(edtDep));
        const double saude = parseDouble(edtSaude);
        double educ        = parseDouble(edtEduc);
        const double prev  = std::min(parseDouble(edtPrev), rend * 0.12);
        if (rend <= 0) { lbl->setText(tr("Informe rendimento.")); return; }

        // Deducao simplificada: 20% rend, teto R$ 16.754,34
        const double simples = std::min(rend * 0.20, 16754.34);
        // Educacao tem limite por dependente
        const double tetoEduc = 3561.50 * std::max(1, dep);
        educ = std::min(educ, tetoEduc);
        const double depDed = dep * 2275.08;
        const double completa = inss + depDed + saude + educ + prev;

        const double baseSimples  = rend - simples;
        const double baseCompleta = rend - completa;
        // Tabela IRPF anual 2024 (12x mensal)
        auto irAnual = [](double base) {
            double ir = 0.0;
            if      (base > 55976.16) ir = base * 0.275 - 10752.00;
            else if (base > 45012.60) ir = base * 0.225 - 7953.24;
            else if (base > 33919.80) ir = base * 0.150 - 4577.28;
            else if (base > 27110.40) ir = base * 0.075 - 2033.28;
            return std::max(0.0, ir);
        };
        const double irS = irAnual(baseSimples);
        const double irC = irAnual(baseCompleta);
        const QString melhor = irS < irC ? tr("Simples") : tr("Completa");
        lbl->setText(tr(
            "<b>Simples (20% — teto R$ 16.754,34):</b><br>"
            "&nbsp;&nbsp;Dedução: R$ %1 · base R$ %2 · IR R$ %3<br><br>"
            "<b>Completa (INSS + dep + saúde + educ + PGBL):</b><br>"
            "&nbsp;&nbsp;Dedução: R$ %4 · base R$ %5 · IR R$ %6<br><br>"
            "<b>Melhor opção: %7</b> (economia R$ %8)")
            .arg(fmtBR(simples, 2),    fmtBR(baseSimples, 2),  fmtBR(irS, 2),
                 fmtBR(completa, 2),   fmtBR(baseCompleta, 2), fmtBR(irC, 2),
                 melhor,               fmtBR(std::abs(irC - irS), 2)));
    });
}

// ── #57 Inflação & Câmbio ────────────────────────────────────────────────
void CalculadoraDialog::buildTabInflacaoCambio(QWidget *tab)
{
    auto *v = new QVBoxLayout(tab);
    v->addWidget(new QLabel(tr("<b>Inflação &amp; Câmbio</b><br>"
        "Poder de compra com IPCA + conversor de moedas com taxa manual."), tab));

    // Inflacao
    auto *grpInf = new QGroupBox(tr("Poder de compra (IPCA)"), tab);
    auto *fI = new QFormLayout(grpInf);
    auto *edtValor = mkLE(grpInf, tr("ex: 1000"));
    auto *edtIPCA  = mkLE(grpInf, tr("ex: 4,5 (% a.a.)"));
    auto *edtAnos  = mkLE(grpInf, tr("ex: 10"));
    fI->addRow(tr("Valor inicial (R$):"), edtValor);
    fI->addRow(tr("IPCA anual (%):"),      edtIPCA);
    fI->addRow(tr("Período (anos):"),      edtAnos);
    auto *btnI = new QPushButton(tr("Calcular IPCA"), grpInf);
    fI->addRow(btnI);
    auto *lblI = new QLabel(grpInf); lblI->setWordWrap(true);
    fI->addRow(lblI);
    v->addWidget(grpInf);

    connect(btnI, &QPushButton::clicked, tab, [=]() {
        const double valor = parseDouble(edtValor);
        const double ipca  = parseDouble(edtIPCA) / 100.0;
        const double anos  = parseDouble(edtAnos);
        if (valor <= 0 || anos <= 0) { lblI->setText(tr("Verifique os valores.")); return; }
        const double futuro = valor * std::pow(1.0 + ipca, anos);
        const double poder  = valor / std::pow(1.0 + ipca, anos);
        lblI->setText(tr(
            "Valor nominal em %1 anos: <b>R$ %2</b><br>"
            "Poder de compra equivalente hoje: <b>R$ %3</b><br>"
            "(R$ %4 hoje = R$ %2 em %1 anos com inflação %5 %)")
            .arg(fmtBR(anos, 0), fmtBR(futuro, 2), fmtBR(poder, 2),
                 fmtBR(valor, 2), fmtBR(ipca * 100.0, 2)));
    });

    // Cambio
    auto *grpC = new QGroupBox(tr("Conversor de moedas"), tab);
    auto *fC = new QFormLayout(grpC);
    auto *edtMontante = mkLE(grpC, tr("ex: 1000"));
    auto *edtTaxa     = mkLE(grpC, tr("ex: 5,15 (R$ por USD)"));
    auto *cbDir       = new QComboBox(grpC);
    cbDir->addItem(tr("Moeda → BRL"), 0);
    cbDir->addItem(tr("BRL → Moeda"), 1);
    fC->addRow(tr("Montante:"), edtMontante);
    fC->addRow(tr("Cotação (R$):"), edtTaxa);
    fC->addRow(tr("Direção:"), cbDir);
    auto *btnC = new QPushButton(tr("Converter"), grpC);
    fC->addRow(btnC);
    auto *lblC = new QLabel(grpC); lblC->setWordWrap(true);
    fC->addRow(lblC);
    v->addWidget(grpC);
    v->addStretch();

    connect(btnC, &QPushButton::clicked, tab, [=]() {
        const double m = parseDouble(edtMontante);
        const double t = parseDouble(edtTaxa);
        if (m <= 0 || t <= 0) { lblC->setText(tr("Verifique os valores.")); return; }
        const double resultado = (cbDir->currentData().toInt() == 0)
            ? m * t
            : m / t;
        lblC->setText(tr("<b>Resultado:</b> %1").arg(fmtBR(resultado, 2)));
    });
}
