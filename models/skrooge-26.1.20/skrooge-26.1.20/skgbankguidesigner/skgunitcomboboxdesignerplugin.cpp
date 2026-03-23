/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a predicat creator for skrooge (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgunitcomboboxdesignerplugin.h"

#include <qicon.h>

#include "skgunitcombobox.h"

SKGUnitComboBoxDesignerPlugin::SKGUnitComboBoxDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGUnitComboBoxDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGUnitComboBoxDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGUnitComboBoxDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGUnitComboBox(iParent);
}

QString SKGUnitComboBoxDesignerPlugin::name() const
{
    return QStringLiteral("SKGUnitComboBox");
}

QString SKGUnitComboBoxDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGUnitComboBoxDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("skrooge.png"));
}

QString SKGUnitComboBoxDesignerPlugin::toolTip() const
{
    return QStringLiteral("A unit combo box");
}

QString SKGUnitComboBoxDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A unit combo box");
}

bool SKGUnitComboBoxDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGUnitComboBoxDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGUnitComboBox\" name=\"SKGUnitComboBox\">\n"
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

QString SKGUnitComboBoxDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgunitcombobox.h");
}
