#include "MainWindow.h"
#include "widgets/ParcelasWidget.h"
#include "widgets/ConfigWidget.h"
#include "widgets/AboutDialog.h"
#include "widgets/HelpDialog.h"
#include "core/session/UserSession.h"
#include "core/database/DatabaseManager.h"
#include "services/ImportService.h"
#include "services/ExportService.h"
#include "repositories/CompraRepository.h"
#include "repositories/ParcelaRepository.h"

#include <QTabWidget>
#include <QLabel>
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
#include <QKeyEvent>
#include <QDialog>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QSqlQuery>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("PeuBud — Orçamento Pessoal"));
    setMinimumSize(960, 620);
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

    buildMenuBar();
    buildThemeBar();

    // ── Status bar ─────────────────────────────────────────────────────────
    m_statusUser = new QLabel(this);
    const QString name = UserSession::instance().name();
    m_statusUser->setText(QStringLiteral("  Usuário: %1").arg(name));
    statusBar()->addWidget(m_statusUser);

    // ── Connections ────────────────────────────────────────────────────────
    connect(m_config, &ConfigWidget::cartoesAlterados,
            m_parcelas, &ParcelasWidget::recarregar);
    connect(m_parcelas, &ParcelasWidget::dadosAlterados,
            m_parcelas, &ParcelasWidget::recarregar);
}

// ── Menu bar ──────────────────────────────────────────────────────────────────

void MainWindow::buildMenuBar()
{
    QMenuBar *mb = menuBar();

    // ── Arquivo ────────────────────────────────────────────────────────────
    QMenu *mArquivo = mb->addMenu(QStringLiteral("&Arquivo"));

    auto *aAbrir = mArquivo->addAction(QStringLiteral("&Abrir…"));
    aAbrir->setShortcut(QKeySequence::Open);
    connect(aAbrir, &QAction::triggered, this, &MainWindow::onAbrir);

    auto *aSalvar = mArquivo->addAction(QStringLiteral("&Salvar"));
    aSalvar->setShortcut(QKeySequence::Save);
    connect(aSalvar, &QAction::triggered, this, &MainWindow::onSalvar);

    auto *aSalvarComo = mArquivo->addAction(QStringLiteral("Salvar &como…"));
    aSalvarComo->setShortcut(QKeySequence::SaveAs);
    connect(aSalvarComo, &QAction::triggered, this, &MainWindow::onSalvarComo);

    mArquivo->addSeparator();

    // Importar submenu
    QMenu *mImportar = mArquivo->addMenu(QStringLiteral("&Importar de"));
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
    QMenu *mExportar = mArquivo->addMenu(QStringLiteral("&Exportar"));
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

    auto *aSair = mArquivo->addAction(QStringLiteral("&Sair"));
    aSair->setShortcut(QKeySequence::Quit);
    connect(aSair, &QAction::triggered, qApp, &QApplication::quit);

    // ── Editar ─────────────────────────────────────────────────────────────
    QMenu *mEditar = mb->addMenu(QStringLiteral("&Editar"));

    auto *aCopiar = mEditar->addAction(QStringLiteral("&Copiar"));
    aCopiar->setShortcut(QKeySequence::Copy);
    connect(aCopiar, &QAction::triggered, this, &MainWindow::onCopiar);

    auto *aColar = mEditar->addAction(QStringLiteral("Co&lar"));
    aColar->setShortcut(QKeySequence::Paste);
    connect(aColar, &QAction::triggered, this, &MainWindow::onColar);

    mEditar->addSeparator();

    auto *aSelAll = mEditar->addAction(QStringLiteral("Selecionar &tudo"));
    aSelAll->setShortcut(QKeySequence::SelectAll);
    connect(aSelAll, &QAction::triggered, this, &MainWindow::onSelecionarTudo);

    // ── Configurações ──────────────────────────────────────────────────────
    QMenu *mConfig = mb->addMenu(QStringLiteral("&Configurações"));

    auto *aCartoes = mConfig->addAction(QStringLiteral("&Cartões…"));
    connect(aCartoes, &QAction::triggered, this, &MainWindow::onCartoes);

    mConfig->addSeparator();

    auto *aPrefs = mConfig->addAction(QStringLiteral("&Preferências…"));
    aPrefs->setEnabled(false); // stub

    // ── Ajuda ──────────────────────────────────────────────────────────────
    QMenu *mAjuda = mb->addMenu(QStringLiteral("A&juda"));

    auto *aAjuda = mAjuda->addAction(QStringLiteral("&Ajuda"));
    aAjuda->setShortcut(QKeySequence::HelpContents);
    connect(aAjuda, &QAction::triggered, this, &MainWindow::onAjuda);

    mAjuda->addSeparator();

    auto *aSobre = mAjuda->addAction(QStringLiteral("&Sobre PeuBud…"));
    connect(aSobre, &QAction::triggered, this, &MainWindow::onSobre);
}

