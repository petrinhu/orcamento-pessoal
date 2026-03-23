/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A Graphic view with more functionalities.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skggraphicsview.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qdesktopservices.h>
#include <qdom.h>
#include <qfileinfo.h>
#include <qgraphicssceneevent.h>
#include <qheaderview.h>
#include <qmath.h>
#include <qmenu.h>
#include <qprintdialog.h>
#include <qprinter.h>
#include <qsvggenerator.h>
#include <qwidgetaction.h>

#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGGraphicsView::SKGGraphicsView(QWidget *iParent)
    : QWidget(iParent)
    , m_oscale(1)
    , m_toolBarVisible(true)
{
    ui.setupUi(this);
    setAntialiasing(true);

    ui.kGraphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui.kGraphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    graphicsView()->installEventFilter(this);

    // Set icons
    ui.kPrint->setIcon(SKGServices::fromTheme(QStringLiteral("printer")));
    ui.kCopy->setIcon(SKGServices::fromTheme(QStringLiteral("edit-copy")));

    // Build contextual menu
    graphicsView()->setContextMenuPolicy(Qt::CustomContextMenu);

    m_mainMenu = new QMenu(graphicsView());

    // Zoom in menu
    auto zoomMenu = new SKGZoomSelector(this);
    zoomMenu->setResetValue(ui.kZoom->resetValue());
    zoomMenu->setValue(ui.kZoom->value());

    auto zoomWidget = new QWidgetAction(this);
    zoomWidget->setDefaultWidget(zoomMenu);
    m_mainMenu->addAction(zoomWidget);

    connect(zoomMenu, &SKGZoomSelector::changed, this, [this](int val) {
        ui.kZoom->setValue(val);
    });
    connect(ui.kZoom, &SKGZoomSelector::changed, this, [zoomMenu](int val) {
        zoomMenu->setValue(val);
    });

    m_actShowToolBar = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("show-menu")), i18nc("Noun, user action", "Show tool bar"));
    if (m_actShowToolBar != nullptr) {
        m_actShowToolBar->setCheckable(true);
        m_actShowToolBar->setChecked(true);
        connect(m_actShowToolBar, &QAction::triggered, this, &SKGGraphicsView::onSwitchToolBarVisibility);
    }

    m_mainMenu->addSeparator();

    QAction *actCopy = m_mainMenu->addAction(ui.kCopy->icon(), ui.kCopy->toolTip());
    connect(actCopy, &QAction::triggered, this, &SKGGraphicsView::onCopy);

    QAction *actPrint = m_mainMenu->addAction(ui.kPrint->icon(), ui.kPrint->toolTip());
    connect(actPrint, &QAction::triggered, this, &SKGGraphicsView::onPrint);

    QAction *actExport = m_mainMenu->addAction(SKGServices::fromTheme(QStringLiteral("document-export")), i18nc("Noun, user action", "Export…"));
    connect(actExport, &QAction::triggered, this, &SKGGraphicsView::onExport);

    connect(graphicsView(), &QHeaderView::customContextMenuRequested, this, &SKGGraphicsView::showMenu);

    connect(ui.kPrint, &QToolButton::clicked, this, &SKGGraphicsView::onPrint);
    connect(ui.kCopy, &QToolButton::clicked, this, &SKGGraphicsView::onCopy);
    connect(ui.kZoom, &SKGZoomSelector::changed, this, &SKGGraphicsView::onZoom);

    // Timer to refresh
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, ui.kZoom, &SKGZoomSelector::initializeZoom, Qt::QueuedConnection);
}

SKGGraphicsView::~SKGGraphicsView()
{
    m_actShowToolBar = nullptr;
    m_actZoomOriginal = nullptr;
    m_mainMenu = nullptr;
    m_actAntialiasing = nullptr;
}

void SKGGraphicsView::setScene(QGraphicsScene *iScene)
{
    graphicsView()->setScene(iScene);
    if (iScene != nullptr) {
        iScene->installEventFilter(this);
    }
    onZoom();
}

QGraphicsView *SKGGraphicsView::graphicsView()
{
    return ui.kGraphicsView;
}

QString SKGGraphicsView::getState() const
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("isToolBarVisible"), isToolBarVisible() ? QStringLiteral("Y") : QStringLiteral("N"));

    return doc.toString();
}

void SKGGraphicsView::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();
    setToolBarVisible(root.attribute(QStringLiteral("isToolBarVisible")) != QStringLiteral("N"));
}

QMenu *SKGGraphicsView::getContextualMenu() const
{
    return m_mainMenu;
}

void SKGGraphicsView::showMenu(const QPoint iPos)
{
    if (m_mainMenu != nullptr) {
        m_mainMenu->popup(graphicsView()->mapToGlobal(iPos));
    }
}

