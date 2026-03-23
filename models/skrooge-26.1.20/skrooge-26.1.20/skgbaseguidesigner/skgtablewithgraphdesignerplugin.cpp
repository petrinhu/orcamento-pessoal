/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A table with graph with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtablewithgraphdesignerplugin.h"

#include <qicon.h>

#include "skgtablewithgraph.h"

SKGTableWithGraphDesignerPlugin::SKGTableWithGraphDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGTableWithGraphDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGTableWithGraphDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGTableWithGraphDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGTableWithGraph(iParent);
}

QString SKGTableWithGraphDesignerPlugin::name() const
{
    return QStringLiteral("SKGTableWithGraph");
}

QString SKGTableWithGraphDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGTableWithGraphDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGTableWithGraphDesignerPlugin::toolTip() const
{
    return QStringLiteral("A Table with graph");
}

QString SKGTableWithGraphDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A Table with graph");
}

bool SKGTableWithGraphDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGTableWithGraphDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGTableWithGraph\" name=\"SKGTableWithGraph\">\n"
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

QString SKGTableWithGraphDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgtablewithgraph.h");
}
