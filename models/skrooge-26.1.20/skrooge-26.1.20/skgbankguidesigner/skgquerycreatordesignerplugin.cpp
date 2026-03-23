/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a query creator for skrooge (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgquerycreatordesignerplugin.h"

#include "skgquerycreator.h"
#include "skgservices.h"

SKGQueryCreatorDesignerPlugin::SKGQueryCreatorDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGQueryCreatorDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGQueryCreatorDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGQueryCreatorDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGQueryCreator(iParent);
}

QString SKGQueryCreatorDesignerPlugin::name() const
{
    return QStringLiteral("SKGQueryCreator");
}

QString SKGQueryCreatorDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGQueryCreatorDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("skrooge.png"));
}

QString SKGQueryCreatorDesignerPlugin::toolTip() const
{
    return QStringLiteral("A query creator");
}

QString SKGQueryCreatorDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A query creator");
}

bool SKGQueryCreatorDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGQueryCreatorDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGQueryCreator\" name=\"SKGQueryCreator\">\n"
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

QString SKGQueryCreatorDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgquerycreator.h");
}