// ── Theme bar ─────────────────────────────────────────────────────────────────

void MainWindow::buildThemeBar()
{
    auto *corner  = new QWidget(this);
    auto *hLayout = new QHBoxLayout(corner);
    hLayout->setContentsMargins(0, 4, 8, 0);
    hLayout->setSpacing(2);

    struct BtnDef { QString label; QString tip; Theme::ThemeMode mode; QString themeIcon; };
    const QList<BtnDef> defs = {
        { QStringLiteral("☀"),  QStringLiteral("Modo claro"),  Theme::ThemeMode::Light, QStringLiteral("weather-clear")         },
        { QStringLiteral("◑"),  QStringLiteral("Modo auto"),   Theme::ThemeMode::Auto,  QStringLiteral("weather-partly-cloudy")  },
        { QStringLiteral("☾"),  QStringLiteral("Modo escuro"), Theme::ThemeMode::Dark,  QStringLiteral("weather-clear-night")    },
    };

    QToolButton **ptrs[3] = { &m_btnLight, &m_btnAuto, &m_btnDark };

    for (int i = 0; i < 3; ++i) {
        auto *btn = new QToolButton(corner);
        const auto &def = defs[i];

        // Prefer theme icon; fall back to Unicode label
        const QIcon icon = QIcon::fromTheme(def.themeIcon);
        if (icon.isNull()) {
            btn->setText(def.label);
            btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        } else {
            btn->setIcon(icon);
            btn->setIconSize({20, 20});
            btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        }

        btn->setToolTip(def.tip);
        btn->setCheckable(true);
        btn->setAutoRaise(true);
        btn->setObjectName(QStringLiteral("themeBtn"));
        hLayout->addWidget(btn);

        *ptrs[i] = btn;

        const Theme::ThemeMode mode = def.mode;
        connect(btn, &QToolButton::clicked, this, [this, mode]() {
            applyThemeMode(mode);
        });
    }

    m_btnAuto->setChecked(true);
    m_tabs->setCornerWidget(corner, Qt::TopRightCorner);
}

void MainWindow::applyThemeMode(Theme::ThemeMode mode)
{
    Theme::setMode(mode);
    m_btnLight->setChecked(mode == Theme::ThemeMode::Light);
    m_btnAuto ->setChecked(mode == Theme::ThemeMode::Auto);
    m_btnDark ->setChecked(mode == Theme::ThemeMode::Dark);
}

// ── Arquivo slots ─────────────────────────────────────────────────────────────

void MainWindow::onAbrir()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Abrir arquivo PeuBud"),
        QString(),
        QStringLiteral("PeuBud (*.pbd);;Todos os arquivos (*.*)"));
    if (path.isEmpty()) return;

    if (!DatabaseManager::instance().importFromFile(path)) {
        QMessageBox::warning(this, QStringLiteral("Erro ao abrir"),
            QStringLiteral("Não foi possível carregar o arquivo.\n"
                           "Verifique se o arquivo é válido e se a senha está correta."));
        return;
    }
    m_parcelas->recarregar();
}

void MainWindow::onSalvar()
{
    // Current session auto-saves on close; this is a no-op confirmation
    QMessageBox::information(this, QStringLiteral("Salvo"),
        QStringLiteral("Os dados são salvos automaticamente ao fechar o PeuBud.\n"
                       "Use \"Salvar como…\" para exportar uma cópia."));
}

void MainWindow::onSalvarComo()
{
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Salvar cópia como"),
        QString(),
        QStringLiteral("PeuBud (*.pbd)"));
    if (path.isEmpty()) return;

    const QString dest = path.endsWith(QStringLiteral(".pbd"), Qt::CaseInsensitive)
                       ? path : path + QStringLiteral(".pbd");
    if (!DatabaseManager::instance().exportToFile(dest)) {
        QMessageBox::warning(this, QStringLiteral("Erro"),
            QStringLiteral("Não foi possível salvar o arquivo."));
    }
}

// ── Import ────────────────────────────────────────────────────────────────────

