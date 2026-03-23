/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A widget to select what to show (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgshowdesignerplugin.h"

#include <qicon.h>

#include "skgservices.h"
#include "skgshow.h"

SKGShowDesignerPlugin::SKGShowDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGShowDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGShowDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGShowDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGShow(iParent);
}

QString SKGShowDesignerPlugin::name() const
{
    return QStringLiteral("SKGShow");
}

QString SKGShowDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGShowDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGShowDesignerPlugin::toolTip() const
{
    return QStringLiteral("A widget to select what to show");
}

QString SKGShowDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A widget to select what to show");
}

bool SKGShowDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGShowDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGShow\" name=\"SKGShow\">\n"
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

QString SKGShowDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgshow.h");
}
