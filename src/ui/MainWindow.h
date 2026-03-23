#pragma once

#include "Theme.h"
#include <QMainWindow>

class ParcelasWidget;
class PrefsWidget;
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

    // Ajuda
    void onConfiguracoes();

    // Ajuda
    void onAjuda();
    void onSobre();

    ParcelasWidget *m_parcelas   = nullptr;
    QLabel         *m_statusUser = nullptr;

    // Theme cycle button (Auto → Light → Dark → Auto)
    QToolButton *m_btnTheme = nullptr;
};
