#include "LancamentoEntradaDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QDateEdit>
#include <QSpinBox>
#include <QRadioButton>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDate>
#include <QLocale>

// ── Helpers ───────────────────────────────────────────────────────────────────

static qint64 parseCentavos(const QString &txt)
{
    // Accepts "1.234,56" or "1234.56" or "1234" → centavos
    QString s = txt.trimmed();
    s.remove(QStringLiteral("R$")).remove(QLatin1Char(' '));
    // BR format: dot = thousands sep, comma = decimal
    if (s.contains(QLatin1Char(',')) && s.contains(QLatin1Char('.'))) {
        s.remove(QLatin1Char('.')); // remove thousands sep
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
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("%1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

// ── Constructor ───────────────────────────────────────────────────────────────

LancamentoEntradaDialog::LancamentoEntradaDialog(const LancamentoEntrada &lancamento,
                                                 int mes, int ano,
                                                 const QList<FonteRenda> &fontes,
                                                 QWidget *parent)
    : QDialog(parent)
    , m_original(lancamento)
    , m_mes(mes)
    , m_ano(ano)
{
    setWindowTitle(lancamento.id > 0 ? tr("Editar lançamento") : tr("Novo lançamento"));
    setModal(true);
    buildUi(fontes);
}

void LancamentoEntradaDialog::buildUi(const QList<FonteRenda> &fontes)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(8);

    // ── Fonte ──────────────────────────────────────────────────────────────
    m_cbFonte = new QComboBox(this);
    m_cbFonte->setEditable(true);
    m_cbFonte->setInsertPolicy(QComboBox::NoInsert);
    for (const auto &f : fontes)
        m_cbFonte->addItem(f.nome, f.id);
    if (auto *comp = m_cbFonte->completer()) {
        comp->setFilterMode(Qt::MatchContains);
        comp->setCaseSensitivity(Qt::CaseInsensitive);
    }
    if (!m_original.fonte.isEmpty()) {
        const int idx = m_cbFonte->findText(m_original.fonte);
        if (idx >= 0) m_cbFonte->setCurrentIndex(idx);
        else          m_cbFonte->setCurrentText(m_original.fonte);
    }
    form->addRow(tr("Fonte:"), m_cbFonte);

    // ── Valor ──────────────────────────────────────────────────────────────
    m_edtValor = new QLineEdit(this);
    m_edtValor->setPlaceholderText(QStringLiteral("0,00"));
    if (m_original.valorBruto > 0)
        m_edtValor->setText(formatCentavos(m_original.valorBruto));
    connect(m_edtValor, &QLineEdit::textChanged, this, &LancamentoEntradaDialog::onValoresChanged);
    form->addRow(tr("Valor bruto:"), m_edtValor);

    // ── Data ───────────────────────────────────────────────────────────────
    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateEdit->setCalendarPopup(true);
    const QDate defaultDate = m_original.dia > 0
        ? QDate(m_ano, m_mes, m_original.dia)
        : QDate(m_ano, m_mes, 1);
    m_dateEdit->setDate(defaultDate);
    form->addRow(tr("Data:"), m_dateEdit);

    root->addLayout(form);

    // ── Impostos (colapsável) ──────────────────────────────────────────────
    m_grpImpostos = new QGroupBox(tr("Impostos retidos (opcional)"), this);
    m_grpImpostos->setCheckable(true);
    m_grpImpostos->setChecked(m_original.irpf > 0 || m_original.inss > 0);

    auto *layImp = new QFormLayout(m_grpImpostos);
    layImp->setSpacing(6);

    m_edtIrpf = new QLineEdit(m_grpImpostos);
    m_edtIrpf->setPlaceholderText(QStringLiteral("0,00"));
    if (m_original.irpf > 0)
        m_edtIrpf->setText(formatCentavos(m_original.irpf));
    connect(m_edtIrpf, &QLineEdit::textChanged, this, &LancamentoEntradaDialog::onValoresChanged);
    layImp->addRow(tr("IRPF retido:"), m_edtIrpf);

    m_edtInss = new QLineEdit(m_grpImpostos);
    m_edtInss->setPlaceholderText(QStringLiteral("0,00"));
    if (m_original.inss > 0)
        m_edtInss->setText(formatCentavos(m_original.inss));
    connect(m_edtInss, &QLineEdit::textChanged, this, &LancamentoEntradaDialog::onValoresChanged);
    layImp->addRow(tr("INSS retido:"), m_edtInss);

    m_lblLiquido = new QLabel(QStringLiteral("R$ 0,00"), m_grpImpostos);
    m_lblLiquido->setObjectName(QStringLiteral("title"));
    layImp->addRow(tr("Valor líquido:"), m_lblLiquido);

    connect(m_grpImpostos, &QGroupBox::toggled, this, &LancamentoEntradaDialog::onValoresChanged);
    root->addWidget(m_grpImpostos);

    onValoresChanged(); // initialise líquido label

    // ── Recorrência ────────────────────────────────────────────────────────
    auto *grpRec = new QGroupBox(tr("Recorrência"), this);
    auto *layRec = new QVBoxLayout(grpRec);
    layRec->setSpacing(4);

    m_rbSomente    = new QRadioButton(tr("Somente este mês"), grpRec);
    m_rbRecorrente = new QRadioButton(tr("Recorrente por"), grpRec);
    m_rbSomente->setChecked(true);

    auto *rowMeses = new QHBoxLayout;
    m_spnMeses = new QSpinBox(grpRec);
    m_spnMeses->setRange(2, 120);
    m_spnMeses->setValue(12);
    m_spnMeses->setSuffix(tr(" meses"));
    m_spnMeses->setEnabled(false);
    rowMeses->addWidget(m_rbRecorrente);
    rowMeses->addWidget(m_spnMeses);
    rowMeses->addStretch();

    layRec->addWidget(m_rbSomente);
    layRec->addLayout(rowMeses);

    connect(m_rbRecorrente, &QRadioButton::toggled, this, &LancamentoEntradaDialog::onRecorrenciaToggled);
    root->addWidget(grpRec);

    // ── Observação ─────────────────────────────────────────────────────────
    auto *formObs = new QFormLayout;
    m_edtObs = new QLineEdit(this);
    m_edtObs->setPlaceholderText(tr("Observação (opcional)"));
    m_edtObs->setMaxLength(200);
    m_edtObs->setText(m_original.obs);
    formObs->addRow(tr("Obs:"), m_edtObs);
    root->addLayout(formObs);

    // ── Buttons ────────────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Save)->setText(tr("Salvar"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    root->addWidget(btns);

    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setMinimumWidth(360);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void LancamentoEntradaDialog::onValoresChanged()
{
    const bool impAtivo = m_grpImpostos->isChecked();
    const qint64 bruto  = parseCentavos(m_edtValor->text());
    const qint64 irpf   = impAtivo ? parseCentavos(m_edtIrpf->text()) : 0LL;
    const qint64 inss   = impAtivo ? parseCentavos(m_edtInss->text()) : 0LL;
    const qint64 liq    = bruto - irpf - inss;
    m_lblLiquido->setText(QStringLiteral("R$ %1").arg(formatCentavos(liq)));
}

void LancamentoEntradaDialog::onRecorrenciaToggled(bool recorrente)
{
    m_spnMeses->setEnabled(recorrente);
}

// ── Result ────────────────────────────────────────────────────────────────────

QList<LancamentoEntrada> LancamentoEntradaDialog::lancamentos() const
{
    const QString fonte = m_cbFonte->currentText().trimmed();
    const int fonteId   = m_cbFonte->currentData().toInt();
    const qint64 bruto  = parseCentavos(m_edtValor->text());
    const bool impAtivo = m_grpImpostos->isChecked();
    const qint64 irpf   = impAtivo ? parseCentavos(m_edtIrpf->text()) : 0LL;
    const qint64 inss   = impAtivo ? parseCentavos(m_edtInss->text()) : 0LL;
    const QString obs   = m_edtObs->text().trimmed();
    const QDate baseDate = m_dateEdit->date();

    const int nMeses = m_rbRecorrente->isChecked() ? m_spnMeses->value() : 1;

    QList<LancamentoEntrada> result;
    for (int i = 0; i < nMeses; ++i) {
        const QDate d = baseDate.addMonths(i);
        LancamentoEntrada l;
        l.id         = (i == 0 && m_original.id > 0) ? m_original.id : 0;
        l.mes        = d.month();
        l.ano        = d.year();
        l.dia        = d.day();
        l.fonte      = fonte;
        l.fonteId    = fonteId;
        l.valorBruto = bruto;
        l.irpf       = irpf;
        l.inss       = inss;
        l.obs        = obs;
        result.append(l);
    }
    return result;
}
