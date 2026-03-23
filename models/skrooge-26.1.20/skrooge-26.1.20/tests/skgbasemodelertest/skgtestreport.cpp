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
#include "skgreport.h"
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

    SKGDocument document1;
    SKGTESTERROR(QStringLiteral("PARAM:initialize"), document1.initialize(), true)

    SKGReport *rep = document1.getReport();
    rep->getPeriod();
    rep->setPeriod(QStringLiteral("2013-01"));
    rep->getPrevious();
    SKGTEST(QStringLiteral("REP:getMonth"), rep->getPeriod(), QStringLiteral("2013-01"))
    SKGTEST(QStringLiteral("REP:getPreviousMonth"), rep->getPreviousPeriod(), QStringLiteral("2012-12"))

    QString html;
    SKGTESTERROR(QStringLiteral("REP:getReportFromTemplate"),
                 SKGReport::getReportFromTemplate(rep, SKGTest::getTestPath(QStringLiteral("IN")) % "/missing.txt", html),
                 false)

    rep->getPrevious();
    rep->setPeriod(QStringLiteral("2013-02"));
    rep->cleanCache();
    delete rep;

    // End test
    SKGENDTEST()
}
