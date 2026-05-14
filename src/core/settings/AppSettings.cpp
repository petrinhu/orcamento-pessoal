#include "AppSettings.h"
#include "services/ParcelaService.h"

#include <QStandardPaths>

AppSettings &AppSettings::instance()
{
    static AppSettings s;
    return s;
}

AppSettings::AppSettings()
    : QObject(nullptr)
    , m_s(QStringLiteral("petrinhu"), QStringLiteral("PeuBud"))
{}

AppSettings::Language AppSettings::language() const
{
    return m_s.value(QStringLiteral("language"), QStringLiteral("pt-BR")).toString() == QStringLiteral("pt-BR")
           ? Language::PtBr : Language::EnIntl;
}

void AppSettings::setLanguage(Language lang)
{
    const QString code = (lang == Language::PtBr)
                         ? QStringLiteral("pt-BR") : QStringLiteral("en-US");
    if (m_s.value(QStringLiteral("language")).toString() == code) return;

    m_s.setValue(QStringLiteral("language"), code);

    // Apply currency default only if the user never explicitly chose one
    if (!m_s.contains(QStringLiteral("currency_manual"))) {
        const QString cur = (lang == Language::PtBr)
                            ? QStringLiteral("BRL") : QStringLiteral("USD");
        m_s.setValue(QStringLiteral("currency"), cur);
        emit currencyChanged(cur);
    }

    emit languageChanged(lang);
}

QString AppSettings::defaultCurrency() const
{
    return m_s.value(QStringLiteral("currency"), QStringLiteral("BRL")).toString();
}

void AppSettings::setDefaultCurrency(const QString &code)
{
    if (m_s.value(QStringLiteral("currency")).toString() == code) return;
    m_s.setValue(QStringLiteral("currency"), code);
    m_s.setValue(QStringLiteral("currency_manual"), true);
    emit currencyChanged(code);
}

QString AppSettings::currencySymbol() const
{
    return ParcelaService::simboloMoeda(defaultCurrency());
}

// ── Save / autosave settings ──────────────────────────────────────────────────

bool AppSettings::autosaveEnabled() const
{
    return m_s.value(QStringLiteral("autosave_enabled"), true).toBool();
}

void AppSettings::setAutosaveEnabled(bool on)
{
    if (m_s.value(QStringLiteral("autosave_enabled"), true).toBool() == on) return;
    m_s.setValue(QStringLiteral("autosave_enabled"), on);
    emit autosaveSettingsChanged();
}

int AppSettings::autosaveIntervalMinutes() const
{
    return m_s.value(QStringLiteral("autosave_interval"), 5).toInt();
}

void AppSettings::setAutosaveIntervalMinutes(int minutes)
{
    if (m_s.value(QStringLiteral("autosave_interval"), 5).toInt() == minutes) return;
    m_s.setValue(QStringLiteral("autosave_interval"), minutes);
    emit autosaveSettingsChanged();
}

QString AppSettings::savesFolder() const
{
    const QString defaultPath =
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
        + QStringLiteral("/PeuBud");
    return m_s.value(QStringLiteral("saves_folder"), defaultPath).toString();
}

void AppSettings::setSavesFolder(const QString &path)
{
    if (m_s.value(QStringLiteral("saves_folder")).toString() == path) return;
    m_s.setValue(QStringLiteral("saves_folder"), path);
    emit autosaveSettingsChanged();
}

QString AppSettings::autosavesFolder() const
{
    const QString defaultPath = savesFolder() + QStringLiteral("/autosaves");
    return m_s.value(QStringLiteral("autosaves_folder"), defaultPath).toString();
}

void AppSettings::setAutosavesFolder(const QString &path)
{
    if (m_s.value(QStringLiteral("autosaves_folder")).toString() == path) return;
    m_s.setValue(QStringLiteral("autosaves_folder"), path);
    emit autosaveSettingsChanged();
}

// ── Startup tab ───────────────────────────────────────────────────────────────

int AppSettings::initialTab() const
{
    return m_s.value(QStringLiteral("initial_tab"), 0).toInt();
}

