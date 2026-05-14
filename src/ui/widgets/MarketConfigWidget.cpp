#include "MarketConfigWidget.h"

#include "core/settings/AppSettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {
const QStringList kProviders = {
    QStringLiteral("none"),
    QStringLiteral("frankfurter"),
    QStringLiteral("brapi"),
    QStringLiteral("b3direct"),
    QStringLiteral("cotahist"),
};

const QStringList kProviderLabels = {
    QObject::tr("Nenhum (manual)"),
    QStringLiteral("Frankfurter (ECB, sem chave)"),
    QStringLiteral("Brapi (token Bearer)"),
    QStringLiteral("B3 oficial (key id)"),
    QStringLiteral("COTAHIST (histórico)"),
};

const QStringList kMoedas = {
    QStringLiteral("USD"), QStringLiteral("EUR"), QStringLiteral("GBP"),
    QStringLiteral("JPY"), QStringLiteral("CHF"), QStringLiteral("ARS"),
    QStringLiteral("CLP"), QStringLiteral("MXN"), QStringLiteral("CAD"),
    QStringLiteral("AUD"), QStringLiteral("CNY"), QStringLiteral("HKD"),
};
}

MarketConfigWidget::MarketConfigWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
    carregarConfig();
}

void MarketConfigWidget::buildUi()
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(10);

    auto *form = new QFormLayout;

    // ── Provider padrão ────────────────────────────────────────────────────
    m_cbProvider = new QComboBox(this);
    for (int i = 0; i < kProviders.size(); ++i) {
        m_cbProvider->addItem(kProviderLabels[i], kProviders[i]);
    }
    form->addRow(tr("Provider padrão:"), m_cbProvider);

    // ── Brapi ──────────────────────────────────────────────────────────────
    m_edtToken = new QLineEdit(this);
    m_edtToken->setEchoMode(QLineEdit::Password);
    m_edtToken->setPlaceholderText(tr("Bearer token (opcional para plano Free)"));
    form->addRow(tr("Token Brapi:"), m_edtToken);

    m_cbPlano = new QComboBox(this);
    m_cbPlano->addItem(QStringLiteral("Free"));
    m_cbPlano->addItem(QStringLiteral("Startup"));
    m_cbPlano->addItem(QStringLiteral("Pro"));
    form->addRow(tr("Plano Brapi:"), m_cbPlano);

    m_btnTestar = new QPushButton(tr("Testar token…"), this);
    m_btnTestar->setEnabled(false); // placeholder ate #58 BrapiProvider entrar
    m_btnTestar->setToolTip(tr("Disponível após #58 BrapiProvider"));
    form->addRow(QString(), m_btnTestar);

    // ── Polling ────────────────────────────────────────────────────────────
    m_spnPolling = new QSpinBox(this);
    m_spnPolling->setRange(15, 3600);
    m_spnPolling->setSingleStep(15);
    m_spnPolling->setSuffix(tr(" s"));
    form->addRow(tr("Polling RT:"), m_spnPolling);

    // ── COTAHIST ───────────────────────────────────────────────────────────
    m_chkCotahist = new QCheckBox(tr("Baixar histórico COTAHIST"), this);
    form->addRow(QString(), m_chkCotahist);

    m_spnAnos = new QSpinBox(this);
    m_spnAnos->setRange(1, 40);
    m_spnAnos->setSuffix(tr(" anos"));
    form->addRow(tr("Histórico:"), m_spnAnos);

    // ── Slots moeda (painel #47) ───────────────────────────────────────────
    m_cbSlot1 = new QComboBox(this);
    m_cbSlot2 = new QComboBox(this);
    for (const QString &m : kMoedas) {
        m_cbSlot1->addItem(m);
        m_cbSlot2->addItem(m);
    }
    auto *rowSlots = new QHBoxLayout;
    rowSlots->addWidget(m_cbSlot1);
    rowSlots->addWidget(new QLabel(QStringLiteral("·"), this));
    rowSlots->addWidget(m_cbSlot2);
    rowSlots->addStretch();
    form->addRow(tr("Painel câmbio (slots):"), rowSlots);

    root->addLayout(form);

    auto *info = new QLabel(
        tr("Mudanças tem efeito imediato em providers e no painel câmbio. "
           "Sem token Brapi, plano Free limita 15 mil req/mês."), this);
    info->setWordWrap(true);
    info->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    root->addWidget(info);

    m_btnSalvar = new QPushButton(tr("Salvar"), this);
    m_btnSalvar->setObjectName(QStringLiteral("primary"));
    root->addWidget(m_btnSalvar);
    root->addStretch();

    connect(m_btnSalvar, &QPushButton::clicked, this, &MarketConfigWidget::onSalvar);
    connect(m_btnTestar, &QPushButton::clicked, this, &MarketConfigWidget::onTestarBrapi);
}

void MarketConfigWidget::carregarConfig()
{
    auto &s = AppSettings::instance();

    const int idx = m_cbProvider->findData(s.marketDefaultProvider());
    if (idx >= 0) m_cbProvider->setCurrentIndex(idx);

    m_edtToken->setText(s.brapiToken());

    const int planoIdx = m_cbPlano->findText(s.brapiPlan());
    if (planoIdx >= 0) m_cbPlano->setCurrentIndex(planoIdx);

    m_spnPolling->setValue(s.marketPollingInterval());
    m_chkCotahist->setChecked(s.cotahistEnabled());
    m_spnAnos->setValue(s.cotahistAnosHistorico());

    const int slot1Idx = m_cbSlot1->findText(s.currencySlot1());
    if (slot1Idx >= 0) m_cbSlot1->setCurrentIndex(slot1Idx);
    const int slot2Idx = m_cbSlot2->findText(s.currencySlot2());
    if (slot2Idx >= 0) m_cbSlot2->setCurrentIndex(slot2Idx);
}

void MarketConfigWidget::onSalvar()
{
    auto &s = AppSettings::instance();
    s.setMarketDefaultProvider(m_cbProvider->currentData().toString());
    s.setBrapiToken(m_edtToken->text());
    s.setBrapiPlan(m_cbPlano->currentText());
    s.setMarketPollingInterval(m_spnPolling->value());
    s.setCotahistEnabled(m_chkCotahist->isChecked());
    s.setCotahistAnosHistorico(m_spnAnos->value());
    s.setCurrencySlot1(m_cbSlot1->currentText());
    s.setCurrencySlot2(m_cbSlot2->currentText());

    QMessageBox::information(this, tr("Mercado"),
        tr("Configuração salva."));
}

void MarketConfigWidget::onTestarBrapi()
{
    // Stub: ativa quando #58 BrapiProvider for entregue
    QMessageBox::information(this, tr("Testar Brapi"),
        tr("Funcionalidade disponível após implementação de BrapiProvider (#58)."));
}
