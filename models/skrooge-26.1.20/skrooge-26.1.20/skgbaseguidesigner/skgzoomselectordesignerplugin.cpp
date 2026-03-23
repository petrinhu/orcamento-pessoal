/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A color button with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgzoomselectordesignerplugin.h"

#include "skgservices.h"
#include "skgzoomselector.h"

SKGZoomSelectorDesignerPlugin::SKGZoomSelectorDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGZoomSelectorDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGZoomSelectorDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGZoomSelectorDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGZoomSelector(iParent);
}

QString SKGZoomSelectorDesignerPlugin::name() const
{
    return QStringLiteral("SKGZoomSelector");
}

QString SKGZoomSelectorDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGZoomSelectorDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGZoomSelectorDesignerPlugin::toolTip() const
{
    return QStringLiteral("A color button with more features");
}

QString SKGZoomSelectorDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A color button with more features");
}

bool SKGZoomSelectorDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGZoomSelectorDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGZoomSelector\" name=\"SKGZoomSelector\">\n"
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

QString SKGZoomSelectorDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgzoomselector.h");
}
