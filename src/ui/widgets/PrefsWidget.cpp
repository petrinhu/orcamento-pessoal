#include "PrefsWidget.h"
#include "ConfigWidget.h"
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
    outer->setContentsMargins(16, 16, 16, 16);
    outer->setSpacing(16);

    auto *scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    scroll->setWidget(content);
    outer->addWidget(scroll);

    auto *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(16);

    // ── Idioma ─────────────────────────────────────────────────────────────
    auto *grpIdioma = new QGroupBox(tr("Idioma"), content);
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

    layout->addWidget(grpIdioma);

    // ── Moeda padrão ───────────────────────────────────────────────────────
    auto *grpMoeda = new QGroupBox(tr("Moeda Padrão"), content);
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
    layout->addWidget(grpMoeda);

    connect(&AppSettings::instance(), &AppSettings::currencyChanged,
            this, [this](const QString &code) {
        m_loading = true;
        syncComboToCurrency(code);
        m_loading = false;
    });

    connect(m_comboCurrency, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PrefsWidget::onCurrencyChanged);

    // ── Salvamento ─────────────────────────────────────────────────────────
    auto *grpSave = new QGroupBox(tr("Salvamento"), content);
    auto *laySave = new QVBoxLayout(grpSave);
    laySave->setSpacing(8);

    // Auto-save toggle + interval
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

    // Saves folder
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

    // Autosaves folder
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

    layout->addWidget(grpSave);

    // ── Cartões ────────────────────────────────────────────────────────────
    auto *grpCartoes = new QGroupBox(tr("Cartões"), content);
    auto *layCartoes = new QVBoxLayout(grpCartoes);
    layCartoes->setContentsMargins(0, 0, 0, 0);

    m_cards = new ConfigWidget(grpCartoes);
    layCartoes->addWidget(m_cards);

    connect(m_cards, &ConfigWidget::cartoesAlterados,
            this,    &PrefsWidget::cartoesAlterados);

    layout->addWidget(grpCartoes);
    layout->addStretch();
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
