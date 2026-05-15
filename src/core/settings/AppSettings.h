#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

// Singleton — persists language, currency and save settings across sessions.
// Uses QSettings("petrinhu", "PeuBud").
class AppSettings : public QObject {
    Q_OBJECT
public:
    static AppSettings &instance();

    enum class Language { PtBr, EnIntl };

    Language language() const;
    void     setLanguage(Language lang);

    // ISO 4217 code: "BRL", "USD", "EUR", …
    QString defaultCurrency() const;
    void    setDefaultCurrency(const QString &code);

    // Convenience: display symbol for the current default currency
    QString currencySymbol() const;

    // ── Market Data Settings ──────────────────────────────────────────────
    QString marketDefaultProvider() const;
    void    setMarketDefaultProvider(const QString &providerName);
    QString brapiToken() const;
    void    setBrapiToken(const QString &token);
    QString brapiPlan() const;  // "Free", "Startup", "Pro"
    void    setBrapiPlan(const QString &plan);
    QString b3OfficialKeyId() const;
    void    setB3OfficialKeyId(const QString &keyId);
    int     marketPollingInterval() const;  // seconds
    void    setMarketPollingInterval(int seconds);
    int     cotahistAnosHistorico() const;  // years of historical data to download
    void    setCotahistAnosHistorico(int anos);

    // ── Save / autosave settings ──────────────────────────────────────────
    bool    autosaveEnabled() const;               // default: true
    void    setAutosaveEnabled(bool on);
    int     autosaveIntervalMinutes() const;        // default: 5
    void    setAutosaveIntervalMinutes(int minutes);
    QString savesFolder() const;                    // default: Documents/PeuBud
    void    setSavesFolder(const QString &path);
    QString autosavesFolder() const;                // default: savesFolder()/autosaves
    void    setAutosavesFolder(const QString &path);

    // ── Startup tab ───────────────────────────────────────────────────────
    int  initialTab() const;          // default: 0 (Dashboard)
    void setInitialTab(int index);

    // ── Tab order (drag & drop persistence) ───────────────────────────────
    // Lista de chaves (objectName de cada widget) na ordem desejada.
    // Vazia = ordem default do código (sem persistência aplicada).
    QStringList tabOrder() const;
    void        setTabOrder(const QStringList &keys);

    // ── Painel câmbio (#46/#47): slots de moeda para o painel LED ────────
    QString currencySlot1() const;            // default: USD
    void    setCurrencySlot1(const QString &code);
    QString currencySlot2() const;            // default: EUR
    void    setCurrencySlot2(const QString &code);

    // ── COTAHIST B3 (#46/#59) ────────────────────────────────────────────
    bool    cotahistEnabled() const;          // default: false
    void    setCotahistEnabled(bool on);

    // ── Modo Iniciante (#104) ─────────────────────────────────────────────
    // Default: true (iniciante). Aplica labels coloquiais + esconde campos
    // avancados em wizards complexos.
    bool modoIniciante() const;
    void setModoIniciante(bool on);

signals:
    void languageChanged(Language lang);
    void currencyChanged(const QString &code);
    void autosaveSettingsChanged();
    void marketSettingsChanged();
    void modoInicianteChanged(bool on);

private:
    explicit AppSettings();
    QSettings m_s;
};
