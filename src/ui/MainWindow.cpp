#include "MainWindow.h"
#include "widgets/DashboardWidget.h"
#include "widgets/TendenciasWidget.h"
#include "widgets/ParcelasWidget.h"
#include "widgets/EntradasWidget.h"
#include "widgets/GastosWidget.h"
#include "widgets/InvestimentosWidget.h"
#include "widgets/PlanejamentoWidget.h"
#include "widgets/RecorrenciaWidget.h"
#include "widgets/DividasWidget.h"
#include "widgets/MetasWidget.h"
#include "widgets/LembretesWidget.h"
#include "widgets/BuscaGlobalDialog.h"
#include "widgets/ReconciliacaoDialog.h"
#include "widgets/PrefsWidget.h"
#include "services/RecorrenciaService.h"
#include "widgets/AboutDialog.h"
#include "widgets/HelpDialog.h"
#include "core/session/UserSession.h"
#include "core/database/DatabaseManager.h"
#include "core/settings/AppSettings.h"
#include "services/ImportService.h"
#include "services/ExportService.h"
#include "widgets/ExportDialog.h"
#include "repositories/CompraRepository.h"
#include "repositories/ParcelaRepository.h"
#include "repositories/LancamentoEntradaRepository.h"
#include "repositories/GastoAvulsoRepository.h"
#include "repositories/LancamentoInvestimentoRepository.h"
#include "repositories/OrcamentoRepository.h"
#include "repositories/TransacaoRecorrenteRepository.h"
#include "repositories/DividaRepository.h"
#include "repositories/MetaRepository.h"
#include "repositories/LembreteRepository.h"
#include "models/Cartao.h"
#include "models/Lembrete.h"
#include "core/CalculadoraHub.h"

#include <QLabel>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QStatusBar>
#include <QToolButton>
#include <QHBoxLayout>
#include <QWidget>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QSqlQuery>
#include <QPushButton>
#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QCloseEvent>
#include <QToolBar>
#include <QUndoStack>
#include <QShortcut>
#include <QKeySequence>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("PeuBud — Orçamento Pessoal"));
    setMinimumSize(960, 620);
    buildUi();
    setupShortcuts();
}

// ── Atalhos globais (#80) ─────────────────────────────────────────────────────
// Ctrl+1..9 muda para aba correspondente (0..8 no QTabWidget).
// Ctrl+, abre Configurações.
// Ctrl+F abre Busca Global.
// Ctrl+B bloqueia a sessão (fecha app; libsecret/KWallet reabre na próxima).
void MainWindow::setupShortcuts()
{
    // Ctrl+1..9 → trocar de aba
    for (int i = 1; i <= 9; ++i) {
        const auto seq = QKeySequence(QStringLiteral("Ctrl+%1").arg(i));
        auto *sc = new QShortcut(seq, this);
        sc->setContext(Qt::ApplicationShortcut);
        const int tabIdx = i - 1;
        connect(sc, &QShortcut::activated, this, [this, tabIdx]() {
            if (tabIdx < m_tabs->count())
                m_tabs->setCurrentIndex(tabIdx);
        });
    }

    // Ctrl+, → Configurações
    auto *scCfg = new QShortcut(QKeySequence(QStringLiteral("Ctrl+,")), this);
    scCfg->setContext(Qt::ApplicationShortcut);
    connect(scCfg, &QShortcut::activated, this, &MainWindow::onConfiguracoes);

    // Ctrl+F → Busca Global
    auto *scFind = new QShortcut(QKeySequence::Find, this);
    scFind->setContext(Qt::ApplicationShortcut);
    connect(scFind, &QShortcut::activated, this, &MainWindow::onBuscaGlobal);

    // Ctrl+B → Bloquear sessão
    auto *scLock = new QShortcut(QKeySequence(QStringLiteral("Ctrl+B")), this);
    scLock->setContext(Qt::ApplicationShortcut);
    connect(scLock, &QShortcut::activated, this, &MainWindow::onBloquear);
}

void MainWindow::onBloquear()
{
    // Fecha a janela principal: closeEvent já trata dirty/autosave e cifra banco
    // pelo destructor de DatabaseManager. Próxima abertura exige login.
    close();
}

