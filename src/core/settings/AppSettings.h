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

    // (#181) Caminhos FIXOS — o usuário não escolhe mais a pasta.
    // ~/peubud/save/<slug>/database.pbd (cofre principal)
    // ~/peubud/save/<slug>/database.auto.pbd (autosave — sobrescreve)
    // mkpath da pasta-pai é garantido ao chamar qualquer um desses getters.
    static QString savePathFixo(const QString &slug);
    static QString autosavePathFixo(const QString &slug);

    // (#181) Caminhos legados para migração transparente no open().
    // data/<slug>.pbd (relativo ao CWD — antigo padrão pré-#181)
    static QString saveLegadoData(const QString &slug);

    // Deprecated (#181): callers legados (BackupService usa diretorioPadrao próprio).
    // Mantidos para não quebrar callers externos; internamente não usados mais.
    QString savesFolder() const;
    void    setSavesFolder(const QString &path);
    QString autosavesFolder() const;
    void    setAutosavesFolder(const QString &path);

    // ── Startup tab ───────────────────────────────────────────────────────
    // Persistência por chave estável (ex.: "tab_dashboard"). Migra
    // automaticamente dados legados de int → chave na primeira leitura.
    QString initialTabKey() const;                  // default: "tab_dashboard"
    void    setInitialTabKey(const QString &chave);

    // Legado — mantido apenas para a migração one-time. Não usar em código novo.
    int  initialTab() const;
    void setInitialTab(int index);

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

    // ── Onboarding (#109) ─────────────────────────────────────────────────
    // Flag first-run. Default false; o OnboardingDialog seta para true ao
    // concluir ou pular. Subsequentemente, login nao re-exibe o wizard.
    bool onboardingFeito() const;
    void setOnboardingFeito(bool feito);

    // ── Colapso dos blocos de ajuda do TabShell ───────────────────────────
    // Estado expandido/recolhido por aba (tabKey) e por bloco ("desc" ou
    // "instr"). Chave QSettings: ui/tabhelp/<tabKey>/<bloco>. Default: true
    // (ambos expandidos no 1º uso).
    bool tabHelpExpanded(const QString &tabKey, const QString &bloco) const;
    void setTabHelpExpanded(const QString &tabKey, const QString &bloco, bool expanded);

signals:
    void languageChanged(Language lang);
    void currencyChanged(const QString &code);
    void autosaveSettingsChanged();
    void marketSettingsChanged();
    void modoInicianteChanged(bool on);

private:
    explicit AppSettings();
    // mutable: getters const (ex.: brapiToken) podem migrar/limpar segredos
    // legados do QSettings de forma transparente.
    mutable QSettings m_s;
};
