#include "ConfigCategoriasWidget.h"
#include "CategoriaDialog.h"
#include "repositories/CategoriaRepository.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QMessageBox>

// Colunas da tree
enum Col { ColNome = 0, ColTipo = 1, ColDespesa = 2, ColCount = 3 };

static const int RoleId      = Qt::UserRole;
static const int RolePaiId   = Qt::UserRole + 1;
static const int RoleTipo    = Qt::UserRole + 2;
static const int RoleDespesa = Qt::UserRole + 3; // #121: tipoDespesa (0|1)

// ── Construção ────────────────────────────────────────────────────────────────

ConfigCategoriasWidget::ConfigCategoriasWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void ConfigCategoriasWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    // Margem inferior ≥ extensão da sombra dos botões (#176): blur=8 + offset=1
    // → ~9px abaixo do botão. 12px garante folga sem clip.
    root->setContentsMargins(0, 0, 0, 12);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(ColCount);
    m_tree->setHeaderLabels({ tr("Nome"), tr("Tipo"), tr("Despesa") });
    m_tree->header()->setSectionResizeMode(ColNome, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(ColTipo, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(ColDespesa, QHeaderView::ResizeToContents);
    m_tree->setRootIsDecorated(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setAlternatingRowColors(true);
    root->addWidget(m_tree);

    auto *barBtns = new QHBoxLayout;
    m_btnAdd  = new QPushButton(tr("Adicionar"), this);
    m_btnEdit = new QPushButton(tr("Editar"),    this);
    m_btnRem  = new QPushButton(tr("Remover"),   this);
    m_btnEdit->setEnabled(false);
    m_btnRem->setEnabled(false);
    barBtns->addWidget(m_btnAdd);
    barBtns->addWidget(m_btnEdit);
    barBtns->addWidget(m_btnRem);
    barBtns->addStretch();
    root->addLayout(barBtns);

    connect(m_btnAdd,  &QPushButton::clicked,
            this, &ConfigCategoriasWidget::onAdicionar);
    connect(m_btnEdit, &QPushButton::clicked,
            this, &ConfigCategoriasWidget::onEditar);
    connect(m_btnRem,  &QPushButton::clicked,
            this, &ConfigCategoriasWidget::onRemover);
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            this, &ConfigCategoriasWidget::onItemDoubleClicked);
    connect(m_tree, &QTreeWidget::itemSelectionChanged,
            this, &ConfigCategoriasWidget::onSelectionChanged);
}

// ── Dados ─────────────────────────────────────────────────────────────────────

static QString tipoDisplay(const QString &t)
{
    if (t == QStringLiteral("gasto"))        return QObject::tr("Gasto");
    if (t == QStringLiteral("entrada"))      return QObject::tr("Entrada");
    if (t == QStringLiteral("investimento")) return QObject::tr("Investimento");
    return t;
}

void ConfigCategoriasWidget::recarregar()
{
    m_tree->clear();

    const auto cats = CategoriaRepository().findAll();

    // Mapear id → QTreeWidgetItem para montar hierarquia
    QHash<int, QTreeWidgetItem *> itemMap;

    // Primeiro passo: criar raízes
    for (const auto &c : cats) {
        if (c.paiId != 0) continue;
        auto *item = new QTreeWidgetItem(m_tree);
        item->setText(ColNome, c.nome);
        item->setText(ColTipo, tipoDisplay(c.tipo));
        if (c.tipo == QStringLiteral("gasto"))
            item->setText(ColDespesa, tipoDespesaLabel(c.tipoDespesa));
        item->setData(ColNome, RoleId,      c.id);
        item->setData(ColNome, RolePaiId,   c.paiId);
        item->setData(ColNome, RoleTipo,    c.tipo);
        item->setData(ColNome, RoleDespesa, tipoDespesaParaInt(c.tipoDespesa));
        QFont f = item->font(ColNome);
        f.setBold(true);
        item->setFont(ColNome, f);
        itemMap.insert(c.id, item);
    }

    // Segundo passo: criar filhos
    for (const auto &c : cats) {
        if (c.paiId == 0) continue;
        QTreeWidgetItem *pai = itemMap.value(c.paiId);
        QTreeWidgetItem *item = pai
            ? new QTreeWidgetItem(pai)
            : new QTreeWidgetItem(m_tree);
        item->setText(ColNome, c.nome);
        item->setText(ColTipo, tipoDisplay(c.tipo));
        if (c.tipo == QStringLiteral("gasto"))
            item->setText(ColDespesa, tipoDespesaLabel(c.tipoDespesa));
        item->setData(ColNome, RoleId,      c.id);
        item->setData(ColNome, RolePaiId,   c.paiId);
        item->setData(ColNome, RoleTipo,    c.tipo);
        item->setData(ColNome, RoleDespesa, tipoDespesaParaInt(c.tipoDespesa));
        itemMap.insert(c.id, item);
    }

    m_tree->expandAll();
    onSelectionChanged();
}

Categoria ConfigCategoriasWidget::categoriaFromItem(QTreeWidgetItem *item) const
{
    Categoria c;
    c.id          = item->data(ColNome, RoleId).toInt();
    c.paiId       = item->data(ColNome, RolePaiId).toInt();
    c.tipo        = item->data(ColNome, RoleTipo).toString();
    c.nome        = item->text(ColNome);
    c.tipoDespesa = tipoDespesaDeInt(item->data(ColNome, RoleDespesa).toInt());
    return c;
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void ConfigCategoriasWidget::onSelectionChanged()
{
    const bool sel = !m_tree->selectedItems().isEmpty();
    m_btnEdit->setEnabled(sel);
    m_btnRem->setEnabled(sel);
}

void ConfigCategoriasWidget::onAdicionar()
{
    // Tipo padrão: "gasto"; se há seleção, usa o tipo do item selecionado
    QString tipo = QStringLiteral("gasto");
    const auto sel = m_tree->selectedItems();
    if (!sel.isEmpty())
        tipo = sel.first()->data(ColNome, RoleTipo).toString();

    const auto pais = CategoriaRepository().findAll();
    auto *dlg = new CategoriaDialog(tipo, pais, this);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        const Categoria c = dlg->result();
        CategoriaRepository().insert(c);
        recarregar();
        emit categoriasAlteradas();
    });
    dlg->open();
}

