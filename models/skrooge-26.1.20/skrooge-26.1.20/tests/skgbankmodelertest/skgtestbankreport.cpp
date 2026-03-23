/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test script.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qdesktopservices.h>

#include "skgbankincludes.h"
#include "skgreportbank.h"
#include "skgtestmacro.h"

/**
 * The main function of the bank report test
 * @param argc the number of arguments
 * @param argv the list of arguments
 */
int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)

    {
        // Test unit et unitvalue
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % QStringLiteral("/advice.skg")), true)
        auto *rep = qobject_cast<SKGReportBank *>(document1.getReport());
        rep->setPeriod(QStringLiteral("2013"));
        rep->get5MainCategoriesVariation();
        rep->getAlarms();
        rep->getInterests();
        rep->getAccountTable();
        rep->getBankTable();
        rep->getBudgetTable();
        rep->getIncomeVsExpenditure();
        rep->getIncomeVsExpenditure();
        rep->getMainCategoriesForPeriod();
        rep->getMainCategoriesForPreviousPeriod();
        rep->get5MainCategoriesVariationIssue();
        rep->getPortfolio();
        rep->getScheduledOperations();
        rep->getUnitTable();
        rep->getNetWorth();
        rep->getAnnualSpending();
        rep->getPersonalFinanceScore();

        rep->getPrevious();

        rep->getTipOfDay();
        rep->getTipsOfDay();

        rep->setTipsOfDay(QStringList() << QStringLiteral("Hello") << QStringLiteral("world"));

        rep->getTipOfDay();
        rep->getTipsOfDay();

        rep->setPointSize(10);
        SKGTEST(QStringLiteral("REP:getPointSize"), rep->getPointSize(), 10)

        QString html;
        SKGTESTERROR(QStringLiteral("SKGReportBank::getReportFromTemplate"),
                     SKGReportBank::getReportFromTemplate(rep, SKGTest::getTestPath(QStringLiteral("IN")) % QStringLiteral("/template.txt"), html),
                     true)
        delete rep;
    }
    // End test
    SKGENDTEST()
}
