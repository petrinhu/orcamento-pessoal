#include "AddCompraDialog.h"
#include "CalculadoraEdit.h"
#include "ReceiptAttachmentWidget.h"
#include "services/ParcelaService.h"
#include <cmath>
#include "models/Cartao.h"
#include "ui/GlossarioDecorator.h"
#include "ui/Theme.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QDateEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QGroupBox>
#include <QDate>
#include <QDialogButtonBox>

AddCompraDialog::AddCompraDialog(const QList<Cartao> &cartoes, QWidget *parent)
    : QDialog(parent), m_cartoes(cartoes)
{
    setWindowTitle(tr("Nova Compra Parcelada"));
    setMinimumWidth(600);
    buildUi();
}

void AddCompraDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    // ── Common header ──────────────────────────────────────────────────────
    auto *headerForm = new QFormLayout;
    headerForm->setSpacing(10);

    m_nome = new QLineEdit(this);
    m_nome->setPlaceholderText(tr("Ex: Notebook, Geladeira..."));
    headerForm->addRow(tr("Descrição *"), m_nome);

    m_cartao = new QComboBox(this);
    m_cartao->addItem(tr("— Sem cartão —"), 0);
    for (const auto &c : m_cartoes)
        m_cartao->addItem(c.apelido, c.id);
    headerForm->addRow(tr("Cartão"), m_cartao);

    m_moeda = new QComboBox(this);
    const QStringList moedas = {QStringLiteral("BRL"), QStringLiteral("USD"),
                                 QStringLiteral("EUR"), QStringLiteral("GBP"),
                                 QStringLiteral("JPY")};
    for (const auto &m : moedas)
        m_moeda->addItem(QStringLiteral("%1  %2").arg(ParcelaService::simboloMoeda(m), m), m);
    headerForm->addRow(tr("Moeda"), m_moeda);

    auto *dataRow = new QHBoxLayout;
    m_dataInicio = new QDateEdit(QDate::currentDate(), this);
    m_dataInicio->setCalendarPopup(true);
    m_dataInicio->setDisplayFormat(uifmt::mesAnoDisplayFormat());
    m_dataInicio->setToolTip(tr("Mês/ano desta parcela"));

    m_parcelaIni = new QSpinBox(this);
    m_parcelaIni->setRange(1, 999);
    m_parcelaIni->setValue(1);
    m_parcelaIni->setPrefix(tr("Parcela "));
    m_parcelaIni->setSuffix(QStringLiteral("ª"));
    m_parcelaIni->setToolTip(tr(
        "Se esta já é a 3ª parcela, coloque 3.\n"
        "O app calculará as parcelas anteriores automaticamente."));

    dataRow->addWidget(m_dataInicio);
    dataRow->addSpacing(12);
    dataRow->addWidget(new QLabel(tr("é a"), this));
    dataRow->addWidget(m_parcelaIni);
    dataRow->addStretch();
    headerForm->addRow(tr("Data da fatura"), dataRow);

    root->addLayout(headerForm);

    // ── Tabs ───────────────────────────────────────────────────────────────
    m_tabs = new QTabWidget(this);

    auto *tabSimples = new QWidget;
    buildTabSimples(tabSimples);
    m_tabs->addTab(tabSimples, tr("Valor por parcela"));

    auto *tabTotal = new QWidget;
    buildTabTotal(tabTotal);
    m_tabs->addTab(tabTotal, tr("Valor total"));

    auto *tabJuros = new QWidget;
    buildTabJuros(tabJuros);
    m_tabs->addTab(tabJuros, tr("Com juros / CET"));

    connect(m_tabs, &QTabWidget::currentChanged, this, &AddCompraDialog::onTabChanged);
    root->addWidget(m_tabs);

    // ── Preview table ──────────────────────────────────────────────────────
    buildPreview();

    auto *grpPreview = new QGroupBox(tr("Pré-visualização das parcelas"), this);
    grpPreview->setObjectName(QStringLiteral("surface"));
    auto *pvLayout = new QVBoxLayout(grpPreview);
    pvLayout->addWidget(m_preview);
    root->addWidget(grpPreview);

    // ── Comprovante (#90) ───────────────────────────────────────────────────
    auto *receiptForm = new QFormLayout;
    m_receipt = new ReceiptAttachmentWidget(this);
    receiptForm->addRow(tr("Comprovante:"), m_receipt);
    root->addLayout(receiptForm);

    // ── Buttons ────────────────────────────────────────────────────────────
    // Margem interna garante folga para a sombra dos botões (#176).
    auto *btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btnBox->button(QDialogButtonBox::Ok)->setText(tr("Adicionar"));
    btnBox->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primary"));
    btnBox->setContentsMargins(0, 6, 10, 10);
    connect(btnBox, &QDialogButtonBox::accepted, this, &AddCompraDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btnBox);

    // Cue de glossário (#166 §9.6): tooltip + sublinhado nos labels-chave.
    // Dialog modal não é alcançado pela varredura do MainWindow (criado
    // on-demand), então se auto-decora aqui.
    GlossarioDecorator::aplicarTooltips(this);
}

