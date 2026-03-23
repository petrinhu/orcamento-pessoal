/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A period editor (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgperiodeditdesignerplugin.h"

#include "skgperiodedit.h"
#include "skgservices.h"

SKGPeriodEditDesignerPlugin::SKGPeriodEditDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGPeriodEditDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGPeriodEditDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGPeriodEditDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGPeriodEdit(iParent);
}

QString SKGPeriodEditDesignerPlugin::name() const
{
    return QStringLiteral("SKGPeriodEdit");
}

QString SKGPeriodEditDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGPeriodEditDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGPeriodEditDesignerPlugin::toolTip() const
{
    return QStringLiteral("A period editor");
}

QString SKGPeriodEditDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A period editor");
}

bool SKGPeriodEditDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGPeriodEditDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGPeriodEdit\" name=\"SKGPeriodEdit\">\n"
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

QString SKGPeriodEditDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgperiodedit.h");
}