void MainWindow::buildUi()
{
    m_undoStack = new QUndoStack(this);
    m_undoStack->setUndoLimit(50);

    m_dashboard   = new DashboardWidget(this);
    m_tendencias  = new TendenciasWidget(this);

    m_parcelas      = new ParcelasWidget(this);
    m_entradas      = new EntradasWidget(this);
    m_gastos        = new GastosWidget(this);
    m_investimentos = new InvestimentosWidget(this);
    m_planejamento  = new PlanejamentoWidget(this);
    m_recorrencias  = new RecorrenciaWidget(this);
    m_dividas       = new DividasWidget(this);
    m_metas         = new MetasWidget(this);
    m_lembretes     = new LembretesWidget(this);
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(m_dashboard,     tr("Dashboard"));      // 0
    m_tabs->addTab(m_entradas,      tr("Entradas"));       // 1
    m_tabs->addTab(m_gastos,        tr("Gastos"));         // 2
    m_tabs->addTab(m_tendencias,    tr("Tendências"));     // 3
    m_tabs->addTab(m_investimentos, tr("Investimentos"));  // 4
    m_tabs->addTab(m_planejamento,  tr("Planejamento"));   // 5
    m_tabs->addTab(m_recorrencias,  tr("Recorrências"));   // 6
    m_tabs->addTab(m_dividas,       tr("Dívidas"));        // 7
    m_tabs->addTab(m_metas,         tr("Metas"));          // 8
    m_tabs->addTab(m_lembretes,     tr("Lembretes"));      // 9
    m_tabs->addTab(m_parcelas,      tr("Cartões"));        // 10
    m_tabs->setCurrentIndex(AppSettings::instance().initialTab());
    setCentralWidget(m_tabs);

    // Pass undo stack to data widgets
    m_parcelas->setUndoStack(m_undoStack);
    m_entradas->setUndoStack(m_undoStack);
    m_gastos->setUndoStack(m_undoStack);
    m_investimentos->setUndoStack(m_undoStack);
    m_planejamento->setUndoStack(m_undoStack);
    m_recorrencias->setUndoStack(m_undoStack);
    m_dividas->setUndoStack(m_undoStack);
    m_metas->setUndoStack(m_undoStack);
    m_lembretes->setUndoStack(m_undoStack);

    buildMenuBar();
    buildThemeBar();

    // ── Status bar ─────────────────────────────────────────────────────────
    m_statusUser = new QLabel(this);
    const QString name = UserSession::instance().name();
    m_statusUser->setText(tr("  Usuário: %1").arg(name));
    statusBar()->addWidget(m_statusUser);

    m_statusTotal = new QLabel(this);
    m_statusTotal->setObjectName(QStringLiteral("subtitle"));
    statusBar()->addPermanentWidget(m_statusTotal);

    // ── TVM calculator → Cartões tab ───────────────────────────────────────
    connect(&CalculadoraHub::instance(), &CalculadoraHub::tvmParaCartoes,
            this, &MainWindow::onTvmParaCartoes);

    // ── Dirty flag ─────────────────────────────────────────────────────────
    // ── Dirty flag + status bar ────────────────────────────────────────────
    connect(m_dashboard, &DashboardWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_dashboard)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });

    connect(m_parcelas, &ParcelasWidget::dadosAlterados,
            m_parcelas, &ParcelasWidget::recarregar);
    connect(m_parcelas, &ParcelasWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_entradas, &EntradasWidget::dadosAlterados,
            m_entradas, &EntradasWidget::recarregar);
    connect(m_entradas, &EntradasWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_gastos, &GastosWidget::dadosAlterados,
            m_gastos, &GastosWidget::recarregar);
    connect(m_gastos, &GastosWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_investimentos, &InvestimentosWidget::dadosAlterados,
            m_investimentos, &InvestimentosWidget::recarregar);
    connect(m_investimentos, &InvestimentosWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_planejamento, &PlanejamentoWidget::dadosAlterados,
            m_planejamento, &PlanejamentoWidget::recarregar);
    connect(m_planejamento, &PlanejamentoWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_recorrencias, &RecorrenciaWidget::dadosAlterados,
            m_recorrencias, &RecorrenciaWidget::recarregar);
    connect(m_recorrencias, &RecorrenciaWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_dividas, &DividasWidget::dadosAlterados,
            m_dividas, &DividasWidget::recarregar);
    connect(m_dividas, &DividasWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_metas, &MetasWidget::dadosAlterados,
            m_metas, &MetasWidget::recarregar);
    connect(m_metas, &MetasWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_lembretes, &LembretesWidget::dadosAlterados,
            m_lembretes, &LembretesWidget::recarregar);
    connect(m_lembretes, &LembretesWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });

    // Tendências se atualiza quando Gastos muda dados (lazy: só se a aba estiver visível)
    connect(m_gastos, &GastosWidget::dadosAlterados, this, [this]() {
        if (m_tabs->currentWidget() == m_tendencias)
            m_tendencias->recarregar();
    });

    // Dashboard se atualiza quando qualquer aba muda dados
    auto refreshDash = [this]() {
        if (m_tabs->currentWidget() == m_dashboard)
            m_dashboard->recarregar();
    };
    connect(m_entradas,      &EntradasWidget::dadosAlterados,      this, refreshDash);
    connect(m_gastos,        &GastosWidget::dadosAlterados,        this, refreshDash);
    connect(m_parcelas,      &ParcelasWidget::dadosAlterados,      this, refreshDash);
    connect(m_investimentos, &InvestimentosWidget::dadosAlterados, this, refreshDash);
    connect(m_recorrencias,  &RecorrenciaWidget::dadosAlterados,   this, refreshDash);

    // Planejamento se atualiza quando Gastos, Entradas ou Cartões mudam
    auto refreshPlanning = [this]() {
        if (m_tabs->currentWidget() == m_planejamento)
            m_planejamento->recarregar();
    };
    connect(m_gastos,    &GastosWidget::dadosAlterados,   this, refreshPlanning);
    connect(m_entradas,  &EntradasWidget::dadosAlterados,  this, refreshPlanning);
    connect(m_parcelas,  &ParcelasWidget::dadosAlterados,  this, refreshPlanning);

    // Dashboard cards clicáveis → navegar para a aba correspondente
    connect(m_dashboard, &DashboardWidget::navegarParaAba,
            m_tabs,      &QTabWidget::setCurrentIndex);

    // Status bar totals
    connect(m_parcelas, &ParcelasWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_parcelas)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_entradas, &EntradasWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_entradas)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_gastos, &GastosWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_gastos)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_investimentos, &InvestimentosWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_investimentos)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_planejamento, &PlanejamentoWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_planejamento)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_dividas, &DividasWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_dividas)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_metas, &MetasWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_metas)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_lembretes, &LembretesWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_lembretes)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_tabs, &QTabWidget::currentChanged, this, [this]() {
        if      (m_tabs->currentWidget() == m_dashboard)     m_dashboard->recarregar();
        else if (m_tabs->currentWidget() == m_entradas)      m_entradas->recarregar();
        else if (m_tabs->currentWidget() == m_gastos)        m_gastos->recarregar();
        else if (m_tabs->currentWidget() == m_tendencias)    m_tendencias->recarregar();
        else if (m_tabs->currentWidget() == m_investimentos) m_investimentos->recarregar();
        else if (m_tabs->currentWidget() == m_planejamento)  m_planejamento->recarregar();
        else if (m_tabs->currentWidget() == m_recorrencias)  m_recorrencias->recarregar();
        else if (m_tabs->currentWidget() == m_dividas)        m_dividas->recarregar();
        else if (m_tabs->currentWidget() == m_metas)          m_metas->recarregar();
        else if (m_tabs->currentWidget() == m_lembretes)      m_lembretes->recarregar();
        else if (m_tabs->currentWidget() == m_parcelas)       m_parcelas->recarregar();
    });

    // ── Gerar recorrências pendentes ao abrir o app ─────────────────────────
    const int geradas = RecorrenciaService().gerarPendentes(QDate::currentDate());
    if (geradas > 0) {
        m_entradas->recarregar();
        m_gastos->recarregar();
        m_recorrencias->recarregar();
        m_planejamento->recarregar();
        QTimer::singleShot(800, this, [this, geradas]() {
            m_statusTotal->setText(
                tr("%1 recorrência(s) gerada(s) automaticamente  ").arg(geradas));
        });
    }

    // ── Alerta de lembretes urgentes ao abrir o app ────────────────────────
    QTimer::singleShot(500, this, [this]() {
        const auto ativos = LembreteRepository().findAtivos();
        QStringList manuaisVencidos, debNaoFeitos, debProximos;

        for (const Lembrete &l : ativos) {
            if (l.naoPago()) {
                const QString txt = l.valor > 0
                    ? tr("• %1 — R$ %2,%3")
                        .arg(l.descricao)
                        .arg(l.valor / 100)
                        .arg(l.valor % 100, 2, 10, QLatin1Char('0'))
                    : tr("• ") + l.descricao;
                debNaoFeitos << txt;
            } else if (l.vencido()) {
                const QString txt = l.debitoAutomatico
                    ? tr("• %1 (déb. auto, vencido em %2)")
                        .arg(l.descricao, QDate::fromString(l.dataVenc, Qt::ISODate)
                             .toString(QStringLiteral("dd/MM")))
                    : tr("• %1 (vencido em %2)")
                        .arg(l.descricao, QDate::fromString(l.dataVenc, Qt::ISODate)
                             .toString(QStringLiteral("dd/MM")));
                manuaisVencidos << txt;
            } else if (l.proximo() && l.debitoAutomatico) {
                const int dias = l.diasParaVencer();
                const QString txt = dias == 0
                    ? tr("• %1 — hoje").arg(l.descricao)
                    : tr("• %1 — em %2 dia(s)").arg(l.descricao).arg(dias);
                debProximos << txt;
            }
        }

        const bool temAlerta = !manuaisVencidos.isEmpty()
                               || !debNaoFeitos.isEmpty();
        const bool temInfo   = !debProximos.isEmpty();

        if (!temAlerta && !temInfo) return;

        QString msg;
        if (!manuaisVencidos.isEmpty())
            msg += tr("<b>Vencidos / a pagar:</b><br>")
                 + manuaisVencidos.join(QStringLiteral("<br>"))
                 + QStringLiteral("<br><br>");
        if (!debNaoFeitos.isEmpty())
            msg += tr("<b>Débitos automáticos não realizados:</b><br>")
                 + debNaoFeitos.join(QStringLiteral("<br>"))
                 + QStringLiteral("<br><br>");
        if (!debProximos.isEmpty())
            msg += tr("<b>Débitos automáticos próximos (≤ 7 dias):</b><br>")
                 + debProximos.join(QStringLiteral("<br>"));

        auto *dlg = new QMessageBox(
            temAlerta ? QMessageBox::Warning : QMessageBox::Information,
            tr("Lembretes"), msg,
            QMessageBox::Ok, this);
        dlg->setTextFormat(Qt::RichText);
        auto *btnIr = dlg->addButton(tr("Ver lembretes"), QMessageBox::ActionRole);
        dlg->exec();
        if (dlg->clickedButton() == btnIr)
            m_tabs->setCurrentIndex(9); // aba Lembretes
    });

    // ── Auto-save timer ────────────────────────────────────────────────────
    m_autosaveTimer = new QTimer(this);
    connect(m_autosaveTimer, &QTimer::timeout, this, &MainWindow::onAutosave);
    connect(&AppSettings::instance(), &AppSettings::autosaveSettingsChanged,
            this, &MainWindow::resetAutosaveTimer);
    resetAutosaveTimer();
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar()
{
    QMenuBar *mb = menuBar();

    // ── Arquivo ────────────────────────────────────────────────────────────
    QMenu *mArquivo = mb->addMenu(tr("&Arquivo"));

    auto *aAbrir = mArquivo->addAction(tr("&Abrir…"));
    aAbrir->setShortcut(QKeySequence::Open);
    connect(aAbrir, &QAction::triggered, this, &MainWindow::onAbrir);

    auto *aSalvar = mArquivo->addAction(tr("&Salvar"));
    aSalvar->setShortcut(QKeySequence::Save);
    connect(aSalvar, &QAction::triggered, this, &MainWindow::onSalvar);

    auto *aSalvarComo = mArquivo->addAction(tr("Salvar &como…"));
    aSalvarComo->setShortcut(QKeySequence::SaveAs);
    connect(aSalvarComo, &QAction::triggered, this, &MainWindow::onSalvarComo);

    mArquivo->addSeparator();

    // Importar submenu
    QMenu *mImportar = mArquivo->addMenu(tr("&Importar de"));
    const QList<QPair<QString,QString>> importFormats = {
        { QStringLiteral("CSV (.csv)…"),        QStringLiteral("csv")  },
        { QStringLiteral("OFX / QFX (.ofx)…"),  QStringLiteral("ofx")  },
        { QStringLiteral("QIF (.qif)…"),         QStringLiteral("qif")  },
        { QStringLiteral("XLSX / XLS (.xlsx)…"), QStringLiteral("xlsx") },
        { QStringLiteral("ODS (.ods)…"),         QStringLiteral("ods")  },
        { QStringLiteral("TXT / CNAB (.txt)…"),  QStringLiteral("txt")  },
        { QStringLiteral("JSON (.json)…"),       QStringLiteral("json") },
        { QStringLiteral("Markdown (.md)…"),     QStringLiteral("md")   },
        { QStringLiteral("HTML (.html)…"),       QStringLiteral("html") },
        { QStringLiteral("PDF (.pdf)…"),         QStringLiteral("pdf")  },
    };
    for (const auto &[label, fmt] : importFormats) {
        auto *act = mImportar->addAction(label);
        const QString f = fmt;
        connect(act, &QAction::triggered, this, [this, f]() { onImportar(f); });
    }

    // Exportar submenu
    QMenu *mExportar = mArquivo->addMenu(tr("&Exportar"));
    const QList<QPair<QString,QString>> exportFormats = {
        { QStringLiteral("CSV (.csv)…"),        QStringLiteral("csv")  },
        { QStringLiteral("OFX (.ofx)…"),        QStringLiteral("ofx")  },
        { QStringLiteral("QIF (.qif)…"),        QStringLiteral("qif")  },
        { QStringLiteral("XLSX (.xlsx)…"),      QStringLiteral("xlsx") },
        { QStringLiteral("XLS (.xls)…"),        QStringLiteral("xls")  },
        { QStringLiteral("ODS (.ods)…"),        QStringLiteral("ods")  },
        { QStringLiteral("TXT (.txt)…"),        QStringLiteral("txt")  },
        { QStringLiteral("JSON (.json)…"),      QStringLiteral("json") },
        { QStringLiteral("Markdown (.md)…"),    QStringLiteral("md")   },
        { QStringLiteral("HTML (.html)…"),      QStringLiteral("html") },
        { QStringLiteral("PDF (.pdf)…"),        QStringLiteral("pdf")  },
    };
    for (const auto &[label, fmt] : exportFormats) {
        auto *act = mExportar->addAction(label);
        const QString f = fmt;
        connect(act, &QAction::triggered, this, [this, f]() { onExportar(f); });
    }

    mArquivo->addSeparator();

    auto *aReconciliar = mArquivo->addAction(tr("Reconciliar &extrato…"));
    connect(aReconciliar, &QAction::triggered, this, &MainWindow::onReconciliar);

    mArquivo->addSeparator();

    auto *aSair = mArquivo->addAction(tr("&Sair"));
    aSair->setShortcut(QKeySequence::Quit);
    connect(aSair, &QAction::triggered, qApp, &QApplication::quit);

    // ── Editar ─────────────────────────────────────────────────────────────
    QMenu *mEditar = mb->addMenu(tr("&Editar"));

    auto *aUndo = m_undoStack->createUndoAction(this, tr("&Desfazer"));
    aUndo->setShortcut(QKeySequence::Undo);
    mEditar->addAction(aUndo);

    auto *aRedo = m_undoStack->createRedoAction(this, tr("&Refazer"));
    aRedo->setShortcut(QKeySequence::Redo);
    mEditar->addAction(aRedo);

    mEditar->addSeparator();

    auto *aCopiar = mEditar->addAction(tr("&Copiar"));
    aCopiar->setShortcut(QKeySequence::Copy);
    connect(aCopiar, &QAction::triggered, this, &MainWindow::onCopiar);

    auto *aColar = mEditar->addAction(tr("Co&lar"));
    aColar->setShortcut(QKeySequence::Paste);
    connect(aColar, &QAction::triggered, this, &MainWindow::onColar);

    mEditar->addSeparator();

    auto *aSelAll = mEditar->addAction(tr("Selecionar &tudo"));
    aSelAll->setShortcut(QKeySequence::SelectAll);
    connect(aSelAll, &QAction::triggered, this, &MainWindow::onSelecionarTudo);

    mEditar->addSeparator();

    auto *aBusca = mEditar->addAction(tr("&Buscar transação…"));
    aBusca->setShortcut(QKeySequence::Find);
    connect(aBusca, &QAction::triggered, this, &MainWindow::onBuscaGlobal);

    // ── Configurações ──────────────────────────────────────────────────────
    QMenu *mConfig = mb->addMenu(tr("&Configurações"));

    auto *aPrefs = mConfig->addAction(tr("&Preferências…"));
    connect(aPrefs, &QAction::triggered, this, &MainWindow::onConfiguracoes);

    // ── Ajuda ──────────────────────────────────────────────────────────────
    QMenu *mAjuda = mb->addMenu(tr("A&juda"));

    auto *aAjuda = mAjuda->addAction(tr("&Ajuda"));
    aAjuda->setShortcut(QKeySequence::HelpContents);
    connect(aAjuda, &QAction::triggered, this, &MainWindow::onAjuda);

    mAjuda->addSeparator();

    auto *aSobre = mAjuda->addAction(tr("&Sobre PeuBud…"));
    connect(aSobre, &QAction::triggered, this, &MainWindow::onSobre);
}

