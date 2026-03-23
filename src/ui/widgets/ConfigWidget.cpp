#include "ConfigWidget.h"
#include "core/database/DatabaseManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QDebug>

ConfigWidget::ConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void ConfigWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(16);

    auto *title = new QLabel(tr("Cartões de Crédito"), this);
    title->setObjectName(QStringLiteral("title"));
    root->addWidget(title);

    auto *subtitle = new QLabel(
        tr("Cadastre aqui os apelidos dos seus cartões."), this);
    subtitle->setObjectName(QStringLiteral("subtitle"));
    root->addWidget(subtitle);

    // Add row
    auto *addRow = new QHBoxLayout;
    m_edtNome = new QLineEdit(this);
    m_edtNome->setPlaceholderText(tr("Ex: Nubank, XP Visa..."));
    m_edtNome->setMaxLength(60);
    m_btnAdd = new QPushButton(tr("+ Adicionar"), this);
    m_btnAdd->setObjectName(QStringLiteral("primary"));
    m_btnAdd->setEnabled(false);
    addRow->addWidget(m_edtNome);
    addRow->addWidget(m_btnAdd);
    root->addLayout(addRow);

    // Table
    m_table = new QTableWidget(0, 1, this);
    m_table->setHorizontalHeaderLabels({tr("Apelido do cartão")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    root->addWidget(m_table);

    // Remove button
    m_btnRem = new QPushButton(tr("Remover selecionado"), this);
    m_btnRem->setEnabled(false);
    root->addWidget(m_btnRem, 0, Qt::AlignLeft);

    root->addStretch();

    connect(m_edtNome, &QLineEdit::textChanged, this, [this](const QString &t) {
        m_btnAdd->setEnabled(!t.trimmed().isEmpty());
    });
    connect(m_btnAdd, &QPushButton::clicked, this, &ConfigWidget::onAdicionar);
    connect(m_btnRem, &QPushButton::clicked, this, &ConfigWidget::onRemover);
    connect(m_table->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, [this]() {
                m_btnRem->setEnabled(!m_table->selectedItems().isEmpty());
            });
}

void ConfigWidget::recarregar()
{
    m_cartoes.clear();
    QSqlQuery q(QStringLiteral("SELECT id, apelido FROM cartoes ORDER BY apelido"),
                DatabaseManager::instance().db());
    while (q.next()) {
        Cartao c;
        c.id      = q.value(0).toInt();
        c.apelido = q.value(1).toString();
        m_cartoes.append(c);
    }

    m_table->setRowCount(0);
    for (const auto &c : m_cartoes) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *item = new QTableWidgetItem(c.apelido);
        item->setData(Qt::UserRole, c.id);
        m_table->setItem(row, 0, item);
    }
}

void ConfigWidget::onAdicionar()
{
    const QString apelido = m_edtNome->text().trimmed();
    if (apelido.isEmpty()) return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral(
        "INSERT INTO cartoes (apelido) VALUES (:ap)"));
    q.bindValue(QStringLiteral(":ap"), apelido);

    if (!q.exec()) {
        if (q.lastError().text().contains(QStringLiteral("UNIQUE")))
            QMessageBox::warning(this, tr("Duplicado"),
                tr("Já existe um cartão com esse apelido."));
        else
            qWarning() << "ConfigWidget::onAdicionar:" << q.lastError().text();
        return;
    }

    m_edtNome->clear();
    recarregar();
    emit cartoesAlterados();
}

void ConfigWidget::onRemover()
{
    const int row = m_table->currentRow();
    if (row < 0) return;
    const int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    const QString apelido = m_table->item(row, 0)->text();

    if (QMessageBox::question(this,
            tr("Remover cartão"),
            tr("Remover \"%1\"?\n"
               "As compras vinculadas ficarão sem cartão.").arg(apelido))
        != QMessageBox::Yes) return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare(QStringLiteral("DELETE FROM cartoes WHERE id = :id"));
    q.bindValue(QStringLiteral(":id"), id);
    q.exec();

    recarregar();
    emit cartoesAlterados();
}
