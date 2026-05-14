#include "ProventosWidget.h"
#include "ProventoDialog.h"

#include "repositories/ProventoRepository.h"

#include <QComboBox>
#include <QDate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

ProventosWidget::ProventosWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    recarregarTabela();
}

void ProventosWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // Filtros
    auto *bar = new QHBoxLayout;
    bar->addWidget(new QLabel(tr("Ticker:"), this));
    m_edtTickerFiltro = new QLineEdit(this);
    m_edtTickerFiltro->setMaximumWidth(120);
    m_edtTickerFiltro->setPlaceholderText(tr("(todos)"));
    bar->addWidget(m_edtTickerFiltro);

    bar->addWidget(new QLabel(tr("Tipo:"), this));
    m_cbTipoFiltro = new QComboBox(this);
    m_cbTipoFiltro->addItem(tr("(todos)"),     QString());
    m_cbTipoFiltro->addItem(tr("Dividendo"),    QStringLiteral("dividendo"));
    m_cbTipoFiltro->addItem(tr("JCP"),           QStringLiteral("jcp"));
    m_cbTipoFiltro->addItem(tr("Rendimento FII"), QStringLiteral("rendimento"));
    m_cbTipoFiltro->addItem(tr("Bonificação"),   QStringLiteral("bonus"));
    bar->addWidget(m_cbTipoFiltro);

    auto *btnFiltrar = new QPushButton(tr("Filtrar"), this);
    bar->addWidget(btnFiltrar);
    bar->addStretch();

    m_btnNovo    = new QPushButton(tr("Novo…"), this);
    m_btnEditar  = new QPushButton(tr("Editar"), this);
    m_btnRemover = new QPushButton(tr("Remover"), this);
    m_btnNovo->setObjectName(QStringLiteral("primary"));
    bar->addWidget(m_btnNovo);
    bar->addWidget(m_btnEditar);
    bar->addWidget(m_btnRemover);

    root->addLayout(bar);

    // Tabela
    m_tabela = new QTableWidget(this);
    m_tabela->setColumnCount(6);
    m_tabela->setHorizontalHeaderLabels({
        tr("Data"), tr("Ticker"), tr("Tipo"),
        tr("Bruto"), tr("Líquido"), tr("Qtd")
    });
    m_tabela->horizontalHeader()->setStretchLastSection(true);
    m_tabela->verticalHeader()->setVisible(false);
    m_tabela->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabela->setAlternatingRowColors(true);
    root->addWidget(m_tabela, 1);

    // Totais
    m_lblTotais = new QLabel(this);
    m_lblTotais->setStyleSheet(QStringLiteral("font-weight: bold;"));
    root->addWidget(m_lblTotais);

    connect(m_btnNovo,    &QPushButton::clicked, this, &ProventosWidget::onNovo);
    connect(m_btnEditar,  &QPushButton::clicked, this, &ProventosWidget::onEditar);
    connect(m_btnRemover, &QPushButton::clicked, this, &ProventosWidget::onRemover);
    connect(btnFiltrar,   &QPushButton::clicked, this, &ProventosWidget::onFiltrar);
    connect(m_tabela, &QTableWidget::cellDoubleClicked,
            this, [this](int, int) { onEditar(); });
}

void ProventosWidget::recarregarTabela()
{
    ProventoRepository repo;
    const QString tickerFiltro = m_edtTickerFiltro->text().trimmed();
    const QString tipoFiltro   = m_cbTipoFiltro->currentData().toString();
    auto lista = tickerFiltro.isEmpty()
        ? repo.findAll()
        : repo.findByTicker(tickerFiltro);

    if (!tipoFiltro.isEmpty()) {
        QList<Provento> filt;
        for (const auto &p : lista) if (p.tipo == tipoFiltro) filt.append(p);
        lista = filt;
    }

    QLocale loc(QLocale::Portuguese, QLocale::Brazil);
    m_tabela->setRowCount(lista.size());
    for (int i = 0; i < lista.size(); ++i) {
        const auto &p = lista[i];
        auto *itD = new QTableWidgetItem(
            p.dataEvento.toString(QStringLiteral("dd/MM/yyyy")));
        itD->setData(Qt::UserRole, QVariant::fromValue(p.id));
        m_tabela->setItem(i, 0, itD);
        m_tabela->setItem(i, 1, new QTableWidgetItem(p.ticker));
        m_tabela->setItem(i, 2, new QTableWidgetItem(p.tipo));
        m_tabela->setItem(i, 3, new QTableWidgetItem(
            loc.toString(p.valorBrutoCent / 100.0, 'f', 2)));
        m_tabela->setItem(i, 4, new QTableWidgetItem(
            loc.toString(p.valorLiquidoCent / 100.0, 'f', 2)));
        m_tabela->setItem(i, 5, new QTableWidgetItem(
            loc.toString(p.quantidade, 'f', 8)));
    }
    m_tabela->resizeColumnsToContents();
    atualizarTotais();
}

void ProventosWidget::atualizarTotais()
{
    ProventoRepository repo;
    const int anoAtual = QDate::currentDate().year();
    const qint64 total = repo.totalLiquidoAno(anoAtual);
    QLocale loc(QLocale::Portuguese, QLocale::Brazil);
    m_lblTotais->setText(tr("Líquido em %1: R$ %2")
        .arg(anoAtual)
        .arg(loc.toString(total / 100.0, 'f', 2)));
}

void ProventosWidget::onFiltrar()
{
    recarregarTabela();
}

void ProventosWidget::onNovo()
{
    ProventoDialog dlg({}, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Provento p = dlg.result();
    ProventoRepository repo;
    if (!repo.insert(p)) {
        QMessageBox::warning(this, tr("Provento"), tr("Falha ao salvar."));
        return;
    }
    recarregarTabela();
}

void ProventosWidget::onEditar()
{
    const int row = m_tabela->currentRow();
    if (row < 0) return;
    const qint64 id = m_tabela->item(row, 0)->data(Qt::UserRole).toLongLong();
    ProventoRepository repo;
    const auto orig = repo.findById(id);
    if (!orig.has_value()) return;
    ProventoDialog dlg(*orig, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Provento p = dlg.result();
    p.id = id;
    if (!repo.update(p)) {
        QMessageBox::warning(this, tr("Provento"), tr("Falha ao atualizar."));
        return;
    }
    recarregarTabela();
}

void ProventosWidget::onRemover()
{
    const int row = m_tabela->currentRow();
    if (row < 0) return;
    const qint64 id = m_tabela->item(row, 0)->data(Qt::UserRole).toLongLong();
    const auto resp = QMessageBox::question(this, tr("Remover"),
        tr("Remover provento selecionado?"));
    if (resp != QMessageBox::Yes) return;
    ProventoRepository repo;
    if (!repo.remove(id)) {
        QMessageBox::warning(this, tr("Provento"), tr("Falha ao remover."));
        return;
    }
    recarregarTabela();
}
