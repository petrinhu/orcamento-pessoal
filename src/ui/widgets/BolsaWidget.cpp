#include "BolsaWidget.h"

#include "core/settings/AppSettings.h"
#include "services/MarketDataService.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QIcon>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {
const QStringList kProviders = {
    QStringLiteral("none"),
    QStringLiteral("frankfurter"),
    QStringLiteral("brapi"),
    QStringLiteral("b3direct"),
    QStringLiteral("cotahist"),
};
}

BolsaWidget::BolsaWidget(MarketDataService *service, QWidget *parent)
    : QWidget(parent), m_service(service)
{
    buildUi();
    carregarProvider();
}

void BolsaWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // ── Barra superior: provider + busca ──────────────────────────────────
    auto *bar = new QHBoxLayout;
    bar->addWidget(new QLabel(tr("Provider:"), this));

    m_cbProvider = new QComboBox(this);
    for (const QString &p : kProviders) m_cbProvider->addItem(p, p);
    bar->addWidget(m_cbProvider);
    bar->addSpacing(12);

    m_edtBusca = new QLineEdit(this);
    m_edtBusca->setPlaceholderText(tr("Buscar ticker (ex: PETR4, MGLU3)"));
    m_edtBusca->setClearButtonEnabled(true);
    bar->addWidget(m_edtBusca, 1);

    m_btnBuscar = new QPushButton(QIcon::fromTheme(QStringLiteral("system-search")), tr("Buscar"), this);
    m_btnBuscar->setObjectName(QStringLiteral("primary"));
    bar->addWidget(m_btnBuscar);

    root->addLayout(bar);

    // ── Tabela de resultados ──────────────────────────────────────────────
    m_tabela = new QTableWidget(this);
    m_tabela->setColumnCount(5);
    m_tabela->setHorizontalHeaderLabels({
        tr("Ticker"), tr("Nome"), tr("Último"), tr("Variação %"), tr("Volume")
    });
    m_tabela->horizontalHeader()->setStretchLastSection(true);
    m_tabela->verticalHeader()->setVisible(false);
    m_tabela->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabela->setAlternatingRowColors(true);
    root->addWidget(m_tabela, 1);

    // ── Status ────────────────────────────────────────────────────────────
    m_lblStatus = new QLabel(this);
    m_lblStatus->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    m_lblStatus->setText(tr("Configure provider em Preferências → Mercado. "
                            "Charts e indicadores: #51 / #52 / #53."));
    m_lblStatus->setWordWrap(true);
    root->addWidget(m_lblStatus);

    connect(m_btnBuscar, &QPushButton::clicked, this, &BolsaWidget::onBuscar);
    connect(m_edtBusca,  &QLineEdit::returnPressed, this, &BolsaWidget::onBuscar);
    connect(m_cbProvider, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &BolsaWidget::onProviderChanged);
}

void BolsaWidget::carregarProvider()
{
    const QString p = AppSettings::instance().marketDefaultProvider();
    const int idx = m_cbProvider->findData(p);
    if (idx >= 0) m_cbProvider->setCurrentIndex(idx);
}

void BolsaWidget::onProviderChanged(int idx)
{
    const QString p = m_cbProvider->itemData(idx).toString();
    AppSettings::instance().setMarketDefaultProvider(p);
    if (m_service) m_service->setActiveProvider(p);
    m_lblStatus->setText(tr("Provider ativo: %1").arg(p));
}

void BolsaWidget::onBuscar()
{
    const QString q = m_edtBusca->text().trimmed();
    if (q.isEmpty()) return;

    if (!m_service || !m_service->activeProvider()) {
        m_lblStatus->setText(tr("Sem provider ativo. Configure em Preferências → Mercado."));
        return;
    }

    m_lblStatus->setText(tr("Buscando '%1' em %2…")
        .arg(q, m_cbProvider->currentData().toString()));
    m_service->search(q, [this, q](const QStringList &results) {
        m_tabela->setRowCount(results.size());
        for (int i = 0; i < results.size(); ++i) {
            m_tabela->setItem(i, 0, new QTableWidgetItem(results[i]));
            m_tabela->setItem(i, 1, new QTableWidgetItem(QStringLiteral("—")));
            m_tabela->setItem(i, 2, new QTableWidgetItem(QStringLiteral("—")));
            m_tabela->setItem(i, 3, new QTableWidgetItem(QStringLiteral("—")));
            m_tabela->setItem(i, 4, new QTableWidgetItem(QStringLiteral("—")));
        }
        m_lblStatus->setText(tr("%1 resultados para '%2'").arg(results.size()).arg(q));
    });
}
