/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGReportPlugin component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestreportplugin.h"
#include "../../../../tests/skgbasemodelertest/skgtestmacro.h"
#include "../skgreportplugin.h"
#include "skgdocumentbank.h"

#include <QAction>

void SKGTESTReportPlugin::TestPlugin()
{
    KLocalizedString::setApplicationDomain("skrooge");

    SKGDocumentBank doc;
    SKGReportPlugin plugin(nullptr, nullptr, KPluginMetaData(), QVariantList());
    SKGTESTPLUGIN(plugin, doc);
    QCOMPARE(plugin.isInPagesChooser(), true);
    QCOMPARE(plugin.isEnabled(), true);

    SKGTESTTRIGGERACTION("open_report");
    SKGTESTTRIGGERACTION("view_open_very_old_operations");
    SKGTESTTRIGGERACTION("view_open_very_far_operations");
}

QTEST_MAIN(SKGTESTReportPlugin)