void AddCompraDialog::buildTabSimples(QWidget *tab)
{
    auto *form = new QFormLayout(tab);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    m_valorParcela = new CalculadoraEdit(tab);
    m_valorParcela->setPlaceholderText(QStringLiteral("0,00"));
    form->addRow(tr("Valor da parcela (R$)"), m_valorParcela);

    m_numParcelas_s = new QSpinBox(tab);
    m_numParcelas_s->setRange(1, 999);
    m_numParcelas_s->setValue(12);
    m_numParcelas_s->setSuffix(QStringLiteral("x"));
    form->addRow(tr("Número de parcelas"), m_numParcelas_s);

    connect(m_valorParcela->lineEdit(),  &QLineEdit::textChanged, this, &AddCompraDialog::recalcular);
    connect(m_numParcelas_s, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AddCompraDialog::recalcular);
}

void AddCompraDialog::buildTabTotal(QWidget *tab)
{
    auto *form = new QFormLayout(tab);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    m_valorTotal_t = new CalculadoraEdit(tab);
    m_valorTotal_t->setPlaceholderText(QStringLiteral("0,00"));
    form->addRow(tr("Valor total (R$)"), m_valorTotal_t);

    m_numParcelas_t = new QSpinBox(tab);
    m_numParcelas_t->setRange(1, 999);
    m_numParcelas_t->setValue(12);
    m_numParcelas_t->setSuffix(QStringLiteral("x"));
    form->addRow(tr("Número de parcelas"), m_numParcelas_t);

    auto *infoLbl = new QLabel(
        tr("Valor por parcela = Total ÷ N (sem juros)"), tab);
    infoLbl->setObjectName(QStringLiteral("subtitle"));
    form->addRow(QString(), infoLbl);

    connect(m_valorTotal_t->lineEdit(),  &QLineEdit::textChanged, this, &AddCompraDialog::recalcular);
    connect(m_numParcelas_t, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AddCompraDialog::recalcular);
}

