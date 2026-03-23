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
        // Test import bankperfect
        SKGError err;
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_BP_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/bankperfect.csv"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("AUTORECONCILE"), err)
            SKGAccountObject account1(&document1);
            SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account1.setName(QStringLiteral("bankperfect")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account1.load(), true)
            auto soluces = account1.getPossibleReconciliations(3547.14);
            SKGTEST(QStringLiteral("ACCOUNT.getPossibleReconciliations"), soluces.count(), 1)
            SKGTESTERROR(QStringLiteral("ACCOUNT.autoReconcile"), account1.autoReconcile(3547.14), true)
        }
    }

    {
        // Test import bankperfect
        SKGError err;
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestautoreconcile/complex.skg"), true)
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("AUTORECONCILE"), err)
            {
                SKGAccountObject account1(&document1);
                SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account1.setName(QStringLiteral("EURO")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account1.load(), true)
                auto soluces = account1.getPossibleReconciliations(3699.67);
                SKGTEST(QStringLiteral("ACCOUNT.getPossibleReconciliations"), soluces.count(), 1)
                SKGTESTERROR(QStringLiteral("ACCOUNT.autoReconcile"), account1.autoReconcile(3699.67), true)

                soluces = account1.getPossibleReconciliations(3699.67);
                SKGTEST(QStringLiteral("ACCOUNT.getPossibleReconciliations"), soluces.count(), 1)
                SKGTESTERROR(QStringLiteral("ACCOUNT.autoReconcile"), account1.autoReconcile(3699.67), true)
            }

            {
                SKGAccountObject account1(&document1);
                SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account1.setName(QStringLiteral("DOLLAR")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account1.load(), true)
                auto soluces = account1.getPossibleReconciliations(3699.67);
                SKGTEST(QStringLiteral("ACCOUNT.getPossibleReconciliations"), soluces.count(), 1)
                SKGTESTERROR(QStringLiteral("ACCOUNT.autoReconcile"), account1.autoReconcile(3699.67), true)

                SKGTESTERROR(QStringLiteral("ACCOUNT.autoReconcile"), account1.autoReconcile(3699.67), true)
            }
        }
    }

    {
        // Test many combinations
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestautoreconcile/many_combinations.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
            document1.dump(DUMPOPERATION | DUMPACCOUNT);
        }

        {
            SKGAccountObject account1(&document1);
            SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account1.setName(QStringLiteral("many combinations")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account1.load(), true)
            auto soluces = account1.getPossibleReconciliations(53, true);
            SKGTEST(QStringLiteral("ACCOUNT.getPossibleReconciliations"), soluces.count(), 1)

            SKGBEGINTRANSACTION(document1, QStringLiteral("CREATE OPERATIONS"), err)
            double oBalance;
            SKGUnitObject oUnit;
            QDate now = QDate::currentDate();
            account1.getInitialBalance(oBalance, oUnit);
            int nb = 38;
            for (int i = 1; i <= nb; ++i) {
                SKGOperationObject op;
                SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(op), true)
                SKGTESTERROR(QStringLiteral("OPE:setDate"), op.setDate(now), true)
                SKGTESTERROR(QStringLiteral("OPE:setUnit"), op.setUnit(oUnit), true)
                SKGTESTERROR(QStringLiteral("OPE:setImported"), op.setImported(true), true)
                SKGTESTERROR(QStringLiteral("OPE:save"), op.save(), true)

                SKGSubOperationObject subop;
                SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op.addSubOperation(subop), true)
                SKGTESTERROR(QStringLiteral("OPE:setQuantity"), subop.setQuantity(10), true)
                SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop.save(), true)
            }

            // Test performance
            soluces = account1.getPossibleReconciliations(-23 + 10 * nb, true);
        }
    }
    // End test
    SKGENDTEST()
}
