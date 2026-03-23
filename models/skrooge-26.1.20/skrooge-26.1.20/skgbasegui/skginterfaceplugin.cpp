/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin interface default implementation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skginterfaceplugin.h"
#include "skgmainpanel.h"

#include "kactioncollection.h"

SKGInterfacePlugin::SKGInterfacePlugin(QObject *iParent)
    : KParts::ReadOnlyPart(iParent)
{
}
SKGInterfacePlugin::~SKGInterfacePlugin() = default;

QStringList SKGInterfacePlugin::processArguments(const QStringList &iArgument)
{
    return iArgument;
}

void SKGInterfacePlugin::close()
{
    disconnect(this);
}

void SKGInterfacePlugin::refresh()
{
}

void SKGInterfacePlugin::registerGlobalAction(const QString &iIdentifier,
                                              QAction *iAction,
                                              const QStringList &iListOfTable,
                                              int iMinSelection,
                                              int iMaxSelection,
                                              int iRanking,
                                              bool iSelectionMustHaveFocus)
{
    actionCollection()->addAction(iIdentifier, iAction);
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGMainPanel::getMainPanel()
            ->registerGlobalAction(iIdentifier, iAction, false, iListOfTable, iMinSelection, iMaxSelection, iRanking, iSelectionMustHaveFocus);
    }
}

QDockWidget *SKGInterfacePlugin::getDockWidget()
{
    return nullptr;
}

SKGTabPage *SKGInterfacePlugin::getWidget()
{
    return nullptr;
}

int SKGInterfacePlugin::getNbDashboardWidgets()
{
    return 0;
}

QString SKGInterfacePlugin::getDashboardWidgetTitle(int iIndex)
{
    Q_UNUSED(iIndex)
    return QString();
}

SKGBoardWidget *SKGInterfacePlugin::getDashboardWidget(int iIndex)
{
    Q_UNUSED(iIndex)
    return nullptr;
}

void SKGInterfacePlugin::initPreferences()
{
}

QWidget *SKGInterfacePlugin::getPreferenceWidget()
{
    return nullptr;
}

KConfigSkeleton *SKGInterfacePlugin::getPreferenceSkeleton()
{
    return nullptr;
}

SKGError SKGInterfacePlugin::savePreferences() const
{
    return SKGError();
}

int SKGInterfacePlugin::getOrder() const
{
    return 999;
}

QString SKGInterfacePlugin::statusTip() const
{
    return toolTip();
}

QString SKGInterfacePlugin::icon() const
{
    return QString();
}

QString SKGInterfacePlugin::toolTip() const
{
    return title();
}

QStringList SKGInterfacePlugin::tips() const
{
    return QStringList();
}

QStringList SKGInterfacePlugin::subPlugins() const
{
    return QStringList();
}

bool SKGInterfacePlugin::isInPagesChooser() const
{
    return false;
}

bool SKGInterfacePlugin::isEnabled() const
{
    return true;
}

SKGAdviceList SKGInterfacePlugin::advice(const QStringList &iIgnoredAdvice)
{
    Q_UNUSED(iIgnoredAdvice)
    return SKGAdviceList();
}

SKGError SKGInterfacePlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    Q_UNUSED(iAdviceIdentifier)
    Q_UNUSED(iSolution)
    return SKGError(ERR_NOTIMPL, QString());
}
