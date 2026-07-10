#include "BolsaWidget.h"
#include "EmptyStateWidget.h"
#include "ui/Theme.h"

#include "core/settings/AppSettings.h"
#include "services/MarketDataService.h"
#include "services/MarketProviderCatalog.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

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
    const QStringList &provKeys = MarketProviderCatalog::keys();
    const QStringList &provLabels = MarketProviderCatalog::labels();
    for (int i = 0; i < provKeys.size(); ++i) {
        m_cbProvider->addItem(provLabels[i], provKeys[i]);
    }
    bar->addWidget(m_cbProvider);
    bar->addSpacing(12);

    m_edtBusca = new QLineEdit(this);
    m_edtBusca->setPlaceholderText(tr("Buscar ticker (ex: PETR4, MGLU3)"));
    m_edtBusca->setClearButtonEnabled(true);
    bar->addWidget(m_edtBusca, 1);

    m_btnBuscar = new QPushButton(tr("Buscar"), this);
    m_btnBuscar->setIcon(Theme::emojiIcon(QStringLiteral("🔍")));
    m_btnBuscar->setObjectName(QStringLiteral("primary"));
    bar->addWidget(m_btnBuscar);

    root->addLayout(bar);

    // ── Stack: empty state (0) + tabela (1) ──────────────────────────────
    // Sem CTA: Bolsa exige input do usuário para buscar (#166 Peça 2 — sem botão).
    m_stack = new QStackedWidget(this);

    auto *emptyState = new EmptyStateWidget(
        QStringLiteral("🏦"),
        tr("Nenhuma cotação pesquisada"),
        tr("Digite o código de uma ação (ex.: PETR4) para ver o preço e as informações mais recentes."),
        QString(), // sem CTA
        m_stack);
    m_stack->addWidget(emptyState); // index 0

    // ── Tabela de resultados ──────────────────────────────────────────────
    m_tabela = new QTableWidget(m_stack);
    m_tabela->setColumnCount(5);
    m_tabela->setHorizontalHeaderLabels({
        tr("Ticker"), tr("Nome"), tr("Último"), tr("Variação %"), tr("Volume")
    });
    m_tabela->horizontalHeader()->setStretchLastSection(true);
    m_tabela->verticalHeader()->setVisible(false);
    m_tabela->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tabela->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tabela->setAlternatingRowColors(true);
    m_stack->addWidget(m_tabela); // index 1

    root->addWidget(m_stack, 1);

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

void BolsaWidget::setBuscaTimeoutMsParaTeste(int ms)
{
    m_buscaTimeoutMs = ms;
}

QString BolsaWidget::mensagemStatusProvider(const QString &solicitado,
                                            bool sucesso,
                                            const QString &ativoReal)
{
    if (ativoReal.isEmpty()) {
        return tr("Nenhum provedor de cotação está ativo no momento. "
                  "Configure um em Preferências → Mercado.");
    }
    if (sucesso) {
        return tr("Provedor de cotação ativo: %1")
            .arg(MarketProviderCatalog::labelFor(ativoReal));
    }
    // UX-02: nunca confirmar `solicitado` como ativo se ele não ficou ativo
    // de fato — a mensagem sempre reflete o provider REAL (`ativoReal`).
    return tr("%1 não está disponível agora — usando %2 em vez disso.")
        .arg(MarketProviderCatalog::labelFor(solicitado),
             MarketProviderCatalog::labelFor(ativoReal));
}

QString BolsaWidget::mensagemTimeoutBusca(const QString &query)
{
    return tr("Não foi possível buscar \"%1\" — o provedor não respondeu. "
              "Verifique sua conexão ou tente outro provedor em "
              "Preferências → Mercado.").arg(query);
}

void BolsaWidget::onProviderChanged(int idx)
{
    const QString solicitado = m_cbProvider->itemData(idx).toString();

    if (!m_service) {
        m_lblStatus->setText(mensagemStatusProvider(solicitado, false, QString()));
        return;
    }

    const bool sucesso = m_service->setActiveProvider(solicitado);
    const QString ativoReal = m_service->activeProviderName();

    // Só persiste como preferência default o que de fato funcionou — salvar
    // um provider que sabemos que falha repetiria a mentira no próximo
    // lançamento do app (MainWindow chama setActiveProvider com o default
    // salvo, sem checar o retorno).
    if (sucesso) AppSettings::instance().setMarketDefaultProvider(solicitado);

    m_lblStatus->setText(mensagemStatusProvider(solicitado, sucesso, ativoReal));
}

void BolsaWidget::onBuscar()
{
    const QString q = m_edtBusca->text().trimmed();
    if (q.isEmpty()) return;

    if (!m_service || !m_service->activeProvider()) {
        m_lblStatus->setText(tr("Sem provider ativo. Configure em Preferências → Mercado."));
        return;
    }

    const quint64 seq = ++m_buscaSeq; // invalida callback/timeout de busca anterior
    const QString providerAtual = m_service->activeProviderName();

    m_lblStatus->setText(tr("Buscando \"%1\" em %2…")
        .arg(q, MarketProviderCatalog::labelFor(providerAtual)));

    if (m_timeoutBusca) {
        m_timeoutBusca->stop();
        m_timeoutBusca->deleteLater();
    }
    m_timeoutBusca = new QTimer(this);
    m_timeoutBusca->setSingleShot(true);
    connect(m_timeoutBusca, &QTimer::timeout, this, [this, seq, q]() {
        if (seq != m_buscaSeq) return; // busca já resolvida ou substituída
        m_lblStatus->setText(mensagemTimeoutBusca(q));
    });
    m_timeoutBusca->start(m_buscaTimeoutMs);

    m_service->search(q, [this, seq, q](const QStringList &results) {
        if (seq != m_buscaSeq) return; // resposta de uma busca já expirada/substituída
        if (m_timeoutBusca) m_timeoutBusca->stop();

        m_tabela->setRowCount(results.size());
        for (int i = 0; i < results.size(); ++i) {
            m_tabela->setItem(i, 0, new QTableWidgetItem(results[i]));
            m_tabela->setItem(i, 1, new QTableWidgetItem(QStringLiteral("—")));
            m_tabela->setItem(i, 2, new QTableWidgetItem(QStringLiteral("—")));
            m_tabela->setItem(i, 3, new QTableWidgetItem(QStringLiteral("—")));
            m_tabela->setItem(i, 4, new QTableWidgetItem(QStringLiteral("—")));
        }
        // Alterna stack: tabela (1) quando há resultados, empty state (0) quando não.
        m_stack->setCurrentIndex(results.isEmpty() ? 0 : 1);
        m_lblStatus->setText(tr("%1 resultados para \"%2\"").arg(results.size()).arg(q));
    });
}
