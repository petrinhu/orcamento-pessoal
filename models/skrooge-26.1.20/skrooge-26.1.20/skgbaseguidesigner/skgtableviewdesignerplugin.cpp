/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A table view with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtableviewdesignerplugin.h"

#include <qicon.h>

#include "skgtableview.h"

SKGTableViewDesignerPlugin::SKGTableViewDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGTableViewDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGTableViewDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGTableViewDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGTableView(iParent);
}

QString SKGTableViewDesignerPlugin::name() const
{
    return QStringLiteral("SKGTableView");
}

QString SKGTableViewDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGTableViewDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGTableViewDesignerPlugin::toolTip() const
{
    return QStringLiteral("A table view with more features");
}

QString SKGTableViewDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A table view with more features");
}

bool SKGTableViewDesignerPlugin::isContainer() const
{
    return true;
}

QString SKGTableViewDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGTableView\" name=\"SKGTableView\">\n"
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

QString SKGTableViewDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgtableview.h");
}
