/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGDebugPlugin component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestdebugplugin.h"
#include "../../../../tests/skgbasemodelertest/skgtestmacro.h"
#include "../skgdebugplugin.h"
#include "skgdocument.h"

#include <QAction>

void SKGTESTDebugPlugin::TestPlugin()
{
    KLocalizedString::setApplicationDomain("skrooge");

    SKGDocument doc;
    SKGDebugPlugin plugin(nullptr, nullptr, KPluginMetaData(), QVariantList());
    SKGTESTPLUGIN(plugin, doc);
    QCOMPARE(plugin.isInPagesChooser(), true);
    QCOMPARE(plugin.isEnabled(), false);

    SKGTESTTRIGGERACTION("debug_restart_profiling");
    SKGTESTTRIGGERACTION("debug_open_profiling");
}

QTEST_MAIN(SKGTESTDebugPlugin)
