#pragma once

#include "Theme.h"
#include <QMainWindow>

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
class QTabWidget;
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
    void buildMenuBar();
    void buildThemeBar();
    void applyThemeMode(Theme::ThemeMode mode);
    void updateThemeButton();
    void resetAutosaveTimer();
    void onAutosave();

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

    // Tab order persistence (drag & drop)
    void aplicarOrdemAbasSalva();
    void salvarOrdemAbas();
    void resetarOrdemAbas();              // restaura ordem default (botao ↺)
    QStringList ordemAbasDefault() const; // chaves na ordem default do codigo

    // Atalhos globais de teclado (#80)
    void setupShortcuts();

    // Recebe dados TVM do CalculadoraHub e abre AddCompraDialog na aba Cartões.
    void onTvmParaCartoes(double pv, int n, double iMensal, double pmt);

    DashboardWidget      *m_dashboard     = nullptr;
    TendenciasWidget     *m_tendencias    = nullptr;
    ParcelasWidget       *m_parcelas      = nullptr;
    class TabShell       *m_shellCartoes  = nullptr; // invólucro da aba Cartões
    EntradasWidget       *m_entradas      = nullptr;
    GastosWidget         *m_gastos        = nullptr;
    InvestimentosWidget  *m_investimentos  = nullptr;
    class BolsaWidget    *m_bolsa          = nullptr;
    class MarketDataService *m_marketService = nullptr;
    class ProventosWidget *m_proventos      = nullptr;
    PlanejamentoWidget   *m_planejamento   = nullptr;
    RecorrenciaWidget    *m_recorrencias   = nullptr;
    DividasWidget        *m_dividas        = nullptr;
    MetasWidget          *m_metas          = nullptr;
    LembretesWidget      *m_lembretes      = nullptr;
    BuscaGlobalDialog    *m_buscaGlobal    = nullptr;
    QTabWidget     *m_tabs       = nullptr;
    QLabel         *m_statusUser  = nullptr;
    QLabel         *m_statusTotal = nullptr;

    // Theme cycle button (Auto → Light → Dark → Auto)
    QToolButton *m_btnTheme = nullptr;

    bool         m_dirty         = false;
    QTimer      *m_autosaveTimer = nullptr;
    QUndoStack  *m_undoStack     = nullptr;
};
