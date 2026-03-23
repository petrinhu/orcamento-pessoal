#pragma once

#include "Theme.h"
#include <QMainWindow>

class ParcelasWidget;
class EntradasWidget;
class GastosWidget;
class PrefsWidget;
class QTabWidget;
class QLabel;
class QToolButton;
class QAction;
class QTimer;
class QCloseEvent;

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
    void onSobre();

    ParcelasWidget *m_parcelas   = nullptr;
    EntradasWidget *m_entradas   = nullptr;
    GastosWidget   *m_gastos     = nullptr;
    QTabWidget     *m_tabs       = nullptr;
    QLabel         *m_statusUser  = nullptr;
    QLabel         *m_statusTotal = nullptr;

    // Theme cycle button (Auto → Light → Dark → Auto)
    QToolButton *m_btnTheme = nullptr;

    bool    m_dirty         = false;
    QTimer *m_autosaveTimer = nullptr;
};
