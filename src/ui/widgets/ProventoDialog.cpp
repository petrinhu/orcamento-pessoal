#include "ProventoDialog.h"
#include "CalculadoraEdit.h"

#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <cmath>

namespace {
qint64 parseCent(const QString &s)
{
    QString t = s; t.remove(QLatin1Char('R')).remove(QLatin1Char('$'))
                  .remove(QLatin1Char(' '));
    if (t.contains(QLatin1Char(','))) {
        t.remove(QLatin1Char('.')).replace(QLatin1Char(','), QLatin1Char('.'));
    }
    return static_cast<qint64>(std::round(t.toDouble() * 100.0));
}
QString fmtCent(qint64 c)
{
    return QString::number(c / 100.0, 'f', 2).replace(QLatin1Char('.'),
                                                       QLatin1Char(','));
}
}

ProventoDialog::ProventoDialog(const Provento &original, QWidget *parent)
    : QDialog(parent), m_original(original)
{
    setWindowTitle(tr("Provento"));
    buildUi();
    recalcularLiquido();
}

void ProventoDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    m_edtTicker = new QLineEdit(this);
    m_edtTicker->setText(m_original.ticker);
    m_edtTicker->setPlaceholderText(QStringLiteral("ex: PETR4, HGLG11"));
    form->addRow(tr("Ticker:"), m_edtTicker);

    m_cbTipo = new QComboBox(this);
    m_cbTipo->addItem(tr("Dividendo (isento PF)"),    QStringLiteral("dividendo"));
    m_cbTipo->addItem(tr("JCP (IR 17,5% retido)"),    QStringLiteral("jcp"));
    m_cbTipo->addItem(tr("Rendimento FII"),             QStringLiteral("rendimento"));
    m_cbTipo->addItem(tr("Bonificação"),                QStringLiteral("bonus"));
    if (!m_original.tipo.isEmpty()) {
        const int idx = m_cbTipo->findData(m_original.tipo);
        if (idx >= 0) m_cbTipo->setCurrentIndex(idx);
    }
    form->addRow(tr("Tipo:"), m_cbTipo);

    m_dateEvento = new QDateEdit(this);
    m_dateEvento->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateEvento->setCalendarPopup(true);
    m_dateEvento->setDate(m_original.dataEvento.isValid()
        ? m_original.dataEvento : QDate::currentDate());
    form->addRow(tr("Data do evento:"), m_dateEvento);

    m_edtBruto = new CalculadoraEdit(this);
    if (m_original.valorBrutoCent > 0)
        m_edtBruto->setText(fmtCent(m_original.valorBrutoCent));
    form->addRow(tr("Valor bruto (R$):"), m_edtBruto);

    m_edtLiquido = new CalculadoraEdit(this);
    if (m_original.valorLiquidoCent > 0)
        m_edtLiquido->setText(fmtCent(m_original.valorLiquidoCent));
    form->addRow(tr("Valor líquido (R$):"), m_edtLiquido);

    m_spnQty = new QDoubleSpinBox(this);
    m_spnQty->setDecimals(8);
    m_spnQty->setRange(0.0, 1e12);
    m_spnQty->setValue(m_original.quantidade);
    form->addRow(tr("Quantidade detida:"), m_spnQty);

    m_spnPorAcao = new QDoubleSpinBox(this);
    m_spnPorAcao->setDecimals(8);
    m_spnPorAcao->setRange(0.0, 1e9);
    m_spnPorAcao->setValue(m_original.porAcao);
    m_spnPorAcao->setSuffix(QStringLiteral(" R$"));
    form->addRow(tr("Por ação:"), m_spnPorAcao);

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

    connect(m_cbTipo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProventoDialog::recalcularLiquido);
    connect(m_edtBruto->lineEdit(), &QLineEdit::editingFinished,
            this, &ProventoDialog::recalcularLiquido);

    setMinimumWidth(400);
}

void ProventoDialog::recalcularLiquido()
{
    // Auto-calculo apenas se liquido vazio ou batendo com bruto antigo
    const qint64 bruto = parseCent(m_edtBruto->text());
    if (bruto <= 0) return;
    const QString tipo = m_cbTipo->currentData().toString();
    // JCP: aliquota IRRF 17,5% desde jan/2026 (LC 224/2025). Era 15% ate 2025.
    const double aliq  = (tipo == QLatin1String("jcp")) ? 0.175 : 0.0;
    const qint64 liquido = static_cast<qint64>(std::round(bruto * (1.0 - aliq)));
    // Apenas atualiza se liquido estiver vazio ou for igual a bruto (estado inicial)
    const qint64 atual = parseCent(m_edtLiquido->text());
    if (atual == 0 || atual == bruto) {
        m_edtLiquido->setText(fmtCent(liquido));
    }
}

Provento ProventoDialog::result() const
{
    Provento p = m_original;
    p.ticker            = m_edtTicker->text().trimmed().toUpper();
    p.tipo              = m_cbTipo->currentData().toString();
    p.dataEvento        = m_dateEvento->date();
    p.valorBrutoCent    = parseCent(m_edtBruto->text());
    p.valorLiquidoCent  = parseCent(m_edtLiquido->text());
    p.quantidade        = m_spnQty->value();
    p.porAcao           = m_spnPorAcao->value();
    p.observacao        = m_edtObs->text().trimmed();
    return p;
}
