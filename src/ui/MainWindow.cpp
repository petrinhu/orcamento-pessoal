#include "MainWindow.h"
#include "widgets/ParcelasWidget.h"
#include "widgets/ConfigWidget.h"
#include "core/session/UserSession.h"

#include <QTabWidget>
#include <QLabel>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Orçamento Pessoal"));
    setMinimumSize(900, 600);
    buildUi();
}

void MainWindow::buildUi()
{
    m_tabs = new QTabWidget(this);
    setCentralWidget(m_tabs);

    m_parcelas = new ParcelasWidget(this);
    m_tabs->addTab(m_parcelas, QStringLiteral("Parcelas"));

    m_config = new ConfigWidget(this);
    m_tabs->addTab(m_config, QStringLiteral("Configurações"));

    // Status bar
    m_statusUser = new QLabel(this);
    const QString name = UserSession::instance().name();
    m_statusUser->setText(QStringLiteral("  Usuário: %1").arg(name));
    statusBar()->addWidget(m_statusUser);

    // Connections
    connect(m_config, &ConfigWidget::cartoesAlterados,
            m_parcelas, &ParcelasWidget::recarregar);
    connect(m_parcelas, &ParcelasWidget::dadosAlterados,
            m_parcelas, &ParcelasWidget::recarregar);
}
