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
#include "skgcolorbuttondesignerplugin.h"

#include "skgcolorbutton.h"
#include "skgservices.h"

SKGColorButtonDesignerPlugin::SKGColorButtonDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGColorButtonDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGColorButtonDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGColorButtonDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGColorButton(iParent);
}

QString SKGColorButtonDesignerPlugin::name() const
{
    return QStringLiteral("SKGColorButton");
}

QString SKGColorButtonDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGColorButtonDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGColorButtonDesignerPlugin::toolTip() const
{
    return QStringLiteral("A color button with more features");
}

QString SKGColorButtonDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A color button with more features");
}

bool SKGColorButtonDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGColorButtonDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGColorButton\" name=\"SKGColorButton\">\n"
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

QString SKGColorButtonDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgcolorbutton.h");
}
