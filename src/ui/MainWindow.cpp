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
#include "ui/UiFormatters.h"
#include "services/ImportService.h"
#include "services/ExportService.h"
#include "services/export/ExportWriterRegistry.h"
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

#include "NavRegistry.h"
#include "widgets/SidebarNav.h"
#include <QIcon>
#include <QLabel>
#include <QStackedWidget>
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
    // Ctrl+1..9 → navegar para página pela posição em navItens()
    for (int i = 1; i <= 9; ++i) {
        const auto seq = QKeySequence(QStringLiteral("Ctrl+%1").arg(i));
        auto *sc = new QShortcut(seq, this);
        sc->setContext(Qt::ApplicationShortcut);
        const int navIdx = i - 1;
        connect(sc, &QShortcut::activated, this, [this, navIdx]() {
            const auto &itens = navItens();
            if (navIdx < itens.size())
                navegarParaChave(itens[navIdx].chave);
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

    m_marketService = new MarketDataService(this);
    // #58: registra BrapiProvider; ativa-o se for o provider salvo em
    // AppSettings (default 'frankfurter' — usuario muda em Prefs/Mercado).
    m_marketService->registerProvider(QStringLiteral("brapi"),
                                       new BrapiProvider(m_marketService));
    m_marketService->setActiveProvider(
        AppSettings::instance().marketDefaultProvider());
    // Nota: m_marketService é leve (registra providers, sem construir UI) —
    // fica eager. Só o BolsaWidget (UI pesada) vira lazy, na fábrica de
    // "tab_bolsa" abaixo.

    // ── SidebarNav (#165): agrupa itens em ordem fixa de navItens() ──────────
    m_sidebar = new SidebarNav(this);
    QString grupoAtual;
    for (const NavItem &item : navItens()) {
        if (item.grupo != grupoAtual && !item.grupo.isEmpty()) {
            m_sidebar->addGrupo(item.grupo);
            grupoAtual = item.grupo;
        }
        m_sidebar->addItem(item.chave, item.emoji, item.titulo);
    }

    // ── Rodapé da sidebar: ações de utilidade (#170) ─────────────────────────
    m_sidebar->addFooterItem(QStringLiteral("act_prefs"), QStringLiteral("⚙️"), tr("Preferências"));
    m_sidebar->addFooterItem(QStringLiteral("act_ajuda"), QStringLiteral("❓"), tr("Ajuda"));
    m_sidebar->addFooterItem(QStringLiteral("act_tema"),  QStringLiteral("🌗"), tr("Tema"));
    m_sidebar->setUsuario(UserSession::instance().name());

    // ── QStackedWidget: vazio — páginas entram sob demanda (#208, PERF-02) ──
    m_stack = new QStackedWidget(this);

    // ── Layout central: sidebar | stack ──────────────────────────────────────
    auto *central = new QWidget(this);
    auto *lay     = new QHBoxLayout(central);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(m_sidebar);
    lay->addWidget(m_stack, 1);
    setCentralWidget(central);

    connect(m_sidebar, &SidebarNav::navegou, this, &MainWindow::navegarParaChave);

    // ── Lazy-load: recarrega widget de conteúdo ao trocar de página (#165) ──
    // Cada branch só é alcançável se a respectiva aba já foi construída
    // (m_shellX só deixa de ser nullptr dentro da fábrica de X em
    // registrarFabricasAbas(), que roda antes de m_shellX poder ser
    // currentWidget()) — nula-segurança preservada mesmo com abas lazy.
    connect(m_stack, &QStackedWidget::currentChanged, this, [this](int) {
        if      (m_stack->currentWidget() == m_shellDashboard)     m_dashboard->recarregar();
        else if (m_stack->currentWidget() == m_shellEntradas)      m_entradas->recarregar();
        else if (m_stack->currentWidget() == m_shellGastos)        m_gastos->recarregar();
        else if (m_stack->currentWidget() == m_shellTendencias)    m_tendencias->recarregar();
        else if (m_stack->currentWidget() == m_shellInvestimentos) m_investimentos->recarregar();
        else if (m_stack->currentWidget() == m_shellPlanejamento)  m_planejamento->recarregar();
        else if (m_stack->currentWidget() == m_shellRecorrencias)  m_recorrencias->recarregar();
        else if (m_stack->currentWidget() == m_shellDividas)       m_dividas->recarregar();
        else if (m_stack->currentWidget() == m_shellMetas)         m_metas->recarregar();
        else if (m_stack->currentWidget() == m_shellLembretes)     m_lembretes->recarregar();
        else if (m_stack->currentWidget() == m_shellCartoes)       m_parcelas->recarregar();
    });

    // ── Fábricas lazy das 13 abas (#208, PERF-02) ────────────────────────────
    registrarFabricasAbas();

    // ── Aba inicial: lê chave estável (migra legado int → chave se necessário)
    // navegarParaChave() dispara garantir() na chave inicial — só ELA é
    // construída no boot; as outras 12 só quando o usuário navegar até lá.
    {
        const QString chaveInicial = AppSettings::instance().initialTabKey();
        navegarParaChave(m_router.registrada(chaveInicial)
                         ? chaveInicial
                         : QStringLiteral("tab_dashboard"));
    }

    // #103/#104/#105 Glossario (chrome): sidebar já existe; a aba inicial
    // também (navegarParaChave acima já a construiu) — cobertura idêntica à
    // de antes do #208 para essa aba+chrome. As demais abas recebem o mesmo
    // tratamento, escopado, dentro de aplicarGlossarioNaAba() na sua própria
    // fábrica (reduz o N do PERF-01: nunca mais varre a MainWindow inteira).
    GlossarioDecorator::aplicarTooltips(this);
    GlossarioDecorator::aplicarLabelsColoquiais(this);
    GlossarioDecorator::aplicarMicrocopies(this);

    buildMenuBar();   // QMenuBar aposentada; reconfigura atalhos via addAction()
    buildUtilBar();   // cluster utilidades topo-direito

    // ── Status bar ─────────────────────────────────────────────────────────
    // m_statusUser removido (#165): redundante com o rodapé da sidebar que
    // já exibe o nome do usuário logado.
    // QSizeGrip desabilitado (#176): o handle nativo não herda o QSS navy e
    // aparece como quadrado branco no canto inferior-direito ao redimensionar.
    // A janela continua redimensionável pelas bordas — apenas o grip visual some.
    statusBar()->setSizeGripEnabled(false);
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

    // ── Gerar recorrências pendentes ao abrir o app ─────────────────────────
    // (#208) guardas de nulo: se a aba correspondente ainda não foi
    // construída, não há nada pra recarregar — ela vai ler o estado já
    // atualizado (recorrências geradas acima) na 1ª vez que for construída.
    const int geradas = RecorrenciaService().gerarPendentes(QDate::currentDate());
    if (geradas > 0) {
        if (m_entradas)     m_entradas->recarregar();
        if (m_gastos)       m_gastos->recarregar();
        if (m_recorrencias) m_recorrencias->recarregar();
        if (m_planejamento) m_planejamento->recarregar();
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
                    // #197 (I18N-01): "R$"+vírgula fixos no PRÓPRIO template
                    // traduzível (pior que hardcoded solto -- tradutor não
                    // consegue corrigir moeda por locale). Fix: símbolo/
                    // separador computados via formatCentavosBRL, fora do tr().
                    ? tr("• %1 — %2").arg(l.descricao, uifmt::formatCentavosBRL(l.valor))
                    : tr("• ") + l.descricao;
                debNaoFeitos << txt;
            } else if (l.vencido()) {
                const QString txt = l.debitoAutomatico
                    ? tr("• %1 (déb. auto, vencido em %2)")
                        .arg(l.descricao, QDate::fromString(l.dataVenc, Qt::ISODate)
                             .toString(uifmt::dataDisplayFormatCurta()))
                    : tr("• %1 (vencido em %2)")
                        .arg(l.descricao, QDate::fromString(l.dataVenc, Qt::ISODate)
                             .toString(uifmt::dataDisplayFormatCurta()));
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
            navegarParaChave(QStringLiteral("tab_lembretes"));
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

// ── Lazy-init por aba (#208, PERF-02) ────────────────────────────────────────
// TabShell em todas as páginas (#158/#165): cada página ganha um invólucro
// com título + descrição + instruções p/ iniciante (texto do registro central
// tabHelp, DRY com o HelpDialog). O shell é o widget efetivamente inserido
// no QStackedWidget; o widget de conteúdo segue intacto (lógica/conexões
// inalteradas).
TabShell *MainWindow::criarShell(const QString &objName, const QString &tabKey,
                                  QWidget *conteudo)
{
    auto *shell = new TabShell(QString(), this);
    shell->setObjectName(objName);
    shell->setTabKey(tabKey);
    shell->applyHelp();
    shell->setContent(conteudo);
    return shell;
}

// Glossário (#103/#104/#105) escopado à aba recém-construída — reduz o N do
// PERF-01 (antes varria a MainWindow inteira, 13 abas, toda vez).
void MainWindow::aplicarGlossarioNaAba(QWidget *shell)
{
    GlossarioDecorator::aplicarTooltips(shell);
    GlossarioDecorator::aplicarLabelsColoquiais(shell);
    GlossarioDecorator::aplicarMicrocopies(shell);
}

void MainWindow::refreshDashboardSeAtivo()
{
    if (m_stack->currentWidget() == m_shellDashboard)
        m_dashboard->recarregar();
}

void MainWindow::refreshPlanejamentoSeAtivo()
{
    if (m_stack->currentWidget() == m_shellPlanejamento)
        m_planejamento->recarregar();
}

void MainWindow::refreshTendenciasSeAtivo()
{
    if (m_stack->currentWidget() == m_shellTendencias)
        m_tendencias->recarregar();
}

// Registra as 13 fábricas em m_router (só o mapa chave→callback; nada é
// construído aqui). Cada fábrica roda no máximo 1 vez, na 1ª navegação para
// a chave (ver LazyPageRouter::garantir). Dentro da fábrica: constrói
// conteúdo+shell, faz TODO o wiring cross-widget que a aba original tinha em
// buildUi (dadosAlterados→recarregar/dirty/refresh-se-visível,
// statusAtualizado→statusbar, setUndoStack), aplica glossário escopado e
// insere no QStackedWidget.
void MainWindow::registrarFabricasAbas()
{
    // ── Dashboard ─────────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_dashboard"), [this]() -> QWidget * {
        m_dashboard = new DashboardWidget(this);
        m_shellDashboard = criarShell(QStringLiteral("tab_dashboard"),
                                       QStringLiteral("tab_dashboard"), m_dashboard);
        aplicarGlossarioNaAba(m_shellDashboard);

        connect(m_dashboard, &DashboardWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellDashboard)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });
        // Dashboard cards clicáveis → navegar para a página correspondente
        // (#157). tabIndex legacy mapeado para chave estável.
        connect(m_dashboard, &DashboardWidget::navegarParaAba,
                this, [this](int tabIndex) {
                    static const QMap<int, QString> kTabParaChave = {
                        { 1,  QStringLiteral("tab_entradas")     },
                        { 2,  QStringLiteral("tab_gastos")       },
                        { 3,  QStringLiteral("tab_cartoes")      },
                        { 9,  QStringLiteral("tab_investimentos")},
                        { 10, QStringLiteral("tab_proventos")    },
                    };
                    const QString chave = kTabParaChave.value(tabIndex);
                    if (!chave.isEmpty()) navegarParaChave(chave);
                });

        m_stack->addWidget(m_shellDashboard);
        return m_shellDashboard;
    });

    // ── Entradas ──────────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_entradas"), [this]() -> QWidget * {
        m_entradas = new EntradasWidget(this);
        m_shellEntradas = criarShell(QStringLiteral("tab_entradas"),
                                      QStringLiteral("tab_entradas"), m_entradas);
        aplicarGlossarioNaAba(m_shellEntradas);
        m_entradas->setUndoStack(m_undoStack);

        connect(m_entradas, &EntradasWidget::dadosAlterados,
                m_entradas, &EntradasWidget::recarregar);
        connect(m_entradas, &EntradasWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_entradas, &EntradasWidget::dadosAlterados,
                this, &MainWindow::refreshDashboardSeAtivo);
        connect(m_entradas, &EntradasWidget::dadosAlterados,
                this, &MainWindow::refreshPlanejamentoSeAtivo);
        connect(m_entradas, &EntradasWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellEntradas)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellEntradas);
        return m_shellEntradas;
    });

    // ── Gastos ────────────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_gastos"), [this]() -> QWidget * {
        m_gastos = new GastosWidget(this);
        m_shellGastos = criarShell(QStringLiteral("tab_gastos"),
                                    QStringLiteral("tab_gastos"), m_gastos);
        aplicarGlossarioNaAba(m_shellGastos);
        m_gastos->setUndoStack(m_undoStack);

        connect(m_gastos, &GastosWidget::dadosAlterados,
                m_gastos, &GastosWidget::recarregar);
        connect(m_gastos, &GastosWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_gastos, &GastosWidget::dadosAlterados,
                this, &MainWindow::refreshDashboardSeAtivo);
        connect(m_gastos, &GastosWidget::dadosAlterados,
                this, &MainWindow::refreshPlanejamentoSeAtivo);
        // Tendências se atualiza quando Gastos muda dados (lazy: só se visível)
        connect(m_gastos, &GastosWidget::dadosAlterados,
                this, &MainWindow::refreshTendenciasSeAtivo);
        connect(m_gastos, &GastosWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellGastos)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellGastos);
        return m_shellGastos;
    });

    // ── Cartões (Parcelas) ───────────────────────────────────────────────
    // objectName do shell é "tab_parcelas" por compat. histórica (dados
    // gravados antes do redesign usavam essa string). A chave de navegação
    // e ajuda é "tab_cartoes" — as duas strings são independentes.
    m_router.registrarFabrica(QStringLiteral("tab_cartoes"), [this]() -> QWidget * {
        m_parcelas = new ParcelasWidget(this);
        m_shellCartoes = criarShell(QStringLiteral("tab_parcelas"),
                                     QStringLiteral("tab_cartoes"), m_parcelas);
        aplicarGlossarioNaAba(m_shellCartoes);
        m_parcelas->setUndoStack(m_undoStack);

        connect(m_parcelas, &ParcelasWidget::dadosAlterados,
                m_parcelas, &ParcelasWidget::recarregar);
        connect(m_parcelas, &ParcelasWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_parcelas, &ParcelasWidget::dadosAlterados,
                this, &MainWindow::refreshDashboardSeAtivo);
        connect(m_parcelas, &ParcelasWidget::dadosAlterados,
                this, &MainWindow::refreshPlanejamentoSeAtivo);
        connect(m_parcelas, &ParcelasWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellCartoes)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellCartoes);
        return m_shellCartoes;
    });

    // ── Recorrências ──────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_recorrencias"), [this]() -> QWidget * {
        m_recorrencias = new RecorrenciaWidget(this);
        m_shellRecorrencias = criarShell(QStringLiteral("tab_recorrencias"),
                                          QStringLiteral("tab_recorrencias"), m_recorrencias);
        aplicarGlossarioNaAba(m_shellRecorrencias);
        m_recorrencias->setUndoStack(m_undoStack);

        connect(m_recorrencias, &RecorrenciaWidget::dadosAlterados,
                m_recorrencias, &RecorrenciaWidget::recarregar);
        connect(m_recorrencias, &RecorrenciaWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_recorrencias, &RecorrenciaWidget::dadosAlterados,
                this, &MainWindow::refreshDashboardSeAtivo);

        m_stack->addWidget(m_shellRecorrencias);
        return m_shellRecorrencias;
    });

    // ── Dívidas ───────────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_dividas"), [this]() -> QWidget * {
        m_dividas = new DividasWidget(this);
        m_shellDividas = criarShell(QStringLiteral("tab_dividas"),
                                     QStringLiteral("tab_dividas"), m_dividas);
        aplicarGlossarioNaAba(m_shellDividas);
        m_dividas->setUndoStack(m_undoStack);

        connect(m_dividas, &DividasWidget::dadosAlterados,
                m_dividas, &DividasWidget::recarregar);
        connect(m_dividas, &DividasWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_dividas, &DividasWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellDividas)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellDividas);
        return m_shellDividas;
    });

    // ── Metas ─────────────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_metas"), [this]() -> QWidget * {
        m_metas = new MetasWidget(this);
        m_shellMetas = criarShell(QStringLiteral("tab_metas"),
                                   QStringLiteral("tab_metas"), m_metas);
        aplicarGlossarioNaAba(m_shellMetas);
        m_metas->setUndoStack(m_undoStack);

        connect(m_metas, &MetasWidget::dadosAlterados,
                m_metas, &MetasWidget::recarregar);
        connect(m_metas, &MetasWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_metas, &MetasWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellMetas)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellMetas);
        return m_shellMetas;
    });

    // ── Planejamento ──────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_planejamento"), [this]() -> QWidget * {
        m_planejamento = new PlanejamentoWidget(this);
        m_shellPlanejamento = criarShell(QStringLiteral("tab_planejamento"),
                                          QStringLiteral("tab_planejamento"), m_planejamento);
        aplicarGlossarioNaAba(m_shellPlanejamento);
        m_planejamento->setUndoStack(m_undoStack);

        connect(m_planejamento, &PlanejamentoWidget::dadosAlterados,
                m_planejamento, &PlanejamentoWidget::recarregar);
        connect(m_planejamento, &PlanejamentoWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_planejamento, &PlanejamentoWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellPlanejamento)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellPlanejamento);
        return m_shellPlanejamento;
    });

    // ── Tendências ────────────────────────────────────────────────────────
    // Sem dadosAlterados/statusAtualizado/undo próprios — é atualizada
    // reativamente quando Gastos muda (wired na fábrica de Gastos acima) e
    // ao trocar de aba (currentChanged, em buildUi).
    m_router.registrarFabrica(QStringLiteral("tab_tendencias"), [this]() -> QWidget * {
        m_tendencias = new TendenciasWidget(this);
        m_shellTendencias = criarShell(QStringLiteral("tab_tendencias"),
                                        QStringLiteral("tab_tendencias"), m_tendencias);
        aplicarGlossarioNaAba(m_shellTendencias);

        m_stack->addWidget(m_shellTendencias);
        return m_shellTendencias;
    });

    // ── Investimentos ─────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_investimentos"), [this]() -> QWidget * {
        m_investimentos = new InvestimentosWidget(this);
        m_shellInvestimentos = criarShell(QStringLiteral("tab_investimentos"),
                                           QStringLiteral("tab_investimentos"), m_investimentos);
        aplicarGlossarioNaAba(m_shellInvestimentos);
        m_investimentos->setUndoStack(m_undoStack);

        connect(m_investimentos, &InvestimentosWidget::dadosAlterados,
                m_investimentos, &InvestimentosWidget::recarregar);
        connect(m_investimentos, &InvestimentosWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_investimentos, &InvestimentosWidget::dadosAlterados,
                this, &MainWindow::refreshDashboardSeAtivo);
        connect(m_investimentos, &InvestimentosWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellInvestimentos)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellInvestimentos);
        return m_shellInvestimentos;
    });

    // ── Proventos ─────────────────────────────────────────────────────────
    // Sem dadosAlterados/statusAtualizado/undo próprios (idem hoje).
    m_router.registrarFabrica(QStringLiteral("tab_proventos"), [this]() -> QWidget * {
        m_proventos = new ProventosWidget(this);
        m_shellProventos = criarShell(QStringLiteral("tab_proventos"),
                                       QStringLiteral("tab_proventos"), m_proventos);
        aplicarGlossarioNaAba(m_shellProventos);

        m_stack->addWidget(m_shellProventos);
        return m_shellProventos;
    });

    // ── Bolsa ─────────────────────────────────────────────────────────────
    // Sem dadosAlterados/statusAtualizado/undo próprios (idem hoje).
    // m_marketService continua eager (leve, sem UI) — só a UI é lazy aqui.
    m_router.registrarFabrica(QStringLiteral("tab_bolsa"), [this]() -> QWidget * {
        m_bolsa = new BolsaWidget(m_marketService, this);
        m_shellBolsa = criarShell(QStringLiteral("tab_bolsa"),
                                   QStringLiteral("tab_bolsa"), m_bolsa);
        aplicarGlossarioNaAba(m_shellBolsa);

        m_stack->addWidget(m_shellBolsa);
        return m_shellBolsa;
    });

    // ── Lembretes ─────────────────────────────────────────────────────────
    m_router.registrarFabrica(QStringLiteral("tab_lembretes"), [this]() -> QWidget * {
        m_lembretes = new LembretesWidget(this);
        m_shellLembretes = criarShell(QStringLiteral("tab_lembretes"),
                                       QStringLiteral("tab_lembretes"), m_lembretes);
        aplicarGlossarioNaAba(m_shellLembretes);
        m_lembretes->setUndoStack(m_undoStack);

        connect(m_lembretes, &LembretesWidget::dadosAlterados,
                m_lembretes, &LembretesWidget::recarregar);
        connect(m_lembretes, &LembretesWidget::dadosAlterados,
                this, [this]() { m_dirty = true; });
        connect(m_lembretes, &LembretesWidget::statusAtualizado,
                this, [this](const QString &msg) {
                    if (m_stack->currentWidget() == m_shellLembretes)
                        m_statusTotal->setText(msg + QStringLiteral("  "));
                });

        m_stack->addWidget(m_shellLembretes);
        return m_shellLembretes;
    });
}

