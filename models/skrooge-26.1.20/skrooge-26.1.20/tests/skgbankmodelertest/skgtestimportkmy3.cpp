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
        // Test import 304313
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/304313.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import SKG
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_SKG"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/advice.skg"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        {
            // Scope of the transaction
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportkmy3/advice.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.exportFile"), imp1.exportFile(), true)
        }
    }

    {
        // Test import wallet
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy3/wallet.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("KMY.setName"), account.setName(QStringLiteral("Espece")), true)
            SKGTESTERROR(QStringLiteral("KMY.load"), account.load(), true)
            SKGBankObject bank;
            SKGTESTERROR(QStringLiteral("KMY.load"), account.getBank(bank), true)
            SKGTEST(QStringLiteral("KMY:getName"), bank.getName(), QString())
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("KMY.setName"), account.setName(QStringLiteral("Courant")), true)
            SKGTESTERROR(QStringLiteral("KMY.load"), account.load(), true)
            SKGBankObject bank;
            SKGTESTERROR(QStringLiteral("KMY.load"), account.getBank(bank), true)
            SKGTEST(QStringLiteral("KMY:getName"), bank.getName(), QStringLiteral("KMYMONEY"))
        }
    }
    // End test
    SKGENDTEST()
}
