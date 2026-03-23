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
        // Test import MT940
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MT940"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.mt940")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmt940/test1.sta"));
            SKGTESTERROR(QStringLiteral("MT940.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MT940.setName"), account.setName(QStringLiteral("NUMERO DE COMPTE IBAN 2")), true)
            SKGTESTERROR(QStringLiteral("MT940.load"), account.load(), true)
            SKGTEST(QStringLiteral("MT940:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("11.4"))
        }
    }

    {
        // Test import MT940
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MT940"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmt940/583501.mt940"));
            SKGTESTERROR(QStringLiteral("MT940.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MT940.setName"), account.setName(QStringLiteral("12345.12")), true)
            SKGTESTERROR(QStringLiteral("MT940.load"), account.load(), true)
            SKGTEST(QStringLiteral("MT940:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("12145.12"))
        }
    }

    {
        // Test import MT940
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MT940"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmt940/341076.mt940"));
            SKGTESTERROR(QStringLiteral("MT940.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MT940.setName"), account.setName(QStringLiteral("2602272001")), true)
            SKGTESTERROR(QStringLiteral("MT940.load"), account.load(), true)
            SKGTEST(QStringLiteral("MT940:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-12.56"))
        }
    }

    {
        // Test import MT940
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MT940"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmt940/267442_1.mt940"));
            SKGTESTERROR(QStringLiteral("MT940.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MT940.setName"), account.setName(QStringLiteral("55555.55")), true)
            SKGTESTERROR(QStringLiteral("MT940.load"), account.load(), true)
            SKGTEST(QStringLiteral("MT940:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1972.1"))
        }
    }

    {
        // Test import MT940
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MT940"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmt940/267442_2.mt940"));
            SKGTESTERROR(QStringLiteral("MT940.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MT940.setName"), account.setName(QStringLiteral("54842.79")), true)
            SKGTESTERROR(QStringLiteral("MT940.load"), account.load(), true)
            SKGTEST(QStringLiteral("MT940:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1030.5"))
        }
    }

    {
        // 280897
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MT940"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmt940/test1.sta"));
            SKGTESTERROR(QStringLiteral("MT940.importFile"), imp1.importFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MT940"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmt940/test1.sta"));
            SKGTESTERROR(QStringLiteral("MT940.importFile"), imp1.importFile(), true)
        }

        {
            bool oExist = false;
            SKGTESTERROR(QStringLiteral("document1.existObjects"),
                         document1.existObjects(QStringLiteral("operation"), QStringLiteral("rc_unit_id=0"), oExist),
                         true)
            SKGTESTBOOL("document1.existObjects", oExist, false)
        }
    }
    // End test
    SKGENDTEST()
}
