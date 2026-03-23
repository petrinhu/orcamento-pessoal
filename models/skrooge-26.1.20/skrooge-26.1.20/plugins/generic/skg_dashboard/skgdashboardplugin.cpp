/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A dashboard
 *
 * @author Stephane MANKOWSKI
 */
#include "skgdashboardplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgdashboardboardwidget.h"
#include "skgdashboardpluginwidget.h"
#include "skgmainpanel.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGDashboardPlugin, "metadata.json")

SKGDashboardPlugin::SKGDashboardPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList &iArg)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
{
    Q_UNUSED(iArg)
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGDashboardPlugin::~SKGDashboardPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
}

bool SKGDashboardPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_dashboard"), title());
    setXMLFile(QStringLiteral("skg_dashboard.rc"));

    // Create yours actions here
    return true;
}

int SKGDashboardPlugin::getNbDashboardWidgets()
{
    return 1;
}

QString SKGDashboardPlugin::getDashboardWidgetTitle(int iIndex)
{
    Q_UNUSED(iIndex)
    return i18nc("Noun, the title of a section", "Sub dashboard");
}

SKGBoardWidget *SKGDashboardPlugin::getDashboardWidget(int iIndex)
{
    Q_UNUSED(iIndex)
    return new SKGDashboardboardWidget(SKGMainPanel::getMainPanel(), m_currentDocument);
}

SKGTabPage *SKGDashboardPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGDashboardPluginWidget(SKGMainPanel::getMainPanel(), m_currentDocument);
}

QString SKGDashboardPlugin::title() const
{
    return i18nc("Noun, a summary of your financial situation", "Dashboard");
}

QString SKGDashboardPlugin::icon() const
{
    return QStringLiteral("dashboard-show");
}

QString SKGDashboardPlugin::toolTip() const
{
    return i18nc("Noun, a summary of your financial situation", "Dashboard");
}

int SKGDashboardPlugin::getOrder() const
{
    return 8;
}

QStringList SKGDashboardPlugin::tips() const
{
    QStringList output;
    output.push_back(
        i18nc("Description of a tips", "<p>… the <a href=\"skg://dashboard_plugin\">dashboard</a> is there to give you a summary of your situation.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… the <a href=\"skg://dashboard_plugin\">dashboard</a> can be reorganized by drag and drop.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… some widgets of the <a href=\"skg://dashboard_plugin\">dashboard</a> have parameters.</p>"));
    return output;
}

bool SKGDashboardPlugin::isInPagesChooser() const
{
    return true;
}

#include <skgdashboardplugin.moc>
