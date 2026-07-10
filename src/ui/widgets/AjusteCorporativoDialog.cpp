#include "AjusteCorporativoDialog.h"
#include "ui/UiFormatters.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QVBoxLayout>

AjusteCorporativoDialog::AjusteCorporativoDialog(
    const AjusteCorporativo &original, QWidget *parent)
    : QDialog(parent), m_original(original)
{
    setWindowTitle(tr("Ajuste corporativo"));
    buildUi();
}

void AjusteCorporativoDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    m_edtTicker = new QLineEdit(this);
    m_edtTicker->setText(m_original.ticker);
    m_edtTicker->setPlaceholderText(QStringLiteral("ex: PETR4, MGLU3"));
    // (#203a/SEC-03) ver comentário gêmeo em ProventoDialog::buildUi.
    m_edtTicker->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[A-Za-z0-9]{1,12}")), m_edtTicker));
    form->addRow(tr("Ticker:"), m_edtTicker);

    m_cbTipo = new QComboBox(this);
    m_cbTipo->addItem(tr("Split (desdobramento)"),       QStringLiteral("split"));
    m_cbTipo->addItem(tr("Inplit (grupamento)"),          QStringLiteral("inplit"));
    m_cbTipo->addItem(tr("Bonificação"),                   QStringLiteral("bonificacao"));
    if (!m_original.tipo.isEmpty()) {
        const int idx = m_cbTipo->findData(m_original.tipo);
        if (idx >= 0) m_cbTipo->setCurrentIndex(idx);
    }
    form->addRow(tr("Tipo:"), m_cbTipo);

    m_dateEx = new QDateEdit(this);
    m_dateEx->setDisplayFormat(uifmt::dataDisplayFormat());
    m_dateEx->setCalendarPopup(true);
    m_dateEx->setDate(m_original.dataEx.isValid()
        ? m_original.dataEx : QDate::currentDate());
    form->addRow(tr("Data ex:"), m_dateEx);

    m_spnFator = new QDoubleSpinBox(this);
    m_spnFator->setDecimals(6);
    m_spnFator->setRange(0.000001, 1000.0);
    m_spnFator->setValue(m_original.fator > 0 ? m_original.fator : 1.0);
    m_spnFator->setSingleStep(0.1);
    m_spnFator->setToolTip(tr(
        "Multiplicador da qty:\n"
        "  Split 2:1 → 2.0\n"
        "  Split 3:1 → 3.0\n"
        "  Inplit 1:10 → 0.1\n"
        "  Bonificação 10% → 1.1"));
    form->addRow(tr("Fator:"), m_spnFator);

    m_edtObs = new QLineEdit(this);
    m_edtObs->setText(m_original.observacao);
    m_edtObs->setMaxLength(200);
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    // Margem interna garante folga para a sombra dos botões (#176).
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Save)->setText(tr("Salvar"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    btns->setContentsMargins(0, 6, 10, 10);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    setMinimumWidth(360);
}

AjusteCorporativo AjusteCorporativoDialog::result() const
{
    AjusteCorporativo a = m_original;
    a.ticker     = m_edtTicker->text().trimmed().toUpper();
    a.tipo       = m_cbTipo->currentData().toString();
    a.dataEx     = m_dateEx->date();
    a.fator      = m_spnFator->value();
    a.observacao = m_edtObs->text().trimmed();
    return a;
}
