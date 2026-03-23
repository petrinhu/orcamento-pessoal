/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin to print pages
 *
 * @author Stephane MANKOWSKI
 */
#include "skgprintplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include <qbuffer.h>
#include <qdesktopservices.h>
#include <qdir.h>
#include <qpainter.h>
#include <qpicture.h>
#include <qprintdialog.h>
#include <qprintpreviewdialog.h>
#include <qsavefile.h>
#include <qtextbrowser.h>

#include "skgmainpanel.h"
#include "skgprint_settings.h"
#include "skgtraces.h"
#include "skgtreeview.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGPrintPlugin, "metadata.json")

SKGPrintPlugin::SKGPrintPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iWidget)
    m_printer.setResolution(QPrinter::HighResolution);
}

SKGPrintPlugin::~SKGPrintPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
}

bool SKGPrintPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_print"), title());
    setXMLFile(QStringLiteral("skg_print.rc"));

    registerGlobalAction(QStringLiteral("file_print"), KStandardAction::print(this, SLOT(onPrint()), actionCollection()), QStringList(), -1);
    registerGlobalAction(QStringLiteral("file_print_preview"),
                         KStandardAction::printPreview(this, SLOT(onPrintPreview()), actionCollection()),
                         QStringList(),
                         -1);

    auto actPrintHtmlAction =
        new QAction(SKGServices::fromTheme(QStringLiteral("preview")), i18nc("Verb, print in an html file", "Print into a html file"), this);
    connect(actPrintHtmlAction, &QAction::triggered, this, &SKGPrintPlugin::onPrintHtml);
    registerGlobalAction(QStringLiteral("file_print_html"), actPrintHtmlAction, QStringList(), -1);

    return true;
}

KConfigSkeleton *SKGPrintPlugin::getPreferenceSkeleton()
{
    return skgprint_settings::self();
}

QString SKGPrintPlugin::title() const
{
    return i18nc("Verb, action to use a printer", "print");
}

int SKGPrintPlugin::getOrder() const
{
    return 2;
}

QStringList SKGPrintPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… you can <a href=\"skg://file_print\">print</a> all opened pages.</p>"));
    return output;
}

void SKGPrintPlugin::onPrint()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    if (SKGMainPanel::getMainPanel() != nullptr) {
        QPointer<QPrintDialog> dialog = new QPrintDialog(&m_printer, SKGMainPanel::getMainPanel());
        dialog->setOption(QAbstractPrintDialog::PrintCurrentPage, true);
        dialog->setMinMax(1, SKGMainPanel::getMainPanel()->getTabWidget()->count());
        if (dialog->exec() == QDialog::Accepted) {
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
            print(&m_printer);
            QApplication::restoreOverrideCursor();
        }
    }
}

void SKGPrintPlugin::onPrintPreview()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // TODO(SMI): QWebEngine does not work
    QPointer<QPrintPreviewDialog> dialog = new QPrintPreviewDialog(SKGMainPanel::getMainPanel());
    connect(dialog.data(), &QPrintPreviewDialog::paintRequested, this, &SKGPrintPlugin::print);
    dialog->exec();
}

void SKGPrintPlugin::onPrintHtml()
{
    QString html;
    getHtml(&m_printer, html);

    QString fileName = QDir::tempPath() % "/skrooge.html";
    QSaveFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream out(&file);
        out << html;

        // Close file
        file.commit();
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
}

