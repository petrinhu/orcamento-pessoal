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

    // ── Scroll area so the content scrolls if window is small ──────────────
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
    auto *grpIdioma = new QGroupBox(QStringLiteral("Idioma"), content);
    auto *layIdioma = new QVBoxLayout(grpIdioma);
    layIdioma->setSpacing(6);

    m_rPtBr   = new QRadioButton(QStringLiteral("Português — pt-BR"), grpIdioma);
    m_rEnIntl = new QRadioButton(QStringLiteral("English — en-intl"),  grpIdioma);
    layIdioma->addWidget(m_rPtBr);
    layIdioma->addWidget(m_rEnIntl);

    const bool isPtBr = (AppSettings::instance().language() == AppSettings::Language::PtBr);
    m_rPtBr  ->setChecked( isPtBr);
    m_rEnIntl->setChecked(!isPtBr);

    connect(m_rPtBr,   &QRadioButton::toggled, this, &PrefsWidget::onLanguageToggled);
    connect(m_rEnIntl, &QRadioButton::toggled, this, &PrefsWidget::onLanguageToggled);

    layout->addWidget(grpIdioma);

    // ── Moeda padrão ───────────────────────────────────────────────────────
    auto *grpMoeda = new QGroupBox(QStringLiteral("Moeda Padrão"), content);
    auto *layMoeda = new QVBoxLayout(grpMoeda);

    auto *lblHint = new QLabel(
        QStringLiteral("Digite para filtrar — os totais da tabela usam o câmbio desta moeda."),
        grpMoeda);
    lblHint->setObjectName(QStringLiteral("subtitle"));
    lblHint->setWordWrap(true);
    layMoeda->addWidget(lblHint);

    m_comboCurrency = new QComboBox(grpMoeda);
    m_comboCurrency->setEditable(true);
    m_comboCurrency->setInsertPolicy(QComboBox::NoInsert);
    m_comboCurrency->addItems(buildCurrencyList());

    // Configure completer for contains-search (type "euro" → finds EUR)
    auto *comp = m_comboCurrency->completer();
    comp->setFilterMode(Qt::MatchContains);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    comp->setCompletionMode(QCompleter::PopupCompletion);

    syncComboToCurrency(AppSettings::instance().defaultCurrency());
    layMoeda->addWidget(m_comboCurrency);
    layout->addWidget(grpMoeda);

    // Keep combo in sync when language change forces a currency reset
    connect(&AppSettings::instance(), &AppSettings::currencyChanged,
            this, [this](const QString &code) {
        m_loading = true;
        syncComboToCurrency(code);
        m_loading = false;
    });

    connect(m_comboCurrency, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PrefsWidget::onCurrencyChanged);

    // ── Cartões ────────────────────────────────────────────────────────────
    auto *grpCartoes = new QGroupBox(QStringLiteral("Cartões"), content);
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
    // Unknown code: set text directly
    m_comboCurrency->setCurrentText(code);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void PrefsWidget::onLanguageToggled()
{
    const auto lang = m_rPtBr->isChecked()
                      ? AppSettings::Language::PtBr
                      : AppSettings::Language::EnIntl;
    AppSettings::instance().setLanguage(lang);
    // currencyChanged signal from AppSettings will update combo via connection above
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
