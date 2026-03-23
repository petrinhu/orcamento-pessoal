/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A table widget with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtablewidgetdesignerplugin.h"

#include "skgservices.h"
#include "skgtablewidget.h"

SKGTableWidgetDesignerPlugin::SKGTableWidgetDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGTableWidgetDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGTableWidgetDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGTableWidgetDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGTableWidget(iParent);
}

QString SKGTableWidgetDesignerPlugin::name() const
{
    return QStringLiteral("SKGTableWidget");
}

QString SKGTableWidgetDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGTableWidgetDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGTableWidgetDesignerPlugin::toolTip() const
{
    return QStringLiteral("A table widget with more features");
}

QString SKGTableWidgetDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A table widget with more features");
}

bool SKGTableWidgetDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGTableWidgetDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGTableWidget\" name=\"SKGTableWidget\">\n"
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

QString SKGTableWidgetDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgtablewidget.h");
}