// ── Menu bar (aposentada em #170) ────────────────────────────────────────────
// QMenuBar removida visualmente via setVisible(false). Atalhos de teclado
// são re-registrados via this->addAction() para continuar funcionando sem
// a barra. Ações de navegação foram para o rodapé da sidebar e para o
// cluster de utilidades (buildUtilBar).

void MainWindow::buildMenuBar()
{
    // Ocultar a barra de menu nativa — ações visuais foram para sidebar/toolbar.
    menuBar()->setVisible(false);

    // ── Atalhos de Arquivo (preservados via QAction no MainWindow) ──────────
    auto *aAbrir = new QAction(tr("Abrir…"), this);
    aAbrir->setShortcut(QKeySequence::Open);
    connect(aAbrir, &QAction::triggered, this, &MainWindow::onAbrir);
    addAction(aAbrir);

    auto *aSalvar = new QAction(tr("Salvar"), this);
    aSalvar->setShortcut(QKeySequence::Save);
    connect(aSalvar, &QAction::triggered, this, &MainWindow::onSalvar);
    addAction(aSalvar);

    auto *aSalvarComo = new QAction(tr("Salvar como…"), this);
    aSalvarComo->setShortcut(QKeySequence::SaveAs);
    connect(aSalvarComo, &QAction::triggered, this, &MainWindow::onSalvarComo);
    addAction(aSalvarComo);

    auto *aSair = new QAction(tr("Sair"), this);
    aSair->setShortcut(QKeySequence::Quit);
    connect(aSair, &QAction::triggered, qApp, &QApplication::quit);
    addAction(aSair);

    // ── Atalhos de Edição ──────────────────────────────────────────────────
    auto *aUndo = m_undoStack->createUndoAction(this, tr("Desfazer"));
    aUndo->setShortcut(QKeySequence::Undo);
    addAction(aUndo);

    auto *aRedo = m_undoStack->createRedoAction(this, tr("Refazer"));
    aRedo->setShortcut(QKeySequence::Redo);
    addAction(aRedo);

    auto *aCopiar = new QAction(tr("Copiar"), this);
    aCopiar->setShortcut(QKeySequence::Copy);
    connect(aCopiar, &QAction::triggered, this, &MainWindow::onCopiar);
    addAction(aCopiar);

    auto *aColar = new QAction(tr("Colar"), this);
    aColar->setShortcut(QKeySequence::Paste);
    connect(aColar, &QAction::triggered, this, &MainWindow::onColar);
    addAction(aColar);

    auto *aSelAll = new QAction(tr("Selecionar tudo"), this);
    aSelAll->setShortcut(QKeySequence::SelectAll);
    connect(aSelAll, &QAction::triggered, this, &MainWindow::onSelecionarTudo);
    addAction(aSelAll);

    auto *aBusca = new QAction(tr("Buscar transação…"), this);
    aBusca->setShortcut(QKeySequence::Find);
    connect(aBusca, &QAction::triggered, this, &MainWindow::onBuscaGlobal);
    addAction(aBusca);

    // ── Atalhos de Ajuda ──────────────────────────────────────────────────
    auto *aAjuda = new QAction(tr("Ajuda"), this);
    aAjuda->setShortcut(QKeySequence::HelpContents);
    connect(aAjuda, &QAction::triggered, this, &MainWindow::onAjuda);
    addAction(aAjuda);

    auto *aGlossario = new QAction(tr("Glossário financeiro…"), this);
    aGlossario->setShortcut(QKeySequence(Qt::Key_F2));
    connect(aGlossario, &QAction::triggered, this, &MainWindow::onGlossario);
    addAction(aGlossario);
}