// ── Theme bar ─────────────────────────────────────────────────────────────────

void MainWindow::buildThemeBar()
{
    // ── Navegação de abas — corner do QTabWidget ───────────────────────────
    auto *navCorner = new QWidget(this);
    auto *navLay    = new QHBoxLayout(navCorner);
    navLay->setContentsMargins(0, 0, 4, 0);
    navLay->setSpacing(1);

    auto makeNavBtn = [&](const char *themeIcon, const QString &fallback,
                          const QString &tip) -> QToolButton * {
        auto *btn = new QToolButton(navCorner);
        btn->setAutoRaise(true);
        btn->setToolTip(tip);
        const QIcon icon = QIcon::fromTheme(QLatin1String(themeIcon));
        if (icon.isNull()) {
            btn->setText(fallback);
            btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        } else {
            btn->setIcon(icon);
            btn->setIconSize({16, 16});
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        }
        navLay->addWidget(btn);
        return btn;
    };

    auto *btnFirst = makeNavBtn("media-skip-backward", QStringLiteral("⏮"), tr("Primeira aba"));
    auto *btnPrev  = makeNavBtn("media-seek-backward", QStringLiteral("◀"), tr("Aba anterior"));
    auto *btnNext  = makeNavBtn("media-seek-forward",  QStringLiteral("▶"), tr("Próxima aba"));
    auto *btnLast  = makeNavBtn("media-skip-forward",  QStringLiteral("⏭"), tr("Última aba"));

    connect(btnFirst, &QToolButton::clicked, this, [this]() { m_tabs->setCurrentIndex(0); });
    connect(btnPrev,  &QToolButton::clicked, this, [this]() {
        m_tabs->setCurrentIndex(qMax(0, m_tabs->currentIndex() - 1));
    });
    connect(btnNext,  &QToolButton::clicked, this, [this]() {
        m_tabs->setCurrentIndex(qMin(m_tabs->count() - 1, m_tabs->currentIndex() + 1));
    });
    connect(btnLast,  &QToolButton::clicked, this, [this]() { m_tabs->setCurrentIndex(m_tabs->count() - 1); });

    m_tabs->setCornerWidget(navCorner, Qt::TopRightCorner);

    // ── Toolbar principal — abaixo dos menus ──────────────────────────────
    auto *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize({20, 20});
    addToolBar(Qt::TopToolBarArea, toolbar);

    auto makeBtn = [&](const char *themeIcon, const QString &fallback,
                       const QString &tip, bool enabled = true) -> QToolButton * {
        auto *btn = new QToolButton(toolbar);
        btn->setAutoRaise(true);
        btn->setEnabled(enabled);
        btn->setToolTip(tip);
        const QIcon icon = QIcon::fromTheme(QLatin1String(themeIcon));
        if (icon.isNull()) {
            btn->setText(fallback);
            btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        } else {
            btn->setIcon(icon);
            btn->setIconSize({20, 20});
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        }
        toolbar->addWidget(btn);
        return btn;
    };

    // Arquivo
    auto *btnAbrir      = makeBtn("document-open",    QStringLiteral("📂"),  tr("Abrir…"));
    auto *btnSalvar     = makeBtn("document-save",    QStringLiteral("💾"),  tr("Salvar"));
    auto *btnSalvarComo = makeBtn("document-save-as", QStringLiteral("💾+"), tr("Salvar como…"));
    connect(btnAbrir,      &QToolButton::clicked, this, &MainWindow::onAbrir);
    connect(btnSalvar,     &QToolButton::clicked, this, &MainWindow::onSalvar);
    connect(btnSalvarComo, &QToolButton::clicked, this, &MainWindow::onSalvarComo);

    toolbar->addSeparator();

    // Funcionalidades (pendentes)
    auto *btnBusca = makeBtn("edit-find", QStringLiteral("🔍"), tr("Buscar transação…"));
    connect(btnBusca, &QToolButton::clicked, this, &MainWindow::onBuscaGlobal);
    auto *btnHoje = makeBtn("go-jump-today", QStringLiteral("📅"), tr("Hoje"));
    connect(btnHoje, &QToolButton::clicked, this, [this]() {
        if      (m_tabs->currentWidget() == m_parcelas)      m_parcelas->scrollParaMesAtual();
        else if (m_tabs->currentWidget() == m_entradas)      m_entradas->scrollParaMesAtual();
        else if (m_tabs->currentWidget() == m_gastos)        m_gastos->scrollParaMesAtual();
        else if (m_tabs->currentWidget() == m_investimentos) m_investimentos->scrollParaMesAtual();
        // PlanejamentoWidget has its own month navigation — no scroll needed
    });
    auto *btnCalc = makeBtn("accessories-calculator", QStringLiteral("🧮"), tr("Calculadora"));
    connect(btnCalc, &QToolButton::clicked, this, [this]() {
        CalculadoraHub::instance().mostrarGlobal(this);
    });

    toolbar->addSeparator();

    // Configurações / Tema / Bloquear
    auto *btnConfig = makeBtn("preferences-system", QStringLiteral("⚙"),  tr("Configurações"));
    connect(btnConfig, &QToolButton::clicked, this, &MainWindow::onConfiguracoes);

    m_btnTheme = new QToolButton(toolbar);
    m_btnTheme->setObjectName(QStringLiteral("themeBtn"));
    m_btnTheme->setAutoRaise(true);
    toolbar->addWidget(m_btnTheme);
    connect(m_btnTheme, &QToolButton::clicked, this, [this]() {
        Theme::ThemeMode next;
        switch (Theme::currentMode()) {
            case Theme::ThemeMode::Auto:  next = Theme::ThemeMode::Light; break;
            case Theme::ThemeMode::Light: next = Theme::ThemeMode::Dark;  break;
            case Theme::ThemeMode::Dark:  next = Theme::ThemeMode::Auto;  break;
        }
        applyThemeMode(next);
    });

    makeBtn("system-lock-screen", QStringLiteral("🔒"), tr("Bloquear  (em breve)"), false);

    updateThemeButton();
}