void AddCompraDialog::buildTabJuros(QWidget *tab)
{
    auto *form = new QFormLayout(tab);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    // Helper: wraps a field widget with a ✏️/🚫 toggle button.
    // locked=true → ✏️ editable input; locked=false → 🚫 auto-calculated output.
    auto makeLockRow = [this, tab](QWidget *field, QPushButton *&lockBtn, bool locked) -> QWidget* {
        auto *w = new QWidget(tab);
        auto *h = new QHBoxLayout(w);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(4);
        h->addWidget(field, 1);
        lockBtn = new QPushButton(w);
        lockBtn->setIcon(Theme::emojiIcon(locked ? QStringLiteral("✏️")
                                                 : QStringLiteral("🚫")));
        lockBtn->setCheckable(true);
        lockBtn->setChecked(locked);
        lockBtn->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        lockBtn->setToolTip(locked
            ? tr("Campo editável. Clique para travá-lo e calculá-lo automaticamente.")
            : tr("Campo calculado automaticamente. Clique para editá-lo manualmente."));
        h->addWidget(lockBtn);
        return w;
    };

    // ── Fields ────────────────────────────────────────────────────────────
    m_valorTotal_j = new CalculadoraEdit(tab);
    m_valorTotal_j->setPlaceholderText(QStringLiteral("0,00"));
    m_valorTotal_j->setToolTip(tr("Valor financiado / principal (PV) — o que você está comprando a prazo."));
    form->addRow(tr("Valor à vista (R$)"),
                 makeLockRow(m_valorTotal_j, m_lockTotal, /*locked=*/true));

    m_numParcelas_j = new QSpinBox(tab);
    m_numParcelas_j->setRange(1, 999);
    m_numParcelas_j->setValue(12);
    m_numParcelas_j->setSuffix(QStringLiteral("x"));
    form->addRow(tr("Número de parcelas"),
                 makeLockRow(m_numParcelas_j, m_lockN, /*locked=*/true));

    m_valorParcela_j = new CalculadoraEdit(tab);
    m_valorParcela_j->setPlaceholderText(QStringLiteral("0,00"));
    m_valorParcela_j->setReadOnly(true); // default output field
    form->addRow(tr("Valor de cada parcela (R$)"),
                 makeLockRow(m_valorParcela_j, m_lockParcela, /*locked=*/false));

    // ── Taxa: juros mensais e CET na mesma linha, indicando que são equivalentes ──
    m_jurosMensais = new QDoubleSpinBox(tab);
    m_jurosMensais->setRange(0.0, 100.0);
    m_jurosMensais->setSingleStep(0.1);
    m_jurosMensais->setDecimals(4);
    m_jurosMensais->setSuffix(QStringLiteral("% a.m."));
    m_jurosMensais->setMinimumWidth(120);

    m_cet_j = new QDoubleSpinBox(tab);
    m_cet_j->setRange(0.0, 100000.0);
    m_cet_j->setSingleStep(0.5);
    m_cet_j->setDecimals(2);
    m_cet_j->setSuffix(QStringLiteral("% a.a."));
    m_cet_j->setMinimumWidth(120);

    // Taxa row: [jurosMensais] [=] [cet] [lockBtn]
    {
        auto *taxaWidget = new QWidget(tab);
        auto *taxaH      = new QHBoxLayout(taxaWidget);
        taxaH->setContentsMargins(0, 0, 0, 0);
        taxaH->setSpacing(4);
        taxaH->addWidget(m_jurosMensais, 1);
        auto *eqLbl = new QLabel(QStringLiteral("="), taxaWidget);
        eqLbl->setAlignment(Qt::AlignCenter);
        eqLbl->setToolTip(tr("Juros mensais e CET são equivalentes — alterar um atualiza o outro."));
        taxaH->addWidget(eqLbl);
        taxaH->addWidget(m_cet_j, 1);
        m_lockTaxa = new QPushButton(taxaWidget);
        m_lockTaxa->setIcon(Theme::emojiIcon(QStringLiteral("✏️")));
        m_lockTaxa->setCheckable(true);
        m_lockTaxa->setChecked(true);
        m_lockTaxa->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_lockTaxa->setToolTip(tr("Campo editável. Clique para travá-lo e calculá-lo automaticamente."));
        taxaH->addWidget(m_lockTaxa);
        form->addRow(tr("Juros mensais / CET a.a."), taxaWidget);
    }

    // ── Valor total pago (PMT × N) ─────────────────────────────────────────
    m_valorTotalPago_j = new CalculadoraEdit(tab);
    m_valorTotalPago_j->setPlaceholderText(QStringLiteral("0,00"));
    m_valorTotalPago_j->setReadOnly(true); // default: output
    m_valorTotalPago_j->setToolTip(tr("Total efetivamente pago = Valor parcela × Nº parcelas (inclui juros)."));
    form->addRow(tr("Valor total pago (R$)"),
                 makeLockRow(m_valorTotalPago_j, m_lockTotalPago, /*locked=*/false));

    m_lblJurosTotais = new QLabel(tr("Juros totais: —"), tab);
    m_lblJurosTotais->setObjectName(QStringLiteral("subtitle"));
    form->addRow(QString(), m_lblJurosTotais);

    // ── Connections ───────────────────────────────────────────────────────

    // Input fields changing → recalculate
    connect(m_valorTotal_j->lineEdit(),  &QLineEdit::textChanged,
            this, &AddCompraDialog::recalcular);
    connect(m_numParcelas_j, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AddCompraDialog::recalcular);
    connect(m_valorParcela_j->lineEdit(), &QLineEdit::textChanged,
            this, &AddCompraDialog::recalcular);

    // Juros mensais ↔ CET cross-sync (edit either one, the other follows)
    connect(m_jurosMensais, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double v) {
                m_cet_j->blockSignals(true);
                m_cet_j->setValue(ParcelaService::cetFromJurosMensais(v));
                m_cet_j->blockSignals(false);
                recalcular();
            });
    connect(m_cet_j, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double v) {
                m_jurosMensais->blockSignals(true);
                m_jurosMensais->setValue(ParcelaService::jurosMensaisFromCet(v));
                m_jurosMensais->blockSignals(false);
                recalcular();
            });

    // Lock buttons: clicking a lock makes that field the new output (unlocked),
    // all others become locked (inputs).
    connect(m_lockTotal, &QPushButton::clicked, this, [this]() {
        m_solveForJ = SolveFor::Total;
        atualizarEstadoCamposJ();
        recalcular();
    });
    connect(m_lockN, &QPushButton::clicked, this, [this]() {
        m_solveForJ = SolveFor::NumParcelas;
        atualizarEstadoCamposJ();
        recalcular();
    });
    connect(m_lockParcela, &QPushButton::clicked, this, [this]() {
        m_solveForJ = SolveFor::Parcela;
        atualizarEstadoCamposJ();
        recalcular();
    });
    connect(m_lockTaxa, &QPushButton::clicked, this, [this]() {
        m_solveForJ = SolveFor::Taxa;
        atualizarEstadoCamposJ();
        recalcular();
    });
    connect(m_lockTotalPago, &QPushButton::clicked, this, [this]() {
        m_totalPagoIsInput = !m_totalPagoIsInput;
        atualizarEstadoCamposJ();
        recalcular();
    });
    connect(m_valorTotalPago_j->lineEdit(), &QLineEdit::textChanged, this, [this]() {
        if (m_totalPagoIsInput) recalcular();
    });
}

