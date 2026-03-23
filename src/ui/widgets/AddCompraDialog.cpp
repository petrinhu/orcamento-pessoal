#include "AddCompraDialog.h"
#include "services/ParcelaService.h"
#include <cmath>
#include "models/Cartao.h"
#include "ui/Theme.h"

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
    m_dataInicio->setDisplayFormat(QStringLiteral("MM/yyyy"));
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

    // ── Buttons ────────────────────────────────────────────────────────────
    auto *btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btnBox->button(QDialogButtonBox::Ok)->setText(tr("Adicionar"));
    btnBox->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("primary"));
    connect(btnBox, &QDialogButtonBox::accepted, this, &AddCompraDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btnBox);
}

void AddCompraDialog::buildTabSimples(QWidget *tab)
{
    auto *form = new QFormLayout(tab);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    m_valorParcela = new QLineEdit(tab);
    m_valorParcela->setPlaceholderText(QStringLiteral("0,00"));
    form->addRow(tr("Valor da parcela (R$)"), m_valorParcela);

    m_numParcelas_s = new QSpinBox(tab);
    m_numParcelas_s->setRange(1, 999);
    m_numParcelas_s->setValue(12);
    m_numParcelas_s->setSuffix(QStringLiteral("x"));
    form->addRow(tr("Número de parcelas"), m_numParcelas_s);

    connect(m_valorParcela,  &QLineEdit::textChanged, this, &AddCompraDialog::recalcular);
    connect(m_numParcelas_s, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AddCompraDialog::recalcular);
}

void AddCompraDialog::buildTabTotal(QWidget *tab)
{
    auto *form = new QFormLayout(tab);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    m_valorTotal_t = new QLineEdit(tab);
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

    connect(m_valorTotal_t,  &QLineEdit::textChanged, this, &AddCompraDialog::recalcular);
    connect(m_numParcelas_t, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AddCompraDialog::recalcular);
}

void AddCompraDialog::buildTabJuros(QWidget *tab)
{
    auto *form = new QFormLayout(tab);
    form->setContentsMargins(16, 16, 16, 16);
    form->setSpacing(10);

    m_valorTotal_j = new QLineEdit(tab);
    m_valorTotal_j->setPlaceholderText(QStringLiteral("0,00"));
    form->addRow(tr("Valor total / financiado (R$)"), m_valorTotal_j);

    m_numParcelas_j = new QSpinBox(tab);
    m_numParcelas_j->setRange(1, 999);
    m_numParcelas_j->setValue(12);
    m_numParcelas_j->setSuffix(QStringLiteral("x"));
    form->addRow(tr("Número de parcelas"), m_numParcelas_j);

    m_valorParcela_j = new QLineEdit(tab);
    m_valorParcela_j->setPlaceholderText(
        tr("Preencha para calcular juros, ou deixe em branco"));
    form->addRow(tr("Valor de cada parcela (R$)"), m_valorParcela_j);

    m_jurosMensais = new QDoubleSpinBox(tab);
    m_jurosMensais->setRange(0.0, 100.0);
    m_jurosMensais->setSingleStep(0.1);
    m_jurosMensais->setDecimals(4);
    m_jurosMensais->setSuffix(QStringLiteral("% a.m."));
    m_jurosMensais->setToolTip(
        tr("Taxa mensal. Preencha junto com Total ou Valor da parcela\n"
           "para que o app calcule o campo faltante.\n"
           "Ou preencha Total + Valor da parcela para calcular a taxa."));
    form->addRow(tr("Juros mensais"), m_jurosMensais);

    m_cet_j = new QDoubleSpinBox(tab);
    m_cet_j->setRange(0.0, 100000.0);
    m_cet_j->setSingleStep(0.5);
    m_cet_j->setDecimals(2);
    m_cet_j->setSuffix(QStringLiteral("% a.a."));
    m_cet_j->setToolTip(tr("CET (Custo Efetivo Total) anual. Equivalente aos juros mensais compostos."));
    form->addRow(tr("CET / Juros anuais"), m_cet_j);

    // Info labels
    m_lblCET          = new QLabel(tr("CET: —"), tab);
    m_lblJurosMensais = new QLabel(tr("Juros mensais: —"), tab);
    m_lblJurosTotais  = new QLabel(tr("Juros totais: —"), tab);
    for (auto *lbl : {m_lblCET, m_lblJurosMensais, m_lblJurosTotais}) {
        lbl->setObjectName(QStringLiteral("subtitle"));
        form->addRow(QString(), lbl);
    }

    connect(m_valorTotal_j,   &QLineEdit::textChanged, this, &AddCompraDialog::recalcular);
    connect(m_valorParcela_j, &QLineEdit::textChanged, this, &AddCompraDialog::recalcular);
    connect(m_numParcelas_j,  QOverload<int>::of(&QSpinBox::valueChanged),
            this, &AddCompraDialog::recalcular);

    // Juros mensais ↔ CET cross-update (bidirectional, each triggers recalcular)
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
            // Juros
            const qint64 vt  = ParcelaService::parsearValor(m_valorTotal_j->text());
            const qint64 vp  = ParcelaService::parsearValor(m_valorParcela_j->text());
            const int n      = m_numParcelas_j->value();
            const double jm  = m_jurosMensais->value();
            if (n <= 0) { atualizarPreview(); return; }

            if (vt > 0 && vp > 0) {
                // Mode 2.5: infer rate
                r = ParcelaService::inferirJuros(vt, n, vp);
                Compra c = compraFromUi(QString(), 0, moeda, dataFat, piIni, vt, n, 0);
                for (int k = 0; k < r.parcelas.size(); ++k)
                    r.parcelas[k].dataFatura =
                        ParcelaService::dataParaParcela(c, k+1)
                            .toString(QStringLiteral("dd/MM/yyyy"));
            } else if (vt > 0 && jm > 0) {
                // Mode 2.4: given rate
                Compra c = compraFromUi(QString(), 0, moeda, dataFat, piIni, vt, n, 0);
                r = ParcelaService::comJurosMensais(c, jm);
            } else if (vp > 0 && jm > 0) {
                // Solve for PV: PV = PMT × [1-(1+i)^-n] / i
                const double i = jm / 100.0;
                const qint64 vtCalc = static_cast<qint64>(
                    std::round(static_cast<double>(vp) * (1.0 - std::pow(1.0 + i, -n)) / i));
                m_valorTotal_j->blockSignals(true);
                m_valorTotal_j->setText(formatFieldValue(vtCalc, moeda));
                m_valorTotal_j->blockSignals(false);
                Compra c = compraFromUi(QString(), 0, moeda, dataFat, piIni, vtCalc, n, 0);
                r = ParcelaService::comJurosMensais(c, jm);
            } else if (vt > 0) {
                // No interest
                Compra c = compraFromUi(QString(), 0, moeda, dataFat, piIni, vt, n, 0);
                r = ParcelaService::semJuros(c);
            } else {
                atualizarPreview(); return;
            }

            // Update info labels
            m_lblCET->setText(tr("CET: %1% a.a.")
                .arg(r.jurosAnuaisPct, 0, 'f', 2));
            m_lblJurosMensais->setText(tr("Juros mensais: %1% a.m.")
                .arg(r.jurosMensaisPct, 0, 'f', 4));
            m_lblJurosTotais->setText(tr("Juros totais: %1")
                .arg(ParcelaService::formatarValor(r.jurosTotais, moeda)));
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

    QDialog::accept();
}
