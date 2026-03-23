/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A filtered SKGTableView (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgfilteredtableviewdesignerplugin.h"

#include <qicon.h>

#include <qplugin.h>

#include "skgfilteredtableview.h"

SKGFilteredTableViewDesignerPlugin::SKGFilteredTableViewDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGFilteredTableViewDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGFilteredTableViewDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGFilteredTableViewDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGFilteredTableView(iParent);
}

QString SKGFilteredTableViewDesignerPlugin::name() const
{
    return QStringLiteral("SKGFilteredTableView");
}

QString SKGFilteredTableViewDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGFilteredTableViewDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGFilteredTableViewDesignerPlugin::toolTip() const
{
    return QStringLiteral("A filtered SKGTableView");
}

QString SKGFilteredTableViewDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A filtered SKGTableView");
}

bool SKGFilteredTableViewDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGFilteredTableViewDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGFilteredTableView\" name=\"SKGFilteredTableView\">\n"
        " <property name=\"geometry\">\n"
        "  <rect>\n"
        "   <x>0</x>\n"
        "   <y>0</y>\n"
        "   <width>100</width>\n"
        "   <height>10</height>\n"
        "  </rect>\n"
        " </property>\n"
        "</widget>\n");
}

QString SKGFilteredTableViewDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgfilteredtableview.h");
}
