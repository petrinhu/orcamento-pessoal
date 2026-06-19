#include "ItemOrcamentoDialog.h"
#include "CalculadoraEdit.h"
#include "ui/UiFormatters.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCompleter>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>

// ── Constructor ───────────────────────────────────────────────────────────────

ItemOrcamentoDialog::ItemOrcamentoDialog(const ItemOrcamento &item,
                                         const QList<Categoria> &categorias,
                                         const QList<FonteRenda> &fontes,
                                         QWidget *parent)
    : QDialog(parent)
    , m_original(item)
    , m_categorias(categorias)
    , m_fontes(fontes)
{
    setWindowTitle(item.id > 0 ? tr("Editar item do orçamento") : tr("Novo item do orçamento"));
    buildUi();
}

void ItemOrcamentoDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(8);

    // ── Nome ───────────────────────────────────────────────────────────────
    m_edtNome = new QLineEdit(this);
    m_edtNome->setPlaceholderText(tr("Ex: Salário, Alimentação, Transporte"));
    m_edtNome->setMaxLength(100);
    m_edtNome->setText(m_original.nome);
    form->addRow(tr("Nome:"), m_edtNome);

    // ── Tipo ───────────────────────────────────────────────────────────────
    m_cbTipo = new QComboBox(this);
    m_cbTipo->addItem(tr("Receita"),  QStringLiteral("receita"));
    m_cbTipo->addItem(tr("Despesa"),  QStringLiteral("despesa"));
    if (m_original.tipo == QStringLiteral("despesa"))
        m_cbTipo->setCurrentIndex(1);
    form->addRow(tr("Tipo:"), m_cbTipo);

    // ── Valor mensal planejado ──────────────────────────────────────────────
    m_edtValor = new CalculadoraEdit(this);
    m_edtValor->setPlaceholderText(QStringLiteral("0,00"));
    if (m_original.valorMensal > 0)
        m_edtValor->setText(uifmt::formatCentavos(m_original.valorMensal));
    form->addRow(tr("Planejado/mês:"), m_edtValor);

    // ── Vínculo real (categoria/fonte) ─────────────────────────────────────
    m_cbCatRef = new QComboBox(this);
    m_cbCatRef->setEditable(true);
    m_cbCatRef->setInsertPolicy(QComboBox::NoInsert);
    if (auto *comp = m_cbCatRef->completer()) {
        comp->setFilterMode(Qt::MatchContains);
        comp->setCaseSensitivity(Qt::CaseInsensitive);
    }
    form->addRow(tr("Vínculo real:"), m_cbCatRef);

    m_lblCatHint = new QLabel(this);
    m_lblCatHint->setObjectName(QStringLiteral("subtitle"));
    m_lblCatHint->setWordWrap(true);

    connect(m_cbTipo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ItemOrcamentoDialog::onTipoChanged);
    onTipoChanged(m_cbTipo->currentIndex()); // initialise combo + hint

    // Pre-fill with existing value
    if (!m_original.categoriaRef.isEmpty())
        m_cbCatRef->setCurrentText(m_original.categoriaRef);

    root->addLayout(form);
    root->addWidget(m_lblCatHint);

    // ── Buttons ────────────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Save)->setText(tr("Salvar"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    setMinimumWidth(380);
    m_edtNome->setFocus();
}

void ItemOrcamentoDialog::onTipoChanged(int /*index*/)
{
    repopulateCatRef();
}

void ItemOrcamentoDialog::repopulateCatRef()
{
    const bool isDespesa =
        m_cbTipo->currentData().toString() == QStringLiteral("despesa");

    const QString prev = m_cbCatRef->currentText();
    m_cbCatRef->clear();
    m_cbCatRef->addItem(QStringLiteral("*")); // wildcard = tudo

    if (isDespesa) {
        for (const auto &c : m_categorias) {
            if (c.tipo == QStringLiteral("gasto"))
                m_cbCatRef->addItem(c.paiId == 0
                    ? c.nome
                    : c.paiNome + QStringLiteral(" > ") + c.nome);
        }
        m_lblCatHint->setText(
            tr("Categoria de Gastos a somar. Use * para todos os gastos."));
        m_cbCatRef->lineEdit()->setPlaceholderText(
            tr("Categoria (ex: Alimentação) ou * para tudo"));
    } else {
        for (const auto &f : m_fontes)
            m_cbCatRef->addItem(f.nome);
        m_lblCatHint->setText(
            tr("Fonte de Entradas a somar. Vazio ou * para todas as entradas."));
        m_cbCatRef->lineEdit()->setPlaceholderText(
            tr("Fonte (ex: Salário) ou vazio para tudo"));
    }

    // Restore previous value
    if (!prev.isEmpty())
        m_cbCatRef->setCurrentText(prev);
}

ItemOrcamento ItemOrcamentoDialog::result() const
{
    ItemOrcamento item;
    item.id           = m_original.id;
    item.nome         = m_edtNome->text().trimmed();
    item.tipo         = m_cbTipo->currentData().toString();
    item.valorMensal  = uifmt::parseCentavos(m_edtValor->text());
    item.categoriaRef = m_cbCatRef->currentText().trimmed();
    return item;
}
