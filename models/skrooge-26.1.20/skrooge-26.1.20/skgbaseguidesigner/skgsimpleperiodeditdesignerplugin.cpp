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
#include "skgsimpleperiodeditdesignerplugin.h"

#include "skgservices.h"
#include "skgsimpleperiodedit.h"

SKGSimplePeriodEditDesignerPlugin::SKGSimplePeriodEditDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGSimplePeriodEditDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGSimplePeriodEditDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGSimplePeriodEditDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGSimplePeriodEdit(iParent);
}

QString SKGSimplePeriodEditDesignerPlugin::name() const
{
    return QStringLiteral("SKGSimplePeriodEdit");
}

QString SKGSimplePeriodEditDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGSimplePeriodEditDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGSimplePeriodEditDesignerPlugin::toolTip() const
{
    return QStringLiteral("A simple period editor");
}

QString SKGSimplePeriodEditDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A simple period editor");
}

bool SKGSimplePeriodEditDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGSimplePeriodEditDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGSimplePeriodEdit\" name=\"SKGSimplePeriodEdit\">\n"
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

QString SKGSimplePeriodEditDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgsimpleperiodedit.h");
}