void MainWindow::applyThemeMode(Theme::ThemeMode mode)
{
    Theme::setMode(mode);
    updateThemeButton();
}

void MainWindow::updateThemeButton()
{
    struct Def { const char *themeIcon; QString fallback; QString tip; };
    static const Def defs[] = {
        { "weather-partly-cloudy", QStringLiteral("◑"), tr("Modo: automático  (clique → claro)") },
        { "weather-clear",         QStringLiteral("☀"), tr("Modo: claro  (clique → escuro)")      },
        { "weather-clear-night",   QStringLiteral("☾"), tr("Modo: escuro  (clique → automático)") },
    };
    // enum ThemeMode { Auto=0, Light=1, Dark=2 }
    const auto &d = defs[static_cast<int>(Theme::currentMode())];
    const QIcon icon = QIcon::fromTheme(QLatin1String(d.themeIcon));
    if (icon.isNull()) {
        m_btnTheme->setText(d.fallback);
        m_btnTheme->setToolButtonStyle(Qt::ToolButtonTextOnly);
    } else {
        m_btnTheme->setIcon(icon);
        m_btnTheme->setIconSize({20, 20});
        m_btnTheme->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    m_btnTheme->setToolTip(d.tip);
}

// ── Close event ───────────────────────────────────────────────────────────────

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!m_dirty) {
        CalculadoraHub::instance().fechar();
        event->accept();
        return;
    }

    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Warning);
    msg.setWindowTitle(tr("Fechar"));
    msg.setText(tr("Há alterações não salvas."));
    msg.setInformativeText(tr("Deseja salvar antes de fechar?"));
    QAbstractButton *btnSalvar   = msg.addButton(tr("Salvar"),      QMessageBox::AcceptRole);
    QAbstractButton *btnFechar   = msg.addButton(tr("Não salvar"),  QMessageBox::DestructiveRole);
    msg.addButton(tr("Cancelar"), QMessageBox::RejectRole);
    msg.setDefaultButton(qobject_cast<QPushButton *>(btnSalvar));
    msg.exec();

    if (msg.clickedButton() == btnSalvar) {
        DatabaseManager::instance().save();
        m_dirty = false;
        CalculadoraHub::instance().fechar();
        event->accept();
    } else if (msg.clickedButton() == btnFechar) {
        CalculadoraHub::instance().fechar();
        event->accept();
    } else {
        event->ignore();
    }
}

