/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A web viewer with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgwebviewdesignerplugin.h"

#include "skgservices.h"
#include "skgwebview.h"

SKGWebViewDesignerPlugin::SKGWebViewDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_initialized = false;
}

void SKGWebViewDesignerPlugin::initialize(QDesignerFormEditorInterface *iCore)
{
    Q_UNUSED(iCore)
    if (m_initialized) {
        return;
    }

    m_initialized = true;
}

bool SKGWebViewDesignerPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *SKGWebViewDesignerPlugin::createWidget(QWidget *iParent)
{
    return new SKGWebView(iParent);
}

QString SKGWebViewDesignerPlugin::name() const
{
    return QStringLiteral("SKGWebView");
}

QString SKGWebViewDesignerPlugin::group() const
{
    return QStringLiteral("SKG Widgets");
}

QIcon SKGWebViewDesignerPlugin::icon() const
{
    return SKGServices::fromTheme(QStringLiteral("quickopen"));
}

QString SKGWebViewDesignerPlugin::toolTip() const
{
    return QStringLiteral("A web viewer with more features");
}

QString SKGWebViewDesignerPlugin::whatsThis() const
{
    return QStringLiteral("A web viewer with more features");
}

bool SKGWebViewDesignerPlugin::isContainer() const
{
    return false;
}

QString SKGWebViewDesignerPlugin::domXml() const
{
    return QStringLiteral(
        "<widget class=\"SKGWebView\" name=\"SKGWebView\">\n"
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

QString SKGWebViewDesignerPlugin::includeFile() const
{
    return QStringLiteral("skgwebview.h");
}
