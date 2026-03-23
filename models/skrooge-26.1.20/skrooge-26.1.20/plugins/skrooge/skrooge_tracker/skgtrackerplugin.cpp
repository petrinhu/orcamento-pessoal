/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to track transactions
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtrackerplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgdocumentbank.h"
#include "skgtraces.h"
#include "skgtracker_settings.h"
#include "skgtrackerpluginwidget.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGTrackerPlugin, "metadata.json")

SKGTrackerPlugin::SKGTrackerPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGTrackerPlugin::~SKGTrackerPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGTrackerPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    if (qobject_cast<SKGDocumentBank *>(iDocument) == nullptr) {
        return false;
    }

    m_currentBankDocument = iDocument;

    setComponentName(QStringLiteral("skrooge_tracker"), title());
    setXMLFile(QStringLiteral("skrooge_tracker.rc"));

    // Create yours actions here
    return true;
}

SKGTabPage *SKGTrackerPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGTrackerPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

KConfigSkeleton *SKGTrackerPlugin::getPreferenceSkeleton()
{
    return skgtracker_settings::self();
}

QString SKGTrackerPlugin::title() const
{
    return i18nc("Noun, something that is used to track items", "Trackers");
}

QString SKGTrackerPlugin::icon() const
{
    return QStringLiteral("crosshairs");
}

QString SKGTrackerPlugin::toolTip() const
{
    return i18nc("A tool tip", "Trackers management");
}

int SKGTrackerPlugin::getOrder() const
{
    return 25;
}

QStringList SKGTrackerPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>…you can follow your refunds by using a <a href=\"skg://skrooge_tracker_plugin\">tracker</a>.</p>"));
    return output;
}

bool SKGTrackerPlugin::isInPagesChooser() const
{
    return true;
}

SKGAdviceList SKGTrackerPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    // Search old trackers
    if (!iIgnoredAdvice.contains(QStringLiteral("skgtrackerplugin_old"))) {
        SKGStringListList result;
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT t_name  FROM v_refund_display WHERE t_close='N' AND julianday('now', 'localtime')-julianday(d_LASTDATE)>300 ORDER BY "
                           "julianday('now', 'localtime')-julianday(d_LASTDATE) DESC;"),
            result);
        int nb = result.count();
        output.reserve(nb);
        for (int i = 1; i < nb; ++i) { // Ignore header
            SKGAdvice ad;
            ad.setUUID("skgtrackerplugin_old|" % result.at(i).at(0));
            ad.setPriority(2);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "'%1' is an old tracker", result.at(i).at(0)));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)",
                      "This tracker does not contain recent transaction. You may want to close it if you do not intend to add other transactions"));
            output.push_back(ad);
        }
    }

    return output;
}

#include <skgtrackerplugin.moc>