// ── Auto-save ─────────────────────────────────────────────────────────────────

void MainWindow::resetAutosaveTimer()
{
    m_autosaveTimer->stop();
    if (AppSettings::instance().autosaveEnabled())
        m_autosaveTimer->start(
            AppSettings::instance().autosaveIntervalMinutes() * 60 * 1000);
}

void MainWindow::onAutosave()
{
    if (!m_dirty) return;
    const QString folder = AppSettings::instance().autosavesFolder();
    QDir().mkpath(folder);
    const QString ts   = QDateTime::currentDateTime().toString(
                           QStringLiteral("yyyyMMdd_HHmmss"));
    const QString slug = UserSession::instance().slug();
    const QString path = QStringLiteral("%1/%2_%3.pbd").arg(folder, slug, ts);
    if (DatabaseManager::instance().exportToFile(path))
        statusBar()->showMessage(tr("Auto-salvo: %1").arg(path), 3000);
}

// ── Arquivo slots ─────────────────────────────────────────────────────────────

void MainWindow::onAbrir()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Abrir arquivo PeuBud"),
        QString(),
        tr("PeuBud (*.pbd);;Todos os arquivos (*.*)"));
    if (path.isEmpty()) return;

    if (!DatabaseManager::instance().importFromFile(path)) {
        QMessageBox::warning(this, tr("Erro ao abrir"),
            tr("Não foi possível carregar o arquivo.\n"
               "Verifique se o arquivo é válido e se a senha está correta."));
        return;
    }
    m_dirty = false;
    m_parcelas->recarregar();
    m_entradas->recarregar();
}

