/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A QTabWidget (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtabwidgetdesignerplugin.h"

#include "skgservices.h"
#include "skgtabwidget.h"

SKGTabWidgetDesignerPlugin::SKGTabWidgetDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGTabWidgetDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGTabWidgetDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGTabWidgetDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGTabWidget(iParent);
}

QString SKGTabWidgetDesignerPlugin::name() const
{
    return QStringLiteral("SKGTabWidget");
}

QString SKGTabWidgetDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGTabWidgetDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGTabWidgetDesignerPlugin::toolTip() const
{
    return QStringLiteral("A QTabWidget with more feature");
}

QString SKGTabWidgetDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A QTabWidget with more feature");
}

bool SKGTabWidgetDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGTabWidgetDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGTabWidget\" name=\"SKGTabWidget\">\n"
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

QString SKGTabWidgetDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgtabwidget.h");
}
