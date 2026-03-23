/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin for debug.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdebugplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>

#include "skgdebugpluginwidget.h"
#include "skgmainpanel.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGDebugPlugin, "metadata.json")

SKGDebugPlugin::SKGDebugPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGDebugPlugin::~SKGDebugPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentDocument = nullptr;
}

bool SKGDebugPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;
    if (m_currentDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skg_debug"), title());
    setXMLFile(QStringLiteral("skg_debug.rc"));

    // Menu
    auto restartProfiling = new QAction(SKGServices::fromTheme(QStringLiteral("fork")),
                                        i18nc("Restart the profiling, a method used for analysing performances", "Restart profiling"),
                                        this);
    connect(restartProfiling, &QAction::triggered, this, &SKGDebugPlugin::onRestartProfiling);
    actionCollection()->setDefaultShortcut(restartProfiling, Qt::CTRL | Qt::Key_Pause);
    registerGlobalAction(QStringLiteral("debug_restart_profiling"), restartProfiling);

    QStringList overlayopen;
    overlayopen.push_back(QStringLiteral("quickopen"));
    auto openProfiling = new QAction(SKGServices::fromTheme(QStringLiteral("fork"), overlayopen),
                                     i18nc("Open the profiling, a method used for analysing performances", "Open profiling"),
                                     this);
    connect(openProfiling, &QAction::triggered, this, &SKGDebugPlugin::onOpenProfiling);
    actionCollection()->setDefaultShortcut(openProfiling, Qt::ALT | Qt::Key_Pause);
    registerGlobalAction(QStringLiteral("debug_open_profiling"), openProfiling);
    return true;
}

SKGTabPage *SKGDebugPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGDebugPluginWidget(SKGMainPanel::getMainPanel(), m_currentDocument);
}

QString SKGDebugPlugin::title() const
{
    return i18nc("Noun, a plugin allowing to access the SQLite database, useful to debug", "Debug");
}

QString SKGDebugPlugin::icon() const
{
    return QStringLiteral("tools-report-bug");
}

QString SKGDebugPlugin::toolTip() const
{
    return i18nc("A tool tip, explaining that the plugin is useful for debugging purposes", "Useful for debug");
}

bool SKGDebugPlugin::isInPagesChooser() const
{
    return true;
}

bool SKGDebugPlugin::isEnabled() const
{
    return (SKGTraces::SKGLevelTrace > 0 || SKGTraces::SKGPerfo);
}

void SKGDebugPlugin::onRestartProfiling()
{
    SKGTraces::cleanProfilingStatistics();
}

void SKGDebugPlugin::onOpenProfiling()
{
    // Call debug plugin
    QString dumpString;
    QStringList dump = SKGTraces::getProfilingStatistics();

    int nbl = dump.count();
    for (int i = 0; i < nbl; ++i) {
        dumpString += dump.at(i);
        dumpString += '\n';
    }

    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGMainPanel::getMainPanel()->openPage("skg://debug_plugin/?sqlResult=" % SKGServices::encodeForUrl(dumpString));
    }
}

#include <skgdebugplugin.moc>
