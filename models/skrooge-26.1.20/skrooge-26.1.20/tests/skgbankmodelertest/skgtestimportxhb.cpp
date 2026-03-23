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
        // Test import XHB
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XHB"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.xhb")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxhb/test.xhb"));
            SKGTESTERROR(QStringLiteral("XHB.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("XHB.setName"), account.setName(QStringLiteral("COURANT")), true)
            SKGTESTERROR(QStringLiteral("XHB.load"), account.load(), true)
            SKGTEST(QStringLiteral("XHB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("35"))
        }

        // test multi import
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XHB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxhb/test.xhb"));
            SKGTESTERROR(QStringLiteral("XHB.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import XHB
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XHB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxhb/example_budget.xhb"));
            SKGTESTERROR(QStringLiteral("XHB.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("XHB.setName"), account.setName(QStringLiteral("Cheque Account")), true)
            SKGTESTERROR(QStringLiteral("XHB.load"), account.load(), true)
            SKGTEST(QStringLiteral("XHB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("5758.22"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("XHB.setName"), account.setName(QStringLiteral("Savings Account")), true)
            SKGTESTERROR(QStringLiteral("XHB.load"), account.load(), true)
            SKGTEST(QStringLiteral("XHB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1024.66"))
        }
    }

    {
        // Test import XHB
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XHB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxhb/comptes.xhb"));
            SKGTESTERROR(QStringLiteral("XHB.importFile"), imp1.importFile(), true)
        }

        {
            SKGStringListList result;
            SKGTESTERROR(QStringLiteral("XHB.executeSelectSqliteOrder"),
                         document1.executeSelectSqliteOrder(QStringLiteral("SELECT distinct t_type FROM account"), result),
                         true)
            SKGTEST(QStringLiteral("XHB:distinct t_type"), result.count(), 6)
        }
    }

    {
        // Test import wallet
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XHB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxhb/wallet.xhb"));
            SKGTESTERROR(QStringLiteral("XHB.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("XHB.setName"), account.setName(QStringLiteral("Espece")), true)
            SKGTESTERROR(QStringLiteral("XHB.load"), account.load(), true)
            SKGBankObject bank;
            SKGTESTERROR(QStringLiteral("XHB.load"), account.getBank(bank), true)
            SKGTEST(QStringLiteral("XHB:getName"), bank.getName(), QString())
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("XHB.setName"), account.setName(QStringLiteral("Courant")), true)
            SKGTESTERROR(QStringLiteral("XHB.load"), account.load(), true)
            SKGBankObject bank;
            SKGTESTERROR(QStringLiteral("XHB.load"), account.getBank(bank), true)
            SKGTEST(QStringLiteral("XHB:getName"), bank.getName(), QStringLiteral("HOMEBANK"))
        }
    }

    {
        // Test import
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XHB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxhb/error_import.xhb"));
            SKGTESTERROR(QStringLiteral("XHB.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import 481144
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_XHB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportxhb/481144.xhb"));
            SKGTESTERROR(QStringLiteral("XHB.importFile"), imp1.importFile(), true)
        }

        {
            SKGStringListList result;
            SKGTESTERROR(QStringLiteral("XHB.executeSelectSqliteOrder"),
                         document1.executeSelectSqliteOrder(QStringLiteral("SELECT count(1) FROM refund"), result),
                         true)
            SKGTEST(QStringLiteral("XHB:tracker"), result.at(1).at(0), "1")
        }
    }
    // End test
    SKGENDTEST()
}
