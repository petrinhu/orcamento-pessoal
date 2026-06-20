#include "DividaDialog.h"
#include "CalculadoraEdit.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QRadioButton>
#include <QGroupBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDate>

#include <cmath>

// Conversões de juros compostos (i em fração, ex.: 1,5% = 0,015).
//   a.a. = (1 + i_m)^12 - 1
//   i_m  = (1 + i_a)^(1/12) - 1
static double mensalParaAnual(double mensalPct)
{
    return (std::pow(1.0 + mensalPct / 100.0, 12.0) - 1.0) * 100.0;
}

static double anualParaMensal(double anualPct)
{
    return (std::pow(1.0 + anualPct / 100.0, 1.0 / 12.0) - 1.0) * 100.0;
}

// ── Constructor ───────────────────────────────────────────────────────────────

DividaDialog::DividaDialog(const Divida &divida, QWidget *parent)
    : QDialog(parent)
    , m_id(divida.id)
{
    setWindowTitle(divida.id == 0 ? tr("Nova dívida / empréstimo")
                                  : tr("Editar dívida / empréstimo"));
    setMinimumWidth(380);
    buildUi(divida);
}

void DividaDialog::buildUi(const Divida &d)
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);

    // ── Tipo ──────────────────────────────────────────────────────────────
    auto *grpTipo = new QGroupBox(tr("Tipo"), this);
    auto *layTipo = new QHBoxLayout(grpTipo);
    m_rDivida     = new QRadioButton(tr("Dívida (eu devo)"),     grpTipo);
    m_rEmprestimo = new QRadioButton(tr("Empréstimo (me devem)"), grpTipo);
    layTipo->addWidget(m_rDivida);
    layTipo->addWidget(m_rEmprestimo);
    (d.tipo == QStringLiteral("emprestimo") ? m_rEmprestimo : m_rDivida)->setChecked(true);
    root->addWidget(grpTipo);

    // ── Form ──────────────────────────────────────────────────────────────
    auto *form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setLabelAlignment(Qt::AlignRight);

    m_edtContraparte = new QLineEdit(d.contraparte, this);
    m_edtContraparte->setPlaceholderText(tr("Nome do credor / devedor"));
    form->addRow(tr("Contraparte:"), m_edtContraparte);

    m_edtDescricao = new QLineEdit(d.descricao, this);
    m_edtDescricao->setPlaceholderText(tr("Descrição opcional"));
    form->addRow(tr("Descrição:"), m_edtDescricao);

    m_edtTotal = new CalculadoraEdit(this);
    m_edtTotal->lineEdit()->setText(uifmt::formatCentavos(d.valorTotal));
    form->addRow(tr("Valor total:"), m_edtTotal);

    m_edtPago = new CalculadoraEdit(this);
    m_edtPago->lineEdit()->setText(uifmt::formatCentavos(d.valorPago));
    form->addRow(tr("Valor já pago:"), m_edtPago);

    m_spnJuros = new QDoubleSpinBox(this);
    m_spnJuros->setRange(0.0, 100.0);
    m_spnJuros->setDecimals(4);
    m_spnJuros->setSuffix(QStringLiteral(" % a.m."));
    m_spnJuros->setValue(d.jurosMensal / 10000.0);
    form->addRow(tr("Juros a.m.:"), m_spnJuros);

    m_spnJurosAnual = new QDoubleSpinBox(this);
    m_spnJurosAnual->setRange(0.0, 100000.0);  // (1+1)^12-1 ~= 409500% no extremo
    m_spnJurosAnual->setDecimals(2);
    m_spnJurosAnual->setSuffix(QStringLiteral(" % a.a."));
    m_spnJurosAnual->setValue(mensalParaAnual(d.jurosMensal / 10000.0));
    form->addRow(tr("Juros a.a.:"), m_spnJurosAnual);

    // Sincronização bidirecional: a.m. é a fonte da verdade ao salvar; editar
    // um campo recalcula o outro pela fórmula composta. O flag-guard evita loop.
    connect(m_spnJuros, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double v) {
                if (m_sincronizando) return;
                m_sincronizando = true;
                m_spnJurosAnual->setValue(mensalParaAnual(v));
                m_sincronizando = false;
            });
    connect(m_spnJurosAnual, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, [this](double v) {
                if (m_sincronizando) return;
                m_sincronizando = true;
                m_spnJuros->setValue(anualParaMensal(v));
                m_sincronizando = false;
            });

    m_dtInicio = new QDateEdit(this);
    m_dtInicio->setCalendarPopup(true);
    m_dtInicio->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dtInicio->setDate(d.dataInicio.isEmpty()
        ? QDate::currentDate()
        : QDate::fromString(d.dataInicio, Qt::ISODate));
    form->addRow(tr("Data início:"), m_dtInicio);

    m_dtVenc = new QDateEdit(this);
    m_dtVenc->setCalendarPopup(true);
    m_dtVenc->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dtVenc->setSpecialValueText(tr("Sem vencimento"));
    m_dtVenc->setMinimumDate(QDate(2000, 1, 1));
    if (d.dataVenc.isEmpty()) {
        m_dtVenc->setDate(m_dtVenc->minimumDate());
    } else {
        m_dtVenc->setDate(QDate::fromString(d.dataVenc, Qt::ISODate));
    }
    form->addRow(tr("Vencimento:"), m_dtVenc);

    m_cbStatus = new QComboBox(this);
    m_cbStatus->addItem(tr("Em andamento"), QStringLiteral("em_andamento"));
    m_cbStatus->addItem(tr("Quitado"),      QStringLiteral("quitado"));
    m_cbStatus->addItem(tr("Atrasado"),     QStringLiteral("atrasado"));
    {
        const int idx = m_cbStatus->findData(
            d.status.isEmpty() ? QStringLiteral("em_andamento") : d.status);
        if (idx >= 0) m_cbStatus->setCurrentIndex(idx);
    }
    form->addRow(tr("Status:"), m_cbStatus);

    m_edtObs = new QLineEdit(d.obs, this);
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    // ── Buttons ───────────────────────────────────────────────────────────
    // Margem interna garante folga para a sombra dos botões (#176).
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    btns->setContentsMargins(0, 6, 10, 10);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

// ── Getter ────────────────────────────────────────────────────────────────────

Divida DividaDialog::divida() const
{
    Divida d;
    d.id          = m_id;
    d.tipo        = m_rEmprestimo->isChecked()
                    ? QStringLiteral("emprestimo")
                    : QStringLiteral("divida");
    d.contraparte = m_edtContraparte->text().trimmed();
    d.descricao   = m_edtDescricao->text().trimmed();
    d.valorTotal  = uifmt::parseCentavos(m_edtTotal->lineEdit()->text());
    d.valorPago   = uifmt::parseCentavos(m_edtPago->lineEdit()->text());
    d.jurosMensal = static_cast<int>(qRound(m_spnJuros->value() * 10000.0));
    d.dataInicio  = m_dtInicio->date().toString(Qt::ISODate);
    d.dataVenc    = (m_dtVenc->date() == m_dtVenc->minimumDate())
                    ? QString()
                    : m_dtVenc->date().toString(Qt::ISODate);
    d.status      = m_cbStatus->currentData().toString();
    d.obs         = m_edtObs->text().trimmed();
    return d;
}
