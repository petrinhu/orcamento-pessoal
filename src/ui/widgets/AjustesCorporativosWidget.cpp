#include "AjustesCorporativosWidget.h"
#include "AjusteCorporativoDialog.h"

#include "services/AjusteCorporativoService.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

AjustesCorporativosWidget::AjustesCorporativosWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregar();
}

void AjustesCorporativosWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto *info = new QLabel(tr(
        "Eventos corporativos (splits, inplits, bonificações) afetam "
        "qty/preço histórico de ativos. Aplicação retroativa via "
        "AjusteCorporativoService::fatorCumulativo."), this);
    info->setWordWrap(true);
    info->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    root->addWidget(info);

    auto *bar = new QHBoxLayout;
    bar->addWidget(new QLabel(tr("Ticker:"), this));
    m_edtFiltro = new QLineEdit(this);
    m_edtFiltro->setPlaceholderText(tr("(todos)"));
    m_edtFiltro->setMaximumWidth(150);
    bar->addWidget(m_edtFiltro);
    auto *btnFiltrar = new QPushButton(tr("Filtrar"), this);
    bar->addWidget(btnFiltrar);
    bar->addStretch();

    m_btnNovo    = new QPushButton(tr("Novo…"), this);
    m_btnNovo->setObjectName(QStringLiteral("primary"));
    m_btnRemover = new QPushButton(tr("Remover"), this);
    bar->addWidget(m_btnNovo);
    bar->addWidget(m_btnRemover);
    root->addLayout(bar);

    m_tabela = new QTableWidget(this);
    m_tabela->setColumnCount(5);
    m_tabela->setHorizontalHeaderLabels({
        tr("Data ex"), tr("Ticker"), tr("Tipo"), tr("Fator"), tr("Obs")
    });
    m_tabela->horizontalHeader()->setStretchLastSection(true);
    m_tabela->verticalHeader()->setVisible(false);
    m_tabela->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabela->setAlternatingRowColors(true);
    root->addWidget(m_tabela, 1);

    connect(m_btnNovo,    &QPushButton::clicked, this, &AjustesCorporativosWidget::onNovo);
    connect(m_btnRemover, &QPushButton::clicked, this, &AjustesCorporativosWidget::onRemover);
    connect(btnFiltrar,   &QPushButton::clicked, this, &AjustesCorporativosWidget::recarregar);
}

void AjustesCorporativosWidget::recarregar()
{
    const QString filtro = m_edtFiltro->text().trimmed();
    auto lista = filtro.isEmpty()
        ? AjusteCorporativoService::findAll()
        : AjusteCorporativoService::findByTicker(filtro);

    m_tabela->setRowCount(lista.size());
    for (int i = 0; i < lista.size(); ++i) {
        const auto &a = lista[i];
        auto *itD = new QTableWidgetItem(
            a.dataEx.toString(QStringLiteral("dd/MM/yyyy")));
        itD->setData(Qt::UserRole, QVariant::fromValue(a.id));
        m_tabela->setItem(i, 0, itD);
        m_tabela->setItem(i, 1, new QTableWidgetItem(a.ticker));
        m_tabela->setItem(i, 2, new QTableWidgetItem(a.tipo));
        m_tabela->setItem(i, 3, new QTableWidgetItem(QString::number(a.fator, 'f', 4)));
        m_tabela->setItem(i, 4, new QTableWidgetItem(a.observacao));
    }
    m_tabela->resizeColumnsToContents();
}

void AjustesCorporativosWidget::onNovo()
{
    AjusteCorporativoDialog dlg({}, this);
    if (dlg.exec() != QDialog::Accepted) return;
    AjusteCorporativo a = dlg.result();
    if (!AjusteCorporativoService::insert(a)) {
        QMessageBox::warning(this, tr("Ajuste"), tr("Falha ao salvar."));
        return;
    }
    recarregar();
}

void AjustesCorporativosWidget::onRemover()
{
    const int row = m_tabela->currentRow();
    if (row < 0) return;
    const qint64 id = m_tabela->item(row, 0)->data(Qt::UserRole).toLongLong();
    const auto resp = QMessageBox::question(this, tr("Remover"),
        tr("Remover ajuste selecionado?"));
    if (resp != QMessageBox::Yes) return;
    if (!AjusteCorporativoService::remove(id)) {
        QMessageBox::warning(this, tr("Ajuste"), tr("Falha ao remover."));
        return;
    }
    recarregar();
}