// ── Cluster de utilidades (topo direito) — #170 ──────────────────────────────
// Substitui buildThemeBar: toolbar fina no topo com Busca, Calculadora e
// o menu "⋯ Mais" (Abrir/Salvar/Importar/Exportar/Reconciliar/Glossário/Sobre/Sair).
// Botão de Tema e Preferências foram para o rodapé da sidebar (act_tema / act_prefs).

void MainWindow::buildUtilBar()
{
    auto *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setFloatable(false);
    toolbar->setIconSize({20, 20});
    addToolBar(Qt::TopToolBarArea, toolbar);

    auto makeBtn = [&](const QString &glyph, const QString &tip) -> QToolButton * {
        auto *btn = new QToolButton(toolbar);
        btn->setAutoRaise(true);
        btn->setIcon(Theme::emojiIcon(glyph, 20));
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setToolTip(tip);
        // #198 (A11Y-01, WCAG 4.1.2): fábrica única — cobre busca/calculadora/
        // hoje de uma vez (botões icon-only, sem texto visível).
        btn->setAccessibleName(tip);
        toolbar->addWidget(btn);
        return btn;
    };

    // Busca global
    auto *btnBusca = makeBtn(QStringLiteral("🔍"), tr("Buscar transação… (Ctrl+F)"));
    connect(btnBusca, &QToolButton::clicked, this, &MainWindow::onBuscaGlobal);

    // Calculadora
    auto *btnCalc = makeBtn(QStringLiteral("🧮"), tr("Calculadora"));
    connect(btnCalc, &QToolButton::clicked, this, [this]() {
        CalculadoraHub::instance().mostrarGlobal(this);
    });

    // Hoje (navega tabela para mês atual)
    auto *btnHoje = makeBtn(QStringLiteral("📅"), tr("Hoje"));
    connect(btnHoje, &QToolButton::clicked, this, [this]() {
        if      (m_stack->currentWidget() == m_shellCartoes)       m_parcelas->scrollParaMesAtual();
        else if (m_stack->currentWidget() == m_shellEntradas)      m_entradas->scrollParaMesAtual();
        else if (m_stack->currentWidget() == m_shellGastos)        m_gastos->scrollParaMesAtual();
        else if (m_stack->currentWidget() == m_shellInvestimentos) m_investimentos->scrollParaMesAtual();
    });

    toolbar->addSeparator();

    // ── "⋯ Mais" — menu de utilidades (InstantPopup) ──────────────────────
    auto *btnMais = new QToolButton(toolbar);
    btnMais->setAutoRaise(true);
    btnMais->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnMais->setIcon(Theme::emojiIcon(QStringLiteral("⋯"), 20));
    btnMais->setText(tr(" Mais"));
    btnMais->setToolTip(tr("Mais opções"));
    btnMais->setPopupMode(QToolButton::InstantPopup);
    toolbar->addWidget(btnMais);

    auto *menuMais = new QMenu(btnMais);
    btnMais->setMenu(menuMais);

    // Arquivo
    {
        auto *a = menuMais->addAction(
            Theme::emojiIcon(QStringLiteral("📂"), 16), tr("Abrir…"));
        a->setShortcut(QKeySequence::Open);
        connect(a, &QAction::triggered, this, &MainWindow::onAbrir);
    }
    {
        auto *a = menuMais->addAction(
            Theme::emojiIcon(QStringLiteral("💾"), 16), tr("Salvar"));
        a->setShortcut(QKeySequence::Save);
        connect(a, &QAction::triggered, this, &MainWindow::onSalvar);
    }
    {
        auto *a = menuMais->addAction(
            Theme::emojiIcon(QStringLiteral("💾"), 16), tr("Salvar como…"));
        a->setShortcut(QKeySequence::SaveAs);
        connect(a, &QAction::triggered, this, &MainWindow::onSalvarComo);
    }

    menuMais->addSeparator();

    // Importar submenu
    auto *mImportar = menuMais->addMenu(tr("📁 Importar de"));
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
    auto *mExportar = menuMais->addMenu(tr("💾 Exportar"));
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

    menuMais->addSeparator();

    {
        auto *a = menuMais->addAction(
            Theme::emojiIcon(QStringLiteral("🔄"), 16), tr("Reconciliar extrato…"));
        connect(a, &QAction::triggered, this, &MainWindow::onReconciliar);
    }

    menuMais->addSeparator();

    {
        auto *a = menuMais->addAction(
            Theme::emojiIcon(QStringLiteral("📝"), 16), tr("Glossário financeiro…"));
        a->setShortcut(QKeySequence(Qt::Key_F2));
        connect(a, &QAction::triggered, this, &MainWindow::onGlossario);
    }
    {
        auto *a = menuMais->addAction(
            Theme::emojiIcon(QStringLiteral("ℹ️"), 16), tr("Sobre PeuBud…"));
        connect(a, &QAction::triggered, this, &MainWindow::onSobre);
    }

    menuMais->addSeparator();

    {
        auto *a = menuMais->addAction(
            Theme::emojiIcon(QStringLiteral("🔒"), 16), tr("Bloquear (fechar)"));
        connect(a, &QAction::triggered, this, &MainWindow::onBloquear);
    }
    {
        auto *a = menuMais->addAction(tr("Sair"));
        a->setShortcut(QKeySequence::Quit);
        connect(a, &QAction::triggered, qApp, &QApplication::quit);
    }
}

