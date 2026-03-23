/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin for monthly report
 *
 * @author Stephane MANKOWSKI
 */
#include "skgmonthlyplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include <qdesktopservices.h>
#include <qdir.h>
#include <qdiriterator.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qstandardpaths.h>

#include "skgmainpanel.h"
#include "skgmonthlypluginwidget.h"
#include "skgreport.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGMonthlyPlugin, "metadata.json")

SKGMonthlyPlugin::SKGMonthlyPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
    , m_mainPage(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGMonthlyPlugin::~SKGMonthlyPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

void SKGMonthlyPlugin::close()
{
    m_mainPage = nullptr;
}
bool SKGMonthlyPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = iDocument;

    setComponentName(QStringLiteral("skg_monthly"), title());
    setXMLFile(QStringLiteral("skg_monthly.rc"));

    // Make needed paths
    QString writablePath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QString path = writablePath % u'/' + KAboutData::applicationData().componentName() % QStringLiteral("/html/default");
    if (!QDir(writablePath).mkpath(KAboutData::applicationData().componentName() % QStringLiteral("/html/default"))) {
        SKGTRACE << "WARNING: Impossible to create  html/default in " << writablePath << Qt::endl;
    }

    // Copy "default" directory in locale
    const auto dirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                KAboutData::applicationData().componentName() % "/html/default",
                                                QStandardPaths::LocateDirectory);
    for (const auto &dir : dirs) {
        QDirIterator it(dir, QStringList() << QStringLiteral("*.html"));
        while (it.hasNext()) {
            QString file = it.next();

            QString target = path % u'/' % QFileInfo(file).fileName();
            QFile(target).remove();
            if (file != target && !QFile(file).copy(target)) {
                SKGTRACE << "WARNING: Impossible to copie " << file << " in " << target << Qt::endl;
            }
        }
    }

    // Create yours actions here
    return true;
}

QStringList SKGMonthlyPlugin::processArguments(const QStringList &iArgument)
{
    // Initialise the main page
    if (m_mainPage == nullptr) {
#ifdef SKG_WEBENGINE
        m_mainPage = new SKGWebView(SKGMainPanel::getMainPanel(), nullptr, false);
#endif
#if !defined(SKG_WEBENGINE)
        m_mainPage = new QLabel(SKGMainPanel::getMainPanel());
        m_mainPage->setTextFormat(Qt::RichText);
        // m_mainPage->setOpenLinks(true);
        connect(m_mainPage, &QLabel::linkActivated, this, [](const QString &val) {
            SKGMainPanel::getMainPanel()->openPage(QUrl(val));
        });
#endif
        SKGMainPanel::getMainPanel()->setMainWidget(m_mainPage);
        refreshMainPage();
        connect(m_currentBankDocument, &SKGDocument::transactionSuccessfullyEnded, this, &SKGMonthlyPlugin::refreshMainPage);
        qApp->processEvents(QEventLoop::AllEvents, 500);
    }
    return iArgument;
}

void SKGMonthlyPlugin::refreshMainPage()
{
    QString html;

    QString templateFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, KAboutData::applicationData().componentName() % "/html/main.txt");
    if (templateFile.isEmpty()) {
        html = i18nc("Error message", "File %1/html/main.txt not found", KAboutData::applicationData().componentName());
    } else {
        if ((m_currentBankDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
            SKGReport *rep = m_currentBankDocument->getReport();

            // Enrich with tips of the day
            rep->setTipsOfDay(SKGMainPanel::getMainPanel()->getTipsOfDay());

            SKGError err = SKGReport::getReportFromTemplate(rep, templateFile, html);
            IFKO(err) html += err.getFullMessageWithHistorical();
            delete rep;
        }
    }
    if (m_mainPage) {
#ifdef SKG_WEBENGINE
        m_mainPage->setHtml(html, QUrl(QStringLiteral("file://")));
#endif
#if !defined(SKG_WEBENGINE)
        m_mainPage->setText(html);
#endif
    }
}

SKGTabPage *SKGMonthlyPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGMonthlyPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QString SKGMonthlyPlugin::title() const
{
    return toolTip();
}

QString SKGMonthlyPlugin::icon() const
{
    return QStringLiteral("view-calendar-journal");
}

QString SKGMonthlyPlugin::toolTip() const
{
    return i18nc("A tool tip", "Monthly report");
}

int SKGMonthlyPlugin::getOrder() const
{
    return 50;
}

QStringList SKGMonthlyPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… you can generate a <a href=\"skg://monthly_plugin\">monthly report</a>.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can download more monthly report <a href=\"skg://monthly_plugin\">templates</a>.</p>"));
    output.push_back(
        i18nc("Description of a tips", "<p>… you can create and share your own <a href=\"skg://monthly_plugin\">monthly report</a> template.</p>"));
    return output;
}

bool SKGMonthlyPlugin::isInPagesChooser() const
{
    return true;
}

#include <skgmonthlyplugin.moc>
