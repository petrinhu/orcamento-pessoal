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
#include "skgbankincludes.h"
#include "skgreportbank.h"
#include "skgtestmacro.h"

/**
 * The main function of the unit test
 * @param argc the number of arguments
 * @param argv the list of arguments
 */
int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)

    // ============================================================================
    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestpfs/personalfinancescore.skg"),
                     true)

        SKGError err;
        auto *rep = qobject_cast<SKGReportBank *>(document1.getReport());

        rep->setPeriod(QStringLiteral("2010"));
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                SKGServices::doubleToString(rep->getPersonalFinanceScoreDetails().value(QStringLiteral("value")).toDouble()),
                QStringLiteral("-0.04761904762"))
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                rep->getPersonalFinanceScoreDetails().value(QStringLiteral("level")).toString(),
                QStringLiteral("danger"))
        rep->setPeriod(QStringLiteral("2011"));
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                SKGServices::doubleToString(rep->getPersonalFinanceScoreDetails().value(QStringLiteral("value")).toDouble()),
                QStringLiteral("3.75"))
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                rep->getPersonalFinanceScoreDetails().value(QStringLiteral("level")).toString(),
                QStringLiteral("warning"))
        rep->setPeriod(QStringLiteral("2012"));
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                SKGServices::doubleToString(rep->getPersonalFinanceScoreDetails().value(QStringLiteral("value")).toDouble()),
                QStringLiteral("9.833333333"))
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                rep->getPersonalFinanceScoreDetails().value(QStringLiteral("level")).toString(),
                QStringLiteral("warning"))
        rep->setPeriod(QStringLiteral("2013"));
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                SKGServices::doubleToString(rep->getPersonalFinanceScoreDetails().value(QStringLiteral("value")).toDouble()),
                QStringLiteral("11.95"))
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                rep->getPersonalFinanceScoreDetails().value(QStringLiteral("level")).toString(),
                QStringLiteral("success"))
        rep->setPeriod(QStringLiteral("2014"));
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                SKGServices::doubleToString(rep->getPersonalFinanceScoreDetails().value(QStringLiteral("value")).toDouble()),
                QStringLiteral("35.95"))
        SKGTEST(QStringLiteral("SKGReportBank:getPersonalFinanceScore"),
                rep->getPersonalFinanceScoreDetails().value(QStringLiteral("level")).toString(),
                QStringLiteral("success"))
    }

    // End test
    SKGENDTEST()
}
