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
    int nboperation = 0;
    {
        // Test export 320066
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy4/320066.skg"), true)
        SKGTESTERROR(QStringLiteral("document1.getNbObjects()"), document1.getNbObjects(QStringLiteral("v_operation_display"), QString(), nboperation), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportkmy4/320066.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.exportFile"), imp1.exportFile(), true)
        }
    }

    {
        // Test import 320066
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportkmy4/320066.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)

            int nboperation2 = 0;
            SKGTESTERROR(QStringLiteral("document1.getNbObjects()"),
                         document1.getNbObjects(QStringLiteral("v_operation_display"), QString(), nboperation2),
                         true)

            SKGTEST(QStringLiteral("document1:nb transactions"), nboperation2, nboperation)
        }
    }

    {
        // Test import randy1
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy4/randy1.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        // Actif
        SKGTESTACCOUNT(document1, QStringLiteral("A000290"), 23182500.00);
        // Action
        /*SKGTESTACCOUNT(document1, QStringLiteral("A000014"),  2432032.27);
        SKGTESTACCOUNT(document1, QStringLiteral("A000016"),  236769.78);
        SKGTESTACCOUNT(document1, QStringLiteral("A000018"),  0.00);
        SKGTESTACCOUNT(document1, QStringLiteral("A000020"),  0.00);
        SKGTESTACCOUNT(document1, QStringLiteral("A000022"),  326254.23);
        SKGTESTACCOUNT(document1, QStringLiteral("A000023"),  459915.34);
        SKGTESTACCOUNT(document1, QStringLiteral("A000025"),  618975.84);
        SKGTESTACCOUNT(document1, QStringLiteral("A000027"),  301501.09);
        SKGTESTACCOUNT(document1, QStringLiteral("A000032"),  12358550.29);
        SKGTESTACCOUNT(document1, QStringLiteral("A000294"),  1193772.30);
        SKGTESTACCOUNT(document1, QStringLiteral("A000300"),  12304427.59);
        SKGTESTACCOUNT(document1, QStringLiteral("A000312"),  2822127.36);
        SKGTESTACCOUNT(document1, QStringLiteral("A000366"),  14150464.39);
        SKGTESTACCOUNT(document1, QStringLiteral("A000371"),  2160798.86);*/
        // Carte de credit
        SKGTESTACCOUNT(document1, QStringLiteral("A000120"), -316721.001);
        SKGTESTACCOUNT(document1, QStringLiteral("A000407"), -1438.158);
        SKGTESTACCOUNT(document1, QStringLiteral("A000409"), 0.00);
        // Cheques
        SKGTESTACCOUNT(document1, QStringLiteral("A000030"), -6371673.033);
        SKGTESTACCOUNT(document1, QStringLiteral("A000040"), 429856.659);
        SKGTESTACCOUNT(document1, QStringLiteral("A000273"), 2479805.892);
        SKGTESTACCOUNT(document1, QStringLiteral("A000378"), 845357.871);
        // Especes
        SKGTESTACCOUNT(document1, QStringLiteral("A000314"), 0.00);
        SKGTESTACCOUNT(document1, QStringLiteral("A000354"), 863.232);
        // Epargne
        SKGTESTACCOUNT(document1, QStringLiteral("A000041"), 402257.682);
        SKGTESTACCOUNT(document1, QStringLiteral("A000301"), 951074.286);
        SKGTESTACCOUNT(document1, QStringLiteral("A000330"), 0.00);
        SKGTESTACCOUNT(document1, QStringLiteral("A000353"), 1753.44);
        SKGTESTACCOUNT(document1, QStringLiteral("A000379"), 4955771.076);
        SKGTESTACCOUNT(document1, QStringLiteral("A000387"), 849701.007);
        SKGTESTACCOUNT(document1, QStringLiteral("A000388"), 4681634.22);
        SKGTESTACCOUNT(document1, QStringLiteral("A000400"), 255912.039);
    }

    {
        // Test import cat
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy4/bug_import_cat.skg"),
                     true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy4/bug_import_cat.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)

            int nb = 0;
            SKGTESTERROR(QStringLiteral("document1.getNbObjects()"),
                         document1.getNbObjects(QStringLiteral("category"), QStringLiteral("t_name='Cantine'"), nb),
                         true)

            SKGTEST(QStringLiteral("document1:nb category"), nb, 1)
        }
    }

    // End test
    SKGENDTEST()
}