void MainWindow::onImportar(const QString &format)
{
    static const QMap<QString,QString> filters = {
        { QStringLiteral("csv"),  QStringLiteral("CSV (*.csv);;Todos (*.*)") },
        { QStringLiteral("ofx"),  QStringLiteral("OFX/QFX (*.ofx *.qfx);;Todos (*.*)") },
        { QStringLiteral("qif"),  QStringLiteral("QIF (*.qif);;Todos (*.*)") },
        { QStringLiteral("xlsx"), QStringLiteral("Excel (*.xlsx *.xls);;Todos (*.*)") },
        { QStringLiteral("ods"),  QStringLiteral("ODS (*.ods);;Todos (*.*)") },
        { QStringLiteral("txt"),  QStringLiteral("Texto/CNAB (*.txt *.cnab);;Todos (*.*)") },
        { QStringLiteral("json"), QStringLiteral("JSON (*.json);;Todos (*.*)") },
        { QStringLiteral("md"),   QStringLiteral("Markdown (*.md);;Todos (*.*)") },
        { QStringLiteral("html"), QStringLiteral("HTML (*.html *.htm);;Todos (*.*)") },
        { QStringLiteral("pdf"),  QStringLiteral("PDF (*.pdf);;Todos (*.*)") },
    };

    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Importar"),
        QString(), filters.value(format, QStringLiteral("Todos (*.*)")));
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
        QMessageBox::warning(this, QStringLiteral("Importar"), result.error);
        return;
    }

    const auto reply = QMessageBox::question(
        this, QStringLiteral("Confirmar importação"),
        QStringLiteral("%1 transação(ões) encontradas. Deseja importar?")
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

    m_parcelas->recarregar();
    QMessageBox::information(this, QStringLiteral("Importar"),
        QStringLiteral("%1 compra(s) importada(s) com sucesso.")
            .arg(result.compras.size()));
}

// ── Export ────────────────────────────────────────────────────────────────────

void MainWindow::onExportar(const QString &format)
{
    static const QMap<QString,QString> filters = {
        { QStringLiteral("csv"),  QStringLiteral("CSV (*.csv)") },
        { QStringLiteral("ofx"),  QStringLiteral("OFX (*.ofx)") },
        { QStringLiteral("qif"),  QStringLiteral("QIF (*.qif)") },
        { QStringLiteral("xlsx"), QStringLiteral("Excel (*.xlsx)") },
        { QStringLiteral("xls"),  QStringLiteral("Excel 97-2003 (*.xls)") },
        { QStringLiteral("ods"),  QStringLiteral("ODS (*.ods)") },
        { QStringLiteral("txt"),  QStringLiteral("Texto (*.txt)") },
        { QStringLiteral("json"), QStringLiteral("JSON (*.json)") },
        { QStringLiteral("md"),   QStringLiteral("Markdown (*.md)") },
        { QStringLiteral("html"), QStringLiteral("HTML (*.html)") },
        { QStringLiteral("pdf"),  QStringLiteral("PDF (*.pdf)") },
    };

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Exportar"),
        QStringLiteral("parcelas.%1").arg(format),
        filters.value(format));
    if (path.isEmpty()) return;

    // ODS stub
    if (format == QStringLiteral("ods")) {
        QMessageBox::information(this, QStringLiteral("Exportar ODS"),
            QStringLiteral("Exportação ODS ainda não implementada.\n"
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
        QMessageBox::warning(this, QStringLiteral("Exportar"),
            QStringLiteral("Não foi possível exportar o arquivo."));
    else
        QMessageBox::information(this, QStringLiteral("Exportar"),
            QStringLiteral("Arquivo exportado com sucesso:\n%1").arg(path));
}

// ── Editar slots ──────────────────────────────────────────────────────────────

void MainWindow::onCopiar()
{
    // Copy selected row(s) as tab-separated text
    const auto *table = m_parcelas->findChild<QWidget*>(QString(), Qt::FindChildrenRecursively);
    Q_UNUSED(table)
    // Trigger native copy on the currently focused widget
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
    QApplication::sendEvent(QApplication::focusWidget() ? QApplication::focusWidget() : this, &ev);
}

void MainWindow::onColar()
{
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
    QApplication::sendEvent(QApplication::focusWidget() ? QApplication::focusWidget() : this, &ev);
}

void MainWindow::onSelecionarTudo()
{
    QKeyEvent ev(QEvent::KeyPress, Qt::Key_A, Qt::ControlModifier);
    QApplication::sendEvent(QApplication::focusWidget() ? QApplication::focusWidget() : this, &ev);
}

// ── Configurações slots ───────────────────────────────────────────────────────

void MainWindow::onCartoes()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Cartões"));
    dlg.resize(400, 320);
    auto *layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(0, 0, 0, 12);

    auto *widget = new ConfigWidget(&dlg);
    layout->addWidget(widget);

    auto *btns = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    layout->addWidget(btns);
    layout->setContentsMargins(12, 0, 12, 12);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
    connect(widget, &ConfigWidget::cartoesAlterados, m_parcelas, &ParcelasWidget::recarregar);

    dlg.exec();
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
