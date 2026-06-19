#include "MetaDialog.h"
#include "CalculadoraEdit.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDate>

// ── Constructor ───────────────────────────────────────────────────────────────

MetaDialog::MetaDialog(const Meta &meta, QWidget *parent)
    : QDialog(parent)
    , m_id(meta.id)
{
    setWindowTitle(meta.id == 0 ? tr("Nova meta") : tr("Editar meta"));
    setMinimumWidth(360);
    buildUi(meta);
}

void MetaDialog::buildUi(const Meta &m)
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setLabelAlignment(Qt::AlignRight);

    m_edtNome = new QLineEdit(m.nome, this);
    m_edtNome->setPlaceholderText(tr("Nome da meta"));
    form->addRow(tr("Nome:"), m_edtNome);

    m_cbCategoria = new QComboBox(this);
    m_cbCategoria->addItem(tr("Emergência"),    QStringLiteral("emergencia"));
    m_cbCategoria->addItem(tr("Viagem"),        QStringLiteral("viagem"));
    m_cbCategoria->addItem(tr("Imóvel"),        QStringLiteral("imovel"));
    m_cbCategoria->addItem(tr("Educação"),      QStringLiteral("educacao"));
    m_cbCategoria->addItem(tr("Aposentadoria"), QStringLiteral("aposentadoria"));
    m_cbCategoria->addItem(tr("Outro"),         QStringLiteral("outro"));
    {
        const int idx = m_cbCategoria->findData(
            m.categoria.isEmpty() ? QStringLiteral("outro") : m.categoria);
        if (idx >= 0) m_cbCategoria->setCurrentIndex(idx);
    }
    form->addRow(tr("Categoria:"), m_cbCategoria);

    m_edtMeta = new CalculadoraEdit(this);
    m_edtMeta->lineEdit()->setText(uifmt::formatCentavos(m.valorMeta));
    form->addRow(tr("Valor da meta:"), m_edtMeta);

    m_edtAtual = new CalculadoraEdit(this);
    m_edtAtual->lineEdit()->setText(uifmt::formatCentavos(m.valorAtual));
    form->addRow(tr("Valor acumulado:"), m_edtAtual);

    m_dtInicio = new QDateEdit(this);
    m_dtInicio->setCalendarPopup(true);
    m_dtInicio->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dtInicio->setDate(m.dataInicio.isEmpty()
        ? QDate::currentDate()
        : QDate::fromString(m.dataInicio, Qt::ISODate));
    form->addRow(tr("Data início:"), m_dtInicio);

    m_dtPrazo = new QDateEdit(this);
    m_dtPrazo->setCalendarPopup(true);
    m_dtPrazo->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dtPrazo->setSpecialValueText(tr("Sem prazo"));
    m_dtPrazo->setMinimumDate(QDate(2000, 1, 1));
    if (m.dataPrazo.isEmpty())
        m_dtPrazo->setDate(m_dtPrazo->minimumDate());
    else
        m_dtPrazo->setDate(QDate::fromString(m.dataPrazo, Qt::ISODate));
    form->addRow(tr("Prazo:"), m_dtPrazo);

    m_cbStatus = new QComboBox(this);
    m_cbStatus->addItem(tr("Em andamento"), QStringLiteral("em_andamento"));
    m_cbStatus->addItem(tr("Concluída"),    QStringLiteral("concluida"));
    m_cbStatus->addItem(tr("Cancelada"),    QStringLiteral("cancelada"));
    {
        const int idx = m_cbStatus->findData(
            m.status.isEmpty() ? QStringLiteral("em_andamento") : m.status);
        if (idx >= 0) m_cbStatus->setCurrentIndex(idx);
    }
    form->addRow(tr("Status:"), m_cbStatus);

    m_edtObs = new QLineEdit(m.obs, this);
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

// ── Getter ────────────────────────────────────────────────────────────────────

Meta MetaDialog::meta() const
{
    Meta m;
    m.id         = m_id;
    m.nome       = m_edtNome->text().trimmed();
    m.categoria  = m_cbCategoria->currentData().toString();
    m.valorMeta  = uifmt::parseCentavos(m_edtMeta->lineEdit()->text());
    m.valorAtual = uifmt::parseCentavos(m_edtAtual->lineEdit()->text());
    m.dataInicio = m_dtInicio->date().toString(Qt::ISODate);
    m.dataPrazo  = (m_dtPrazo->date() == m_dtPrazo->minimumDate())
                   ? QString()
                   : m_dtPrazo->date().toString(Qt::ISODate);
    m.status     = m_cbStatus->currentData().toString();
    m.obs        = m_edtObs->text().trimmed();
    return m;
}
