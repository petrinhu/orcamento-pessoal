#include "LembreteDialog.h"
#include "CalculadoraEdit.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QDateEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QDate>

// ── Helpers ───────────────────────────────────────────────────────────────────

static qint64 parseCentavos(const QString &txt)
{
    QString s = txt.trimmed();
    s.remove(QStringLiteral("R$")).remove(QLatin1Char(' '));
    if (s.contains(QLatin1Char(',')) && s.contains(QLatin1Char('.'))) {
        s.remove(QLatin1Char('.'));
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    } else if (s.contains(QLatin1Char(','))) {
        s.replace(QLatin1Char(','), QLatin1Char('.'));
    }
    bool ok = false;
    const double val = s.toDouble(&ok);
    return ok ? static_cast<qint64>(qRound(val * 100.0)) : 0LL;
}

static QString formatCentavos(qint64 c)
{
    if (c == 0) return QString();
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("%1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

// ── Constructor ───────────────────────────────────────────────────────────────

LembreteDialog::LembreteDialog(const Lembrete &lembrete, QWidget *parent)
    : QDialog(parent)
    , m_id(lembrete.id)
{
    setWindowTitle(lembrete.id == 0 ? tr("Novo lembrete") : tr("Editar lembrete"));
    setMinimumWidth(360);
    buildUi(lembrete);
}

void LembreteDialog::buildUi(const Lembrete &l)
{
    auto *root = new QVBoxLayout(this);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setLabelAlignment(Qt::AlignRight);

    m_edtDescricao = new QLineEdit(l.descricao, this);
    m_edtDescricao->setPlaceholderText(tr("Ex: Conta de luz, Netflix…"));
    form->addRow(tr("Descrição:"), m_edtDescricao);

    m_edtValor = new CalculadoraEdit(this);
    m_edtValor->lineEdit()->setText(formatCentavos(l.valor));
    m_edtValor->lineEdit()->setPlaceholderText(tr("Opcional"));
    form->addRow(tr("Valor:"), m_edtValor);

    m_dtVenc = new QDateEdit(this);
    m_dtVenc->setCalendarPopup(true);
    m_dtVenc->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dtVenc->setDate(l.dataVenc.isEmpty()
        ? QDate::currentDate()
        : QDate::fromString(l.dataVenc, Qt::ISODate));
    form->addRow(tr("Vencimento:"), m_dtVenc);

    m_cbFreq = new QComboBox(this);
    m_cbFreq->addItem(tr("Único"),  QStringLiteral("unico"));
    m_cbFreq->addItem(tr("Mensal"), QStringLiteral("mensal"));
    m_cbFreq->addItem(tr("Anual"),  QStringLiteral("anual"));
    {
        const int idx = m_cbFreq->findData(
            l.frequencia.isEmpty() ? QStringLiteral("unico") : l.frequencia);
        if (idx >= 0) m_cbFreq->setCurrentIndex(idx);
    }
    form->addRow(tr("Frequência:"), m_cbFreq);

    // ── Débito automático ──────────────────────────────────────────────────
    m_chkDebito = new QCheckBox(tr("Débito automático"), this);
    m_chkDebito->setChecked(l.debitoAutomatico);
    m_chkDebito->setToolTip(tr(
        "Marque se o valor é debitado automaticamente pela instituição financeira.\n"
        "Caso o débito não ocorra, use \"Marcar como não debitado\" para acionamento manual."));
    form->addRow(QString(), m_chkDebito);

    // ── Status ────────────────────────────────────────────────────────────
    m_cbStatus = new QComboBox(this);
    m_cbStatus->addItem(tr("Pendente"),        QStringLiteral("pendente"));
    m_cbStatus->addItem(tr("Pago"),            QStringLiteral("pago"));
    m_cbStatus->addItem(tr("Não debitado"),    QStringLiteral("nao_pago"));
    m_cbStatus->addItem(tr("Ignorado"),        QStringLiteral("ignorado"));
    {
        const int idx = m_cbStatus->findData(
            l.status.isEmpty() ? QStringLiteral("pendente") : l.status);
        if (idx >= 0) m_cbStatus->setCurrentIndex(idx);
    }
    // "Não debitado" só faz sentido com débito automático
    connect(m_chkDebito, &QCheckBox::toggled, this, &LembreteDialog::onDebitoToggled);
    onDebitoToggled(l.debitoAutomatico);
    form->addRow(tr("Status:"), m_cbStatus);

    m_edtObs = new QLineEdit(l.obs, this);
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}

void LembreteDialog::onDebitoToggled(bool checked)
{
    // "Não debitado" só faz sentido para débito automático.
    // Se desmarcou débito automático e status era nao_pago, volta para pendente.
    if (!checked
        && m_cbStatus->currentData().toString() == QStringLiteral("nao_pago"))
    {
        m_cbStatus->setCurrentIndex(
            m_cbStatus->findData(QStringLiteral("pendente")));
    }
    // Mostra/oculta o item "Não debitado" removendo-o e re-adicionando conforme necessário.
    const int idxNaoPago = m_cbStatus->findData(QStringLiteral("nao_pago"));
    if (checked && idxNaoPago < 0)
        m_cbStatus->addItem(tr("Não debitado"), QStringLiteral("nao_pago"));
    else if (!checked && idxNaoPago >= 0)
        m_cbStatus->removeItem(idxNaoPago);
}

// ── Getter ────────────────────────────────────────────────────────────────────

Lembrete LembreteDialog::lembrete() const
{
    Lembrete l;
    l.id              = m_id;
    l.descricao       = m_edtDescricao->text().trimmed();
    l.valor           = parseCentavos(m_edtValor->lineEdit()->text());
    l.dataVenc        = m_dtVenc->date().toString(Qt::ISODate);
    l.frequencia      = m_cbFreq->currentData().toString();
    l.debitoAutomatico = m_chkDebito->isChecked();
    l.status          = m_cbStatus->currentData().toString();
    l.obs             = m_edtObs->text().trimmed();
    return l;
}
