/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin to advice
 *
 * @author Stephane MANKOWSKI
 */
#include "skgadviceplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgadviceboardwidget.h"
#include "skgmainpanel.h"
#include "skgtipofdayboardwidget.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGAdvicePlugin, "metadata.json")

SKGAdvicePlugin::SKGAdvicePlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , currentDocument(nullptr)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iWidget)
}

SKGAdvicePlugin::~SKGAdvicePlugin()
{
    SKGTRACEINFUNC(10)
    currentDocument = nullptr;
}

bool SKGAdvicePlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_advice"), title());
    setXMLFile(QStringLiteral("skg_advice.rc"));

    // Create yours actions here
    return true;
}

int SKGAdvicePlugin::getNbDashboardWidgets()
{
    SKGTRACEINFUNC(1)
    return 2;
}

QString SKGAdvicePlugin::getDashboardWidgetTitle(int iIndex)
{
    SKGTRACEINFUNC(1)
    if (iIndex == 0) {
        return i18nc("Noun, a list of items", "Advice");
    }
    return i18nc("Noun, a list of items", "Tip of the day");
}

SKGBoardWidget *SKGAdvicePlugin::getDashboardWidget(int iIndex)
{
    if (iIndex == 0) {
        return new SKGAdviceBoardWidget(SKGMainPanel::getMainPanel(), currentDocument);
    }
    return new SKGTipOfDayBoardWidget(SKGMainPanel::getMainPanel(), currentDocument);
}

QString SKGAdvicePlugin::title() const
{
    return i18nc("The title", "Advice");
}

QString SKGAdvicePlugin::icon() const
{
    return QStringLiteral("help-hint");
}

QString SKGAdvicePlugin::toolTip() const
{
    return i18nc("The tool tip", "Advice");
}

int SKGAdvicePlugin::getOrder() const
{
    return 1;
}

QStringList SKGAdvicePlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tip",
                           "<p>… Skrooge can give you advice on how to manage your accounts. See the <a href=\"skg://dashboard_plugin\">dashboard</a>.</p>"));
    output.push_back(i18nc("Description of a tip",
                           "<p>… Skrooge can automatically apply recommended corrections. See the <a href=\"skg://dashboard_plugin\">dashboard</a>.</p>"));
    return output;
}

#include <skgadviceplugin.moc>
