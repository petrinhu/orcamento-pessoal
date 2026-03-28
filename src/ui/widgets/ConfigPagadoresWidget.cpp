#include "ConfigPagadoresWidget.h"
#include "PagadorDialog.h"
#include "repositories/PagadorRepository.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

// ── Construction ──────────────────────────────────────────────────────────────

ConfigPagadoresWidget::ConfigPagadoresWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void ConfigPagadoresWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    auto *subtitle = new QLabel(
        tr("Cadastre os beneficiários e pagadores que aparecem nos lançamentos de gastos."), this);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setWordWrap(true);
    root->addWidget(subtitle);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto *toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    auto *btnAdd = new QPushButton(tr("+ Adicionar"), this);
    btnAdd->setObjectName(QStringLiteral("primary"));
    m_btnEdit = new QPushButton(tr("Editar"), this);
    m_btnRem  = new QPushButton(tr("Remover"), this);
    m_btnEdit->setEnabled(false);
    m_btnRem->setEnabled(false);

    toolbar->addWidget(btnAdd);
    toolbar->addWidget(m_btnEdit);
    toolbar->addWidget(m_btnRem);
    toolbar->addStretch();
    root->addLayout(toolbar);

    // ── Table ──────────────────────────────────────────────────────────────
    m_table = new QTableWidget(0, 2, this);
    m_table->setHorizontalHeaderLabels({tr("Nome"), tr("Tipo")});
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setMaximumHeight(220);
    root->addWidget(m_table);

    // ── Connections ────────────────────────────────────────────────────────
    connect(btnAdd,    &QPushButton::clicked, this, &ConfigPagadoresWidget::onAdicionar);
    connect(m_btnEdit, &QPushButton::clicked, this, &ConfigPagadoresWidget::onEditar);
    connect(m_btnRem,  &QPushButton::clicked, this, &ConfigPagadoresWidget::onRemover);

    connect(m_table, &QTableWidget::itemDoubleClicked,
            this, &ConfigPagadoresWidget::onEditar);

    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                const bool sel = m_table->currentRow() >= 0;
                m_btnEdit->setEnabled(sel);
                m_btnRem->setEnabled(sel);
            });
}

// ── Data ──────────────────────────────────────────────────────────────────────

void ConfigPagadoresWidget::recarregar()
{
    const auto pagadores = PagadorRepository().findAll();

    m_table->setRowCount(0);
    for (const auto &p : pagadores) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setRowHeight(row, 26);

        auto *itNome = new QTableWidgetItem(p.nome);
        itNome->setFlags(itNome->flags() & ~Qt::ItemIsEditable);
        itNome->setData(Qt::UserRole, p.id);
        m_table->setItem(row, 0, itNome);

        auto *itTipo = new QTableWidgetItem(tipoPagadorLabel(p.tipo));
        itTipo->setFlags(itTipo->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 1, itTipo);
    }
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void ConfigPagadoresWidget::onAdicionar()
{
    Pagador vazio;
    auto *dlg = new PagadorDialog(vazio, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        Pagador p = dlg->resultado();
        if (!PagadorRepository().insert(p)) {
            QMessageBox::warning(this, tr("Erro"),
                tr("Não foi possível salvar. Verifique se o nome já existe."));
            return;
        }
        recarregar();
        emit pagadoresAlterados();
    });
    dlg->open();
}

void ConfigPagadoresWidget::onEditar()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    const int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();

    auto opt = PagadorRepository().findById(id);
    if (!opt) return;

    auto *dlg = new PagadorDialog(*opt, this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    connect(dlg, &QDialog::accepted, this, [this, dlg]() {
        Pagador p = dlg->resultado();
        if (!PagadorRepository().update(p)) {
            QMessageBox::warning(this, tr("Erro"),
                tr("Não foi possível atualizar."));
            return;
        }
        recarregar();
        emit pagadoresAlterados();
    });
    dlg->open();
}

void ConfigPagadoresWidget::onRemover()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    const int id     = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    const QString nome = m_table->item(row, 0)->text();

    if (QMessageBox::question(this,
            tr("Remover beneficiário"),
            tr("Remover \"%1\"?\n"
               "Os lançamentos existentes mantêm o nome registrado.").arg(nome))
        != QMessageBox::Yes) return;

    PagadorRepository().remove(id);
    recarregar();
    emit pagadoresAlterados();
}
