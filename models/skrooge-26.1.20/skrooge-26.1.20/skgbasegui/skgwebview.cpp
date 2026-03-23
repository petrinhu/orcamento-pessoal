/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A web viewer with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgwebview.h"

#include <qapplication.h>
#include <qclipboard.h>
#include <qdesktopservices.h>
#include <qevent.h>
#include <qpointer.h>
#include <qprintdialog.h>
#include <qprintpreviewdialog.h>
#include <qtextdocument.h>
#include <qtextdocumentwriter.h>
#ifdef SKG_WEBENGINE
#include <qwebenginepage.h>
#include <qwebenginesettings.h>
#endif
#include <qfileinfo.h>
#include <qmath.h>
#include <qpainter.h>

#include <kstandardaction.h>
#include <qdir.h>
#include <qdom.h>
#include <qicon.h>
#include <qmenu.h>
#include <qnetworkreply.h>
#include <qnetworkrequest.h>
#include <qsavefile.h>

#include <cmath>

#include "skgmainpanel.h"
#include "skgtraces.h"
#ifdef SKG_WEBENGINE
class SKGWebEnginePage : public QWebEnginePage
{
    Q_OBJECT
    Q_DISABLE_COPY(SKGWebEnginePage)
public:
    explicit SKGWebEnginePage(QObject *p = nullptr)
        : QWebEnginePage(p)
    {
    }

    virtual bool acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) override
    {
        if (type == QWebEnginePage::NavigationTypeLinkClicked) {
            if (url.toString().startsWith(QStringLiteral("https://linkclicked/"))) {
                SKGWebView *v = qobject_cast<SKGWebView *>(this->parent());
                if (v) {
                    v->emitLinkClicked(url);
                    return false;
                }
            } else {
                SKGMainPanel::getMainPanel()->openPage(url);
                return false;
            }
        }
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }
};

SKGWebView::SKGWebView(QWidget *iParent, const char *name, bool iWithContextualMenu)
    : QWebEngineView(iParent)
    , m_ContextualMenu(iWithContextualMenu)
{
    setObjectName(name);
    const auto &p = new SKGWebEnginePage(this);
    p->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    p->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    setPage(p);
    if (m_ContextualMenu) {
        this->installEventFilter(this);
        page()->installEventFilter(this);
    }

    connect(this, &SKGWebView::fileExporter, this, [](const QString &iFileName) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(iFileName));
    });
}

void SKGWebView::emitLinkClicked(const QUrl &iURL)
{
    Q_EMIT linkClicked(iURL);
}
#else
SKGWebView::SKGWebView(QWidget *iParent, const char *name)
    : QScrollArea(iParent)
{
    auto label = new QLabel(this);
    setWidget(label);
    setWidgetResizable(true);

    setObjectName(name);
    label->setTextFormat(Qt::RichText);
    connect(label, &QLabel::linkActivated, this, [this](const QString &val) {
        SKGMainPanel::getMainPanel()->openPage(val);
    });
}
#endif

SKGWebView::~SKGWebView() = default;

QString SKGWebView::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);
#if defined(SKG_WEBENGINE)
    root.setAttribute(QStringLiteral("zoomFactor"), SKGServices::intToString(qMax(qRound(30.0 * log10(zoomFactor())), -10)));
#endif
    return doc.toString();
}

void SKGWebView::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();
#if defined(SKG_WEBENGINE)
    QString zoomPosition = root.attribute(QStringLiteral("zoomFactor"));
    if (zoomPosition.isEmpty()) {
        zoomPosition = '0';
    }
    double z = qPow(10, (static_cast<qreal>(SKGServices::stringToInt(zoomPosition)) / 30.0));
    setZoomFactor(z);
    Q_EMIT zoomChanged(z);
#endif
}

#if defined(SKG_WEBENGINE)
void SKGWebView::contextMenuEvent(QContextMenuEvent *iEvent)
{
    if (iEvent != nullptr) {
        auto menu = new QMenu(this);
#ifdef SKG_WEBENGINE
        menu->addAction(pageAction(QWebEnginePage::Copy));
#endif
        QAction *actPrint = menu->addAction(SKGServices::fromTheme(QStringLiteral("printer")), i18nc("Action", "Print…"));
        connect(actPrint, &QAction::triggered, this, &SKGWebView::onPrint);

        menu->addAction(KStandardAction::printPreview(this, SLOT(onPrintPreview()), this));

        QAction *actExport = menu->addAction(SKGServices::fromTheme(QStringLiteral("document-export")), i18nc("Noun, user action", "Export…"));
        connect(actExport, &QAction::triggered, this, &SKGWebView::onExport);

        menu->popup(this->mapToGlobal(iEvent->pos()));

        iEvent->accept();
    }
}
#endif
bool SKGWebView::eventFilter(QObject *iObject, QEvent *iEvent)
{
    SKGTRACEINFUNC(10)
    if ((iEvent != nullptr) && iEvent->type() == QEvent::Wheel) {
        auto *e = dynamic_cast<QWheelEvent *>(iEvent);
        if (e != nullptr) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u) {
                int numDegrees = e->angleDelta().y() / 8;
                int numTicks = numDegrees / 15;

                if (numTicks > 0) {
                    onZoomIn();
                } else {
                    onZoomOut();
                }
                e->setAccepted(true);
                return true;
            }
        }
    }
    return QWidget::eventFilter(iObject, iEvent);
}

