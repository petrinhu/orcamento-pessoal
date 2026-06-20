#include "MainWindow.h"
#include "widgets/DashboardWidget.h"
#include "widgets/TendenciasWidget.h"
#include "widgets/ParcelasWidget.h"
#include "widgets/TabShell.h"
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
#include "widgets/GlossarioDialog.h"
#include "core/session/UserSession.h"
#include "core/database/DatabaseManager.h"
#include "services/BackupService.h"
#include "services/BrapiProvider.h"
#include "GlossarioDecorator.h"
#include "services/MarketDataService.h"
#include "ui/widgets/BolsaWidget.h"
#include "ui/widgets/CurrencyPanelWidget.h"
#include "ui/widgets/ProventosWidget.h"
#include "core/settings/AppSettings.h"
#include "services/ImportService.h"
#include "services/ExportService.h"
#include "widgets/ExportDialog.h"
#include "repositories/CompraRepository.h"
#include "repositories/ParcelaRepository.h"
#include "repositories/CartaoRepository.h"
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

#include <QIcon>
#include <QLabel>
#include <QTabBar>
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
    setWindowIcon(QIcon(QStringLiteral(":/icon/icon.png")));
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

// ── Tab order persistence (drag & drop) ──────────────────────────────────────

QStringList MainWindow::ordemAbasDefault() const
{
    // Mesma sequencia dos addTab em buildUi. Fonte unica para o reset.
    return {
        QStringLiteral("tab_dashboard"),
        QStringLiteral("tab_entradas"),
        QStringLiteral("tab_gastos"),
        QStringLiteral("tab_parcelas"),
        QStringLiteral("tab_recorrencias"),
        QStringLiteral("tab_dividas"),
        QStringLiteral("tab_metas"),
        QStringLiteral("tab_planejamento"),
        QStringLiteral("tab_tendencias"),
        QStringLiteral("tab_investimentos"),
        QStringLiteral("tab_proventos"),
        QStringLiteral("tab_bolsa"),
        QStringLiteral("tab_lembretes"),
    };
}

void MainWindow::aplicarOrdemAbasSalva()
{
    if (!m_tabs) return;
    const QStringList ordem = AppSettings::instance().tabOrder();
    if (ordem.isEmpty()) return; // primeiro start: usa ordem default do código

    auto *bar = m_tabs->tabBar();
    // Para cada chave salva (na ordem desejada), mover aba correspondente
    // para a posicao final acumulada. Chaves desconhecidas ignoradas;
    // abas novas (em código mas nao em ordem salva) permanecem na cauda.
    int destino = 0;
    for (const QString &chave : ordem) {
        for (int i = destino; i < m_tabs->count(); ++i) {
            const QWidget *w = m_tabs->widget(i);
            if (w && w->objectName() == chave) {
                if (i != destino) bar->moveTab(i, destino);
                ++destino;
                break;
            }
        }
    }
}

void MainWindow::resetarOrdemAbas()
{
    if (!m_tabs) return;
    // Limpa a persistencia (volta ao default no proximo start) e reordena ao
    // vivo para a ordem default, sem reiniciar o app. moveTab preserva texto e
    // icone de cada aba; o widget atual permanece selecionado.
    AppSettings::instance().setTabOrder({});

    auto *bar = m_tabs->tabBar();
    QWidget *atual = m_tabs->currentWidget();
    int destino = 0;
    for (const QString &chave : ordemAbasDefault()) {
        for (int i = destino; i < m_tabs->count(); ++i) {
            const QWidget *w = m_tabs->widget(i);
            if (w && w->objectName() == chave) {
                if (i != destino) bar->moveTab(i, destino);
                ++destino;
                break;
            }
        }
    }
    if (atual) m_tabs->setCurrentWidget(atual);
}

void MainWindow::salvarOrdemAbas()
{
    if (!m_tabs) return;
    QStringList ordem;
    ordem.reserve(m_tabs->count());
    for (int i = 0; i < m_tabs->count(); ++i) {
        const QWidget *w = m_tabs->widget(i);
        if (w && !w->objectName().isEmpty()) ordem.append(w->objectName());
    }
    AppSettings::instance().setTabOrder(ordem);
}

