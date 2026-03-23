/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A tree view with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtreeviewdesignerplugin.h"

#include <qicon.h>

#include "skgtreeview.h"

SKGTreeViewDesignerPlugin::SKGTreeViewDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGTreeViewDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGTreeViewDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGTreeViewDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGTreeView(iParent);
}

QString SKGTreeViewDesignerPlugin::name() const
{
    return QStringLiteral("SKGTreeView");
}

QString SKGTreeViewDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGTreeViewDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGTreeViewDesignerPlugin::toolTip() const
{
    return QStringLiteral("A tree view with more features");
}

QString SKGTreeViewDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A tree view with more features");
}

bool SKGTreeViewDesignerPlugin::isContainer() const
{
    return true;
}

QString SKGTreeViewDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGTreeView\" name=\"SKGTreeView\">\n"
        " <property name=\"geometry\">\n"
        "  <rect>\n"
        "   <x>0</x>\n"
        "   <y>0</y>\n"
        "   <width>100</width>\n"
        "   <height>100</height>\n"
        "  </rect>\n"
        " </property>\n"
        "</widget>\n");
}

QString SKGTreeViewDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgtreeview.h");
}
