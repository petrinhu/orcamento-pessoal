#pragma once

#include "LazyPageRouter.h"
#include "Theme.h"
#include <QMainWindow>
#include <QMap>

class DashboardWidget;
class TendenciasWidget;
class ParcelasWidget;
class EntradasWidget;
class GastosWidget;
class InvestimentosWidget;
class PlanejamentoWidget;
class RecorrenciaWidget;
class DividasWidget;
class MetasWidget;
class LembretesWidget;
class BuscaGlobalDialog;
class PrefsWidget;
class QStackedWidget;
class SidebarNav;
class QLabel;
class QToolButton;
class QAction;
class QTimer;
class QCloseEvent;
class QUndoStack;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void buildMenuBar();   // mantida vazia — preserva atalhos via this->addAction()
    void buildUtilBar();   // cluster utilidades topo-direito (Busca/Calc/Mais)
    void applyThemeMode(Theme::ThemeMode mode);
    void resetAutosaveTimer();
    void onAutosave();

    // Lazy-init por aba (#208, PERF-02): registra as 13 fábricas em
    // m_router. Cada fábrica só roda na 1ª navegação para a chave — constrói
    // conteúdo+TabShell, faz o wiring cross-widget daquela aba e insere no
    // QStackedWidget. Ver AUDITORIA_2026-06-22.md §3.5.
    void registrarFabricasAbas();
    class TabShell *criarShell(const QString &objName, const QString &tabKey,
                                QWidget *conteudo);
    // Glossário escopado à aba recém-construída (reduz o N do PERF-01) — em
    // vez de rodar 1x sobre a MainWindow inteira (13 abas), roda 1x por aba
    // no momento em que ela é materializada.
    void aplicarGlossarioNaAba(QWidget *shell);

    // Refresh cross-widget "se a aba-alvo estiver visível" (#208): extraídos
    // p/ métodos porque múltiplas fábricas de aba conectam ao mesmo slot.
    void refreshDashboardSeAtivo();
    void refreshPlanejamentoSeAtivo();
    void refreshTendenciasSeAtivo();

    // (#178) Garante que `dir` existe antes de gravar. Se não existe, exibe
    // QMessageBox com [Cancelar] / [Criar pasta]. Retorna true se a pasta
    // existe (ou foi criada); false se o usuário cancelou ou mkpath falhou.
    // NÃO usar no autosave (sem diálogo por ciclo) — use apenas no save manual.
    bool garantirPastaExiste(const QString &dir);

    // Arquivo
    void onAbrir();
    void onSalvar();
    void onSalvarComo();
    void onImportar(const QString &format);
    void onExportar(const QString &format);

    // Editar
    void onCopiar();
    void onColar();
    void onSelecionarTudo();

    // Configurações / Ajuda
    void onConfiguracoes();
    void onAjuda();
    void onGlossario();
    void onSobre();

    void onReconciliar();
    void onBuscaGlobal();
    void onBloquear();

    // Navegação por chave estável (#165)
    void navegarParaChave(const QString &chave);

    // Atalhos globais de teclado (#80)
    void setupShortcuts();

    // Recebe dados TVM do CalculadoraHub e abre AddCompraDialog na aba Cartões.
    void onTvmParaCartoes(double pv, int n, double iMensal, double pmt);

    // Cada página é um widget de conteúdo (m_<x>) envolvido por um TabShell
    // (m_shell<X>). O shell é o widget efetivamente inserido no stack e carrega
    // o objectName de persistência (tab_<x>); o widget de conteúdo
    // segue intacto (lógica e conexões inalteradas).
    DashboardWidget      *m_dashboard      = nullptr;
    class TabShell       *m_shellDashboard = nullptr;
    TendenciasWidget     *m_tendencias     = nullptr;
    class TabShell       *m_shellTendencias = nullptr;
    ParcelasWidget       *m_parcelas       = nullptr;
    class TabShell       *m_shellCartoes   = nullptr;
    EntradasWidget       *m_entradas       = nullptr;
    class TabShell       *m_shellEntradas  = nullptr;
    GastosWidget         *m_gastos         = nullptr;
    class TabShell       *m_shellGastos    = nullptr;
    InvestimentosWidget  *m_investimentos  = nullptr;
    class TabShell       *m_shellInvestimentos = nullptr;
    class BolsaWidget    *m_bolsa          = nullptr;
    class TabShell       *m_shellBolsa     = nullptr;
    class MarketDataService *m_marketService = nullptr;
    class ProventosWidget *m_proventos     = nullptr;
    class TabShell       *m_shellProventos = nullptr;
    PlanejamentoWidget   *m_planejamento   = nullptr;
    class TabShell       *m_shellPlanejamento = nullptr;
    RecorrenciaWidget    *m_recorrencias   = nullptr;
    class TabShell       *m_shellRecorrencias = nullptr;
    DividasWidget        *m_dividas        = nullptr;
    class TabShell       *m_shellDividas   = nullptr;
    MetasWidget          *m_metas          = nullptr;
    class TabShell       *m_shellMetas     = nullptr;
    LembretesWidget      *m_lembretes      = nullptr;
    class TabShell       *m_shellLembretes = nullptr;
    BuscaGlobalDialog    *m_buscaGlobal    = nullptr;
    QStackedWidget *m_stack      = nullptr;
    SidebarNav     *m_sidebar    = nullptr;
    LazyPageRouter  m_router; // (#208) substitui o antigo QMap<chave, QWidget*> m_paginas
    QLabel         *m_statusTotal = nullptr;

    bool         m_dirty              = false;
    bool         m_autosaveAvisado    = false; // (#178) avisa falha de pasta uma vez por sessão
    QTimer      *m_autosaveTimer      = nullptr;
    QUndoStack  *m_undoStack          = nullptr;
};