void MainWindow::onSalvar()
{
    if (DatabaseManager::instance().save()) {
        m_dirty = false;
        statusBar()->showMessage(tr("Arquivo salvo."), 3000);
    } else {
        QMessageBox::warning(this, tr("Erro"), tr("Não foi possível salvar."));
    }
}

void MainWindow::onSalvarComo()
{
    const QString startDir = AppSettings::instance().savesFolder();
    QDir().mkpath(startDir);

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Salvar cópia como"),
        startDir,
        tr("PeuBud (*.pbd)"));
    if (path.isEmpty()) return;

    const QString dest = path.endsWith(QStringLiteral(".pbd"), Qt::CaseInsensitive)
                       ? path : path + QStringLiteral(".pbd");
    if (!DatabaseManager::instance().exportToFile(dest)) {
        QMessageBox::warning(this, tr("Erro"),
            tr("Não foi possível salvar o arquivo."));
    }
}

// ── Import ────────────────────────────────────────────────────────────────────

void MainWindow::onImportar(const QString &format)
{
    static const QMap<QString,QString> filters = {
        { QStringLiteral("csv"),  tr("CSV (*.csv);;Todos (*.*)") },
        { QStringLiteral("ofx"),  tr("OFX/QFX (*.ofx *.qfx);;Todos (*.*)") },
        { QStringLiteral("qif"),  tr("QIF (*.qif);;Todos (*.*)") },
        { QStringLiteral("xlsx"), tr("Excel (*.xlsx *.xls);;Todos (*.*)") },
        { QStringLiteral("ods"),  tr("ODS (*.ods);;Todos (*.*)") },
        { QStringLiteral("txt"),  tr("Texto/CNAB (*.txt *.cnab);;Todos (*.*)") },
        { QStringLiteral("json"), tr("JSON (*.json);;Todos (*.*)") },
        { QStringLiteral("md"),   tr("Markdown (*.md);;Todos (*.*)") },
        { QStringLiteral("html"), tr("HTML (*.html *.htm);;Todos (*.*)") },
        { QStringLiteral("pdf"),  tr("PDF (*.pdf);;Todos (*.*)") },
    };

    const QString path = QFileDialog::getOpenFileName(
        this, tr("Importar"),
        QString(), filters.value(format, tr("Todos (*.*)")));
    if (path.isEmpty()) return;

    ImportService::Result result;
    if      (format == QStringLiteral("csv"))  result = ImportService::fromCsv(path);
    else if (format == QStringLiteral("ofx"))  result = ImportService::fromOfx(path);
    else if (format == QStringLiteral("xlsx")) result = ImportService::fromXlsx(path);
    else if (format == QStringLiteral("qif"))  result = ImportService::fromQif(path);
    else if (format == QStringLiteral("json")) result = ImportService::fromJson(path);
    else if (format == QStringLiteral("ods"))  result = ImportService::fromOds(path);
    else if (format == QStringLiteral("txt"))  result = ImportService::fromTxt(path);
    else if (format == QStringLiteral("md"))   result = ImportService::fromMd(path);
    else if (format == QStringLiteral("html")) result = ImportService::fromHtml(path);
    else if (format == QStringLiteral("pdf"))  result = ImportService::fromPdf(path);

    if (!result.isValid()) {
        QMessageBox::warning(this, tr("Importar"), result.error);
        return;
    }

    const auto reply = QMessageBox::question(
        this, tr("Confirmar importação"),
        tr("%1 transação(ões) encontradas. Deseja importar?")
            .arg(result.compras.size()));
    if (reply != QMessageBox::Yes) return;

    CompraRepository  cr;
    ParcelaRepository pr;
    for (int i = 0; i < result.compras.size(); ++i) {
        auto compra = result.compras[i];
        if (!cr.insert(compra)) continue;
        for (auto &p : result.parcelas[i]) {
            p.compraId = compra.id;
            pr.insert(p);
        }
    }

    m_dirty = true;
    m_parcelas->recarregar();
    QMessageBox::information(this, tr("Importar"),
        tr("%1 compra(s) importada(s) com sucesso.")
            .arg(result.compras.size()));
}