void MainWindow::buildUi()
{
    m_undoStack = new QUndoStack(this);
    m_undoStack->setUndoLimit(50);

    m_marketService = new MarketDataService(this);
    // #58: registra BrapiProvider; ativa-o se for o provider salvo em
    // AppSettings (default 'frankfurter' — usuario muda em Prefs/Mercado).
    m_marketService->registerProvider(QStringLiteral("brapi"),
                                       new BrapiProvider(m_marketService));
    m_marketService->setActiveProvider(
        AppSettings::instance().marketDefaultProvider());
    m_bolsa         = new BolsaWidget(m_marketService, this);
    m_proventos     = new ProventosWidget(this);
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
    m_tabs->setMovable(true); // drag-and-drop reorder

    // ── TabShell em todas as abas (#158): cada aba ganha um invólucro com
    // título + descrição + instruções p/ iniciante (texto do registro central
    // tabHelp, DRY com o HelpDialog). A chave estavel de persistencia de ordem
    // (objectName tab_<x>) passa a viver no shell, que e o widget efetivamente
    // inserido na aba; o widget de conteúdo segue intacto (lógica/conexões
    // inalteradas). O navegador de cards do Dashboard e a BuscaGlobal usam
    // índice — os índices das abas não mudam, só o widget em cada slot.
    auto montarShell = [this](TabShell *&shell, const QString &objName,
                              const QString &tabKey, QWidget *conteudo) {
        shell = new TabShell(QString(), this);
        shell->setObjectName(objName);
        shell->setTabKey(tabKey);
        shell->applyHelp();
        shell->setContent(conteudo);
    };

    // objectName = chave de persistência de ordem (ordemAbasDefault). A aba
    // Cartões mantém tab_parcelas (compat. com ordem já salva); a tabKey de
    // ajuda é tab_cartoes — as duas strings são independentes.
    montarShell(m_shellDashboard,     QStringLiteral("tab_dashboard"),     QStringLiteral("tab_dashboard"),     m_dashboard);
    montarShell(m_shellEntradas,      QStringLiteral("tab_entradas"),      QStringLiteral("tab_entradas"),      m_entradas);
    montarShell(m_shellGastos,        QStringLiteral("tab_gastos"),        QStringLiteral("tab_gastos"),        m_gastos);
    montarShell(m_shellCartoes,       QStringLiteral("tab_parcelas"),      QStringLiteral("tab_cartoes"),       m_parcelas);
    montarShell(m_shellRecorrencias,  QStringLiteral("tab_recorrencias"),  QStringLiteral("tab_recorrencias"),  m_recorrencias);
    montarShell(m_shellDividas,       QStringLiteral("tab_dividas"),       QStringLiteral("tab_dividas"),       m_dividas);
    montarShell(m_shellMetas,         QStringLiteral("tab_metas"),         QStringLiteral("tab_metas"),         m_metas);
    montarShell(m_shellPlanejamento,  QStringLiteral("tab_planejamento"),  QStringLiteral("tab_planejamento"),  m_planejamento);
    montarShell(m_shellTendencias,    QStringLiteral("tab_tendencias"),    QStringLiteral("tab_tendencias"),    m_tendencias);
    montarShell(m_shellInvestimentos, QStringLiteral("tab_investimentos"), QStringLiteral("tab_investimentos"), m_investimentos);
    montarShell(m_shellProventos,     QStringLiteral("tab_proventos"),     QStringLiteral("tab_proventos"),     m_proventos);
    montarShell(m_shellBolsa,         QStringLiteral("tab_bolsa"),         QStringLiteral("tab_bolsa"),         m_bolsa);
    montarShell(m_shellLembretes,     QStringLiteral("tab_lembretes"),     QStringLiteral("tab_lembretes"),     m_lembretes);

    // Ordem default (1º start e apos reset). A chave estavel de cada aba e o
    // objectName do shell; a ordem real exibida pode diferir desta se o usuario
    // reordenou por drag & drop (restaurada em aplicarOrdemAbasSalva).
    m_tabs->addTab(m_shellDashboard,     tr("Dashboard"));      // 0
    m_tabs->addTab(m_shellEntradas,      tr("Entradas"));       // 1
    m_tabs->addTab(m_shellGastos,        tr("Gastos"));         // 2
    m_tabs->addTab(m_shellCartoes,       tr("Cartões"));        // 3
    m_tabs->addTab(m_shellRecorrencias,  tr("Recorrências"));   // 4
    m_tabs->addTab(m_shellDividas,       tr("Dívidas"));        // 5
    m_tabs->addTab(m_shellMetas,         tr("Metas"));          // 6
    m_tabs->addTab(m_shellPlanejamento,  tr("Planejamento/Orçamento")); // 7
    m_tabs->addTab(m_shellTendencias,    tr("Tendências"));     // 8
    m_tabs->addTab(m_shellInvestimentos, tr("Investimentos"));  // 9
    m_tabs->addTab(m_shellProventos,     tr("Proventos"));      // 10 (#66)
    m_tabs->addTab(m_shellBolsa,         tr("Bolsa"));          // 11 (#50)
    m_tabs->addTab(m_shellLembretes,     tr("Lembretes"));      // 12

    // Restaura ordem salva (drag & drop)
    aplicarOrdemAbasSalva();

    m_tabs->setCurrentIndex(AppSettings::instance().initialTab());
    setCentralWidget(m_tabs);

    // #103 Glossario: aplica tooltips automaticos em labels/buttons com siglas
    // financeiras conhecidas (CET, FIRE, DY, IRPF, etc). Hover mostra
    // definicao em pt-BR sem o usuario precisar saber a sigla.
    GlossarioDecorator::aplicarTooltips(this);

    // #104 Modo Iniciante: se ativo (default), renomeia labels/botoes/grupos
    // para versao coloquial cadastrada em GlossarioService::labelColoquial.
    // Ex.: "FIRE" -> "Aposentadoria Antecipada (FIRE)". No-op em modo Avancado.
    GlossarioDecorator::aplicarLabelsColoquiais(this);

    // #105 Microcopy: para cada QFormLayout descendente cujo label tem sigla
    // cadastrada, insere abaixo do campo uma linha cinza pequena explicando
    // em pt-BR plain (ex.: "CET" -> "= juros total ao ano (com IOF e tarifas)").
    // Idempotente. Roda apos labels coloquiais para nao quebrar lookup do termo.
    GlossarioDecorator::aplicarMicrocopies(this);

    // Persiste ordem ao soltar aba arrastada
    connect(m_tabs->tabBar(), &QTabBar::tabMoved,
            this, &MainWindow::salvarOrdemAbas);

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

    // #47: painel câmbio LED na statusbar (slots USD-BRL + EUR-BRL ou
    // configuracao em Prefs/Mercado). Sem rede ainda — recarrega quando
    // BrapiProvider expor rates via signal (follow-up #58).
    auto *currencyPanel = new CurrencyPanelWidget(this);
    statusBar()->addPermanentWidget(currencyPanel);

    // ── TVM calculator → Cartões tab ───────────────────────────────────────
    connect(&CalculadoraHub::instance(), &CalculadoraHub::tvmParaCartoes,
            this, &MainWindow::onTvmParaCartoes);

    // ── Dirty flag ─────────────────────────────────────────────────────────
    // ── Dirty flag + status bar ────────────────────────────────────────────
    connect(m_dashboard, &DashboardWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellDashboard)
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
        if (m_tabs->currentWidget() == m_shellTendencias)
            m_tendencias->recarregar();
    });

    // Dashboard se atualiza quando qualquer aba muda dados
    auto refreshDash = [this]() {
        if (m_tabs->currentWidget() == m_shellDashboard)
            m_dashboard->recarregar();
    };
    connect(m_entradas,      &EntradasWidget::dadosAlterados,      this, refreshDash);
    connect(m_gastos,        &GastosWidget::dadosAlterados,        this, refreshDash);
    connect(m_parcelas,      &ParcelasWidget::dadosAlterados,      this, refreshDash);
    connect(m_investimentos, &InvestimentosWidget::dadosAlterados, this, refreshDash);
    connect(m_recorrencias,  &RecorrenciaWidget::dadosAlterados,   this, refreshDash);

    // Planejamento se atualiza quando Gastos, Entradas ou Cartões mudam
    auto refreshPlanning = [this]() {
        if (m_tabs->currentWidget() == m_shellPlanejamento)
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
                if (m_tabs->currentWidget() == m_shellCartoes)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_entradas, &EntradasWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellEntradas)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_gastos, &GastosWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellGastos)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_investimentos, &InvestimentosWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellInvestimentos)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_planejamento, &PlanejamentoWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellPlanejamento)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_dividas, &DividasWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellDividas)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_metas, &MetasWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellMetas)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_lembretes, &LembretesWidget::statusAtualizado,
            this, [this](const QString &msg) {
                if (m_tabs->currentWidget() == m_shellLembretes)
                    m_statusTotal->setText(msg + QStringLiteral("  "));
            });
    connect(m_tabs, &QTabWidget::currentChanged, this, [this]() {
        if      (m_tabs->currentWidget() == m_shellDashboard)     m_dashboard->recarregar();
        else if (m_tabs->currentWidget() == m_shellEntradas)      m_entradas->recarregar();
        else if (m_tabs->currentWidget() == m_shellGastos)        m_gastos->recarregar();
        else if (m_tabs->currentWidget() == m_shellTendencias)    m_tendencias->recarregar();
        else if (m_tabs->currentWidget() == m_shellInvestimentos) m_investimentos->recarregar();
        else if (m_tabs->currentWidget() == m_shellPlanejamento)  m_planejamento->recarregar();
        else if (m_tabs->currentWidget() == m_shellRecorrencias)  m_recorrencias->recarregar();
        else if (m_tabs->currentWidget() == m_shellDividas)       m_dividas->recarregar();
        else if (m_tabs->currentWidget() == m_shellMetas)         m_metas->recarregar();
        else if (m_tabs->currentWidget() == m_shellLembretes)     m_lembretes->recarregar();
        else if (m_tabs->currentWidget() == m_shellCartoes)       m_parcelas->recarregar();
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
            m_tabs->setCurrentWidget(m_shellLembretes); // aba Lembretes (índice varia)
    });

    // ── Auto-save timer ────────────────────────────────────────────────────
    m_autosaveTimer = new QTimer(this);
    connect(m_autosaveTimer, &QTimer::timeout, this, &MainWindow::onAutosave);
    connect(&AppSettings::instance(), &AppSettings::autosaveSettingsChanged,
            this, &MainWindow::resetAutosaveTimer);
    resetAutosaveTimer();

    // ── Backup automatico (#79): tick a cada hora; verifica shouldRunNow ─
    auto *backupTimer = new QTimer(this);
    backupTimer->setInterval(60 * 60 * 1000); // 1h
    connect(backupTimer, &QTimer::timeout, this, []() {
        const auto cfg  = BackupService::load();
        if (!BackupService::shouldRunNow(cfg)) return;
        const QString slug = UserSession::instance().slug();
        if (slug.isEmpty()) return;
        BackupService::runBackup(cfg, slug, [](const QString &dest) {
            return DatabaseManager::instance().exportToFile(dest);
        });
    });
    backupTimer->start();
    // Tick imediato no startup (faz backup se passou janela enquanto app fechado)
    QTimer::singleShot(5000, this, []() {
        const auto cfg = BackupService::load();
        if (!BackupService::shouldRunNow(cfg)) return;
        const QString slug = UserSession::instance().slug();
        if (slug.isEmpty()) return;
        BackupService::runBackup(cfg, slug, [](const QString &dest) {
            return DatabaseManager::instance().exportToFile(dest);
        });
    });
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar()
{
    QMenuBar *mb = menuBar();

    // ── Arquivo ────────────────────────────────────────────────────────────
    QMenu *mArquivo = mb->addMenu(tr("&Arquivo"));

    auto *aAbrir = mArquivo->addAction(tr("📁 &Abrir…"));
    aAbrir->setShortcut(QKeySequence::Open);
    connect(aAbrir, &QAction::triggered, this, &MainWindow::onAbrir);

    auto *aSalvar = mArquivo->addAction(tr("💾 &Salvar"));
    aSalvar->setShortcut(QKeySequence::Save);
    connect(aSalvar, &QAction::triggered, this, &MainWindow::onSalvar);

    auto *aSalvarComo = mArquivo->addAction(tr("💾 Salvar &como…"));
    aSalvarComo->setShortcut(QKeySequence::SaveAs);
    connect(aSalvarComo, &QAction::triggered, this, &MainWindow::onSalvarComo);

    mArquivo->addSeparator();

    // Importar submenu
    QMenu *mImportar = mArquivo->addMenu(tr("📁 &Importar de"));
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
    QMenu *mExportar = mArquivo->addMenu(tr("💾 &Exportar"));
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

    auto *aReconciliar = mArquivo->addAction(tr("🔄 Reconciliar &extrato…"));
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

    auto *aBusca = mEditar->addAction(tr("🔍 &Buscar transação…"));
    aBusca->setShortcut(QKeySequence::Find);
    connect(aBusca, &QAction::triggered, this, &MainWindow::onBuscaGlobal);

    // ── Configurações ──────────────────────────────────────────────────────
    QMenu *mConfig = mb->addMenu(tr("&Configurações"));

    auto *aPrefs = mConfig->addAction(tr("⚙️ &Preferências…"));
    connect(aPrefs, &QAction::triggered, this, &MainWindow::onConfiguracoes);

    // ── Ajuda ──────────────────────────────────────────────────────────────
    QMenu *mAjuda = mb->addMenu(tr("A&juda"));

    auto *aAjuda = mAjuda->addAction(tr("❓ &Ajuda"));
    aAjuda->setShortcut(QKeySequence::HelpContents);
    connect(aAjuda, &QAction::triggered, this, &MainWindow::onAjuda);

    auto *aGlossario = mAjuda->addAction(tr("📝 &Glossário financeiro…"));
    aGlossario->setShortcut(QKeySequence(Qt::Key_F2));
    connect(aGlossario, &QAction::triggered, this, &MainWindow::onGlossario);

    mAjuda->addSeparator();

    auto *aSobre = mAjuda->addAction(tr("ℹ️ &Sobre PeuBud…"));
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

    auto makeNavBtn = [&](const QString &glyph, const QString &tip) -> QToolButton * {
        auto *btn = new QToolButton(navCorner);
        btn->setAutoRaise(true);
        btn->setToolTip(tip);
        // Emoji colorido como texto (Noto Color Emoji) — sem QIcon::fromTheme,
        // que renderiza ícone de linha monocromático invisível no tema escuro.
        btn->setText(glyph);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        navLay->addWidget(btn);
        return btn;
    };

    auto *btnReset = makeNavBtn(QStringLiteral("🔄"),
                                tr("Restaurar ordem padrão das abas"));
    auto *btnFirst = makeNavBtn(QStringLiteral("⏮️"), tr("Primeira aba"));
    auto *btnPrev  = makeNavBtn(QStringLiteral("◀️"), tr("Aba anterior"));
    auto *btnNext  = makeNavBtn(QStringLiteral("▶️"), tr("Próxima aba"));
    auto *btnLast  = makeNavBtn(QStringLiteral("⏭️"), tr("Última aba"));

    connect(btnReset, &QToolButton::clicked, this, &MainWindow::resetarOrdemAbas);
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

    auto makeBtn = [&](const QString &glyph, const QString &tip,
                       bool enabled = true) -> QToolButton * {
        auto *btn = new QToolButton(toolbar);
        btn->setAutoRaise(true);
        btn->setEnabled(enabled);
        btn->setToolTip(tip);
        // Emoji colorido como texto (Noto Color Emoji) — sem QIcon::fromTheme,
        // que renderiza ícone de linha monocromático invisível no tema escuro.
        btn->setText(glyph);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        toolbar->addWidget(btn);
        return btn;
    };

    // Arquivo
    auto *btnAbrir      = makeBtn(QStringLiteral("📂"),  tr("Abrir…"));
    auto *btnSalvar     = makeBtn(QStringLiteral("💾"),  tr("Salvar"));
    auto *btnSalvarComo = makeBtn(QStringLiteral("💾➕"), tr("Salvar como…"));
    connect(btnAbrir,      &QToolButton::clicked, this, &MainWindow::onAbrir);
    connect(btnSalvar,     &QToolButton::clicked, this, &MainWindow::onSalvar);
    connect(btnSalvarComo, &QToolButton::clicked, this, &MainWindow::onSalvarComo);

    toolbar->addSeparator();

    // Funcionalidades (pendentes)
    auto *btnBusca = makeBtn(QStringLiteral("🔍"), tr("Buscar transação…"));
    connect(btnBusca, &QToolButton::clicked, this, &MainWindow::onBuscaGlobal);
    auto *btnHoje = makeBtn(QStringLiteral("📅"), tr("Hoje"));
    connect(btnHoje, &QToolButton::clicked, this, [this]() {
        if      (m_tabs->currentWidget() == m_shellCartoes)       m_parcelas->scrollParaMesAtual();
        else if (m_tabs->currentWidget() == m_shellEntradas)      m_entradas->scrollParaMesAtual();
        else if (m_tabs->currentWidget() == m_shellGastos)        m_gastos->scrollParaMesAtual();
        else if (m_tabs->currentWidget() == m_shellInvestimentos) m_investimentos->scrollParaMesAtual();
        // PlanejamentoWidget has its own month navigation — no scroll needed
    });
    auto *btnCalc = makeBtn(QStringLiteral("🧮"), tr("Calculadora"));
    connect(btnCalc, &QToolButton::clicked, this, [this]() {
        CalculadoraHub::instance().mostrarGlobal(this);
    });

    toolbar->addSeparator();

    // Configurações / Tema / Bloquear
    auto *btnConfig = makeBtn(QStringLiteral("⚙️"),  tr("Configurações"));
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

    makeBtn(QStringLiteral("🔒"), tr("Bloquear  (em breve)"), false);

    updateThemeButton();
}

