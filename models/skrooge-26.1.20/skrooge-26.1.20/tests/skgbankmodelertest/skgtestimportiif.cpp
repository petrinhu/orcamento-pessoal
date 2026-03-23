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
        // Test import full_check.iif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_IIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportiif/full_check.iif"));
            imp1.setCodec(QStringLiteral("UTF-8"));
            SKGTESTERROR(QStringLiteral("IIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Checking")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-36.15"))
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_IIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportiif/full_check.iif"));
            imp1.setCodec(QStringLiteral("UTF-8"));
            SKGTESTERROR(QStringLiteral("IIF.exportFile"), imp1.exportFile(), true)
        }
    }

    {
        // Test import full_deposit.iif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_IIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportiif/full_deposit.iif"));
            SKGTESTERROR(QStringLiteral("IIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Checking")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("10000"))
        }
    }

    {
        // Test import full_bill.iif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_IIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportiif/full_bill.iif"));
            SKGTESTERROR(QStringLiteral("IIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Accounts Payable")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-59.25"))
        }
    }

    {
        // Test import full_cash_sale.iif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_IIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportiif/full_cash_sale.iif"));
            SKGTESTERROR(QStringLiteral("IIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Undeposited Funds")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1969.98"))
        }
    }

    {
        // Test import full_transfer.iif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_IIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportiif/full_transfer.iif"));
            SKGTESTERROR(QStringLiteral("IIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Checking")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-500"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Savings")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("500"))
        }
    }

    {
        // Test import full_bill_payment.iif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_IIF"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportiif/full_bill_payment.iif"));
            SKGTESTERROR(QStringLiteral("IIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Checking")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-35"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Accounts Payable")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("35"))
        }
    }

    {
        // Test import full_customer_payment.iif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_IIF"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportiif/full_customer_payment.iif"));
            SKGTESTERROR(QStringLiteral("IIF.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject la;
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Undeposited Funds")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("53.5"))
            la = account;
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("IIF.setName"), account.setName(QStringLiteral("Accounts Receivable")), true)
            SKGTESTERROR(QStringLiteral("IIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("IIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-53.5"))
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_IIF"), err)
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportiif/export_all.iif"));
            SKGTESTERROR(QStringLiteral("IIF.exportFile"), exp1.exportFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_IIF"), err)
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportiif/export_la.iif"));
            QMap<QString, QString> params;
            params[QStringLiteral("uuid_of_selected_accounts_or_operations")] = la.getUniqueID();
            exp1.setExportParameters(params);
            SKGTESTERROR(QStringLiteral("IIF.exportFile"), exp1.exportFile(), true)
        }
    }
    // End test
    SKGENDTEST()
}