void AppSettings::setInitialTab(int index)
{
    if (m_s.value(QStringLiteral("initial_tab"), 0).toInt() == index) return;
    m_s.setValue(QStringLiteral("initial_tab"), index);
}

// ── Market data (#46) ────────────────────────────────────────────────────────

QString AppSettings::marketDefaultProvider() const
{
    return m_s.value(QStringLiteral("market/default_provider"),
                     QStringLiteral("frankfurter")).toString();
}

void AppSettings::setMarketDefaultProvider(const QString &providerName)
{
    if (m_s.value(QStringLiteral("market/default_provider")).toString() == providerName) return;
    m_s.setValue(QStringLiteral("market/default_provider"), providerName);
    emit marketSettingsChanged();
}

QString AppSettings::brapiToken() const
{
    return m_s.value(QStringLiteral("market/brapi_token")).toString();
}

void AppSettings::setBrapiToken(const QString &token)
{
    if (m_s.value(QStringLiteral("market/brapi_token")).toString() == token) return;
    m_s.setValue(QStringLiteral("market/brapi_token"), token);
    emit marketSettingsChanged();
}

QString AppSettings::brapiPlan() const
{
    return m_s.value(QStringLiteral("market/brapi_plan"),
                     QStringLiteral("Free")).toString();
}

void AppSettings::setBrapiPlan(const QString &plan)
{
    if (m_s.value(QStringLiteral("market/brapi_plan")).toString() == plan) return;
    m_s.setValue(QStringLiteral("market/brapi_plan"), plan);
    emit marketSettingsChanged();
}

QString AppSettings::b3OfficialKeyId() const
{
    return m_s.value(QStringLiteral("market/b3_key_id")).toString();
}

void AppSettings::setB3OfficialKeyId(const QString &keyId)
{
    if (m_s.value(QStringLiteral("market/b3_key_id")).toString() == keyId) return;
    m_s.setValue(QStringLiteral("market/b3_key_id"), keyId);
    emit marketSettingsChanged();
}

int AppSettings::marketPollingInterval() const
{
    return m_s.value(QStringLiteral("market/polling_seconds"), 60).toInt();
}

void AppSettings::setMarketPollingInterval(int seconds)
{
    if (m_s.value(QStringLiteral("market/polling_seconds"), 60).toInt() == seconds) return;
    m_s.setValue(QStringLiteral("market/polling_seconds"), seconds);
    emit marketSettingsChanged();
}

int AppSettings::cotahistAnosHistorico() const
{
    return m_s.value(QStringLiteral("market/cotahist_anos"), 5).toInt();
}

void AppSettings::setCotahistAnosHistorico(int anos)
{
    if (m_s.value(QStringLiteral("market/cotahist_anos"), 5).toInt() == anos) return;
    m_s.setValue(QStringLiteral("market/cotahist_anos"), anos);
    emit marketSettingsChanged();
}

QString AppSettings::currencySlot1() const
{
    return m_s.value(QStringLiteral("market/slot1"), QStringLiteral("USD")).toString();
}

void AppSettings::setCurrencySlot1(const QString &code)
{
    if (m_s.value(QStringLiteral("market/slot1")).toString() == code) return;
    m_s.setValue(QStringLiteral("market/slot1"), code);
    emit marketSettingsChanged();
}

QString AppSettings::currencySlot2() const
{
    return m_s.value(QStringLiteral("market/slot2"), QStringLiteral("EUR")).toString();
}

void AppSettings::setCurrencySlot2(const QString &code)
{
    if (m_s.value(QStringLiteral("market/slot2")).toString() == code) return;
    m_s.setValue(QStringLiteral("market/slot2"), code);
    emit marketSettingsChanged();
}

bool AppSettings::cotahistEnabled() const
{
    return m_s.value(QStringLiteral("market/cotahist_enabled"), false).toBool();
}

void AppSettings::setCotahistEnabled(bool on)
{
    if (m_s.value(QStringLiteral("market/cotahist_enabled"), false).toBool() == on) return;
    m_s.setValue(QStringLiteral("market/cotahist_enabled"), on);
    emit marketSettingsChanged();
}