void AddCompraDialog::atualizarEstadoCamposJ()
{
    const bool totalOut   = (m_solveForJ == SolveFor::Total);
    const bool nOut       = (m_solveForJ == SolveFor::NumParcelas);
    const bool taxaOut    = (m_solveForJ == SolveFor::Taxa);
    const bool parcelaOut = (m_solveForJ == SolveFor::Parcela);

    m_valorTotal_j->setReadOnly(totalOut);
    m_numParcelas_j->setReadOnly(nOut);
    m_jurosMensais->setReadOnly(taxaOut);
    m_cet_j->setReadOnly(taxaOut);
    m_valorParcela_j->setReadOnly(parcelaOut);

    const QString ttEditable  = tr("Campo editável. Clique para travá-lo e calculá-lo automaticamente.");
    const QString ttCalculado = tr("Campo calculado automaticamente. Clique para editá-lo manualmente.");

    m_lockTotal->setChecked(!totalOut);
    m_lockTotal->setIcon(Theme::emojiIcon(totalOut ? QStringLiteral("🚫") : QStringLiteral("✏️")));
    m_lockTotal->setToolTip(totalOut ? ttCalculado : ttEditable);

    m_lockN->setChecked(!nOut);
    m_lockN->setIcon(Theme::emojiIcon(nOut ? QStringLiteral("🚫") : QStringLiteral("✏️")));
    m_lockN->setToolTip(nOut ? ttCalculado : ttEditable);

    m_lockTaxa->setChecked(!taxaOut);
    m_lockTaxa->setIcon(Theme::emojiIcon(taxaOut ? QStringLiteral("🚫") : QStringLiteral("✏️")));
    m_lockTaxa->setToolTip(taxaOut ? ttCalculado : ttEditable);

    m_lockParcela->setChecked(!parcelaOut);
    m_lockParcela->setIcon(Theme::emojiIcon(parcelaOut ? QStringLiteral("🚫") : QStringLiteral("✏️")));
    m_lockParcela->setToolTip(parcelaOut ? ttCalculado : ttEditable);

    // TotalPago: independent lock (not part of SolveFor)
    m_valorTotalPago_j->setReadOnly(!m_totalPagoIsInput);
    m_lockTotalPago->setChecked(m_totalPagoIsInput);
    m_lockTotalPago->setIcon(Theme::emojiIcon(m_totalPagoIsInput ? QStringLiteral("✏️") : QStringLiteral("🚫")));
    m_lockTotalPago->setToolTip(m_totalPagoIsInput ? ttEditable : ttCalculado);
}

void AddCompraDialog::buildPreview()
{
    m_preview = new QTableWidget(0, 3, this);
    m_preview->setHorizontalHeaderLabels({
        tr("Nº"),
        tr("Mês/Ano"),
        tr("Valor")
    });
    m_preview->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_preview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_preview->setSelectionMode(QAbstractItemView::NoSelection);
    m_preview->setMaximumHeight(200);
    m_preview->setAlternatingRowColors(true);
}

