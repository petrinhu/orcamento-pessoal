#include "AppSettings.h"
#include "services/ParcelaService.h"
#include "core/secret/SecretStoreService.h"

#include <QStandardPaths>

// App name compartilhado com o cofre do sistema (mesmo usado para o salt
// no LoginDialog). Mantém os segredos do PeuBud agrupados na mesma coleção.
static const QString kSecretAppName = QStringLiteral("orcamento-pessoal");
static const QString kBrapiTokenKey = QStringLiteral("market/brapi_token");

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

QStringList AppSettings::tabOrder() const
{
    return m_s.value(QStringLiteral("tab_order")).toStringList();
}

void AppSettings::setTabOrder(const QStringList &keys)
{
    m_s.setValue(QStringLiteral("tab_order"), keys);
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
    // Backend: cofre do sistema (libsecret). API key não deve ficar em claro
    // em ~/.config. Migração transparente de valores antigos persistidos em
    // QSettings (versões anteriores guardavam em texto claro).
    SecretStoreService store(kSecretAppName);
    const QByteArray fromVault = store.retrieve(kBrapiTokenKey);
    if (!fromVault.isEmpty())
        return QString::fromUtf8(fromVault);

    // Migração lazy: se ainda houver valor legado no QSettings, move-o para o
    // cofre e limpa o QSettings, retornando o valor já migrado.
    const QString legacy =
        m_s.value(kBrapiTokenKey).toString();
    if (!legacy.isEmpty()) {
        if (store.store(kBrapiTokenKey, legacy.toUtf8()))
            m_s.remove(kBrapiTokenKey);   // só apaga após gravar no cofre
        return legacy;
    }

    return {};
}

void AppSettings::setBrapiToken(const QString &token)
{
    if (brapiToken() == token) return;

    SecretStoreService store(kSecretAppName);
    if (token.isEmpty())
        store.remove(kBrapiTokenKey);
    else
        store.store(kBrapiTokenKey, token.toUtf8());

    // Garante que não sobre cópia legada em claro no QSettings.
    m_s.remove(kBrapiTokenKey);

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

// ── Modo Iniciante (#104) ─────────────────────────────────────────────────────

bool AppSettings::modoIniciante() const
{
    return m_s.value(QStringLiteral("ui/modo_iniciante"), true).toBool();
}

void AppSettings::setModoIniciante(bool on)
{
    if (m_s.value(QStringLiteral("ui/modo_iniciante"), true).toBool() == on) return;
    m_s.setValue(QStringLiteral("ui/modo_iniciante"), on);
    emit modoInicianteChanged(on);
}

// ── Onboarding (#109) ─────────────────────────────────────────────────────────

bool AppSettings::onboardingFeito() const
{
    return m_s.value(QStringLiteral("ui/onboarding_feito"), false).toBool();
}

void AppSettings::setOnboardingFeito(bool feito)
{
    m_s.setValue(QStringLiteral("ui/onboarding_feito"), feito);
}

// ── Colapso dos blocos de ajuda do TabShell ─────────────────────────────────────

static QString tabHelpKey(const QString &tabKey, const QString &bloco)
{
    return QStringLiteral("ui/tabhelp/%1/%2").arg(tabKey, bloco);
}

bool AppSettings::tabHelpExpanded(const QString &tabKey, const QString &bloco) const
{
    return m_s.value(tabHelpKey(tabKey, bloco), true).toBool();
}

void AppSettings::setTabHelpExpanded(const QString &tabKey, const QString &bloco, bool expanded)
{
    const QString k = tabHelpKey(tabKey, bloco);
    if (m_s.value(k, true).toBool() == expanded) return;
    m_s.setValue(k, expanded);
}
