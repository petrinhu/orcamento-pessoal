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
#include <QFileDialog>
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

    auto *lblSavesFolder = new QLabel(tr("Pasta de salvamentos:"), grpSave);
    laySave->addWidget(lblSavesFolder);
    auto *rowSaves = new QHBoxLayout;
    m_edtSavesFolder = new QLineEdit(AppSettings::instance().savesFolder(), grpSave);
    auto *btnBrowseSaves = new QPushButton(QStringLiteral("…"), grpSave);
    btnBrowseSaves->setFixedWidth(32);
    rowSaves->addWidget(m_edtSavesFolder);
    rowSaves->addWidget(btnBrowseSaves);
    laySave->addLayout(rowSaves);
    connect(btnBrowseSaves, &QPushButton::clicked, this, &PrefsWidget::onBrowseSavesFolder);
    connect(m_edtSavesFolder, &QLineEdit::editingFinished, this, [this]() {
        AppSettings::instance().setSavesFolder(m_edtSavesFolder->text().trimmed());
    });

    auto *lblAutosavesFolder = new QLabel(tr("Pasta de auto-salvamentos:"), grpSave);
    laySave->addWidget(lblAutosavesFolder);
    auto *rowAutosaves = new QHBoxLayout;
    m_edtAutosavesFolder = new QLineEdit(AppSettings::instance().autosavesFolder(), grpSave);
    auto *btnBrowseAutosaves = new QPushButton(QStringLiteral("…"), grpSave);
    btnBrowseAutosaves->setFixedWidth(32);
    rowAutosaves->addWidget(m_edtAutosavesFolder);
    rowAutosaves->addWidget(btnBrowseAutosaves);
    laySave->addLayout(rowAutosaves);
    connect(btnBrowseAutosaves, &QPushButton::clicked,
            this, &PrefsWidget::onBrowseAutosavesFolder);
    connect(m_edtAutosavesFolder, &QLineEdit::editingFinished, this, [this]() {
        AppSettings::instance().setAutosavesFolder(m_edtAutosavesFolder->text().trimmed());
    });

    layGeral->addWidget(grpSave);

    // ── Grupo: Comportamento ───────────────────────────────────────────────
    auto *grpComport  = new QGroupBox(tr("Comportamento"), scrollGeral);
    auto *layComport  = new QVBoxLayout(grpComport);
    auto *rowInitTab  = new QHBoxLayout;
    auto *lblInitTab  = new QLabel(tr("Aba inicial:"), grpComport);
    m_comboInitialTab = new QComboBox(grpComport);
    m_comboInitialTab->addItem(tr("Dashboard"),      0);
    m_comboInitialTab->addItem(tr("Entradas"),       1);
    m_comboInitialTab->addItem(tr("Gastos"),         2);
    m_comboInitialTab->addItem(tr("Tendências"),     3);
    m_comboInitialTab->addItem(tr("Investimentos"),  4);
    m_comboInitialTab->addItem(tr("Planejamento"),   5);
    m_comboInitialTab->addItem(tr("Recorrências"),   6);
    m_comboInitialTab->addItem(tr("Dívidas"),        7);
    m_comboInitialTab->addItem(tr("Metas"),          8);
    m_comboInitialTab->addItem(tr("Lembretes"),      9);
    m_comboInitialTab->addItem(tr("Cartões"),        10);
    m_comboInitialTab->setCurrentIndex(AppSettings::instance().initialTab());
    rowInitTab->addWidget(lblInitTab);
    rowInitTab->addWidget(m_comboInitialTab);
    rowInitTab->addStretch();
    layComport->addLayout(rowInitTab);
    connect(m_comboInitialTab, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [](int index) {
        AppSettings::instance().setInitialTab(index);
    });

    layGeral->addWidget(grpComport);
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

void PrefsWidget::onBrowseSavesFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Selecionar pasta de salvamentos"),
        m_edtSavesFolder->text());
    if (!dir.isEmpty()) {
        m_edtSavesFolder->setText(dir);
        AppSettings::instance().setSavesFolder(dir);
    }
}

void PrefsWidget::onBrowseAutosavesFolder()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, tr("Selecionar pasta de auto-salvamentos"),
        m_edtAutosavesFolder->text());
    if (!dir.isEmpty()) {
        m_edtAutosavesFolder->setText(dir);
        AppSettings::instance().setAutosavesFolder(dir);
    }
}
