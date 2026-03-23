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
#include "skgimportexportmanager.h"
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
    {
        // Test import solde initial
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy5/solde_initial.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        SKGTESTACCOUNT(document1, QStringLiteral("AAA"), 123456.00);

        int nboperation = 0;
        SKGTESTERROR(QStringLiteral("document1.getNbObjects()"), document1.getNbObjects(QStringLiteral("v_operation_display"), QString(), nboperation), true)
        SKGTEST(QStringLiteral("document1:nb transactions"), nboperation, 0)
    }

    {
        // Test import steffy
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy5/steffie.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        SKGTESTACCOUNT(document1, QStringLiteral("A000007"), 2695.00);
        SKGTESTACCOUNT(document1, QStringLiteral("A000548"), 175494.48);
        SKGTESTACCOUNT(document1, QStringLiteral("A000549"), 1186034.71);
        SKGTESTACCOUNT(document1, QStringLiteral("A000573"), 30968132.93);
        SKGTESTACCOUNT(document1, QStringLiteral("A000584"), 2493455.65);
        SKGTESTACCOUNT(document1, QStringLiteral("A000429"), -3723.51);
        SKGTESTACCOUNT(document1, QStringLiteral("A000433"), -12809.09);

        // Closed accounts
        SKGTESTACCOUNT(document1, QStringLiteral("A000004"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000005"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000006"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000490"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000430"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000431"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000432"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000533"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000435"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000436"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000437"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000487"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000492"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000495"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000497"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000499"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000526"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000543"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000553"), 0.0);
        SKGTESTACCOUNT(document1, QStringLiteral("A000560"), 0.0);
    }

    // End test
    SKGENDTEST()
}
