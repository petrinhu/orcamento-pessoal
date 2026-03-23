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
        // Test import GSK
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GSB"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.gsb")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgsb/test-obfuscated.gsb"));
            SKGTESTERROR(QStringLiteral("GSB.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GSB.setName"), account.setName(QStringLiteral("Account 0")), true)
            SKGTESTERROR(QStringLiteral("GSB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1029"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GSB.setName"), account.setName(QStringLiteral("Account 4")), true)
            SKGTESTERROR(QStringLiteral("GSB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("7.5"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GSB.setName"), account.setName(QStringLiteral("Account 1")), true)
            SKGTESTERROR(QStringLiteral("GSB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("757.5"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GSB.setName"), account.setName(QStringLiteral("Account 3")), true)
            SKGTESTERROR(QStringLiteral("GSB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("43.5"))
        }
    }

    {
        // Test import GSK
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GSB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgsb/version_0.5.9.gsb"));
            SKGTESTERROR(QStringLiteral("GSB.importFile"), imp1.importFile(), false)
        }
    }

    {
        // Double import
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GSB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgsb/essai.gsb"));
            SKGTESTERROR(QStringLiteral("GSB.importFile"), imp1.importFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GSB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgsb/essai.gsb"));
            SKGTESTERROR(QStringLiteral("GSB.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Split
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GSB"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgsb/split.gsb"));
            SKGTESTERROR(QStringLiteral("GSB.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GSB.setName"), account.setName(QStringLiteral("Compte banque A")), true)
            SKGTESTERROR(QStringLiteral("GSB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("350"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GSB.setName"), account.setName(QStringLiteral("Compte banque B")), true)
            SKGTESTERROR(QStringLiteral("GSB.load"), account.load(), true)
            SKGTEST(QStringLiteral("GSB:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-9400"))
        }
    }

    {
        // Budget
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GSB"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgsb/imputations_budgetaires.gsb"));
            SKGTESTERROR(QStringLiteral("GSB.importFile"), imp1.importFile(), true)
        }
        {
            SKGObjectBase::SKGListSKGObjectBase transactions;
            SKGTESTERROR(QStringLiteral("GSB.getObjects"),
                         document1.getObjects(QStringLiteral("v_operation_display"), QStringLiteral("d_date='2013-08-11'"), transactions),
                         true)
            SKGTEST(QStringLiteral("GSB:count"), transactions.count(), 1)
            SKGOperationObject op(transactions[0]);
            SKGTEST(QStringLiteral("GSB:Budgetary allocation"), op.getProperty(i18nc("Noun", "Budgetary allocation")), QStringLiteral("imputation1"))
            SKGTEST(QStringLiteral("GSB:Fiscal year"), op.getProperty(i18nc("Noun", "Fiscal year")), QStringLiteral("2013"))
            SKGTEST(QStringLiteral("GSB:category"), op.getAttribute(QStringLiteral("t_CATEGORY")), QStringLiteral("categorie1"))
        }
        {
            SKGObjectBase::SKGListSKGObjectBase transactions;
            SKGTESTERROR(QStringLiteral("GSB.getObjects"),
                         document1.getObjects(QStringLiteral("v_operation_display"), QStringLiteral("d_date='2013-08-12'"), transactions),
                         true)
            SKGTEST(QStringLiteral("GSB:count"), transactions.count(), 1)
            SKGOperationObject op(transactions[0]);
            SKGTEST(QStringLiteral("GSB:Budgetary allocation"),
                    op.getProperty(i18nc("Noun", "Budgetary allocation")),
                    "imputation1" % OBJECTSEPARATOR % "subimputation1")
            SKGTEST(QStringLiteral("GSB:Fiscal year"), op.getProperty(i18nc("Noun", "Fiscal year")), QStringLiteral("2013"))
            SKGTEST(QStringLiteral("GSB:category"), op.getAttribute(QStringLiteral("t_CATEGORY")), "categorie1" % OBJECTSEPARATOR % "subcategorie1")
        }
    }
    // End test
    SKGENDTEST()
}
