/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A QLineEdit with calculator (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgcalculatoreditdesignerplugin.h"

#include "skgcalculatoredit.h"
#include "skgservices.h"

SKGCalculatorEditDesignerPlugin::SKGCalculatorEditDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGCalculatorEditDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGCalculatorEditDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGCalculatorEditDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGCalculatorEdit(iParent);
}

QString SKGCalculatorEditDesignerPlugin::name() const
{
    return QStringLiteral("SKGCalculatorEdit");
}

QString SKGCalculatorEditDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGCalculatorEditDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGCalculatorEditDesignerPlugin::toolTip() const
{
    return QStringLiteral("A QLineEdit with calculator included");
}

QString SKGCalculatorEditDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A QLineEdit with calculator included");
}

bool SKGCalculatorEditDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGCalculatorEditDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGCalculatorEdit\" name=\"SKGCalculatorEdit\">\n"
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

QString SKGCalculatorEditDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgcalculatoredit.h");
}