void AddCompraDialog::onTabChanged(int)
{
    recalcular();
}

// Build a temporary Compra from current UI state for preview
static Compra compraFromUi(
    const QString &nome, int cartaoId, const QString &moeda,
    const QDate &dataInicio, int parcelaIni,
    qint64 valorTotal, int numParcelas, qint64 jurosMensais100k)
{
    Compra c;
    c.nome            = nome;
    c.cartaoId        = cartaoId;
    c.moeda           = moeda;
    c.mesInicio       = dataInicio.month();
    c.anoInicio       = dataInicio.year();
    c.parcelaInicial  = parcelaIni;
    c.valorTotal      = valorTotal;
    c.numParcelas     = numParcelas;
    c.jurosMensais    = jurosMensais100k;
    return c;
}

// Format centavos as a plain number "1.234,56" (no currency symbol) for edit fields
static QString formatFieldValue(qint64 centavos, const QString &moeda)
{
    const QString full = ParcelaService::formatarValor(centavos, moeda);
    const int idx = full.indexOf(QLatin1Char(' '));
    return idx >= 0 ? full.mid(idx + 1) : full;
}

void AddCompraDialog::recalcular()
{
    m_parcelas.clear();
    const QString moeda = m_moeda->currentData().toString();
    const QDate dataFat = m_dataInicio->date();
    const int piIni     = m_parcelaIni->value();

    ResultadoFinanceiro r;
    const int tab = m_tabs->currentIndex();

    try {
        if (tab == 0) {
            // Simples
            const qint64 vp = ParcelaService::parsearValor(m_valorParcela->text());
            const int n     = m_numParcelas_s->value();
            if (vp <= 0 || n <= 0) { atualizarPreview(); return; }
            Compra c = compraFromUi(QString(), 0, moeda, dataFat, piIni,
                                    vp * n, n, 0);
            r = ParcelaService::semJuros(c);
        } else if (tab == 1) {
            // Total
            const qint64 vt = ParcelaService::parsearValor(m_valorTotal_t->text());
            const int n     = m_numParcelas_t->value();
            if (vt <= 0 || n <= 0) { atualizarPreview(); return; }
            Compra c = compraFromUi(QString(), 0, moeda, dataFat, piIni, vt, n, 0);
            r = ParcelaService::semJuros(c);
        } else {
            // Juros tab — lock-based recalculation via ParcelaService::recalcular
            DadosFinanceiros d;
            d.valorTotal      = ParcelaService::parsearValor(m_valorTotal_j->text());
            d.numParcelas     = m_numParcelas_j->value();
            d.valorParcela    = ParcelaService::parsearValor(m_valorParcela_j->text());
            d.jurosMensaisPct = m_jurosMensais->value();
            d.cetPct          = m_cet_j->value();

            // If TotalPago is input: derive PMT = TotalPago / N
            const bool tpInput = m_totalPagoIsInput;
            if (tpInput && d.numParcelas > 0) {
                const qint64 tp = ParcelaService::parsearValor(m_valorTotalPago_j->text());
                if (tp > 0) d.valorParcela = tp / d.numParcelas;
            }

            // When TotalPago drives PMT and SolveFor==Parcela, solve for Total instead
            const SolveFor effectiveSolve = (tpInput && m_solveForJ == SolveFor::Parcela)
                                            ? SolveFor::Total
                                            : m_solveForJ;

            d = ParcelaService::recalcular(d, effectiveSolve);

            // Write the computed field back to the output widget (signals blocked)
            switch (effectiveSolve) {
            case SolveFor::Total:
                m_valorTotal_j->blockSignals(true);
                m_valorTotal_j->setText(formatFieldValue(d.valorTotal, moeda));
                m_valorTotal_j->blockSignals(false);
                break;
            case SolveFor::NumParcelas:
                m_numParcelas_j->blockSignals(true);
                m_numParcelas_j->setValue(d.numParcelas);
                m_numParcelas_j->blockSignals(false);
                break;
            case SolveFor::Taxa:
                m_jurosMensais->blockSignals(true);
                m_jurosMensais->setValue(d.jurosMensaisPct);
                m_jurosMensais->blockSignals(false);
                m_cet_j->blockSignals(true);
                m_cet_j->setValue(d.cetPct);
                m_cet_j->blockSignals(false);
                break;
            case SolveFor::Parcela:
                m_valorParcela_j->blockSignals(true);
                m_valorParcela_j->setText(formatFieldValue(d.valorParcela, moeda));
                m_valorParcela_j->blockSignals(false);
                break;
            }

            // Update TotalPago output (when it's not the input)
            if (!tpInput) {
                const qint64 tpCalc = d.valorParcela * static_cast<qint64>(d.numParcelas);
                m_valorTotalPago_j->blockSignals(true);
                m_valorTotalPago_j->setText(formatFieldValue(tpCalc, moeda));
                m_valorTotalPago_j->blockSignals(false);
            }

            m_lblJurosTotais->setText(tr("Juros totais: %1")
                .arg(ParcelaService::formatarValor(d.jurosTotais, moeda)));

            // Generate parcelas for preview
            if (d.valorTotal <= 0 || d.numParcelas <= 0) { atualizarPreview(); return; }
            Compra c = compraFromUi(QString(), 0, moeda, dataFat, piIni,
                                    d.valorTotal, d.numParcelas, 0);
            if (d.jurosMensaisPct > 1e-9)
                r = ParcelaService::comJurosMensais(c, d.jurosMensaisPct);
            else
                r = ParcelaService::semJuros(c);
        }
    } catch (...) {
        atualizarPreview();
        return;
    }

    m_parcelas = r.parcelas;
    atualizarPreview();
}

