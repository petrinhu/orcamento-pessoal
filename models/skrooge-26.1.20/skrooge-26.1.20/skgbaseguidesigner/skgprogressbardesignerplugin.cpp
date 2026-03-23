/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A progress bar with colors
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgprogressbardesignerplugin.h"

#include "skgprogressbar.h"
#include "skgservices.h"

SKGProgressBarDesignerPlugin::SKGProgressBarDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGProgressBarDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGProgressBarDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGProgressBarDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGProgressBar(iParent);
}

QString SKGProgressBarDesignerPlugin::name() const
{
    return QStringLiteral("SKGProgressBar");
}

QString SKGProgressBarDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGProgressBarDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGProgressBarDesignerPlugin::toolTip() const
{
    return QStringLiteral("A progress bar with colors");
}

QString SKGProgressBarDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A progress bar with colors");
}

bool SKGProgressBarDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGProgressBarDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGProgressBar\" name=\"SKGProgressBar\">\n"
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

QString SKGProgressBarDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgprogressbar.h");
}
