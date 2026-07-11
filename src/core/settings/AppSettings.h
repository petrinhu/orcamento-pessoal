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

    // (#192/DIST-03/SEC-02) Diretório FIXO (QStandardPaths::AppDataLocation
    // — $XDG_DATA_HOME/<org>/<app> no Linux) onde o .db em claro de trabalho
    // vive durante a sessão. Antes era "data/" relativo ao CWD do processo:
    // o `.desktop` do pacote não seta `Path=`, então o lançamento via menu
    // tinha CWD imprevisível — quebrava 100% na 1ª execução do pacote
    // instalado. mkpath + chmod 0700 são garantidos a cada chamada
    // (idempotente).
    static QString workingDbDir();
    // (#192) workingDbDir() + "/.<slug>.db" — mesmo nome de arquivo de
    // antes, só o diretório-base deixou de ser relativo ao CWD.
    static QString workingDbPath(const QString &slug);

    // (ADR-001 #212 §5.1) Registro de usuários da INSTALAÇÃO inteira — ao
    // contrário de savePathFixo/workingDbPath (que são por-usuário/slug),
    // este é um único arquivo para todos os usuários da máquina. Reusa
    // workingDbDir() (mesmo diretório-base 0700, mkpath já garantido ali).
    static QString userRegistryPath();

    // (ADR-001 §6/D6, Fatia 4) Fonte ÚNICA de verdade do slug — usada tanto
    // para derivar o path do cofre (savePathFixo/workingDbPath) quanto por
    // UserSession::slug() e UserAccountService::slugify() (ambos delegam
    // aqui). NFD decompõe "ã" em 'a' + combining tilde; removemos as marcas
    // de combinação (Mark_NonSpacing) ANTES do filtro [a-z0-9_] — "João" ->
    // "joao" (não "joo", o bug de UserSession::slug() pré-Fatia 4, que só
    // removia caracteres fora de a-z0-9_ sem decompor Unicode primeiro).
    static QString slugify(const QString &username);

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

    // ── Densidade das tabelas (redesign #167 T1, §11 manual de estilo) ────
    // Default: false (Confortável, ~48px/linha). true = Compacta (~34px/linha).
    // Toggle em Preferências > Geral > Apresentação. Aplica AO VIVO — o
    // caller (PrefsWidget) também chama Theme::setDensidadeTabelasCompacta()
    // + Theme::aplicarDensidadeTabelas() no mesmo handler (não precisa
    // reabrir o app, diferente do Modo Iniciante).
    bool tabelaDensidadeCompacta() const;
    void setTabelaDensidadeCompacta(bool compacta);

signals:
    void languageChanged(Language lang);
    void currencyChanged(const QString &code);
    void autosaveSettingsChanged();
    void marketSettingsChanged();
    void modoInicianteChanged(bool on);
    void tabelaDensidadeChanged(bool compacta);

private:
    explicit AppSettings();
    // mutable: getters const (ex.: brapiToken) podem migrar/limpar segredos
    // legados do QSettings de forma transparente.
    mutable QSettings m_s;
};
