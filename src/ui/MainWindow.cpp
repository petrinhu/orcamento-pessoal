#include "MainWindow.h"
#include "widgets/ParcelasWidget.h"
#include "widgets/EntradasWidget.h"
#include "widgets/PrefsWidget.h"
#include "widgets/AboutDialog.h"
#include "widgets/HelpDialog.h"
#include "core/session/UserSession.h"
#include "core/database/DatabaseManager.h"
#include "core/settings/AppSettings.h"
#include "services/ImportService.h"
#include "services/ExportService.h"
#include "repositories/CompraRepository.h"
#include "repositories/ParcelaRepository.h"

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("PeuBud — Orçamento Pessoal"));
    setMinimumSize(960, 620);
    buildUi();
}

void MainWindow::buildUi()
{
    m_parcelas = new ParcelasWidget(this);
    m_entradas = new EntradasWidget(this);
    m_tabs     = new QTabWidget(this);
    m_tabs->addTab(m_parcelas, tr("Parcelas"));
    m_tabs->addTab(m_entradas, tr("Entradas"));
    setCentralWidget(m_tabs);

    buildMenuBar();
    buildThemeBar();

    // ── Status bar ─────────────────────────────────────────────────────────
    m_statusUser = new QLabel(this);
    const QString name = UserSession::instance().name();
    m_statusUser->setText(tr("  Usuário: %1").arg(name));
    statusBar()->addWidget(m_statusUser);

    // ── Dirty flag ─────────────────────────────────────────────────────────
    connect(m_parcelas, &ParcelasWidget::dadosAlterados,
            m_parcelas, &ParcelasWidget::recarregar);
    connect(m_parcelas, &ParcelasWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });
    connect(m_entradas, &EntradasWidget::dadosAlterados,
            m_entradas, &EntradasWidget::recarregar);
    connect(m_entradas, &EntradasWidget::dadosAlterados,
            this, [this]() { m_dirty = true; });

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

    auto *aSair = mArquivo->addAction(tr("&Sair"));
    aSair->setShortcut(QKeySequence::Quit);
    connect(aSair, &QAction::triggered, qApp, &QApplication::quit);

    // ── Editar ─────────────────────────────────────────────────────────────
    QMenu *mEditar = mb->addMenu(tr("&Editar"));

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
    auto *corner  = new QWidget(this);
    auto *hLayout = new QHBoxLayout(corner);
    hLayout->setContentsMargins(0, 4, 8, 0);
    hLayout->setSpacing(0);

    m_btnTheme = new QToolButton(corner);
    m_btnTheme->setObjectName(QStringLiteral("themeBtn"));
    m_btnTheme->setAutoRaise(true);
    hLayout->addWidget(m_btnTheme);

    connect(m_btnTheme, &QToolButton::clicked, this, [this]() {
        Theme::ThemeMode next;
        switch (Theme::currentMode()) {
            case Theme::ThemeMode::Auto:  next = Theme::ThemeMode::Light; break;
            case Theme::ThemeMode::Light: next = Theme::ThemeMode::Dark;  break;
            case Theme::ThemeMode::Dark:  next = Theme::ThemeMode::Auto;  break;
        }
        applyThemeMode(next);
    });

    updateThemeButton();
    menuBar()->setCornerWidget(corner, Qt::TopRightCorner);
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
    if (!m_dirty) { event->accept(); return; }

    QMessageBox msg(this);
    msg.setIcon(QMessageBox::Warning);
    msg.setWindowTitle(tr("Alterações não salvas"));
    msg.setText(tr("Há alterações não salvas."));
    msg.setInformativeText(tr("Deseja salvar antes de fechar?"));
    QAbstractButton *btnSalvar   = msg.addButton(tr("Salvar"),            QMessageBox::AcceptRole);
    QAbstractButton *btnFechar   = msg.addButton(tr("Fechar sem salvar"), QMessageBox::DestructiveRole);
    msg.addButton(tr("Cancelar"), QMessageBox::RejectRole);
    msg.setDefaultButton(qobject_cast<QPushButton *>(btnSalvar));
    msg.exec();

    if (msg.clickedButton() == btnSalvar) {
        DatabaseManager::instance().save();
        m_dirty = false;
        event->accept();
    } else if (msg.clickedButton() == btnFechar) {
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
    static const QMap<QString,QString> filters = {
        { QStringLiteral("csv"),  tr("CSV (*.csv)") },
        { QStringLiteral("ofx"),  tr("OFX (*.ofx)") },
        { QStringLiteral("qif"),  tr("QIF (*.qif)") },
        { QStringLiteral("xlsx"), tr("Excel (*.xlsx)") },
        { QStringLiteral("xls"),  tr("Excel 97-2003 (*.xls)") },
        { QStringLiteral("ods"),  tr("ODS (*.ods)") },
        { QStringLiteral("txt"),  tr("Texto (*.txt)") },
        { QStringLiteral("json"), tr("JSON (*.json)") },
        { QStringLiteral("md"),   tr("Markdown (*.md)") },
        { QStringLiteral("html"), tr("HTML (*.html)") },
        { QStringLiteral("pdf"),  tr("PDF (*.pdf)") },
    };

    const QString path = QFileDialog::getSaveFileName(
        this, tr("Exportar"),
        QStringLiteral("parcelas.%1").arg(format),
        filters.value(format));
    if (path.isEmpty()) return;

    // ODS stub
    if (format == QStringLiteral("ods")) {
        QMessageBox::information(this, tr("Exportar ODS"),
            tr("Exportação ODS ainda não implementada.\n"
               "Use CSV ou XLSX para abrir no LibreOffice Calc."));
        return;
    }

    // Build dataset
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

    bool ok = false;
    if      (format == QStringLiteral("csv"))  ok = ExportService::toCsv(path, d);
    else if (format == QStringLiteral("ofx"))  ok = ExportService::toOfx(path, d);
    else if (format == QStringLiteral("qif"))  ok = ExportService::toQif(path, d);
    else if (format == QStringLiteral("xlsx") ||
             format == QStringLiteral("xls")) ok = ExportService::toXlsx(path, d);
    else if (format == QStringLiteral("txt"))  ok = ExportService::toTxt(path, d);
    else if (format == QStringLiteral("json")) ok = ExportService::toJson(path, d);
    else if (format == QStringLiteral("md"))   ok = ExportService::toMarkdown(path, d);
    else if (format == QStringLiteral("html")) ok = ExportService::toHtml(path, d);
    else if (format == QStringLiteral("pdf"))  ok = ExportService::toPdf(path, d);

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
    dlg.setMinimumWidth(500);
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
    if (m_tabs->currentWidget() == m_entradas) m_entradas->copiar();
    else m_parcelas->copiar();
}
void MainWindow::onColar()         { /* sem suporte — paste requer contexto da célula */ }
void MainWindow::onSelecionarTudo()
{
    if (m_tabs->currentWidget() == m_entradas) m_entradas->selecionarTudo();
    else m_parcelas->selecionarTudo();
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
