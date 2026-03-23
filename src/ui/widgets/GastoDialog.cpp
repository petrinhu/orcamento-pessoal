#include "GastoDialog.h"
#include "CalculadoraEdit.h"

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
#include <QDialogButtonBox>
#include <QPushButton>
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
    const qint64 abs_c = c < 0 ? -c : c;
    return QStringLiteral("%1%2,%3")
        .arg(c < 0 ? QStringLiteral("-") : QString())
        .arg(abs_c / 100)
        .arg(abs_c % 100, 2, 10, QLatin1Char('0'));
}

// ── Tipo entries (order matches QComboBox index) ───────────────────────────────

struct TipoEntry { TipoPagamento tipo; const char *label; };
static const TipoEntry kTipos[] = {
    { TipoPagamento::Pix,          "PIX"          },
    { TipoPagamento::Debito,       "Débito"        },
    { TipoPagamento::Dinheiro,     "Dinheiro"      },
    { TipoPagamento::Boleto,       "Boleto"        },
    { TipoPagamento::Transferencia,"Transferência" },
    { TipoPagamento::Fixo,         "Fixo"          },
    { TipoPagamento::Outro,        "Outro"         },
};

// ── Constructor ───────────────────────────────────────────────────────────────

GastoDialog::GastoDialog(const GastoAvulso &gasto,
                         int mes, int ano,
                         const QStringList &categorias,
                         QWidget *parent)
    : QDialog(parent)
    , m_original(gasto)
    , m_mes(mes)
    , m_ano(ano)
{
    setWindowTitle(gasto.id > 0 ? tr("Editar gasto") : tr("Novo gasto"));
    setModal(true);
    buildUi(categorias);
}

void GastoDialog::buildUi(const QStringList &categorias)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(8);

    // ── Descrição ──────────────────────────────────────────────────────────
    m_edtDescricao = new QLineEdit(this);
    m_edtDescricao->setPlaceholderText(tr("Ex: Aluguel, Mercado, Netflix"));
    m_edtDescricao->setMaxLength(200);
    m_edtDescricao->setText(m_original.descricao);
    form->addRow(tr("Descrição:"), m_edtDescricao);

    // ── Categoria ──────────────────────────────────────────────────────────
    m_cbCategoria = new QComboBox(this);
    m_cbCategoria->setEditable(true);
    m_cbCategoria->setInsertPolicy(QComboBox::NoInsert);
    m_cbCategoria->addItem(QString()); // empty = sem categoria
    for (const auto &c : categorias)
        m_cbCategoria->addItem(c);
    if (auto *comp = m_cbCategoria->completer()) {
        comp->setFilterMode(Qt::MatchContains);
        comp->setCaseSensitivity(Qt::CaseInsensitive);
    }
    if (!m_original.categoria.isEmpty())
        m_cbCategoria->setCurrentText(m_original.categoria);
    form->addRow(tr("Categoria:"), m_cbCategoria);

    // ── Tipo de pagamento ──────────────────────────────────────────────────
    m_cbTipo = new QComboBox(this);
    const TipoPagamento curTipo = tipoPgtoDeTexto(m_original.tipoPgto);
    int defaultIdx = 0;
    for (int i = 0; i < static_cast<int>(std::size(kTipos)); ++i) {
        m_cbTipo->addItem(tr(kTipos[i].label),
                          tipoPgtoParaTexto(kTipos[i].tipo));
        if (kTipos[i].tipo == curTipo)
            defaultIdx = i;
    }
    m_cbTipo->setCurrentIndex(defaultIdx);
    form->addRow(tr("Tipo:"), m_cbTipo);

    // ── Valor ──────────────────────────────────────────────────────────────
    m_edtValor = new CalculadoraEdit(this);
    m_edtValor->setPlaceholderText(QStringLiteral("0,00"));
    if (m_original.valor > 0)
        m_edtValor->setText(formatCentavos(m_original.valor));
    form->addRow(tr("Valor:"), m_edtValor);

    // ── Data ───────────────────────────────────────────────────────────────
    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setDisplayFormat(QStringLiteral("dd/MM/yyyy"));
    m_dateEdit->setCalendarPopup(true);
    const QDate defaultDate = (m_original.dia > 0)
        ? QDate(m_ano, m_mes, m_original.dia)
        : QDate(m_ano, m_mes, 1);
    m_dateEdit->setDate(defaultDate);
    form->addRow(tr("Data:"), m_dateEdit);

    // ── Obs ────────────────────────────────────────────────────────────────
    m_edtObs = new QLineEdit(this);
    m_edtObs->setPlaceholderText(tr("Observação (opcional)"));
    m_edtObs->setMaxLength(200);
    m_edtObs->setText(m_original.obs);
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    // ── Recorrência ────────────────────────────────────────────────────────
    auto *grpRec = new QGroupBox(tr("Recorrência"), this);
    auto *layRec = new QVBoxLayout(grpRec);
    layRec->setSpacing(4);

    m_rbSomente    = new QRadioButton(tr("Somente este mês"), grpRec);
    m_rbRecorrente = new QRadioButton(tr("Repetir por"), grpRec);
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

    connect(m_rbRecorrente, &QRadioButton::toggled,
            this, &GastoDialog::onRecorrenciaToggled);
    root->addWidget(grpRec);

    // ── Buttons ────────────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Save)->setText(tr("Salvar"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    setMinimumWidth(380);
    m_edtDescricao->setFocus();
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void GastoDialog::onRecorrenciaToggled(bool recorrente)
{
    m_spnMeses->setEnabled(recorrente);
}

// ── Result ────────────────────────────────────────────────────────────────────

QList<GastoAvulso> GastoDialog::gastos() const
{
    const QString descricao = m_edtDescricao->text().trimmed();
    const QString categoria = m_cbCategoria->currentText().trimmed();
    const QString tipoPgto  = m_cbTipo->currentData().toString();
    const qint64  valor     = parseCentavos(m_edtValor->text());
    const QString obs       = m_edtObs->text().trimmed();
    const QDate   baseDate  = m_dateEdit->date();
    const int     nMeses    = m_rbRecorrente->isChecked() ? m_spnMeses->value() : 1;

    QList<GastoAvulso> result;
    for (int i = 0; i < nMeses; ++i) {
        const QDate d = baseDate.addMonths(i);
        GastoAvulso g;
        g.id        = (i == 0 && m_original.id > 0) ? m_original.id : 0;
        g.mes       = d.month();
        g.ano       = d.year();
        g.dia       = d.day();
        g.descricao = descricao;
        g.categoria = categoria;
        g.tipoPgto  = tipoPgto;
        g.valor     = valor;
        g.obs       = obs;
        result.append(g);
    }
    return result;
}
