#include "CalculadoraDialog.h"
#include "services/TvmService.h"
#include "services/ParcelaService.h"
#include "services/FinanceEngine.h"
#include "services/FiscalService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
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

        auto *lbl = new QLabel(row.lbl + QStringLiteral(":"), tab);
        lbl->setMinimumWidth(46);

        row.fld->setPlaceholderText(QStringLiteral("0"));
        row.fld->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

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
        auto *lbl = new QLabel(tr("CET:"), tab);
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
        m_tvmData->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
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
        auto *iData = new QTableWidgetItem(row.data.toString(QStringLiteral("dd/MM/yyyy")));
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

    // ── Helpers ───────────────────────────────────────────────────────────────

    auto parseF = [](QLineEdit *le) -> double {
        QString s = le->text().trimmed();
        s.remove(QLatin1Char('R')).remove(QLatin1Char('$'))
         .remove(QLatin1Char('%')).remove(QLatin1Char(' '));
        if (s.contains(QLatin1Char(','))) {
            s.remove(QLatin1Char('.'));
            s.replace(QLatin1Char(','), QLatin1Char('.'));
        }
        return s.toDouble();
    };

    auto fmtF = [](double v, int dec = 4) -> QString {
        if (v == std::floor(v) && std::abs(v) < 1e12)
            return QString::number(static_cast<qint64>(v));
        QString s = QString::number(v, 'f', dec);
        while (s.endsWith(QLatin1Char('0'))) s.chop(1);
        if (s.endsWith(QLatin1Char('.'))) s.chop(1);
        return s.replace(QLatin1Char('.'), QLatin1Char(','));
    };

    // Creates a styled QLineEdit and QLabel pair for a form row
    auto makeLE = [](QWidget *parent) -> QLineEdit* {
        auto *le = new QLineEdit(parent);
        le->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        le->setPlaceholderText(QStringLiteral("0"));
        return le;
    };

    // Creates a result label (hidden until first calculation)
    auto makeResLbl = [](QWidget *parent) -> QLabel* {
        auto *lbl = new QLabel(parent);
        lbl->setWordWrap(true);
        lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        lbl->hide();
        return lbl;
    };

    // Creates "↑ Usar na Comum" button (hidden until first calculation)
    auto makeUsarBtn = [this](QWidget *parent,
                              std::shared_ptr<double> val) -> QPushButton* {
        auto *btn = new QPushButton(tr("↑ Usar na Comum"), parent);
        btn->hide();
        connect(btn, &QPushButton::clicked, this, [this, val]() {
            setValorInicial(*val);
            m_tabs->setCurrentIndex(0);
        });
        return btn;
    };

    // ── Wizard 1: CET ↔ Taxa mensal ──────────────────────────────────────────
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
        auto *edtIn  = makeLE(panel);
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

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(rbMensal, &QRadioButton::toggled, this, [lblUnit](bool on) {
            lblUnit->setText(on ? QStringLiteral("% a.m.") : QStringLiteral("% a.a."));
        });
        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double v = parseF(edtIn);
            if (v <= 0.0) { lblRes->setText(tr("Informe um valor maior que zero.")); lblRes->show(); return; }
            double res;
            if (rbMensal->isChecked()) {
                res = TvmService::cetFromMensal(v);
                lblRes->setText(tr("CET: <b>%1 % a.a.</b>").arg(fmtF(res, 4)));
            } else {
                res = TvmService::mensalFromCet(v);
                lblRes->setText(tr("Taxa mensal: <b>%1 % a.m.</b>").arg(fmtF(res, 6)));
            }
            *resVal = res;
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 2: Equivalência de taxas ───────────────────────────────────────
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
        auto *edtTxa = makeLE(panel);
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

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double taxa = parseF(edtTxa);
            const int    nOrig = diasPer.at(cbOrig->currentIndex());
            const int    nDest = diasPer.at(cbDest->currentIndex());
            // #69: dedup -> FinanceEngine::taxaEquivalente (regime composto)
            const double equiv = FinanceEngine::taxaEquivalente(
                                     taxa / 100.0, nDest, nOrig) * 100.0;
            *resVal = equiv;
            lblRes->setText(tr("Taxa equivalente: <b>%1 % por %2</b>")
                .arg(fmtF(equiv, 6), cbDest->currentText().toLower()));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 3: Parcelas — Price ────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form  = new QFormLayout;
        auto *edtPV = makeLE(panel);
        auto *edtI  = makeLE(panel);
        auto *edtN  = makeLE(panel);
        form->addRow(tr("Valor financiado (PV):"), edtPV);
        form->addRow(tr("Taxa mensal (%):"), edtI);
        form->addRow(tr("Nº de parcelas (N):"), edtN);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double pv = parseF(edtPV);
            const double i  = parseF(edtI) / 100.0;
            const double n  = parseF(edtN);
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
                .arg(fmtF(pmt, 2), fmtF(total, 2), fmtF(juros, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 4: Parcelas — SAC ──────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form  = new QFormLayout;
        auto *edtPV = makeLE(panel);
        auto *edtI  = makeLE(panel);
        auto *edtN  = makeLE(panel);
        form->addRow(tr("Valor financiado (PV):"), edtPV);
        form->addRow(tr("Taxa mensal (%):"), edtI);
        form->addRow(tr("Nº de parcelas (N):"), edtN);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double pv = parseF(edtPV);
            const double i  = parseF(edtI) / 100.0;
            const int    n  = static_cast<int>(parseF(edtN));
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
                .arg(fmtF(pmt1, 2), fmtF(pmtN, 2), fmtF(total, 2), fmtF(juros, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 5: Valor futuro ────────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form  = new QFormLayout;
        auto *edtPV = makeLE(panel);
        auto *edtI  = makeLE(panel);
        auto *edtN  = makeLE(panel);
        form->addRow(tr("Capital inicial (PV):"), edtPV);
        form->addRow(tr("Taxa mensal (%):"), edtI);
        form->addRow(tr("Prazo (meses):"), edtN);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double pv = parseF(edtPV);
            const double i  = parseF(edtI) / 100.0;
            const double n  = parseF(edtN);
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
                .arg(fmtF(fv, 2), fmtF(ganho, 2), fmtF(ganho / pv * 100.0, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 6: Renda passiva ───────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form   = new QFormLayout;
        auto *edtCap = makeLE(panel);
        auto *edtI   = makeLE(panel);
        form->addRow(tr("Capital (R$):"), edtCap);
        form->addRow(tr("Taxa mensal (%):"), edtI);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double cap = parseF(edtCap);
            const double i   = parseF(edtI) / 100.0;
            if (cap <= 0.0 || i <= 0.0) {
                lblRes->setText(tr("Informe capital e taxa maiores que zero.")); lblRes->show(); return;
            }
            const double pmt   = cap * i;
            const double anual = pmt * 12.0;
            *resVal = pmt;
            lblRes->setText(
                tr("Renda mensal: <b>R$ %1</b><br>"
                   "Renda anual: R$ %2")
                .arg(fmtF(pmt, 2), fmtF(anual, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 7: ROI ─────────────────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form    = new QFormLayout;
        auto *edtInv  = makeLE(panel);
        auto *edtRet  = makeLE(panel);
        auto *edtMeses = makeLE(panel);
        edtMeses->setPlaceholderText(tr("Opcional"));
        form->addRow(tr("Investimento (R$):"), edtInv);
        form->addRow(tr("Retorno total (R$):"), edtRet);
        form->addRow(tr("Prazo (meses, opcional):"), edtMeses);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double inv   = parseF(edtInv);
            const double ret   = parseF(edtRet);
            const double meses = parseF(edtMeses);
            if (inv <= 0.0) {
                lblRes->setText(tr("Investimento deve ser maior que zero.")); lblRes->show(); return;
            }
            const double ganho = ret - inv;
            const double roi   = ganho / inv * 100.0;
            *resVal = roi;
            QString txt = tr("Ganho/Perda: <b>R$ %1</b><br>ROI: <b>%2 %</b>")
                .arg(fmtF(ganho, 2), fmtF(roi, 4));
            if (meses > 0.0 && (1.0 + roi / 100.0) > 0.0) {
                // #69: dedup -> FinanceEngine::taxaEquivalente
                const double iMes = FinanceEngine::taxaEquivalente(
                                        roi / 100.0, 1, static_cast<int>(meses)) * 100.0;
                txt += tr("<br>Taxa mensal equiv.: %1 % a.m.").arg(fmtF(iMes, 4));
            }
            lblRes->setText(txt);
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 8: Comprometimento de renda ────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form    = new QFormLayout;
        auto *edtPMT  = makeLE(panel);
        auto *edtRend = makeLE(panel);
        form->addRow(tr("Parcela / dívida mensal (R$):"), edtPMT);
        form->addRow(tr("Renda mensal líquida (R$):"), edtRend);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double pmt  = parseF(edtPMT);
            const double rend = parseF(edtRend);
            if (rend <= 0.0) {
                lblRes->setText(tr("Renda deve ser maior que zero.")); lblRes->show(); return;
            }
            const double pct = pmt / rend * 100.0;
            *resVal = pct;
            const QString semaforo = (pct < 30.0) ? tr("✔ Saudável (< 30 %)")
                                   : (pct < 40.0) ? tr("⚠ Atenção (30–40 %)")
                                                  : tr("✘ Comprometido (> 40 %)");
            lblRes->setText(
                tr("Comprometimento: <b>%1 %</b><br>%2<br>Disponível: R$ %3")
                .arg(fmtF(pct, 2), semaforo, fmtF(rend - pmt, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 9: Saldo devedor (quitação antecipada) ─────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form  = new QFormLayout;
        auto *edtPV = makeLE(panel);
        auto *edtI  = makeLE(panel);
        auto *edtN  = makeLE(panel);
        auto *edtK  = makeLE(panel);
        form->addRow(tr("Valor financiado (PV):"), edtPV);
        form->addRow(tr("Taxa mensal (%):"), edtI);
        form->addRow(tr("Prazo total (N):"), edtN);
        form->addRow(tr("Parcelas já pagas (k):"), edtK);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double pv = parseF(edtPV);
            const double i  = parseF(edtI) / 100.0;
            const double n  = parseF(edtN);
            const double k  = parseF(edtK);
            if (pv <= 0.0 || n <= 0.0 || k < 0.0 || k >= n) {
                lblRes->setText(tr("Verifique os valores (k deve ser < N).")); lblRes->show(); return;
            }
            double pmt = 0.0;
            double sd  = 0.0;
            // #69: dedup -> FinanceEngine::pmt (sinal invertido para Price BR)
            pmt = -FinanceEngine::pmt(pv, 0.0, i, static_cast<int>(n));
            if (i == 0.0) {
                sd = pv - k * pmt;
            } else {
                // Saldo apos k parcelas: pv*(1+i)^k - pmt*((1+i)^k - 1)/i
                // (sem FinanceEngine helper para essa formula derivada)
                const double pot = std::pow(1.0 + i, k);
                sd = pv * pot - pmt * (pot - 1.0) / i;
            }
            *resVal = sd;
            lblRes->setText(
                tr("Parcela (PMT): R$ %1<br>"
                   "Saldo devedor após %2 parcelas: <b>R$ %3</b><br>"
                   "Parcelas restantes: %4")
                .arg(fmtF(pmt, 2),
                     QString::number(static_cast<qint64>(k)),
                     fmtF(sd, 2),
                     QString::number(static_cast<qint64>(n - k))));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 10: Meta financeira ────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form  = new QFormLayout;
        auto *edtPV = makeLE(panel);
        auto *edtFV = makeLE(panel);
        auto *edtI  = makeLE(panel);
        form->addRow(tr("Capital atual (R$):"), edtPV);
        form->addRow(tr("Meta (R$):"), edtFV);
        form->addRow(tr("Taxa mensal (%):"), edtI);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double pv = parseF(edtPV);
            const double fv = parseF(edtFV);
            const double i  = parseF(edtI) / 100.0;
            if (pv <= 0.0 || fv <= pv || i <= 0.0) {
                lblRes->setText(tr("Meta deve ser maior que o capital, e taxa > 0.")); lblRes->show(); return;
            }
            const double n    = std::log(fv / pv) / std::log(1.0 + i);
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

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 11: Recálculo com amortização parcial ──────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form     = new QFormLayout;
        auto *edtSD    = makeLE(panel);
        auto *edtI     = makeLE(panel);
        auto *edtNR    = makeLE(panel);
        auto *edtAmort = makeLE(panel);
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

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double sd    = parseF(edtSD);
            const double i     = parseF(edtI) / 100.0;
            const double nr    = parseF(edtNR);
            const double amort = parseF(edtAmort);
            if (sd <= 0.0 || i <= 0.0 || nr <= 0.0 || amort <= 0.0 || amort >= sd) {
                lblRes->setText(tr("Verifique os valores (amortização deve ser < saldo devedor)."));
                lblRes->show(); return;
            }
            // #69: dedup -> FinanceEngine::pmt (sinal invertido para Price BR)
            const double pmtOrig      = -FinanceEngine::pmt(sd, 0.0, i, static_cast<int>(nr));
            const double novoSD       = sd - amort;
            const double jurosOrigTotal = pmtOrig * nr - sd;

            if (rbRedN->isChecked()) {
                const double arg = 1.0 - novoSD * i / pmtOrig;
                if (arg <= 0.0) {
                    lblRes->setText(tr("Amortização insuficiente para reduzir parcelas."));
                    lblRes->show(); return;
                }
                const double nNovo   = -std::log(arg) / std::log(1.0 + i);
                const int    nInt    = static_cast<int>(std::ceil(nNovo));
                const double jurosNovo = pmtOrig * nNovo - novoSD;
                const double economia  = jurosOrigTotal - jurosNovo;
                *resVal = pmtOrig;
                lblRes->setText(
                    tr("PMT: R$ %1 (inalterada)<br>"
                       "Novo N: <b>%2 parcelas</b> (era %3)<br>"
                       "Economia de juros: <b>R$ %4</b>")
                    .arg(fmtF(pmtOrig, 2),
                         QString::number(nInt),
                         QString::number(static_cast<qint64>(nr)),
                         fmtF(economia, 2)));
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
                    .arg(fmtF(pmtNovo, 2),
                         fmtF(pmtOrig, 2),
                         QString::number(static_cast<qint64>(nr)),
                         fmtF(economia, 2)));
            }
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 12: Comparador RF — CDB vs LCI/LCA ────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form      = new QFormLayout;
        auto *edtValor  = makeLE(panel);
        auto *edtMeses  = makeLE(panel);
        auto *edtCDB    = makeLE(panel);
        auto *edtLCI    = makeLE(panel);
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

        auto *lblRes = makeResLbl(panel);
        auto  resVal = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double valor = parseF(edtValor);
            const double meses = parseF(edtMeses);
            const double taxaCDB = parseF(edtCDB);
            const double taxaLCI = parseF(edtLCI);
            if (valor <= 0.0 || meses <= 0.0 || taxaCDB <= 0.0) {
                lblRes->setText(tr("Verifique os valores."));
                lblRes->show(); return;
            }
            // IR regressivo CDB
            double ir = 0.225;
            if (meses > 24.0)       ir = 0.15;
            else if (meses > 12.0)  ir = 0.175;
            else if (meses > 6.0)   ir = 0.20;

            // #69: dedup -> FinanceEngine::taxaEquivalente
            const double taxaCDBm = FinanceEngine::taxaEquivalente(taxaCDB / 100.0, 1, 12);
            const double taxaLCIm = FinanceEngine::taxaEquivalente(taxaLCI / 100.0, 1, 12);
            const int    n        = static_cast<int>(std::round(meses));

            const double brutosCDB  = valor * (std::pow(1.0 + taxaCDBm, n) - 1.0);
            const double liquidoCDB = brutosCDB * (1.0 - ir);
            const double liquidoLCI = valor * (std::pow(1.0 + taxaLCIm, n) - 1.0);

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
                .arg(fmtF(brutosCDB, 2),
                     fmtF(ir * 100.0, 1),
                     fmtF(brutosCDB * ir, 2),
                     fmtF(liquidoCDB, 2),
                     fmtF(liquidoLCI, 2),
                     cdbMelhor ? tr("CDB") : tr("LCI/LCA"),
                     fmtF(breakEvenAA, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 13: FIRE Number (Regra dos 25) ────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form       = new QFormLayout;
        auto *edtDesp    = makeLE(panel);
        auto *edtTaxa    = makeLE(panel);
        auto *edtInfla   = makeLE(panel);
        auto *edtAnos    = makeLE(panel);
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

        auto *lblRes = makeResLbl(panel);
        auto  resVal = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double desp   = parseF(edtDesp);
            const double taxa   = parseF(edtTaxa);
            const double infla  = parseF(edtInfla);
            const double anos   = parseF(edtAnos);
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
                extra = tr("<br>Taxa real: %1% a.a.").arg(fmtF(taxaReal, 2));
            if (anos > 0.0)
                extra += tr("<br>FIRE em %1 anos (inflação): <b>R$ %2</b>")
                         .arg(fmtF(anos, 0), fmtF(fireFuturo, 2));

            lblRes->setText(
                tr("FIRE Number (hoje): <b>R$ %1</b><br>"
                   "= R$ %2 × 12 × 25%3")
                .arg(fmtF(fireHoje, 2), fmtF(desp, 2), extra));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 14: Lean FIRE ─────────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form     = new QFormLayout;
        auto *edtDesp  = makeLE(panel);
        auto *edtLean  = makeLE(panel);
        edtDesp->setPlaceholderText(tr("ex: 5000"));
        edtLean->setPlaceholderText(tr("ex: 3000"));
        form->addRow(tr("Despesas atuais (R$/mês):"), edtDesp);
        form->addRow(tr("Despesas enxutas (R$/mês):"), edtLean);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes = makeResLbl(panel);
        auto  resVal = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double desp = parseF(edtDesp);
            const double lean = parseF(edtLean);
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
                .arg(fmtF(leanFire, 2),
                     fmtF(lean, 2),
                     fireNorm > 0.0
                         ? tr("<br>Redução vs FIRE normal: <b>R$ %1</b>").arg(fmtF(economia, 2))
                         : QString()));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 15: Fat FIRE ───────────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form     = new QFormLayout;
        auto *edtDesp  = makeLE(panel);
        auto *edtMult  = makeLE(panel);
        edtDesp->setPlaceholderText(tr("ex: 15000"));
        edtMult->setPlaceholderText(tr("33 a 40 (padrão: 33)"));
        form->addRow(tr("Despesas mensais (R$):"), edtDesp);
        form->addRow(tr("Multiplicador:"),          edtMult);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes = makeResLbl(panel);
        auto  resVal = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double desp = parseF(edtDesp);
            double mult = parseF(edtMult);
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
                .arg(fmtF(fatFire, 2),
                     fmtF(desp, 2),
                     fmtF(mult, 0),
                     fmtF(extra, 2),
                     fmtF((mult / 25.0 - 1.0) * 100.0, 1)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 16: Coast FIRE ─────────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form      = new QFormLayout;
        auto *edtFire   = makeLE(panel);
        auto *edtTaxa   = makeLE(panel);
        auto *edtAnos   = makeLE(panel);
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

        auto *lblRes = makeResLbl(panel);
        auto  resVal = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double fire = parseF(edtFire);
            const double taxa = parseF(edtTaxa);
            const double anos = parseF(edtAnos);
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
                .arg(fmtF(coastFire, 2),
                     fmtF(anos, 0),
                     fmtF(taxa, 2),
                     fmtF(fire, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 17: Barista FIRE ───────────────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form       = new QFormLayout;
        auto *edtDesp    = makeLE(panel);
        auto *edtParcial = makeLE(panel);
        edtDesp->setPlaceholderText(tr("ex: 8000"));
        edtParcial->setPlaceholderText(tr("ex: 2000"));
        form->addRow(tr("Despesas mensais totais (R$):"),   edtDesp);
        form->addRow(tr("Renda parcial futura (R$/mês):"),  edtParcial);
        lay->addLayout(form);

        auto *btnCalc = new QPushButton(tr("Calcular"), panel);
        btnCalc->setObjectName(QStringLiteral("primary"));
        lay->addWidget(btnCalc);

        auto *lblRes = makeResLbl(panel);
        auto  resVal = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double desp    = parseF(edtDesp);
            const double parcial = parseF(edtParcial);
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
                .arg(fmtF(baristaFire, 2),
                     fmtF(gap, 2),
                     fmtF(desp, 2),
                     fmtF(parcial, 2),
                     fmtF(reducao, 2),
                     fmtF(fireNorm > 0.0 ? reducao / fireNorm * 100.0 : 0.0, 1)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 18: Geographic Arbitrage ──────────────────────────────────────
    {
        auto *panel = new QWidget(m_wizStack);
        auto *lay   = new QVBoxLayout(panel);
        lay->setContentsMargins(4, 8, 4, 4);
        lay->setSpacing(8);

        auto *form       = new QFormLayout;
        auto *edtRenda   = makeLE(panel);
        auto *edtOrigem  = makeLE(panel);
        auto *edtDestino = makeLE(panel);
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

        auto *lblRes = makeResLbl(panel);
        auto  resVal = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double renda   = parseF(edtRenda);
            const double origem  = parseF(edtOrigem);
            const double destino = parseF(edtDestino);
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
                .arg(fmtF(fator, 2),
                     fmtF(poderCompra, 2),
                     fmtF(equivalenteLD, 2),
                     fmtF(poderCompra - renda, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 19 — Reserva de Emergência (#61) ──────────────────────────────
    // Compara 3 aplicacoes para acumulo: Poupanca, CDB 100% CDI (IR
    // regressivo) e Tesouro Selic (IR + IOF). Usa FiscalService.
    {
        auto *panel  = new QWidget;
        auto *lay    = new QVBoxLayout(panel);
        lay->setContentsMargins(8, 8, 8, 4);
        lay->setSpacing(6);

        lay->addWidget(new QLabel(tr("<b>Reserva de Emergência</b>"), panel));

        auto *form      = new QFormLayout;
        auto *edtDesp   = makeLE(panel);
        auto *edtMeses  = makeLE(panel);
        auto *edtSelic  = makeLE(panel);
        auto *edtGuarda = makeLE(panel);
        auto *edtAtual  = makeLE(panel);
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

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double desp   = parseF(edtDesp);
            double       meses  = parseF(edtMeses);
            const double selic  = parseF(edtSelic) / 100.0;
            const double guarda = parseF(edtGuarda);
            const double atual  = parseF(edtAtual);
            if (meses <= 0.0) meses = 6.0;
            if (desp <= 0.0) {
                lblRes->setText(tr("Informe as despesas."));
                lblRes->show(); return;
            }

            const double meta = desp * meses;
            const double gap  = std::max(0.0, meta - atual);
            *resVal = meta;

            QString txt = tr("Meta: <b>R$ %1</b> (%2× despesas mensais)<br>"
                             "Gap atual: <b>R$ %3</b><br><br>")
                              .arg(fmtF(meta, 2), fmtF(meses, 0), fmtF(gap, 2));

            if (selic <= 0.0 || guarda <= 0.0) {
                txt += tr("<i>Informe Selic e aporte para comparativo de aplicações.</i>");
                lblRes->setText(txt); lblRes->show(); btnUsar->show(); return;
            }

            // Taxas mensais liquidas (assumindo >720d -> IR 15% em CDB/Tesouro)
            const double mPoup     = FiscalService::poupancaMensal(selic);
            const double cdiAnual  = selic; // CDB 100% CDI ~ Selic
            const double mBruta    = FinanceEngine::taxaEquivalente(cdiAnual, 1, 12);
            const double irRF      = FiscalService::irRendaFixaPorDias(721); // 15%
            const double mCDB      = mBruta * (1.0 - irRF);
            const double mTesouro  = mCDB; // mesma aliquota, custodia 0,2% a.a.
                                          // simplificada como 0 aqui

            auto tempoAcumular = [&](double i) -> double {
                if (i <= 0.0) return std::ceil(gap / guarda);
                return std::ceil(std::log(1.0 + gap * i / guarda) / std::log(1.0 + i));
            };
            auto rendaMensal = [&](double i) { return meta * i; };

            txt += tr("<table cellspacing='4'>"
                      "<tr><th align='left'>Aplicação</th>"
                      "<th align='right'>Taxa líq./mês</th>"
                      "<th align='right'>Meses p/ meta</th>"
                      "<th align='right'>Renda plena</th></tr>");
            auto linha = [&](const QString &nome, double m) {
                txt += tr("<tr><td>%1</td><td align='right'>%2%</td>"
                          "<td align='right'>%3</td>"
                          "<td align='right'>R$ %4</td></tr>")
                          .arg(nome, fmtF(m * 100.0, 4),
                               fmtF(tempoAcumular(m), 0),
                               fmtF(rendaMensal(m), 2));
            };
            linha(tr("Poupança"),      mPoup);
            linha(tr("CDB 100% CDI"),  mCDB);
            linha(tr("Tesouro Selic"), mTesouro);
            txt += QStringLiteral("</table>");

            lblRes->setText(txt);
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }

    // ── Wizard 20 — CLT vs PJ (#62) ──────────────────────────────────────────
    // CLT: INSS + IRPF mensais via FiscalService (tabelas 2024).
    // PJ : Simples Nacional anexos I-V (calculado por RBT12) ou Lucro
    //      Presumido (~16,33% efetivo para servicos).
    // Bonus: estima break-even — faturamento PJ que iguala liquido CLT.
    {
        auto *panel  = new QWidget;
        auto *lay    = new QVBoxLayout(panel);
        lay->setContentsMargins(8, 8, 8, 4);
        lay->setSpacing(6);

        lay->addWidget(new QLabel(tr("<b>CLT vs PJ</b>"), panel));

        auto *form    = new QFormLayout;
        auto *edtCLT  = makeLE(panel);
        auto *edtPJ   = makeLE(panel);
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

        auto *lblRes  = makeResLbl(panel);
        auto  resVal  = std::make_shared<double>(0.0);
        auto *btnUsar = makeUsarBtn(panel, resVal);
        lay->addWidget(lblRes);
        lay->addWidget(btnUsar);
        lay->addStretch();

        connect(btnCalc, &QPushButton::clicked, this, [=]() {
            const double bruto = parseF(edtCLT);
            const double pjFat = parseF(edtPJ);
            const int    reg   = cbReg->currentData().toInt();
            if (bruto <= 0.0 || pjFat <= 0.0) {
                lblRes->setText(tr("Verifique os valores.")); lblRes->show(); return;
            }

            // CLT via FiscalService
            const qint64 brutoCent = static_cast<qint64>(std::round(bruto * 100.0));
            const qint64 inssCent  = FiscalService::inssMensal2024(brutoCent);
            const qint64 baseCent  = brutoCent - inssCent;
            const qint64 irpfCent  = FiscalService::irpfMensal2024(baseCent);
            const double inss      = inssCent / 100.0;
            const double irpf      = irpfCent / 100.0;
            const double liqCLT    = bruto - inss - irpf;

            // PJ via Simples (RBT12 = faturamento * 12) ou Lucro Presumido
            double aliqPJ = 0.0;
            QString regNome;
            if (reg == 0) {
                aliqPJ = FiscalService::lucroPresumidoServicosAliquota();
                regNome = tr("Lucro Presumido (serviços)");
            } else {
                const qint64 rbt12 = static_cast<qint64>(std::round(pjFat * 12.0 * 100.0));
                aliqPJ = FiscalService::simplesAliquotaEfetiva(reg, rbt12);
                regNome = cbReg->currentText();
            }
            const double impPJ = pjFat * aliqPJ;
            const double liqPJ = pjFat - impPJ;
            const double diff  = liqPJ - liqCLT;
            *resVal = diff;

            // Break-even: faturamento PJ que iguala liqCLT
            // pjFat * (1 - aliq) = liqCLT  ->  pjFat = liqCLT / (1 - aliq)
            // (aproximacao: aliq constante; no Simples, RBT12 cresce com pjFat,
            // entao o resultado real e maior; suficiente para orientacao)
            const double breakEven = aliqPJ < 1.0 ? liqCLT / (1.0 - aliqPJ) : 0.0;

            const QString sinal = diff >= 0.0 ? QStringLiteral("+") : QString();
            lblRes->setText(
                tr("<b>CLT</b><br>"
                   "&nbsp;&nbsp;INSS: R$ %1 · IRPF: R$ %2 · <b>Líquido: R$ %3</b><br><br>"
                   "<b>PJ — %4</b><br>"
                   "&nbsp;&nbsp;Alíquota efetiva: %5%<br>"
                   "&nbsp;&nbsp;Impostos: R$ %6 · <b>Líquido: R$ %7</b><br><br>"
                   "Diferença PJ − CLT: <b>%8R$ %9</b><br>"
                   "Break-even PJ (≈ liq. CLT): <b>R$ %10/mês</b>")
                .arg(fmtF(inss, 2),
                     fmtF(irpf, 2),
                     fmtF(liqCLT, 2),
                     regNome,
                     fmtF(aliqPJ * 100.0, 4),
                     fmtF(impPJ, 2),
                     fmtF(liqPJ, 2),
                     sinal,
                     fmtF(std::abs(diff), 2),
                     fmtF(breakEven, 2)));
            lblRes->show();
            btnUsar->show();
        });

        m_wizStack->addWidget(panel);
    }
}

// ── Stubs das abas novas (#43 #54 #55 #56 #57) ────────────────────────────────
// Esqueletos vazios para destravar o build enquanto os wizards internos não
// estão implementados. Cada aba recebe um placeholder explicativo.
namespace {
void placeholderTab(QWidget *tab, const QString &titulo, const QString &itemId)
{
    auto *v = new QVBoxLayout(tab);
    v->setContentsMargins(20, 20, 20, 20);
    auto *lbl = new QLabel(tab);
    lbl->setText(QStringLiteral("<h3>%1</h3><p>Wizards em implementação (%2). Ver <code>TODO.md</code>.</p>")
                     .arg(titulo, itemId));
    lbl->setTextFormat(Qt::RichText);
    lbl->setAlignment(Qt::AlignCenter);
    lbl->setWordWrap(true);
    v->addWidget(lbl);
    v->addStretch();
}
} // anonymous

void CalculadoraDialog::buildTabBolsa(QWidget *tab)          { placeholderTab(tab, tr("Bolsa de Valores"),    QStringLiteral("#43")); }
void CalculadoraDialog::buildTabImoveis(QWidget *tab)        { placeholderTab(tab, tr("Imóveis"),             QStringLiteral("#54")); }
void CalculadoraDialog::buildTabRendaFixa(QWidget *tab)      { placeholderTab(tab, tr("Renda Fixa"),          QStringLiteral("#55")); }
void CalculadoraDialog::buildTabIrTributos(QWidget *tab)     { placeholderTab(tab, tr("IR & Tributos"),       QStringLiteral("#56")); }
void CalculadoraDialog::buildTabInflacaoCambio(QWidget *tab) { placeholderTab(tab, tr("Inflação & Câmbio"),   QStringLiteral("#57")); }
