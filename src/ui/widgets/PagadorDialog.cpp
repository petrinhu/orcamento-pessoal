#include "PagadorDialog.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QPushButton>

struct TipoPagadorEntry { TipoPagador tipo; const char *label; };
static const TipoPagadorEntry kTipos[] = {
    { TipoPagador::Ambos,     "Ambos"     },
    { TipoPagador::Recebedor, "Recebedor" },
    { TipoPagador::Pagador,   "Pagador"   },
};

// ── Constructor ───────────────────────────────────────────────────────────────

PagadorDialog::PagadorDialog(const Pagador &pagador, QWidget *parent)
    : QDialog(parent)
    , m_original(pagador)
{
    setWindowTitle(pagador.id > 0 ? tr("Editar beneficiário") : tr("Novo beneficiário"));
    buildUi();
}

void PagadorDialog::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(12);

    auto *form = new QFormLayout;
    form->setSpacing(8);

    // ── Nome ──────────────────────────────────────────────────────────────
    m_edtNome = new QLineEdit(this);
    m_edtNome->setPlaceholderText(tr("Ex: Supermercado Extra, Netflix…"));
    m_edtNome->setMaxLength(120);
    m_edtNome->setText(m_original.nome);
    form->addRow(tr("Nome:"), m_edtNome);

    // ── Tipo ──────────────────────────────────────────────────────────────
    m_cbTipo = new QComboBox(this);
    const TipoPagador curTipo = tipoPagadorDeTexto(m_original.tipo);
    int defaultIdx = 0;
    for (int i = 0; i < static_cast<int>(std::size(kTipos)); ++i) {
        m_cbTipo->addItem(tr(kTipos[i].label),
                          tipoPagadorParaTexto(kTipos[i].tipo));
        if (kTipos[i].tipo == curTipo)
            defaultIdx = i;
    }
    m_cbTipo->setCurrentIndex(defaultIdx);
    form->addRow(tr("Tipo:"), m_cbTipo);

    // ── Obs ───────────────────────────────────────────────────────────────
    m_edtObs = new QLineEdit(this);
    m_edtObs->setPlaceholderText(tr("Observação (opcional)"));
    m_edtObs->setMaxLength(200);
    m_edtObs->setText(m_original.obs);
    form->addRow(tr("Obs:"), m_edtObs);

    root->addLayout(form);

    // ── Buttons ───────────────────────────────────────────────────────────
    // Margem interna garante folga para a sombra dos botões (#176).
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Save)->setText(tr("Salvar"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    btns->setContentsMargins(0, 6, 10, 10);

    auto *saveBtn = btns->button(QDialogButtonBox::Save);
    saveBtn->setEnabled(!m_original.nome.isEmpty());
    connect(m_edtNome, &QLineEdit::textChanged, this, [saveBtn](const QString &t) {
        saveBtn->setEnabled(!t.trimmed().isEmpty());
    });

    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    setMinimumWidth(340);
    m_edtNome->setFocus();
}

// ── Result ────────────────────────────────────────────────────────────────────

Pagador PagadorDialog::resultado() const
{
    Pagador p;
    p.id   = m_original.id;
    p.nome = m_edtNome->text().trimmed();
    p.tipo = m_cbTipo->currentData().toString();
    p.obs  = m_edtObs->text().trimmed();
    return p;
}
