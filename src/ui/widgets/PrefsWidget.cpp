#include "PrefsWidget.h"
#include "AjustesCorporativosWidget.h"
#include "AuditLogViewer.h"
#include "BackupConfigWidget.h"
#include "ConfigCategoriasWidget.h"
#include "ConfigFontesWidget.h"
#include "ConfigPagadoresWidget.h"
#include "ConfigWidget.h"
#include "MarketConfigWidget.h"
#include "core/settings/AppSettings.h"
#include "core/session/UserSession.h"
#include "ui/NavRegistry.h"
#include "ui/Theme.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QComboBox>
#include <QCompleter>
#include <QScrollArea>
#include <QLabel>
#include <QCheckBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
// (#181) QFileDialog removido — Browse de pastas de save não existe mais.
#include <QTabWidget>

// ── Currency list ─────────────────────────────────────────────────────────────

QStringList PrefsWidget::buildCurrencyList()
{
    return {
        QStringLiteral("BRL — Real Brasileiro (R$)"),
        QStringLiteral("USD — US Dollar ($)"),
        QStringLiteral("EUR — Euro (\u20ac)"),
        QStringLiteral("GBP — British Pound (\u00a3)"),
        QStringLiteral("JPY — Japanese Yen (\u00a5)"),
        QStringLiteral("CAD — Canadian Dollar (CA$)"),
        QStringLiteral("AUD — Australian Dollar (A$)"),
        QStringLiteral("CHF — Swiss Franc (Fr)"),
        QStringLiteral("CNY — Chinese Yuan (\u00a5)"),
        QStringLiteral("HKD — Hong Kong Dollar (HK$)"),
        QStringLiteral("SGD — Singapore Dollar (S$)"),
        QStringLiteral("KRW — Korean Won (\u20a9)"),
        QStringLiteral("INR — Indian Rupee (\u20b9)"),
        QStringLiteral("MXN — Peso Mexicano (MX$)"),
        QStringLiteral("ARS — Peso Argentino ($)"),
        QStringLiteral("CLP — Peso Chileno ($)"),
        QStringLiteral("COP — Peso Colombiano ($)"),
        QStringLiteral("PEN — Sol Peruano (S/)"),
        QStringLiteral("UYU — Peso Uruguaio ($U)"),
        QStringLiteral("BOB — Boliviano (Bs)"),
        QStringLiteral("PYG — Guaran\u00ed Paraguaio (\u20b2)"),
        QStringLiteral("GTQ — Quetzal Guatemalteco (Q)"),
        QStringLiteral("NOK — Norwegian Krone (kr)"),
        QStringLiteral("SEK — Swedish Krona (kr)"),
        QStringLiteral("DKK — Danish Krone (kr)"),
        QStringLiteral("PLN — Polish Zloty (z\u0142)"),
        QStringLiteral("CZK — Czech Koruna (K\u010d)"),
        QStringLiteral("TRY — Turkish Lira (\u20ba)"),
        QStringLiteral("RUB — Russian Ruble (\u20bd)"),
        QStringLiteral("ZAR — South African Rand (R)"),
    };
}

// ── Construction ──────────────────────────────────────────────────────────────

PrefsWidget::PrefsWidget(QWidget *parent)
    : QWidget(parent)
{
    buildUi();
}