void ConfigCategoriasWidget::onEditar()
{
    const auto sel = m_tree->selectedItems();
    if (sel.isEmpty()) return;

    const Categoria cat = categoriaFromItem(sel.first());
    const auto pais = CategoriaRepository().findAll();
    auto *dlg = new CategoriaDialog(cat, pais, this);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        CategoriaRepository().update(dlg->result());
        recarregar();
        emit categoriasAlteradas();
    });
    dlg->open();
}

void ConfigCategoriasWidget::onRemover()
{
    const auto sel = m_tree->selectedItems();
    if (sel.isEmpty()) return;

    const Categoria cat = categoriaFromItem(sel.first());

    // Verificar se tem filhos
    if (sel.first()->childCount() > 0) {
        QMessageBox::warning(this, tr("Remover categoria"),
            tr("Remova primeiro as subcategorias de \"%1\".").arg(cat.nome));
        return;
    }

    const auto btn = QMessageBox::question(this,
        tr("Remover categoria"),
        tr("Remover \"%1\"?\n\nLançamentos existentes perderão esta categoria.").arg(cat.nome),
        QMessageBox::Yes | QMessageBox::No);
    if (btn != QMessageBox::Yes) return;

    CategoriaRepository().remove(cat.id);
    recarregar();
    emit categoriasAlteradas();
}

void ConfigCategoriasWidget::onItemDoubleClicked(QTreeWidgetItem *item, int /*col*/)
{
    if (!item) return;
    const Categoria cat = categoriaFromItem(item);
    const auto pais = CategoriaRepository().findAll();
    auto *dlg = new CategoriaDialog(cat, pais, this);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        CategoriaRepository().update(dlg->result());
        recarregar();
        emit categoriasAlteradas();
    });
    dlg->open();
}
