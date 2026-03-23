/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A combo box with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcomboboxdesignerplugin.h"

#include "skgcombobox.h"
#include "skgservices.h"

SKGComboBoxDesignerPlugin::SKGComboBoxDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGComboBoxDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGComboBoxDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGComboBoxDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGComboBox(iParent);
}

QString SKGComboBoxDesignerPlugin::name() const
{
    return QStringLiteral("SKGComboBox");
}

QString SKGComboBoxDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGComboBoxDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGComboBoxDesignerPlugin::toolTip() const
{
    return QStringLiteral("A combo box with more features");
}

QString SKGComboBoxDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A combo box with more features");
}

bool SKGComboBoxDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGComboBoxDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGComboBox\" name=\"SKGComboBox\">\n"
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

QString SKGComboBoxDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgcombobox.h");
}