SKGError SKGPrintPlugin::getHtml(QPrinter *iPrinter, QString &oHtml) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if ((SKGMainPanel::getMainPanel() != nullptr) && (iPrinter != nullptr)) {
        QString html;

        // Get printer options
        int docCopies;
        int pageCopies;
        if (iPrinter->collateCopies()) {
            docCopies = 1;
            pageCopies = iPrinter->copyCount();
        } else {
            docCopies = iPrinter->copyCount();
            pageCopies = 1;
        }
        int fromPage = qMin(iPrinter->fromPage(), iPrinter->toPage());
        int toPage = qMax(iPrinter->fromPage(), iPrinter->toPage());

        // Compute the number of pages
        SKGTabWidget *tabs = SKGMainPanel::getMainPanel()->getTabWidget();
        int nbpages = tabs->count();

        if (fromPage == 0 && toPage == 0) {
            fromPage = 1;
            toPage = nbpages;
        }
        if (iPrinter->printRange() == QPrinter::CurrentPage) {
            fromPage = SKGMainPanel::getMainPanel()->currentPageIndex() + 1;
            toPage = fromPage;
        }

        SKGTRACEL(10) << "Nb copy document=" << docCopies << Qt::endl;
        SKGTRACEL(10) << "Nb copy page=" << docCopies << Qt::endl;
        SKGTRACEL(10) << "From=" << fromPage << Qt::endl;
        SKGTRACEL(10) << "To=" << toPage << Qt::endl;

        // Copy document
        for (int c = 1; !err && c <= docCopies; ++c) {
            for (int i = 1; !err && i <= nbpages; ++i) {
                // Compute page
                int pageToTreat = (iPrinter->pageOrder() == QPrinter::LastPageFirst ? nbpages + 1 - i : i);

                // Do we have to print it
                if (pageToTreat >= fromPage && pageToTreat <= toPage) {
                    // Yes, get the widget
                    // Copy pages
                    for (int p = 1; !err && p <= pageCopies; ++p) {
                        auto *page = qobject_cast<SKGTabPage *>(tabs->widget(pageToTreat - 1));
                        if (page != nullptr) {
                            // Add page break if needed
                            if (!html.isEmpty()) {
                                html = html % "<div style=\"page-break-after:always\"></div>";
                            }

                            // Add widgets
                            QList<QWidget *> widgets = page->printableWidgets();
                            int nbw = widgets.count();
                            for (int j = 0; !err && j < nbw; ++j) {
                                QWidget *w = widgets.at(j);
                                if (w != nullptr) {
                                    // Add widget
                                    /*SKGTreeView* tw = qobject_cast< SKGTreeView* >(w);
                                    if (tw) {
                                        QTextBrowser* tb = tw->getTextBrowser();
                                        if (tb) {
                                            html = html % tb->toHtml();
                                            delete tb;
                                        }
                                    } else {*/
#ifdef SKG_WEBENGINE
                                    auto q = qobject_cast<QWebEngineView *>(w);
#endif
#if !defined(SKG_WEBENGINE)
                                    auto q = qobject_cast<QTextBrowser *>(w);
#endif
                                    if (q != nullptr) {
#ifdef SKG_WEBENGINE
                                        q->page()->toHtml([&](const QString &result) {
                                            html = html % result;
                                        });
                                        qApp->processEvents(QEventLoop::AllEvents, 5000);
#endif
#if !defined(SKG_WEBENGINE)
                                        html = html % q->toHtml();
#endif
                                    } else {
                                        // Save palette
                                        QPalette previousPalette = w->palette();

                                        // Draw in a picture with white background
                                        QPalette palette;
                                        palette.setColor(QPalette::Window, Qt::white);
                                        w->setPalette(palette);
                                        QImage image = w->grab().toImage();

                                        // Restore palette
                                        w->setPalette(previousPalette);

                                        QByteArray byteArray;
                                        QBuffer buffer(&byteArray);
                                        image.save(&buffer, "PNG");

                                        QString imgBase64 = QString::fromLatin1(byteArray.toBase64().data());
                                        html = html % "<img src=\"data:image/png;base64," % imgBase64 % "\" />";
                                    }
                                    // }
                                }
                            }
                        }
                    }
                }
            }
        }

        oHtml = "<body style=\"background-color:white;\">" + html + "</body>";
    }
    return err;
}

void SKGPrintPlugin::print(QPrinter *iPrinter)
{
    SKGTRACEINFUNC(10)
    if ((SKGMainPanel::getMainPanel() != nullptr) && (iPrinter != nullptr)) {
        QString html;
        SKGError err = getHtml(iPrinter, html);

        // Print
        m_toPrint.setFixedSize(QSize(iPrinter->width(), iPrinter->height()));
#ifdef SKG_WEBENGINE
        // TODO(SMI): QWebEngine - doesn't work - bad quality printing
        disconnect(&m_toPrint);
        connect(&m_toPrint, &QWebEngineView::loadFinished, &m_toPrint, [this, iPrinter]() {
            m_toPrint.print(iPrinter);
        });
        m_toPrint.setHtml(html, QUrl(QStringLiteral("file://")));
#endif
#if !defined(SKG_WEBENGINE)
        m_toPrint.setHtml(html);
        m_toPrint.print(iPrinter);
#endif

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Print successfully done.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Print failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

#include <skgprintplugin.moc>
