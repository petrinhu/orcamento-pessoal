/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGFilePlugin component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestfileplugin.h"
#include "../../../../tests/skgbasemodelertest/skgtestmacro.h"
#include "../skgfileplugin.h"
#include "skgdocument.h"

#include <QAction>

void SKGTESTFilePlugin::TestPlugin()
{
    KLocalizedString::setApplicationDomain("skrooge");

    SKGDocument doc;
    SKGFilePlugin plugin(nullptr, nullptr, KPluginMetaData(), QVariantList());
    SKGTESTPLUGIN(plugin, doc);
    QCOMPARE(plugin.isInPagesChooser(), false);
    QCOMPARE(plugin.isEnabled(), true);

    SKGTESTTRIGGERACTION("file_new");
    SKGTESTTRIGGERACTION("file_open");
    SKGTESTTRIGGERACTION("file_save");
    SKGTESTTRIGGERACTION("file_save_as");
    SKGTESTTRIGGERACTION("file_change_password");
}

QTEST_MAIN(SKGTESTFilePlugin)
