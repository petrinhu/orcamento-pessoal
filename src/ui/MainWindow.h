#pragma once

#include <QMainWindow>

class ParcelasWidget;
class ConfigWidget;
class QTabWidget;
class QLabel;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();

    QTabWidget     *m_tabs     = nullptr;
    ParcelasWidget *m_parcelas = nullptr;
    ConfigWidget   *m_config   = nullptr;
    QLabel         *m_statusUser = nullptr;
};