// ── Export ────────────────────────────────────────────────────────────────────

void MainWindow::onExportar(const QString &format)
{
    // ── Step 1: escolher seções e formato ────────────────────────────────────
    const QStringList available = {
        tr("Entradas"), tr("Gastos"), tr("Investimentos"),
        tr("Planejamento"), tr("Recorrências"),
        tr("Dívidas"), tr("Metas"), tr("Lembretes"), tr("Cartões"),
    };

    ExportDialog dlg(available, format, this);
    if (dlg.exec() != QDialog::Accepted) return;

    const QStringList selected = dlg.selectedSections();
    const QString     fmt      = dlg.selectedFormat();

    if (selected.isEmpty()) {
        QMessageBox::information(this, tr("Exportar"),
            tr("Nenhuma seção selecionada."));
        return;
    }

    // ── Step 2: escolher caminho ──────────────────────────────────────────────
    static const QMap<QString,QString> filters = {
        { QStringLiteral("csv"),  tr("CSV (*.csv)") },
        { QStringLiteral("ofx"),  tr("OFX (*.ofx)") },
        { QStringLiteral("qif"),  tr("QIF (*.qif)") },
        { QStringLiteral("xlsx"), tr("Excel (*.xlsx)") },
        { QStringLiteral("ods"),  tr("ODS (*.ods)") },
        { QStringLiteral("txt"),  tr("Texto (*.txt)") },
        { QStringLiteral("json"), tr("JSON (*.json)") },
        { QStringLiteral("md"),   tr("Markdown (*.md)") },
        { QStringLiteral("html"), tr("HTML (*.html)") },
        { QStringLiteral("pdf"),  tr("PDF (*.pdf)") },
    };

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Exportar"),
        QStringLiteral("orcamento.%1").arg(fmt),
        filters.value(fmt, tr("Todos os arquivos (*)")));
    if (path.isEmpty()) return;

    // ODS stub
    if (fmt == QStringLiteral("ods")) {
        QMessageBox::information(this, tr("Exportar ODS"),
            tr("Exportação ODS ainda não implementada.\n"
               "Use CSV ou XLSX para abrir no LibreOffice Calc."));
        return;
    }

    // ── Step 3: construir seções ──────────────────────────────────────────────
    QList<ExportService::Section> sections;
    QList<ExportService::OFXTx>   txs;
    const bool needsTxs = (fmt == QStringLiteral("ofx") || fmt == QStringLiteral("qif"));

    if (selected.contains(tr("Entradas"))) {
        LancamentoEntradaRepository repo;
        const auto entradas = repo.findAll();
        sections.append({ tr("Entradas"), ExportService::buildTableEntradas(entradas) });
        if (needsTxs) txs += ExportService::ofxFromEntradas(entradas);
    }
    if (selected.contains(tr("Gastos"))) {
        GastoAvulsoRepository repo;
        const auto gastos = repo.findAll();
        sections.append({ tr("Gastos"), ExportService::buildTableGastos(gastos) });
        if (needsTxs) txs += ExportService::ofxFromGastos(gastos);
    }
    if (selected.contains(tr("Investimentos"))) {
        LancamentoInvestimentoRepository repo;
        sections.append({ tr("Investimentos"),
                          ExportService::buildTableInvestimentos(repo.findAll()) });
    }
    if (selected.contains(tr("Planejamento"))) {
        OrcamentoRepository repo;
        sections.append({ tr("Planejamento"),
                          ExportService::buildTablePlanejamento(repo.findAll()) });
    }
    if (selected.contains(tr("Recorrências"))) {
        TransacaoRecorrenteRepository repo;
        sections.append({ tr("Recorrências"),
                          ExportService::buildTableRecorrencias(repo.findAll()) });
    }
    if (selected.contains(tr("Dívidas"))) {
        DividaRepository repo;
        sections.append({ tr("Dívidas"),
                          ExportService::buildTableDividas(repo.findAll()) });
    }
    if (selected.contains(tr("Metas"))) {
        MetaRepository repo;
        sections.append({ tr("Metas"),
                          ExportService::buildTableMetas(repo.findAll()) });
    }
    if (selected.contains(tr("Lembretes"))) {
        LembreteRepository repo;
        sections.append({ tr("Lembretes"),
                          ExportService::buildTableLembretes(repo.findAll()) });
    }
    if (selected.contains(tr("Cartões"))) {
        CompraRepository  cr;
        ParcelaRepository pr;
        ExportService::Dataset d;
        d.compras = cr.findAll();
        for (const auto &c : d.compras)
            d.parcelas.append(pr.findByCompra(c.id));
        QSqlQuery qc(QStringLiteral("SELECT id, apelido FROM cartoes ORDER BY apelido"),
                     DatabaseManager::instance().db());
        while (qc.next()) {
            Cartao ct;
            ct.id      = qc.value(0).toInt();
            ct.apelido = qc.value(1).toString();
            d.cartoes.append(ct);
        }
        sections.append({ tr("Cartões"), ExportService::buildTable(d) });
        if (needsTxs) txs += ExportService::ofxFromDataset(d);
    }

    // ── Step 4: exportar ──────────────────────────────────────────────────────
    bool ok = false;
    if      (fmt == QStringLiteral("ofx"))  ok = ExportService::toMultiOfx(path, txs);
    else if (fmt == QStringLiteral("qif"))  ok = ExportService::toMultiQif(path, txs);
    else if (fmt == QStringLiteral("csv"))  ok = ExportService::toMultiCsv(path, sections);
    else if (fmt == QStringLiteral("xlsx") ||
             fmt == QStringLiteral("xls")) ok = ExportService::toMultiXlsx(path, sections);
    else if (fmt == QStringLiteral("json")) ok = ExportService::toMultiJson(path, sections);
    else if (fmt == QStringLiteral("md"))   ok = ExportService::toMultiMarkdown(path, sections);
    else if (fmt == QStringLiteral("html")) ok = ExportService::toMultiHtml(path, sections);
    else if (fmt == QStringLiteral("pdf"))  ok = ExportService::toMultiPdf(path, sections);
    else if (fmt == QStringLiteral("txt"))  ok = ExportService::toMultiTxt(path, sections);

    if (!ok)
        QMessageBox::warning(this, tr("Exportar"),
            tr("Não foi possível exportar o arquivo."));
    else
        QMessageBox::information(this, tr("Exportar"),
            tr("Arquivo exportado com sucesso:\n%1").arg(path));
}