void SKGWebView::onZoomIn()
{
    _SKGTRACEINFUNC(10)
#if defined(SKG_WEBENGINE)
    int z = qMin(static_cast<int>(qRound(30.0 * log10(zoomFactor()))) + 1, 10);
    setZoomFactor(qPow(10, static_cast<qreal>(z) / 30.0));
    Q_EMIT zoomChanged(z);
#endif
}

void SKGWebView::onZoomOut()
{
    _SKGTRACEINFUNC(10)
#if defined(SKG_WEBENGINE)
    int z = qMax(static_cast<int>(qRound(30.0 * log10(zoomFactor()))) - 1, -10);
    setZoomFactor(qPow(10, static_cast<qreal>(z) / 30.0));
    Q_EMIT zoomChanged(z);
#endif
}

void SKGWebView::onZoomOriginal()
{
    _SKGTRACEINFUNC(10)
#if defined(SKG_WEBENGINE)
    setZoomFactor(0);
    Q_EMIT zoomChanged(0);
#endif
}

void SKGWebView::exportInFile(const QString &iFileName)
{
    QString extension = QFileInfo(iFileName).suffix().toUpper();
    if (extension == QStringLiteral("ODT")) {
#ifdef SKG_WEBENGINE
        this->page()->toHtml([iFileName, this](const QString &result) {
            QTextDocument doc;
            QTextDocumentWriter docWriter(iFileName);
            doc.setHtml(result);
            docWriter.write(&doc);

            Q_EMIT fileExporter(iFileName);
        });
#endif
    } else if (extension == QStringLiteral("PDF")) {
#ifdef SKG_WEBENGINE
        this->page()->printToPdf(iFileName);
        connect(page(), &QWebEnginePage::pdfPrintingFinished, this, &SKGWebView::fileExporter);
#endif
    } else if (extension == QStringLiteral("HTML") || extension == QStringLiteral("HTM")) {
#ifdef SKG_WEBENGINE
        this->page()->toHtml([iFileName, this](const QString &result) {
            QSaveFile file(iFileName);
            if (file.open(QIODevice::WriteOnly)) {
                QTextStream out(&file);
                out << result;

                // Close file
                file.commit();
                Q_EMIT fileExporter(iFileName);
            }
        });
#endif
    } else {
        QImage image(this->size(), QImage::Format_ARGB32);
        QPainter painter(&image);
        this->render(&painter);
        painter.end();
        image.save(iFileName);

        Q_EMIT fileExporter(iFileName);
    }
}

void SKGWebView::onExport()
{
    _SKGTRACEINFUNC(10)
    QString fileName = SKGMainPanel::getSaveFileName(
        QStringLiteral("kfiledialog:///IMPEXP"),
        QStringLiteral("application/pdf text/html application/vnd.oasis.opendocument.text image/png image/jpeg image/gif image/tiff"),
        this);
    if (fileName.isEmpty()) {
        return;
    }

    exportInFile(fileName);
}

void SKGWebView::onPrintPreview()
{
    SKGTRACEINFUNC(10)
    QPointer<QPrintPreviewDialog> dialog = new QPrintPreviewDialog(this);
#ifdef SKG_WEBENGINE
    // TODO(SMI): QWebEngine
    connect(dialog.data(), &QPrintPreviewDialog::paintRequested, page(), [&](QPrinter *printer) {
        print(printer);
    });
#endif
    dialog->exec();
}

void SKGWebView::onPrint()
{
    _SKGTRACEINFUNC(10)
    QPointer<QPrintDialog> dialog = new QPrintDialog(&m_printer, this);
    if (dialog->exec() == QDialog::Accepted) {
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#ifdef SKG_WEBENGINE
        print(&m_printer);
#endif
        QApplication::restoreOverrideCursor();
    }
}

#include "skgwebview.moc"
