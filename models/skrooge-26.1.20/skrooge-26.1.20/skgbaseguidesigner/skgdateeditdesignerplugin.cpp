/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A date edit with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdateeditdesignerplugin.h"

#include "skgdateedit.h"
#include "skgservices.h"

SKGDateEditDesignerPlugin::SKGDateEditDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGDateEditDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGDateEditDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGDateEditDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGDateEdit(iParent);
}

QString SKGDateEditDesignerPlugin::name() const
{
    return QStringLiteral("SKGDateEdit");
}

QString SKGDateEditDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGDateEditDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGDateEditDesignerPlugin::toolTip() const
{
    return QStringLiteral("A date editor with more features");
}

QString SKGDateEditDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A date editor with more features");
}

bool SKGDateEditDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGDateEditDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGDateEdit\" name=\"SKGDateEdit\">\n"
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

QString SKGDateEditDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgdateedit.h");
}
