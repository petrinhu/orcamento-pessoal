/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A Graphic view with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skggraphicsviewdesignerplugin.h"

#include "skggraphicsview.h"
#include "skgservices.h"

SKGGraphicsViewDesignerPlugin::SKGGraphicsViewDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGGraphicsViewDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGGraphicsViewDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGGraphicsViewDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGGraphicsView(iParent);
}

QString SKGGraphicsViewDesignerPlugin::name() const
{
    return QStringLiteral("SKGGraphicsView");
}

QString SKGGraphicsViewDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGGraphicsViewDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGGraphicsViewDesignerPlugin::toolTip() const
{
    return QStringLiteral("A Graphic view");
}

QString SKGGraphicsViewDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A Graphic view");
}

bool SKGGraphicsViewDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGGraphicsViewDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGGraphicsView\" name=\"SKGGraphicsView\">\n"
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

QString SKGGraphicsViewDesignerPlugin::includeFile() const
{
    return QStringLiteral("skggraphicsview.h");
}
