#include "GastoDialog.h"
#include "CalculadoraEdit.h"
#include "PagadorDialog.h"
#include "ReceiptAttachmentWidget.h"
#include "repositories/PagadorRepository.h"
#include "ui/UiFormatters.h"

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
#include <QStandardItemModel>
#include <QStandardItem>
#include <algorithm>

// ── Helpers ───────────────────────────────────────────────────────────────────

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

// ── Grouped category model helper ─────────────────────────────────────────────

static QStandardItemModel *buildGroupedCatModel(const QList<Categoria> &cats,
                                                 QObject *parent)
{
    auto *model = new QStandardItemModel(parent);

    // Empty / no-category option
    auto *noneItem = new QStandardItem(QObject::tr("— sem categoria —"));
    noneItem->setData(0,    Qt::UserRole);
    noneItem->setData(QString(), Qt::UserRole + 1);
    model->appendRow(noneItem);

    // Separate roots from children
    QList<Categoria> roots, children;
    for (const auto &c : cats) {
        if (c.paiId == 0) roots.append(c);
        else              children.append(c);
    }

    for (const auto &root : roots) {
        // Does root have children?
        bool hasChild = false;
        for (const auto &ch : children)
            if (ch.paiId == root.id) { hasChild = true; break; }

        if (hasChild) {
            // Root = non-selectable group header
            auto *header = new QStandardItem(root.nome);
            QFont f = header->font();
            f.setBold(true);
            header->setFont(f);
            header->setFlags(Qt::ItemIsEnabled);  // not selectable
            header->setData(-root.id, Qt::UserRole); // negative id = header
            header->setData(QString(),         Qt::UserRole + 1);
            model->appendRow(header);

            for (const auto &ch : children) {
                if (ch.paiId != root.id) continue;
                auto *child = new QStandardItem(
                    QStringLiteral("  \u2514\u2500 ") + ch.nome);
                child->setData(ch.id,   Qt::UserRole);
                child->setData(ch.nome, Qt::UserRole + 1);
                model->appendRow(child);
            }
        } else {
            // Root with no children = selectable
            auto *item = new QStandardItem(root.nome);
            QFont f = item->font(); f.setBold(true); item->setFont(f);
            item->setData(root.id,   Qt::UserRole);
            item->setData(root.nome, Qt::UserRole + 1);
            model->appendRow(item);
        }
    }
    return model;
}

// ── Constructor ───────────────────────────────────────────────────────────────

GastoDialog::GastoDialog(const GastoAvulso &gasto,
                         int mes, int ano,
                         const QList<Categoria> &categorias,
                         const QList<Pagador> &pagadores,
                         QWidget *parent)
    : QDialog(parent)
    , m_original(gasto)
    , m_mes(mes)
    , m_ano(ano)
    , m_pagadores(pagadores)
{
    setWindowTitle(gasto.id > 0 ? tr("Editar gasto") : tr("Novo gasto"));
    buildUi(categorias);
}