void MainWindow::applyThemeMode(Theme::ThemeMode mode)
{
    Theme::setMode(mode);
    // Sem m_btnTheme para atualizar — o emoji do rodapé é fixo (🌗 = cicla).
}

// ── Navegação por chave estável (#165 / #170) ────────────────────────────────
// Chaves "act_*" são ações de utilidade (rodapé da sidebar), não páginas.
// Roteadas aqui antes de consultar m_router.

void MainWindow::navegarParaChave(const QString &chave)
{
    // Dispatcher de ações do rodapé (#170)
    if (chave.startsWith(QStringLiteral("act_"))) {
        if (chave == QStringLiteral("act_prefs")) {
            onConfiguracoes();
        } else if (chave == QStringLiteral("act_ajuda")) {
            onAjuda();
        } else if (chave == QStringLiteral("act_tema")) {
            // Cicla: Auto → Light → Dark → Auto
            Theme::ThemeMode next = Theme::ThemeMode::Auto;
            switch (Theme::currentMode()) {
                case Theme::ThemeMode::Auto:  next = Theme::ThemeMode::Light; break;
                case Theme::ThemeMode::Light: next = Theme::ThemeMode::Dark;  break;
                case Theme::ThemeMode::Dark:  next = Theme::ThemeMode::Auto;  break;
            }
            applyThemeMode(next);
        }
        return; // ações de rodapé não alteram o stack nem o item ativo
    }

    // (#208) garantir() constrói a aba na 1ª navegação (idempotente nas
    // seguintes) — é aqui que o "lazy" acontece de fato.
    QWidget *shell = m_router.garantir(chave);
    if (!shell) return;
    m_stack->setCurrentWidget(shell);
    m_sidebar->setAtivo(chave);
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

// ── Utilitário: garantia de pasta (#178) ─────────────────────────────────────

// Garante que `dir` existe antes de gravar.
// - Se já existe → retorna true imediatamente.
// - Se não existe → exibe aviso com dois botões:
//     [Criar pasta]  → tenta QDir().mkpath(dir); se falhar, mostra erro e
//                      retorna false; se ok, retorna true.
//     [Cancelar]     → retorna false (operação de save deve ser abortada).
// Invariante: nunca chamar dentro do loop do autosave — use mkpath silencioso
// lá e o flag m_autosaveAvisado para notificar apenas uma vez por sessão.
bool MainWindow::garantirPastaExiste(const QString &dir)
{
    if (QDir(dir).exists())
        return true;

    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Warning);
    msg.setWindowTitle(tr("Pasta de destino não encontrada"));
    msg.setText(tr("A pasta de destino não existe:"));
    msg.setInformativeText(dir);
    QPushButton *btnCriar    = msg.addButton(tr("Criar pasta"),  QMessageBox::AcceptRole);
    QPushButton *btnCancelar = msg.addButton(tr("Cancelar"),     QMessageBox::RejectRole);
    msg.setDefaultButton(btnCriar);
    msg.exec();

    if (msg.clickedButton() == btnCancelar)
        return false;

    // Usuário escolheu [Criar pasta]
    if (!QDir().mkpath(dir)) {
        QMessageBox::critical(this, tr("Erro ao criar pasta"),
            tr("Não foi possível criar a pasta:\n%1\n\n"
               "Verifique permissões e tente novamente.").arg(dir));
        return false;
    }
    return true;
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

    // (#181) Caminho fixo do autosave: ~/peubud/save/<slug>/database.auto.pbd
    // mkpath é garantido por autosavePathFixo(); sem diálogo (autosave é silencioso).
    const QString slug = UserSession::instance().slug();
    const QString path = AppSettings::autosavePathFixo(slug);

    // (#178/#181) Se o mkpath interno falhou (permissão negada), a pasta não
    // existirá — notifica uma única vez por sessão; não abre popup a cada ciclo.
    const QString dir = QFileInfo(path).absolutePath();
    if (!QDir(dir).exists()) {
        if (!m_autosaveAvisado) {
            m_autosaveAvisado = true;
            statusBar()->showMessage(
                tr("Auto-save: pasta '%1' não pôde ser criada (sem permissão?). "
                   "Verifique as permissões do diretório ~/peubud/save/.")
                    .arg(dir),
                10000);
        }
        return;
    }
    m_autosaveAvisado = false;

    if (DatabaseManager::instance().exportToFile(path))
        statusBar()->showMessage(tr("Auto-salvo em %1").arg(path), 3000);
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
    // (#208) guarda de nulo: aba ainda não construída não tem nada pra
    // recarregar — ela vai ler o arquivo recém-aberto na 1ª vez que for
    // construída (mesmo efeito observável do recarregar() eager de hoje).
    if (m_parcelas) m_parcelas->recarregar();
    if (m_entradas) m_entradas->recarregar();
}