void PrefsWidget::buildUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *tabs = new QTabWidget(this);
    outer->addWidget(tabs);

    // ── Aba: Geral ─────────────────────────────────────────────────────────
    auto *pageGeral   = new QWidget(tabs);
    auto *scrollGeral = new QScrollArea(tabs);
    scrollGeral->setWidgetResizable(true);
    scrollGeral->setFrameShape(QFrame::NoFrame);
    scrollGeral->setWidget(pageGeral);

    auto *layGeral = new QVBoxLayout(pageGeral);
    layGeral->setContentsMargins(16, 16, 16, 16);
    layGeral->setSpacing(16);

    // Idioma
    auto *grpIdioma = new QGroupBox(tr("Idioma"), pageGeral);
    auto *layIdioma = new QVBoxLayout(grpIdioma);
    layIdioma->setSpacing(6);

    m_rPtBr   = new QRadioButton(QStringLiteral("Português — pt-BR"), grpIdioma);
    m_rEnIntl = new QRadioButton(QStringLiteral("English — en-intl"),  grpIdioma);
    layIdioma->addWidget(m_rPtBr);
    layIdioma->addWidget(m_rEnIntl);

    m_lblRestartHint = new QLabel(
        tr("Reinicie o aplicativo para aplicar o novo idioma."), grpIdioma);
    m_lblRestartHint->setObjectName(QStringLiteral("subtitle"));
    m_lblRestartHint->setVisible(false);
    layIdioma->addWidget(m_lblRestartHint);

    const bool isPtBr = (AppSettings::instance().language() == AppSettings::Language::PtBr);
    m_rPtBr  ->setChecked( isPtBr);
    m_rEnIntl->setChecked(!isPtBr);

    connect(m_rPtBr,   &QRadioButton::toggled, this, &PrefsWidget::onLanguageToggled);
    connect(m_rEnIntl, &QRadioButton::toggled, this, &PrefsWidget::onLanguageToggled);

    layGeral->addWidget(grpIdioma);

    // Moeda padrão
    auto *grpMoeda = new QGroupBox(tr("Moeda Padrão"), pageGeral);
    auto *layMoeda = new QVBoxLayout(grpMoeda);

    auto *lblHint = new QLabel(
        tr("Digite para filtrar — os totais da tabela usam o câmbio desta moeda."),
        grpMoeda);
    lblHint->setObjectName(QStringLiteral("subtitle"));
    lblHint->setWordWrap(true);
    layMoeda->addWidget(lblHint);

    m_comboCurrency = new QComboBox(grpMoeda);
    m_comboCurrency->setEditable(true);
    m_comboCurrency->setInsertPolicy(QComboBox::NoInsert);
    m_comboCurrency->addItems(buildCurrencyList());

    auto *comp = m_comboCurrency->completer();
    comp->setFilterMode(Qt::MatchContains);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    comp->setCompletionMode(QCompleter::PopupCompletion);

    syncComboToCurrency(AppSettings::instance().defaultCurrency());
    layMoeda->addWidget(m_comboCurrency);
    layGeral->addWidget(grpMoeda);

    connect(&AppSettings::instance(), &AppSettings::currencyChanged,
            this, [this](const QString &code) {
        m_loading = true;
        syncComboToCurrency(code);
        m_loading = false;
    });
    connect(m_comboCurrency, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PrefsWidget::onCurrencyChanged);

    // Salvamento
    auto *grpSave = new QGroupBox(tr("Salvamento"), pageGeral);
    auto *laySave = new QVBoxLayout(grpSave);
    laySave->setSpacing(8);

    auto *rowAutosave = new QHBoxLayout;
    m_chkAutosave = new QCheckBox(tr("Salvar automaticamente a cada"), grpSave);
    m_chkAutosave->setChecked(AppSettings::instance().autosaveEnabled());
    rowAutosave->addWidget(m_chkAutosave);

    m_spnInterval = new QSpinBox(grpSave);
    m_spnInterval->setRange(1, 60);
    m_spnInterval->setSuffix(tr(" min"));
    m_spnInterval->setValue(AppSettings::instance().autosaveIntervalMinutes());
    m_spnInterval->setEnabled(m_chkAutosave->isChecked());
    rowAutosave->addWidget(m_spnInterval);
    rowAutosave->addStretch();
    laySave->addLayout(rowAutosave);

    connect(m_chkAutosave, &QCheckBox::toggled, this, &PrefsWidget::onAutosaveToggled);
    connect(m_spnInterval, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PrefsWidget::onIntervalChanged);

    // (#181) Pasta de save FIXA — o usuário não escolhe mais.
    // Exibe o caminho informativo (somente leitura) em vez dos botões Browse.
    // O slug pode ainda ser vazio aqui (sessão sem login em debug) — toleramos.
    {
        auto *lblLocal = new QLabel(tr("Local do cofre (fixo):"), grpSave);
        laySave->addWidget(lblLocal);
        // Usa slug vazio como placeholder se ainda não logado (teórico em prefs).
        const QString slugAtual = UserSession::instance().slug();
        const QString pathExibido = AppSettings::savePathFixo(
            slugAtual.isEmpty() ? QStringLiteral("<usuário>") : slugAtual);
        auto *lblPath = new QLabel(pathExibido, grpSave);
        lblPath->setWordWrap(true);
        // Estilo sutil de campo read-only: monoespaçado, cor subtext.
        lblPath->setObjectName(QStringLiteral("caminhoFixo"));
        laySave->addWidget(lblPath);

        auto *lblAuto = new QLabel(tr("Auto-save (fixo):"), grpSave);
        laySave->addWidget(lblAuto);
        const QString pathAutoExibido = AppSettings::autosavePathFixo(
            slugAtual.isEmpty() ? QStringLiteral("<usuário>") : slugAtual);
        auto *lblPathAuto = new QLabel(pathAutoExibido, grpSave);
        lblPathAuto->setWordWrap(true);
        lblPathAuto->setObjectName(QStringLiteral("caminhoFixo"));
        laySave->addWidget(lblPathAuto);
    }

    layGeral->addWidget(grpSave);

    // ── Grupo: Comportamento ───────────────────────────────────────────────
    auto *grpComport  = new QGroupBox(tr("Comportamento"), scrollGeral);
    auto *layComport  = new QVBoxLayout(grpComport);
    auto *rowInitTab  = new QHBoxLayout;
    auto *lblInitTab  = new QLabel(tr("Aba inicial:"), grpComport);
    m_comboInitialTab = new QComboBox(grpComport);
    // Itens derivados da NavRegistry (ordem fixa, chave estável no userData).
    // Persiste a chave — não o índice — eliminando a fragilidade de ordem.
    for (const NavItem &item : navItens())
        m_comboInitialTab->addItem(item.titulo, item.chave);

    // Seleciona o item cujo userData == chave salva (fallback: índice 0).
    {
        const QString chaveSalva = AppSettings::instance().initialTabKey();
        int selIdx = 0;
        for (int i = 0; i < m_comboInitialTab->count(); ++i) {
            if (m_comboInitialTab->itemData(i).toString() == chaveSalva) {
                selIdx = i;
                break;
            }
        }
        m_comboInitialTab->setCurrentIndex(selIdx);
    }
    rowInitTab->addWidget(lblInitTab);
    rowInitTab->addWidget(m_comboInitialTab);
    rowInitTab->addStretch();
    layComport->addLayout(rowInitTab);
    connect(m_comboInitialTab, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        const QString chave = m_comboInitialTab->itemData(index).toString();
        if (!chave.isEmpty())
            AppSettings::instance().setInitialTabKey(chave);
    });

    layGeral->addWidget(grpComport);

    // ── Grupo: Apresentação (#104 — Modo Iniciante vs Avançado) ────────────
    auto *grpModo = new QGroupBox(tr("Apresentação"), pageGeral);
    auto *modoLay = new QVBoxLayout(grpModo);
    m_chkIniciante = new QCheckBox(
        tr("Modo Iniciante (usar termos do dia-a-dia em vez de siglas)"),
        grpModo);
    m_chkIniciante->setToolTip(tr(
        "Mostra 'Aposentadoria Antecipada' em vez de 'FIRE', "
        "'Imposto de Renda' em vez de 'IRPF', etc. "
        "Toggle para Avançado se preferir siglas técnicas."));
    m_chkIniciante->setChecked(AppSettings::instance().modoIniciante());
    auto *lblAviso = new QLabel(
        tr("<i>Reabra o app para aplicar mudanças em todos os labels.</i>"),
        grpModo);
    lblAviso->setWordWrap(true);
    lblAviso->setStyleSheet(QStringLiteral("color: gray; font-size: 11px;"));
    modoLay->addWidget(m_chkIniciante);
    modoLay->addWidget(lblAviso);
    connect(m_chkIniciante, &QCheckBox::toggled,
            this, [](bool on) { AppSettings::instance().setModoIniciante(on); });

    // #167 T1 (§11 manual de estilo): densidade das tabelas. Diferente do
    // Modo Iniciante, este toggle aplica AO VIVO — Theme::aplicarDensidadeTabelas()
    // varre todas as QTableWidget já construídas na sessão, sem reiniciar.
    m_chkTabelaCompacta = new QCheckBox(
        tr("Tabelas compactas (linhas mais estreitas)"), grpModo);
    m_chkTabelaCompacta->setToolTip(tr(
        "Confortável (padrão): linhas de ~48px, mais fáceis de tocar/ler. "
        "Compacta: linhas de ~34px, mais itens visíveis por tela."));
    m_chkTabelaCompacta->setChecked(AppSettings::instance().tabelaDensidadeCompacta());
    modoLay->addWidget(m_chkTabelaCompacta);
    connect(m_chkTabelaCompacta, &QCheckBox::toggled, this, [](bool on) {
        AppSettings::instance().setTabelaDensidadeCompacta(on);
        Theme::setDensidadeTabelasCompacta(on);
        Theme::aplicarDensidadeTabelas();
    });

    layGeral->addWidget(grpModo);

    layGeral->addStretch();

    tabs->addTab(scrollGeral, tr("Geral"));

    // ── Aba: Cartões ───────────────────────────────────────────────────────
    m_cards = new ConfigWidget(tabs);
    connect(m_cards, &ConfigWidget::cartoesAlterados,
            this,    &PrefsWidget::cartoesAlterados);
    tabs->addTab(m_cards, tr("Cartões"));

    // ── Aba: Fontes de Renda ───────────────────────────────────────────────
    m_fontes = new ConfigFontesWidget(tabs);
    connect(m_fontes, &ConfigFontesWidget::fontesAlteradas,
            this,     &PrefsWidget::fontesAlteradas);
    tabs->addTab(m_fontes, tr("Fontes de Renda"));

    // ── Aba: Beneficiários ─────────────────────────────────────────────────
    auto *pagPage    = new QWidget(tabs);
    auto *pagScroll  = new QScrollArea(tabs);
    pagScroll->setWidgetResizable(true);
    pagScroll->setFrameShape(QFrame::NoFrame);
    pagScroll->setWidget(pagPage);

    auto *pagLay = new QVBoxLayout(pagPage);
    pagLay->setContentsMargins(16, 16, 16, 16);
    m_pagadores = new ConfigPagadoresWidget(pagPage);
    connect(m_pagadores, &ConfigPagadoresWidget::pagadoresAlterados,
            this,        &PrefsWidget::pagadoresAlterados);
    pagLay->addWidget(m_pagadores);
    pagLay->addStretch();

    tabs->addTab(pagScroll, tr("Beneficiários"));

    // ── Aba: Categorias ────────────────────────────────────────────────────
    auto *catPage   = new QWidget(tabs);
    auto *catScroll = new QScrollArea(tabs);
    catScroll->setWidgetResizable(true);
    catScroll->setFrameShape(QFrame::NoFrame);
    catScroll->setWidget(catPage);

    auto *catLay = new QVBoxLayout(catPage);
    catLay->setContentsMargins(16, 16, 16, 16);
    m_categorias = new ConfigCategoriasWidget(catPage);
    connect(m_categorias, &ConfigCategoriasWidget::categoriasAlteradas,
            this,         &PrefsWidget::categoriasAlteradas);
    catLay->addWidget(m_categorias);
    catLay->addStretch();

    tabs->addTab(catScroll, tr("Categorias"));

    // ── Mercado (#46) ──────────────────────────────────────────────────────
    auto *marketConf = new MarketConfigWidget(tabs);
    tabs->addTab(marketConf, tr("Mercado"));

    // ── Backup (#79) ────────────────────────────────────────────────────────
    auto *backupConf = new BackupConfigWidget(tabs);
    tabs->addTab(backupConf, tr("Backup"));

    // ── Ajustes corporativos (#65) ──────────────────────────────────────────
    auto *ajustes = new AjustesCorporativosWidget(tabs);
    tabs->addTab(ajustes, tr("Ajustes"));

    // ── Auditoria (#92) ────────────────────────────────────────────────────
    auto *auditViewer = new AuditLogViewer(tabs);
    tabs->addTab(auditViewer, tr("Auditoria"));
}