void GastoDialog::buildUi(const QList<Categoria> &categorias)
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
    m_cbCategoria->setModel(buildGroupedCatModel(categorias, this));

    // Pre-select: by id first, then by name snapshot
    int catSelectIdx = 0; // default: sem categoria
    if (m_original.categoriaId > 0) {
        for (int i = 0; i < m_cbCategoria->model()->rowCount(); ++i) {
            if (m_cbCategoria->model()->index(i, 0)
                    .data(Qt::UserRole).toInt() == m_original.categoriaId) {
                catSelectIdx = i; break;
            }
        }
    } else if (!m_original.categoria.isEmpty()) {
        for (int i = 1; i < m_cbCategoria->model()->rowCount(); ++i) {
            if (m_cbCategoria->model()->index(i, 0)
                    .data(Qt::UserRole + 1).toString()
                    .compare(m_original.categoria, Qt::CaseInsensitive) == 0) {
                catSelectIdx = i; break;
            }
        }
    }
    m_cbCategoria->setCurrentIndex(catSelectIdx);
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
    form->addRow(tr("Tipo de pagamento:"), m_cbTipo);

    // ── Valor ──────────────────────────────────────────────────────────────
    m_edtValor = new CalculadoraEdit(this);
    m_edtValor->setPlaceholderText(QStringLiteral("0,00"));
    if (m_original.valor > 0)
        m_edtValor->setText(uifmt::formatCentavos(m_original.valor));
    form->addRow(tr("Valor:"), m_edtValor);

    // ── Data ───────────────────────────────────────────────────────────────
    m_dateEdit = new QDateEdit(this);
    m_dateEdit->setDisplayFormat(uifmt::dataDisplayFormat());
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

    // ── Comprovante (#90) ───────────────────────────────────────────────────
    m_receipt = new ReceiptAttachmentWidget(this);
    m_receipt->setReceipt(m_original.receipt);
    form->addRow(tr("Comprovante:"), m_receipt);

    // ── Beneficiário ───────────────────────────────────────────────────────
    m_cbBeneficiario = new QComboBox(this);
    m_cbBeneficiario->setEditable(true);
    m_cbBeneficiario->setInsertPolicy(QComboBox::NoInsert);
    m_cbBeneficiario->lineEdit()->setPlaceholderText(
        tr("Ex: Supermercado Extra, Netflix… (opcional)"));
    popularBeneficiarios();
    if (!m_original.pagador.isEmpty())
        m_cbBeneficiario->setCurrentText(m_original.pagador);
    if (auto *comp = m_cbBeneficiario->completer()) {
        comp->setFilterMode(Qt::MatchContains);
        comp->setCaseSensitivity(Qt::CaseInsensitive);
    }

    auto *rowBenef = new QHBoxLayout;
    rowBenef->setSpacing(4);
    rowBenef->addWidget(m_cbBeneficiario);
    auto *btnNovoBenef = new QPushButton(QStringLiteral("+"), this);
    btnNovoBenef->setFixedWidth(28);
    btnNovoBenef->setToolTip(tr("Criar novo beneficiário"));
    rowBenef->addWidget(btnNovoBenef);
    form->addRow(tr("Beneficiário:"), rowBenef);

    connect(btnNovoBenef, &QPushButton::clicked,
            this, &GastoDialog::onCriarBeneficiario);

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
    // Margem interna garante folga para a sombra dos botões (#176).
    auto *btns = new QDialogButtonBox(
        QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    btns->button(QDialogButtonBox::Save)->setText(tr("Salvar"));
    btns->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    btns->setContentsMargins(0, 6, 10, 10);
    connect(btns, &QDialogButtonBox::accepted, this, &GastoDialog::onAccept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);

    setMinimumWidth(380);
    m_edtDescricao->setFocus();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

void GastoDialog::popularBeneficiarios()
{
    m_cbBeneficiario->clear();
    m_cbBeneficiario->addItem(QString()); // vazio = sem beneficiário
    for (const auto &p : m_pagadores)
        m_cbBeneficiario->addItem(p.nome);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void GastoDialog::onRecorrenciaToggled(bool recorrente)
{
    m_spnMeses->setEnabled(recorrente);
}

void GastoDialog::onCriarBeneficiario()
{
    Pagador vazio;
    // Pre-fill nome with what the user typed (if any)
    vazio.nome = m_cbBeneficiario->currentText().trimmed();

    auto *dlg = new PagadorDialog(vazio, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        Pagador p = dlg->resultado();
        PagadorRepository repo;
        if (!repo.insert(p)) {
            // Might already exist — find it
            auto all = repo.findAll();
            auto it = std::find_if(all.cbegin(), all.cend(),
                [&](const Pagador &x){
                    return x.nome.compare(p.nome, Qt::CaseInsensitive) == 0;
                });
            if (it != all.cend()) p = *it;
        }
        m_pagadores.append(p);
        popularBeneficiarios();
        m_cbBeneficiario->setCurrentText(p.nome);
    });
    dlg->open();
}

void GastoDialog::onAccept()
{
    // Resolve categoria
    const int catIdx = m_cbCategoria->currentIndex();
    const QModelIndex mi = m_cbCategoria->model()->index(catIdx, 0);
    m_resolvedCategoriaId   = mi.data(Qt::UserRole).toInt();
    m_resolvedCategoriaNome = mi.data(Qt::UserRole + 1).toString();
    // negative id = group header (should not be selected), treat as none
    if (m_resolvedCategoriaId < 0) {
        m_resolvedCategoriaId   = 0;
        m_resolvedCategoriaNome.clear();
    }

    // Resolve pagador before accepting
    const QString nomePag = m_cbBeneficiario->currentText().trimmed();
    m_resolvedPagadorNome = nomePag;
    m_resolvedPagadorId   = 0;

    if (!nomePag.isEmpty()) {
        auto it = std::find_if(m_pagadores.cbegin(), m_pagadores.cend(),
            [&](const Pagador &p){
                return p.nome.compare(nomePag, Qt::CaseInsensitive) == 0;
            });
        if (it != m_pagadores.cend()) {
            m_resolvedPagadorId = it->id;
        } else {
            // New name typed directly — find or create
            m_resolvedPagadorId = PagadorRepository().findOrCreate(nomePag);
        }
    }

    accept();
}

// ── Result ────────────────────────────────────────────────────────────────────

QList<GastoAvulso> GastoDialog::gastos() const
{
    const QString descricao = m_edtDescricao->text().trimmed();
    const QString tipoPgto  = m_cbTipo->currentData().toString();
    const qint64  valor     = uifmt::parseCentavos(m_edtValor->text());
    const QString obs       = m_edtObs->text().trimmed();
    const QDate   baseDate  = m_dateEdit->date();
    const int     nMeses    = m_rbRecorrente->isChecked() ? m_spnMeses->value() : 1;

    QList<GastoAvulso> result;
    for (int i = 0; i < nMeses; ++i) {
        const QDate d = baseDate.addMonths(i);
        GastoAvulso g;
        g.id          = (i == 0 && m_original.id > 0) ? m_original.id : 0;
        g.mes         = d.month();
        g.ano         = d.year();
        g.dia         = d.day();
        g.descricao   = descricao;
        g.categoria   = m_resolvedCategoriaNome;
        g.categoriaId = m_resolvedCategoriaId;
        g.tipoPgto    = tipoPgto;
        g.valor       = valor;
        g.obs         = obs;
        g.pagador     = m_resolvedPagadorNome;
        g.pagadorId   = m_resolvedPagadorId;
        if (i == 0) g.receipt = m_receipt->receipt();
        result.append(g);
    }
    return result;
}
