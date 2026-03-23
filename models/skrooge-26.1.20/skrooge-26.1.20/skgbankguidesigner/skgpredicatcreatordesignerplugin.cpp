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
#include "skgpredicatcreatordesignerplugin.h"

#include "skgpredicatcreator.h"
#include "skgservices.h"

SKGPredicatCreatorDesignerPlugin::SKGPredicatCreatorDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGPredicatCreatorDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGPredicatCreatorDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGPredicatCreatorDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGPredicatCreator(iParent, nullptr);
}

QString SKGPredicatCreatorDesignerPlugin::name() const
{
    return QStringLiteral("SKGPredicatCreator");
}

QString SKGPredicatCreatorDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGPredicatCreatorDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("skrooge.png"));
}

QString SKGPredicatCreatorDesignerPlugin::toolTip() const
{
    return QStringLiteral("A predicat creator");
}

QString SKGPredicatCreatorDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A predicat creator");
}

bool SKGPredicatCreatorDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGPredicatCreatorDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGPredicatCreator\" name=\"SKGPredicatCreator\">\n"
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

QString SKGPredicatCreatorDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgpredicatcreator.h");
}