void SKGGraphicsView::setToolBarVisible(bool iVisibility)
{
    if (m_toolBarVisible != iVisibility) {
        m_toolBarVisible = iVisibility;
        ui.toolBar->setVisible(m_toolBarVisible);
        if (m_actShowToolBar != nullptr) {
            m_actShowToolBar->setChecked(m_toolBarVisible);
        }
        Q_EMIT toolBarVisiblyChanged();
    }
}

bool SKGGraphicsView::isToolBarVisible() const
{
    return m_toolBarVisible;
}

void SKGGraphicsView::addToolbarWidget(QWidget *iWidget)
{
    ui.layout->insertWidget(3, iWidget);
}

void SKGGraphicsView::onSwitchToolBarVisibility()
{
    setToolBarVisible(!isToolBarVisible());
}

void SKGGraphicsView::setAntialiasing(bool iAntialiasing)
{
    graphicsView()->setRenderHint(QPainter::Antialiasing, iAntialiasing);
}

bool SKGGraphicsView::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if (iObject == graphicsView()->scene() && (iEvent != nullptr) && iEvent->type() == QEvent::GraphicsSceneWheel) {
        auto *e = dynamic_cast<QGraphicsSceneWheelEvent *>(iEvent);
        if ((e != nullptr) && e->orientation() == Qt::Vertical && ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u)) {
            int numDegrees = e->delta() / 8;
            int numTicks = numDegrees / 15;

            if (numTicks > 0) {
                ui.kZoom->zoomIn();
            } else {
                ui.kZoom->zoomOut();
            }
            e->setAccepted(true);
            return true;
        }
    } else if (iObject == graphicsView() && (iEvent != nullptr) && iEvent->type() == QEvent::Resize) {
        Q_EMIT resized();
        if (ui.kZoom->value() == ui.kZoom->resetValue()) {
            m_timer.start(300);
        }
    }
    return QWidget::eventFilter(iObject, iEvent);
}

void SKGGraphicsView::initializeZoom()
{
    ui.kZoom->initializeZoom();
}

void SKGGraphicsView::onZoom()
{
    _SKGTRACEINFUNC(10)
    int sliderValue = ui.kZoom->value();
    if (graphicsView()->scene() != nullptr) {
        if (sliderValue == -10) {
            graphicsView()->fitInView(graphicsView()->scene()->sceneRect(), Qt::KeepAspectRatio);
            m_oscale = 1;
        } else {
            qreal scale = qPow(qreal(1.5), (sliderValue + 10.0) / qreal(4));
            graphicsView()->scale(scale / m_oscale, scale / m_oscale);
            m_oscale = scale;
        }
    }
}

void SKGGraphicsView::onPrint()
{
    _SKGTRACEINFUNC(10)
    QPrinter printer;
    QPointer<QPrintDialog> dialog = new QPrintDialog(&printer, this);
    if (dialog->exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        graphicsView()->render(&painter);
        painter.end();
    }
}

void SKGGraphicsView::onExport()
{
    _SKGTRACEINFUNC(10)
    QString fileName = SKGMainPanel::getSaveFileName(QStringLiteral("kfiledialog:///IMPEXP"),
                                                     QStringLiteral("application/pdf image/svg+xml image/png image/jpeg image/gif image/tiff"),
                                                     this,
                                                     nullptr);
    if (fileName.isEmpty()) {
        return;
    }

    exportInFile(fileName);

    QDesktopServices::openUrl(QUrl(fileName));
}

void SKGGraphicsView::exportInFile(const QString &iFileName)
{
    _SKGTRACEINFUNC(10)
    QString extension = QFileInfo(iFileName).suffix().toUpper();
    if (extension == QStringLiteral("PDF")) {
        QPrinter printer;
        printer.setOutputFileName(iFileName);
        QPainter painter(&printer);
        graphicsView()->render(&painter);
        painter.end();
    } else if (extension == QStringLiteral("SVG")) {
        QSvgGenerator generator;
        generator.setFileName(iFileName);
        generator.setSize(QSize(200, 200));
        generator.setViewBox(QRect(0, 0, 200, 200));
        generator.setTitle(i18nc("Title of the content SVG export", "Skrooge SVG export"));
        generator.setDescription(i18nc("Description of the content SVG export", "A SVG drawing created by the Skrooge."));

        QPainter painter(&generator);
        graphicsView()->render(&painter);
        painter.end();
    } else {
        QImage image(graphicsView()->size(), QImage::Format_ARGB32);
        QPainter painter(&image);
        graphicsView()->render(&painter);
        painter.end();
        image.save(iFileName);
    }
}

void SKGGraphicsView::onCopy()
{
    _SKGTRACEINFUNC(10)
    QClipboard *clipboard = QApplication::clipboard();
    if (clipboard != nullptr) {
        QImage image(graphicsView()->size(), QImage::Format_ARGB32);
        QPainter painter(&image);
        graphicsView()->render(&painter);
        painter.end();
        clipboard->setImage(image);
    }
}
