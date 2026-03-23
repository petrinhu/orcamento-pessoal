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
        // Test import GSK
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MMB"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.mmb")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmmb/test.mmb"));
            SKGTESTERROR(QStringLiteral("MMB.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MMB.setName"), account.setName(QStringLiteral("COURANT")), true)
            SKGTESTERROR(QStringLiteral("MMB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-144.05"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MMB.setName"), account.setName(QStringLiteral("CPT2")), true)
            SKGTESTERROR(QStringLiteral("MMB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("40"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MMB.setName"), account.setName(QStringLiteral("NOTFAVORITE")), true)
            SKGTESTERROR(QStringLiteral("MMB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("150"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MMB.setName"), account.setName(QStringLiteral("LEP")), true)
            SKGTESTERROR(QStringLiteral("MMB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("6000"))
        }
    }

    {
        // Test import GSK
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MMB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmmb/jd.mmb"));
            SKGTESTERROR(QStringLiteral("MMB.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("MMB.setName"), account.setName(QStringLiteral("credit")), true)
            SKGTESTERROR(QStringLiteral("MMB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-974.04"))
        }
    }
    // End test
    SKGENDTEST()
}
