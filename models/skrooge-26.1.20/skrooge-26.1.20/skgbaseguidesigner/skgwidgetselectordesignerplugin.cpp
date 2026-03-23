/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A widget selector (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgwidgetselectordesignerplugin.h"

#include "skgservices.h"
#include "skgwidgetselector.h"

SKGWidgetSelectorDesignerPlugin::SKGWidgetSelectorDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGWidgetSelectorDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGWidgetSelectorDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGWidgetSelectorDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGWidgetSelector(iParent);
}

QString SKGWidgetSelectorDesignerPlugin::name() const
{
    return QStringLiteral("SKGWidgetSelector");
}

QString SKGWidgetSelectorDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGWidgetSelectorDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGWidgetSelectorDesignerPlugin::toolTip() const
{
    return QStringLiteral("A widget selector");
}

QString SKGWidgetSelectorDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A widget selector");
}

bool SKGWidgetSelectorDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGWidgetSelectorDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGWidgetSelector\" name=\"SKGWidgetSelector\">\n"
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

QString SKGWidgetSelectorDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgwidgetselector.h");
}
