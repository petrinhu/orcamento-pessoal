/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGMainPanel component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestmainpanel.h"
#include "skgboardwidget.h"
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgtestmacro.h"
#include "skgtraces.h"
#include <utility>

void SKGTESTMainPanel::Test()
{
    KLocalizedString::setApplicationDomain("skrooge");

    // Initialize document
    SKGDocumentBank doc;

    // Create main panel
    SKGMainPanel mainpanel(nullptr, &doc);

    QVERIFY2(!doc.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/advice.skg"), "Load document failed");
    SKGError err;
    {
        // Scope of the transaction
        SKGBEGINTRANSACTION(doc, QStringLiteral("PROP"), err)
        doc.setParameter(QStringLiteral("SKG_LAST_BUDGET_PROCESSING"), QString());
    }

    // Check plugins
    {
        SKGBEGINTRANSACTION(doc, QStringLiteral("PLUGINS"), err)
        int i = 0;
        SKGInterfacePlugin *plugin = nullptr;
        do {
            plugin = mainpanel.getPluginByIndex(i);
            if (plugin != nullptr) {
                SKGTRACE << i << ": " << plugin->title() << "(" << plugin->icon() << ")" << Qt::endl;
                plugin->statusTip();
                plugin->toolTip();
                plugin->tips();
                plugin->getOrder();
                plugin->isInPagesChooser();
                plugin->isEnabled();

                SKGTabPage *tab = plugin->getWidget();
                if (tab != nullptr) {
                    tab->printableWidgets();
                    tab->activateEditor();
                    tab->getBookmarkID();
                    tab->setState(tab->getState());

                    if (tab->isZoomable()) {
                        tab->zoomableWidget();
                        tab->setZoomPosition(5);
                        QCOMPARE(tab->zoomPosition(), 5);
                    }
                }

                int nbd = plugin->getNbDashboardWidgets();
                for (int j = 0; j < nbd + 1; ++j) {
                    SKGTRACE << "    Dashboard " << j << "/" << nbd + 1 << ": " << plugin->getDashboardWidgetTitle(j) << Qt::endl;
                    SKGBoardWidget *bw = plugin->getDashboardWidget(j);
                    if (bw != nullptr) {
                        bw->setState(bw->getState());
                        bw->getDefaultStateAttribute();
                        bw->getFirstSelectedObject();
                    }
                }

                plugin->getDockWidget();
                plugin->getPreferenceWidget();
                plugin->getPreferenceSkeleton();
                plugin->savePreferences();

                SKGAdviceList adviceList = plugin->advice(QStringList());
                for (const auto &advice : std::as_const(adviceList)) {
                    SKGTRACE << "    Advice: " << advice.getUUID() << Qt::endl;
                    for (int k = 0; k < 5; ++k) {
                        plugin->executeAdviceCorrection(advice.getUUID(), k);
                    }
                }
            }
            ++i;
        } while (plugin != nullptr);

        // Compute advices
        SKGTRACE << "getAdvice" << Qt::endl;
        mainpanel.getAdvice();
    }

    // Functions
    SKGTRACE << "processArguments" << Qt::endl;
    mainpanel.processArguments(QStringList());

    mainpanel.openPage(QStringLiteral("skg://skrooge_operation_plugin"));
    SKGTabPage *page = mainpanel.currentPage();
    mainpanel.switchPinPage(page);
    mainpanel.currentPageHistoryItem();

    mainpanel.openPage(0, true);
    mainpanel.openPage(0, false);
    mainpanel.setCurrentPage(0);

    mainpanel.getNbSelectedObjects();

    mainpanel.countPages();
    mainpanel.page(0);
    mainpanel.currentPageIndex();

    mainpanel.closeAllOtherPages(page);
    mainpanel.closeAllPages(true);

    mainpanel.splashScreen();

    mainpanel.getTabWidget();

    mainpanel.setContextVisibility(1, true);
    mainpanel.setContextVisibility(1, false);
}

QTEST_MAIN(SKGTESTMainPanel)
