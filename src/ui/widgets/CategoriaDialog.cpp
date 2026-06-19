#include "CategoriaDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>

// ── Construção ────────────────────────────────────────────────────────────────

CategoriaDialog::CategoriaDialog(const QString &tipo,
                                 const QList<Categoria> &pais,
                                 QWidget *parent)
    : QDialog(parent)
{
    build(tipo, pais);
    setWindowTitle(tr("Nova categoria"));
}

CategoriaDialog::CategoriaDialog(const Categoria &cat,
                                 const QList<Categoria> &pais,
                                 QWidget *parent)
    : QDialog(parent), m_editId(cat.id)
{
    build(cat.tipo, pais);
    setWindowTitle(tr("Editar categoria"));

    m_edtNome->setText(cat.nome);

    // Selecionar tipo
    const int tipoIdx = m_cbTipo->findData(cat.tipo);
    if (tipoIdx >= 0) m_cbTipo->setCurrentIndex(tipoIdx);

    // Selecionar classificação de despesa (#121)
    const int despIdx = m_cbDespesa->findData(tipoDespesaParaInt(cat.tipoDespesa));
    if (despIdx >= 0) m_cbDespesa->setCurrentIndex(despIdx);

    // Selecionar pai
    for (int i = 0; i < m_cbPai->count(); ++i) {
        if (m_cbPai->itemData(i).toInt() == cat.paiId) {
            m_cbPai->setCurrentIndex(i);
            break;
        }
    }
}

void CategoriaDialog::build(const QString &tipo, const QList<Categoria> &pais)
{
    setMinimumWidth(320);

    auto *lay = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    lay->addLayout(form);

    m_edtNome = new QLineEdit(this);
    form->addRow(tr("Nome:"), m_edtNome);

    m_cbTipo = new QComboBox(this);
    m_cbTipo->addItem(tr("Gasto"),        QStringLiteral("gasto"));
    m_cbTipo->addItem(tr("Entrada"),      QStringLiteral("entrada"));
    m_cbTipo->addItem(tr("Investimento"), QStringLiteral("investimento"));
    const int idx = m_cbTipo->findData(tipo);
    if (idx >= 0) m_cbTipo->setCurrentIndex(idx);
    form->addRow(tr("Tipo:"), m_cbTipo);

    m_cbPai = new QComboBox(this);
    form->addRow(tr("Categoria pai:"), m_cbPai);

    // #121: classificação fixo×variável (relevante apenas para gastos).
    // Default = Variável (índice 0), coerente com o schema (#118).
    m_cbDespesa = new QComboBox(this);
    m_cbDespesa->addItem(tipoDespesaLabel(TipoDespesa::Variavel),
                         tipoDespesaParaInt(TipoDespesa::Variavel));
    m_cbDespesa->addItem(tipoDespesaLabel(TipoDespesa::Fixo),
                         tipoDespesaParaInt(TipoDespesa::Fixo));
    m_cbDespesa->setToolTip(
        tr("Despesa fixa = recorrente/previsível (aluguel, assinatura).\n"
           "Variável = eventual/discricionária (lazer, mercado avulso)."));
    form->addRow(tr("Tipo de despesa:"), m_cbDespesa);

    populatePais(pais, tipo);
    atualizarVisibilidadeDespesa();

    connect(m_cbTipo, &QComboBox::currentIndexChanged, this, [this, pais](int) {
        populatePais(pais, m_cbTipo->currentData().toString());
        atualizarVisibilidadeDespesa();
    });

    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    lay->addWidget(btns);

    auto *btnSave = btns->button(QDialogButtonBox::Save);
    btnSave->setEnabled(!m_edtNome->text().trimmed().isEmpty());
    connect(m_edtNome, &QLineEdit::textChanged, this, [btnSave](const QString &t) {
        btnSave->setEnabled(!t.trimmed().isEmpty());
    });

    connect(btns, &QDialogButtonBox::accepted, this, &CategoriaDialog::onAccept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void CategoriaDialog::populatePais(const QList<Categoria> &pais, const QString &tipo)
{
    const int prevPaiId = m_cbPai->currentData().toInt();
    m_cbPai->clear();
    m_cbPai->addItem(tr("(sem pai — categoria raiz)"), 0);

    for (const auto &p : pais) {
        if (p.tipo != tipo) continue;
        if (p.paiId != 0)   continue; // só raízes como pai possível
        if (p.id == m_editId) continue; // não pode ser pai de si mesmo
        m_cbPai->addItem(p.nome, p.id);
    }

    // Restaurar seleção anterior se ainda disponível
    for (int i = 0; i < m_cbPai->count(); ++i) {
        if (m_cbPai->itemData(i).toInt() == prevPaiId) {
            m_cbPai->setCurrentIndex(i);
            break;
        }
    }
}

void CategoriaDialog::atualizarVisibilidadeDespesa()
{
    // Fixo×Variável só faz sentido para gastos; oculta para entrada/investimento.
    const bool ehGasto = m_cbTipo->currentData().toString() == QStringLiteral("gasto");
    if (auto *form = qobject_cast<QFormLayout *>(m_cbDespesa->parentWidget()->layout()))
        form->setRowVisible(m_cbDespesa, ehGasto);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void CategoriaDialog::onAccept()
{
    m_result.id          = m_editId;
    m_result.nome        = m_edtNome->text().trimmed();
    m_result.tipo        = m_cbTipo->currentData().toString();
    m_result.paiId       = m_cbPai->currentData().toInt();
    // Classificação só é relevante para gastos; demais ficam em Variável (default).
    m_result.tipoDespesa = (m_result.tipo == QStringLiteral("gasto"))
        ? tipoDespesaDeInt(m_cbDespesa->currentData().toInt())
        : TipoDespesa::Variavel;
    accept();
}

Categoria CategoriaDialog::result() const
{
    return m_result;
}
