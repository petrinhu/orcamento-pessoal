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
        // Test import KMY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.kmy")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/rapprochement-incorrect.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("KMY.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("KMY.load"), account.load(), true)
            SKGTEST(QStringLiteral("KMY:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("750"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("KMY.setName"), account.setName(QStringLiteral("CEL Donald")), true)
            SKGTESTERROR(QStringLiteral("KMY.load"), account.load(), true)
            SKGTEST(QStringLiteral("KMY:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("150"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("KMY.setName"), account.setName(QStringLiteral("PEEs")), true)
            SKGTESTERROR(QStringLiteral("KMY.load"), account.load(), true)
            SKGTEST(QStringLiteral("KMY:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("640"))
        }

        {
            // Scope of the transaction
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportkmy1/rapprochement-incorrect.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.exportFile"), imp1.exportFile(), true)
        }
    }

    {
        // Test import KMY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/mytest.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("KMY:getNbObjects(recurrentoperation)"), document1.getNbObjects(QStringLiteral("recurrentoperation"), QString(), nb), true)
        SKGTEST(QStringLiteral("KMY:getNbObjects(recurrentoperation)"), nb, 8)
    }

    {
        // Test import KMY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/action.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("KMY.setName"), account.setName(QStringLiteral("courant")), true)
            SKGTESTERROR(QStringLiteral("KMY.load"), account.load(), true)
            SKGTEST(QStringLiteral("KMY:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-600"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("KMY.setName"), account.setName(QStringLiteral("actions")), true)
            SKGTESTERROR(QStringLiteral("KMY.load"), account.load(), true)
            SKGTEST(QStringLiteral("KMY:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("100"))
        }

        // test multi import
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/action.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import KMY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/252869.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("KMY:getNbObjects(recurrentoperation)"),
                     document1.getNbObjects(QStringLiteral("bank"), QStringLiteral("t_name='Test Bank'"), nb),
                     true)
        SKGTEST(QStringLiteral("KMY:getNbObjects(recurrentoperation)"), nb, 1)
    }

    {
        // 384119
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/384119.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("KMY:getNbObjects(transactions without suboperation)"),
                     document1.getNbObjects(QStringLiteral("operation"),
                                            QStringLiteral("NOT EXISTS (SELECT 1 FROM suboperation where rd_operation_id=operation.id)"),
                                            nb),
                     true)
        SKGTEST(QStringLiteral("KMY:getNbObjects(transactions without suboperation)"), nb, 0)
    }

    {
        // Non utf8
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportkmy/non_utf8.kmy"));
            SKGTESTERROR(QStringLiteral("KMY.importFile"), imp1.importFile(), true)
        }
    }
    // End test
    SKGENDTEST()
}