// ── Configurações slot ────────────────────────────────────────────────────────

void MainWindow::onConfiguracoes()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Configurações"));
    dlg.setMinimumSize(520, 480);
    auto *lay   = new QVBoxLayout(&dlg);
    auto *prefs = new PrefsWidget(&dlg);
    lay->addWidget(prefs);
    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    lay->addWidget(btns);
    connect(btns,  &QDialogButtonBox::rejected,    &dlg,       &QDialog::accept);
    connect(prefs, &PrefsWidget::cartoesAlterados, m_parcelas, &ParcelasWidget::recarregar);
    connect(prefs, &PrefsWidget::moedaAlterada,    m_parcelas, &ParcelasWidget::recarregar);
    connect(prefs, &PrefsWidget::moedaAlterada,    m_entradas, &EntradasWidget::recarregar);
    connect(prefs, &PrefsWidget::fontesAlteradas,  m_entradas, &EntradasWidget::recarregar);
    dlg.exec();
}

// ── Editar slots ──────────────────────────────────────────────────────────────

void MainWindow::onCopiar()
{
    if      (m_tabs->currentWidget() == m_entradas)      m_entradas->copiar();
    else if (m_tabs->currentWidget() == m_gastos)        m_gastos->copiar();
    else if (m_tabs->currentWidget() == m_investimentos) m_investimentos->copiar();
    else if (m_tabs->currentWidget() == m_metas)         m_metas->copiar();
    else if (m_tabs->currentWidget() == m_lembretes)     m_lembretes->copiar();
    else                                                 m_parcelas->copiar();
}
void MainWindow::onColar()         { /* sem suporte — paste requer contexto da célula */ }
void MainWindow::onSelecionarTudo()
{
    if      (m_tabs->currentWidget() == m_entradas)      m_entradas->selecionarTudo();
    else if (m_tabs->currentWidget() == m_gastos)        m_gastos->selecionarTudo();
    else if (m_tabs->currentWidget() == m_investimentos) m_investimentos->selecionarTudo();
    else if (m_tabs->currentWidget() == m_metas)         m_metas->selecionarTudo();
    else if (m_tabs->currentWidget() == m_lembretes)     m_lembretes->selecionarTudo();
    else                                                 m_parcelas->selecionarTudo();
}

void MainWindow::onReconciliar()
{
    auto *dlg = new ReconciliacaoDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void MainWindow::onBuscaGlobal()
{
    if (!m_buscaGlobal) {
        m_buscaGlobal = new BuscaGlobalDialog(this);
        m_buscaGlobal->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_buscaGlobal, &BuscaGlobalDialog::destroyed,
                this, [this]() { m_buscaGlobal = nullptr; });
        connect(m_buscaGlobal, &BuscaGlobalDialog::navegarParaAba,
                m_tabs, &QTabWidget::setCurrentIndex);
    }
    m_buscaGlobal->show();
    m_buscaGlobal->raise();
    m_buscaGlobal->activateWindow();
}

// ── Ajuda slots ───────────────────────────────────────────────────────────────

void MainWindow::onAjuda()
{
    auto *dlg = new HelpDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->show();
}

void MainWindow::onSobre()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onTvmParaCartoes(double pv, int n, double iMensal, double pmt)
{
    // Switch to Cartões tab (index 3) and open AddCompraDialog pre-filled.
    const int cartaoTabIdx = m_tabs->indexOf(m_parcelas);
    if (cartaoTabIdx >= 0)
        m_tabs->setCurrentIndex(cartaoTabIdx);
    m_parcelas->novaCompraDeCalculadora(pv, n, iMensal, pmt);
}