void MainWindow::onSalvar()
{
    // (#181) Caminho fixo: ~/peubud/save/<slug>/database.pbd
    // mkpath é garantido por AppSettings::savePathFixo() no DatabaseManager::save().
    // (#178/#181) Se o mkpath falhou (permissão negada), save() retorna false —
    // a mensagem de erro orienta o usuário sem diálogo de criação de pasta.
    if (DatabaseManager::instance().save()) {
        m_dirty = false;
        m_autosaveAvisado = false;
        statusBar()->showMessage(tr("Arquivo salvo."), 3000);
    } else {
        const QString slug = UserSession::instance().slug();
        QMessageBox::warning(this, tr("Erro ao salvar"),
            tr("Não foi possível salvar o arquivo em:\n%1\n\n"
               "Verifique se há espaço em disco e permissões de escrita.")
                .arg(AppSettings::savePathFixo(slug)));
    }
}

void MainWindow::onSalvarComo()
{
    // (#181) "Salvar como" virou "Salvar cópia como" — exporta uma cópia
    // cifrada para um destino escolhido pelo usuário. O cofre principal
    // permanece em ~/peubud/save/<slug>/database.pbd (inalterado).
    const QString startDir = QDir::homePath();

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Salvar cópia como"),
        startDir,
        tr("PeuBud (*.pbd)"));
    if (path.isEmpty()) return;

    const QString dest = path.endsWith(QStringLiteral(".pbd"), Qt::CaseInsensitive)
                       ? path : path + QStringLiteral(".pbd");

    // Garante que a pasta de destino da cópia existe antes de escrever.
    const QString destDir = QFileInfo(dest).absolutePath();
    if (!garantirPastaExiste(destDir)) {
        statusBar()->showMessage(tr("Exportação cancelada."), 3000);
        return;
    }

    if (!DatabaseManager::instance().exportToFile(dest)) {
        QMessageBox::warning(this, tr("Erro ao exportar cópia"),
            tr("Não foi possível exportar a cópia para:\n%1\n\n"
               "Verifique se há espaço em disco e permissões de escrita.")
                .arg(dest));
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
    if (m_parcelas) m_parcelas->recarregar(); // (#208) guarda de nulo
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
    // (#200/ARQ-05) Lookup no registry de writers em vez de switch(string) —
    // formato novo = novo IExportWriter registrado em ExportWriterRegistry,
    // sem tocar MainWindow nem ExportService.h/.cpp (OCP).
    bool ok = false;
    if (const IExportWriter *writer = ExportWriterRegistry::find(fmt))
        ok = writer->write(path, sections, txs);

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
    // ApplicationModal: bloqueia a principal enquanto Configurações está aberta.
    // Necessário explicitamente no Wayland/KDE Plasma — exec() bloqueia o event
    // loop mas sem esta flag o compositor pode não enforçar a relação parent-child
    // visualmente (usuário consegue clicar na principal). Combinado com parent=this
    // (acima), o dialog também minimiza junto com a MainWindow (#177).
    dlg.setWindowModality(Qt::ApplicationModal);
    auto *lay   = new QVBoxLayout(&dlg);
    // Margem padrão: left/top/right = 12, bottom = 12.
    // O btnRow abaixo tem contentsMargins próprias que garantem folga para
    // a sombra do botão Fechar (#176): ButtonShadowFilter usa blur=8, offsetY=1
    // → extensão ~5px; 10px na direita e 10px abaixo dão folga segura sem
    // depender do clip do QDialogButtonBox (parent imediato do botão).
    lay->setContentsMargins(12, 12, 12, 12);
    auto *prefs = new PrefsWidget(&dlg);
    lay->addWidget(prefs);

    // Linha manual de botão: btnFechar é filho direto de &dlg, não do row.
    // O layout do dlg (margem 12px) + btnRow (margem bottom/right 10px) somam
    // folga suficiente para que a sombra (~5px) não estoure a borda do diálogo.
    auto *btnRow = new QHBoxLayout;
    btnRow->setContentsMargins(0, 6, 10, 10);
    btnRow->addStretch();
    auto *btnFechar = new QPushButton(tr("Fechar"), &dlg);
    btnFechar->setIcon(Theme::emojiIcon(QStringLiteral("❌"), 14));
    connect(btnFechar, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnRow->addWidget(btnFechar);
    lay->addLayout(btnRow);
    // (#208) guardas de nulo: aba ainda não construída não tem widget pra
    // conectar — quando for construída (lazy), lê o estado já atualizado
    // por essas mudanças de Preferências (mesmo efeito observável).
    if (m_parcelas) connect(prefs, &PrefsWidget::cartoesAlterados, m_parcelas, &ParcelasWidget::recarregar);
    if (m_parcelas) connect(prefs, &PrefsWidget::moedaAlterada,    m_parcelas, &ParcelasWidget::recarregar);
    if (m_entradas) connect(prefs, &PrefsWidget::moedaAlterada,    m_entradas, &EntradasWidget::recarregar);
    if (m_entradas) connect(prefs, &PrefsWidget::fontesAlteradas,  m_entradas, &EntradasWidget::recarregar);
    dlg.exec();
}

// ── Editar slots ──────────────────────────────────────────────────────────────

void MainWindow::onCopiar()
{
    // (#208) fallback "else" era m_parcelas->copiar() incondicional — seguro
    // no modelo eager (m_parcelas sempre existia) mas não no lazy (aba
    // Cartões pode nunca ter sido construída). Guarda de nulo: se nenhuma
    // aba com copiar() explícito está ativa E Cartões nunca foi visitada,
    // não há o que copiar (mesmo efeito prático do copiar() num widget fora
    // de foco/sem seleção).
    if      (m_stack->currentWidget() == m_shellEntradas)      m_entradas->copiar();
    else if (m_stack->currentWidget() == m_shellGastos)        m_gastos->copiar();
    else if (m_stack->currentWidget() == m_shellInvestimentos) m_investimentos->copiar();
    else if (m_stack->currentWidget() == m_shellMetas)         m_metas->copiar();
    else if (m_stack->currentWidget() == m_shellLembretes)     m_lembretes->copiar();
    else if (m_parcelas)                                       m_parcelas->copiar();
}
void MainWindow::onColar()         { /* sem suporte — paste requer contexto da célula */ }
void MainWindow::onSelecionarTudo()
{
    // (#208) idem onCopiar(): guarda de nulo no fallback.
    if      (m_stack->currentWidget() == m_shellEntradas)      m_entradas->selecionarTudo();
    else if (m_stack->currentWidget() == m_shellGastos)        m_gastos->selecionarTudo();
    else if (m_stack->currentWidget() == m_shellInvestimentos) m_investimentos->selecionarTudo();
    else if (m_stack->currentWidget() == m_shellMetas)         m_metas->selecionarTudo();
    else if (m_stack->currentWidget() == m_shellLembretes)     m_lembretes->selecionarTudo();
    else if (m_parcelas)                                       m_parcelas->selecionarTudo();
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
        // tabIndex legacy do BuscaGlobalDialog mapeado para chave estável (#157)
        connect(m_buscaGlobal, &BuscaGlobalDialog::navegarParaAba,
                this, [this](int tabIndex) {
                    static const QMap<int, QString> kTabParaChave = {
                        { 1, QStringLiteral("tab_entradas")     },
                        { 2, QStringLiteral("tab_gastos")       },
                        { 3, QStringLiteral("tab_cartoes")      },
                        { 5, QStringLiteral("tab_dividas")      },
                        { 9, QStringLiteral("tab_investimentos")},
                    };
                    const QString chave = kTabParaChave.value(tabIndex);
                    if (!chave.isEmpty()) navegarParaChave(chave);
                });
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
    // Navega para Cartões e abre AddCompraDialog pre-preenchido (#165).
    // (#208) navegarParaChave() já chama m_router.garantir() de forma
    // síncrona — m_parcelas está garantidamente construído neste ponto.
    navegarParaChave(QStringLiteral("tab_cartoes"));
    m_parcelas->novaCompraDeCalculadora(pv, n, iMensal, pmt);
}