void MainWindow::applyThemeMode(Theme::ThemeMode mode)
{
    Theme::setMode(mode);
    updateThemeButton();
}

void MainWindow::updateThemeButton()
{
    struct Def { QString glyph; QString tip; };
    static const Def defs[] = {
        { QStringLiteral("🌗"),  tr("Modo: automático  (clique → claro)") },
        { QStringLiteral("☀️"), tr("Modo: claro  (clique → escuro)")      },
        { QStringLiteral("🌙"),  tr("Modo: escuro  (clique → automático)") },
    };
    // enum ThemeMode { Auto=0, Light=1, Dark=2 }
    const auto &d = defs[static_cast<int>(Theme::currentMode())];
    // Emoji colorido como texto (Noto Color Emoji) — sem QIcon::fromTheme.
    m_btnTheme->setText(d.glyph);
    m_btnTheme->setToolButtonStyle(Qt::ToolButtonTextOnly);
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

    CompraRepository  cr;
    ParcelaRepository pr;

    // Anota matches contra compras existentes (#84). Duplicate sao filtradas;
    // Precise/Imprecise/NotFound seguem para importacao mas o usuario eh
    // avisado da contagem para decidir conscientemente.
    QList<MatchData> existing;
    for (const Compra &c : cr.findAll()) {
        MatchData m;
        m.amount = c.valorTotal;
        m.payee  = c.nome;
        m.date   = QDate(c.anoInicio, c.mesInicio, 1);
        existing.append(m);
    }
    const auto counts = ImportService::annotateMatches(result, existing);

    QString summary = tr("%1 transação(ões) encontradas.")
                        .arg(result.compras.size());
    if (counts.duplicate > 0 || counts.precise > 0 || counts.imprecise > 0) {
        summary += tr("\n\nMatches encontrados:");
        if (counts.duplicate > 0)
            summary += tr("\n  • %1 duplicata(s) (mesmo bankId)").arg(counts.duplicate);
        if (counts.precise > 0)
            summary += tr("\n  • %1 match(es) preciso(s)").arg(counts.precise);
        if (counts.imprecise > 0)
            summary += tr("\n  • %1 match(es) impreciso(s) (revisar)").arg(counts.imprecise);
    }
    summary += tr("\n\nDeseja importar? Duplicatas serão ignoradas.");

    const auto reply = QMessageBox::question(
        this, tr("Confirmar importação"), summary);
    if (reply != QMessageBox::Yes) return;

    int imported = 0;
    int skipped  = 0;
    for (int i = 0; i < result.compras.size(); ++i) {
        const bool isDuplicate = i < result.matches.size()
                                 && result.matches[i] == MatchResult::Duplicate;
        if (isDuplicate) { ++skipped; continue; }
        auto compra = result.compras[i];
        if (!cr.insert(compra)) continue;
        for (auto &p : result.parcelas[i]) {
            p.compraId = compra.id;
            pr.insert(p);
        }
        ++imported;
    }

    m_dirty = true;
    m_parcelas->recarregar();
    QMessageBox::information(this, tr("Importar"),
        tr("%1 compra(s) importada(s).%2")
            .arg(imported)
            .arg(skipped > 0 ? tr(" %1 duplicata(s) ignorada(s).").arg(skipped)
                             : QString()));
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
        CartaoRepository  cartaoRepo;
        ExportService::Dataset d;
        d.compras = cr.findAll();
        for (const auto &c : d.compras)
            d.parcelas.append(pr.findByCompra(c.id));
        d.cartoes = cartaoRepo.findAll();
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
    if      (m_tabs->currentWidget() == m_shellEntradas)      m_entradas->copiar();
    else if (m_tabs->currentWidget() == m_shellGastos)        m_gastos->copiar();
    else if (m_tabs->currentWidget() == m_shellInvestimentos) m_investimentos->copiar();
    else if (m_tabs->currentWidget() == m_shellMetas)         m_metas->copiar();
    else if (m_tabs->currentWidget() == m_shellLembretes)     m_lembretes->copiar();
    else                                                      m_parcelas->copiar();
}
void MainWindow::onColar()         { /* sem suporte — paste requer contexto da célula */ }
void MainWindow::onSelecionarTudo()
{
    if      (m_tabs->currentWidget() == m_shellEntradas)      m_entradas->selecionarTudo();
    else if (m_tabs->currentWidget() == m_shellGastos)        m_gastos->selecionarTudo();
    else if (m_tabs->currentWidget() == m_shellInvestimentos) m_investimentos->selecionarTudo();
    else if (m_tabs->currentWidget() == m_shellMetas)         m_metas->selecionarTudo();
    else if (m_tabs->currentWidget() == m_shellLembretes)     m_lembretes->selecionarTudo();
    else                                                      m_parcelas->selecionarTudo();
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

void MainWindow::onGlossario()
{
    GlossarioDialog dlg(this);
    dlg.exec();
}

void MainWindow::onSobre()
{
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::onTvmParaCartoes(double pv, int n, double iMensal, double pmt)
{
    // Switch to Cartões tab (index 3) and open AddCompraDialog pre-filled.
    const int cartaoTabIdx = m_tabs->indexOf(m_shellCartoes);
    if (cartaoTabIdx >= 0)
        m_tabs->setCurrentIndex(cartaoTabIdx);
    m_parcelas->novaCompraDeCalculadora(pv, n, iMensal, pmt);
}