void PrefsWidget::syncComboToCurrency(const QString &code)
{
    const QString prefix = code + QStringLiteral(" —");
    for (int i = 0; i < m_comboCurrency->count(); ++i) {
        if (m_comboCurrency->itemText(i).startsWith(prefix)) {
            m_comboCurrency->setCurrentIndex(i);
            return;
        }
    }
    m_comboCurrency->setCurrentText(code);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void PrefsWidget::onLanguageToggled()
{
    const auto lang = m_rPtBr->isChecked()
                      ? AppSettings::Language::PtBr
                      : AppSettings::Language::EnIntl;
    AppSettings::instance().setLanguage(lang);
    m_lblRestartHint->setVisible(true);
}

void PrefsWidget::onCurrencyChanged(int /*index*/)
{
    if (m_loading) return;
    const QString entry = m_comboCurrency->currentText().trimmed();
    if (entry.length() < 3) return;
    const QString code = entry.left(3).toUpper();
    AppSettings::instance().setDefaultCurrency(code);
    emit moedaAlterada();
}

void PrefsWidget::onAutosaveToggled(bool checked)
{
    m_spnInterval->setEnabled(checked);
    AppSettings::instance().setAutosaveEnabled(checked);
}

void PrefsWidget::onIntervalChanged(int minutes)
{
    AppSettings::instance().setAutosaveIntervalMinutes(minutes);
}

// (#181) onBrowseSavesFolder / onBrowseAutosavesFolder removidos.
// Pastas de save são FIXAS (~/peubud/save/<slug>/); usuário não escolhe mais.