void AddCompraDialog::atualizarPreview()
{
    m_preview->setRowCount(0);
    const QString moeda = m_moeda->currentData().toString();

    for (const auto &p : m_parcelas) {
        const int row = m_preview->rowCount();
        m_preview->insertRow(row);
        m_preview->setItem(row, 0, new QTableWidgetItem(
            QStringLiteral("%1").arg(p.numero)));
        m_preview->setItem(row, 1, new QTableWidgetItem(p.dataFatura));
        m_preview->setItem(row, 2, new QTableWidgetItem(
            ParcelaService::formatarValor(p.valor, moeda)));
        m_preview->item(row, 2)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    }
}

void AddCompraDialog::accept()
{
    if (m_nome->text().trimmed().isEmpty()) {
        m_nome->setFocus();
        return;
    }
    if (m_parcelas.isEmpty()) return;

    const QString moeda  = m_moeda->currentData().toString();
    const QDate dataFat  = m_dataInicio->date();
    const int tab        = m_tabs->currentIndex();
    const int piIni      = m_parcelaIni->value();

    m_compra.nome           = m_nome->text().trimmed();
    m_compra.cartaoId       = m_cartao->currentData().toInt();
    m_compra.moeda          = moeda;
    m_compra.mesInicio      = dataFat.month();
    m_compra.anoInicio      = dataFat.year();
    m_compra.parcelaInicial = piIni;
    m_compra.numParcelas    = m_parcelas.size();

    if (tab == 0) {
        m_compra.valorTotal = ParcelaService::parsearValor(m_valorParcela->text()) * m_parcelas.size();
    } else if (tab == 1) {
        m_compra.valorTotal = ParcelaService::parsearValor(m_valorTotal_t->text());
    } else {
        m_compra.valorTotal   = ParcelaService::parsearValor(m_valorTotal_j->text());
        m_compra.jurosMensais = static_cast<qint64>(
            std::round(m_jurosMensais->value() * 10000.0));
    }

    m_compra.receipt = m_receipt->receipt();

    QDialog::accept();
}

void AddCompraDialog::preencherJuros(double pv, int n, double iMensal)
{
    // Switch to the Juros tab (index 2).
    m_tabs->setCurrentIndex(2);

    // Set PV (valor à vista) — format as BRL string with 2 decimal places.
    if (pv != 0.0) {
        const qint64 pvCentavos = static_cast<qint64>(std::round(std::abs(pv) * 100.0));
        m_valorTotal_j->lineEdit()->setText(
            ParcelaService::formatarValor(pvCentavos));
    }

    // Set N (number of parcelas).
    if (n > 0)
        m_numParcelas_j->setValue(n);

    // Set monthly interest rate.
    if (iMensal > 0.0)
        m_jurosMensais->setValue(iMensal);

    // Solve for PMT (parcela) — the most common use case.
    m_solveForJ = SolveFor::Parcela;
    atualizarEstadoCamposJ();
    recalcular();
}
