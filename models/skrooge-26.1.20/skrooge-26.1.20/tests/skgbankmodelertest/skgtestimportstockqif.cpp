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

#include <klocalizedstring.h>

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
        // Test import QIF 1
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_STOCK_UNIT"), err)
            SKGUnitObject unit;
            SKGTESTERROR(QStringLiteral("QIF.createCurrencyUnit"), SKGUnitObject::createCurrencyUnit(&document1, QStringLiteral("FRF"), unit), true)
        }
        document1.dump(DUMPUNIT);
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_STOCK_1"), err)
            SKGUnitObject unit;
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportstockqif/La Poste GMO.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            SKGImportExportManager imp2(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportstockqif/La Poste GMO (Caisse).qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp2.importFile(), true)
            SKGImportExportManager imp3(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportstockqif/goog.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp3.importFile(), true)
        }
        document1.dump(DUMPOPERATION | DUMPUNIT | DUMPACCOUNT);
    }

    {
        // Test import QIF -Correction bug 2307068
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_2307068"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportstockqif/2307068-BNP CC.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)

            SKGImportExportManager imp12(&document1,
                                         QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportstockqif/2307068-Compte Titre.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp12.importFile(), true)
        }

        document1.dump(DUMPOPERATION | DUMPUNIT | DUMPACCOUNT);
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("BNP CC")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            QLocale::setDefault(QLocale::C);
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::toCurrencyString(account.getCurrentAmount(), QString(), 2), QStringLiteral("-520.00"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("Compte Titre")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("520"))
        }
    }
    // End test
    SKGENDTEST()
}
