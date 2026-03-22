#pragma once

#include "Theme.h"
#include <QMainWindow>

class ParcelasWidget;
class ConfigWidget;
class QTabWidget;
class QLabel;
class QToolButton;
class QAction;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void buildMenuBar();
    void buildThemeBar();
    void applyThemeMode(Theme::ThemeMode mode);
    void updateThemeButton();

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

    // Configurações
    void onCartoes();

    // Ajuda
    void onAjuda();
    void onSobre();

    QTabWidget     *m_tabs       = nullptr;
    ParcelasWidget *m_parcelas   = nullptr;
    ConfigWidget   *m_config     = nullptr;
    QLabel         *m_statusUser = nullptr;

    // Theme cycle button (Auto → Light → Dark → Auto)
    QToolButton *m_btnTheme = nullptr;
};
