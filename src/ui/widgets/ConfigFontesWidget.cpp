#include "ConfigFontesWidget.h"
#include "repositories/FonteRendaRepository.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

ConfigFontesWidget::ConfigFontesWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void ConfigFontesWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    auto *subtitle = new QLabel(
        tr("Cadastre as origens de renda que aparecem no dropdown da aba Entradas."), this);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    subtitle->setWordWrap(true);
    root->addWidget(subtitle);

    // Add row
    auto *addRow = new QHBoxLayout;
    m_edtNome = new QLineEdit(this);
    m_edtNome->setPlaceholderText(tr("Ex: Amil, GAMA, Freelance…"));
    m_edtNome->setMaxLength(80);
    m_btnAdd = new QPushButton(tr("+ Adicionar"), this);
    m_btnAdd->setObjectName(QStringLiteral("primary"));
    m_btnAdd->setEnabled(false);
    addRow->addWidget(m_edtNome);
    addRow->addWidget(m_btnAdd);
    root->addLayout(addRow);

    // Table
    m_table = new QTableWidget(0, 1, this);
    m_table->setHorizontalHeaderLabels({tr("Fonte de renda")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setMaximumHeight(180);
    root->addWidget(m_table);

    // Remove button
    m_btnRem = new QPushButton(tr("Remover selecionada"), this);
    m_btnRem->setEnabled(false);
    root->addWidget(m_btnRem, 0, Qt::AlignLeft);

    connect(m_edtNome, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_btnAdd->setEnabled(!t.trimmed().isEmpty());
    });
    connect(m_btnAdd, &QPushButton::clicked, this, &ConfigFontesWidget::onAdicionar);
    connect(m_btnRem, &QPushButton::clicked, this, &ConfigFontesWidget::onRemover);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                m_btnRem->setEnabled(!m_table->selectedItems().isEmpty());
            });
}

void ConfigFontesWidget::recarregar()
{
    FonteRendaRepository repo;
    const auto fontes = repo.findAll();

    m_table->setRowCount(0);
    for (const auto &f : fontes) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *item = new QTableWidgetItem(f.nome);
        item->setData(Qt::UserRole, f.id);
        m_table->setItem(row, 0, item);
    }
}

void ConfigFontesWidget::onAdicionar()
{
    const QString nome = m_edtNome->text().trimmed();
    if (nome.isEmpty()) return;

    FonteRendaRepository repo;
    FonteRenda f;
    f.nome = nome;
    if (!repo.insert(f)) {
        QMessageBox::warning(this, tr("Duplicado"),
            tr("Já existe uma fonte com esse nome."));
        return;
    }

    m_edtNome->clear();
    recarregar();
    emit fontesAlteradas();
}

void ConfigFontesWidget::onRemover()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    const int id   = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    const QString nome = m_table->item(row, 0)->text();

    if (QMessageBox::question(this,
            tr("Remover fonte"),
            tr("Remover \"%1\"?\n"
               "Os lançamentos existentes mantêm o nome registrado.").arg(nome))
        != QMessageBox::Yes) return;

    FonteRendaRepository repo;
    repo.remove(id);
    recarregar();
    emit fontesAlteradas();
}
