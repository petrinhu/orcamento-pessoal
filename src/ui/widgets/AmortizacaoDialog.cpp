#include "AmortizacaoDialog.h"
#include "services/ParcelaService.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

AmortizacaoDialog::AmortizacaoDialog(const Compra &compra, QWidget *parent)
    : QDialog(parent)
{
    buildUi(compra);
}

void AmortizacaoDialog::buildUi(const Compra &compra)
{
    setWindowTitle(tr("Tabela de amortização — %1").arg(compra.nome));
    resize(640, 420);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(10);

    // ── title info ────────────────────────────────────────────────────────
    const DadosFinanceiros d = ParcelaService::dadosFromCompra(compra);
    auto *info = new QLabel(
        tr("Sistema Price (SAF)  •  Juros a.m.: %1  •  CET a.a.: %2")
            .arg(QString::number(d.jurosMensaisPct, 'f', 4) + QLatin1Char('%'))
            .arg(QString::number(d.cetPct, 'f', 2) + QLatin1Char('%')),
        this);
    info->setObjectName(QStringLiteral("subtitle"));
    root->addWidget(info);

    // ── table ─────────────────────────────────────────────────────────────
    m_table = new QTableWidget(this);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);

    const QStringList headers = {
        tr("Nº"), tr("Data"), tr("Saldo devedor"),
        tr("Juros"), tr("Amortização"), tr("Parcela")
    };
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);

    const auto rows = ParcelaService::tabelaAmortizacao(compra);
    m_table->setRowCount(rows.size());

    const QString moeda = compra.moeda;

    for (int r = 0; r < rows.size(); ++r) {
        const auto &row = rows[r];

        auto *itNum    = new QTableWidgetItem(QString::number(row.numero));
        auto *itData   = new QTableWidgetItem(row.data.toString(QStringLiteral("dd/MM/yyyy")));
        auto *itSaldo  = new QTableWidgetItem(ParcelaService::formatarValor(row.saldoDevedor, moeda));
        auto *itJuros  = new QTableWidgetItem(ParcelaService::formatarValor(row.juros,        moeda));
        auto *itAmort  = new QTableWidgetItem(ParcelaService::formatarValor(row.amortizacao,  moeda));
        auto *itParc   = new QTableWidgetItem(ParcelaService::formatarValor(row.parcela,      moeda));

        itNum->setTextAlignment(Qt::AlignCenter);
        itData->setTextAlignment(Qt::AlignCenter);
        for (auto *it : {itSaldo, itJuros, itAmort, itParc})
            it->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        m_table->setItem(r, 0, itNum);
        m_table->setItem(r, 1, itData);
        m_table->setItem(r, 2, itSaldo);
        m_table->setItem(r, 3, itJuros);
        m_table->setItem(r, 4, itAmort);
        m_table->setItem(r, 5, itParc);
    }

    // Auto-fit columns
    auto *hh = m_table->horizontalHeader();
    for (int c = 0; c < m_table->columnCount() - 1; ++c) {
        hh->setSectionResizeMode(c, QHeaderView::ResizeToContents);
    }
    hh->setSectionResizeMode(m_table->columnCount() - 1, QHeaderView::Stretch);

    root->addWidget(m_table);

    // ── close button ──────────────────────────────────────────────────────
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(btns);
}
