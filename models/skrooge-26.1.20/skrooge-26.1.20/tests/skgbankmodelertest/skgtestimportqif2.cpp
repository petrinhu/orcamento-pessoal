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

    QDate d(1970, 1, 1);

    {
        // 275963
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGUnitObject unit_euro(&document1);
        SKGUnitObject unit_dollar(&document1);
        SKGError err;
        {
            SKGAccountObject account;

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit_euro.setSymbol(QStringLiteral("EUR")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setType"), unit_euro.setType(SKGUnitObject::PRIMARY), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(d), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_dollar.setName(QStringLiteral("dollar")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setSymbol"), unit_dollar.setSymbol(QStringLiteral("USD")), true)
            SKGTESTERROR(QStringLiteral("UNIT:setType"), unit_dollar.setType(SKGUnitObject::CURRENCY), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_dollar.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_dollar_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_dollar.addUnitValue(unit_dollar_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_dollar_val1.setQuantity(2), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_dollar_val1.setDate(d), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_dollar_val1.save(), true)

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:setNumber"), bank.setNumber(QStringLiteral("0003")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("CODEVI")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setInitialBalance"), account.setInitialBalance(100, unit_dollar), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/t2.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("CODEVI")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-9335.94"))
        }
    }

    {
        // 275963
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/export_qif.skg"),
                     true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_BP_QIF"), err)
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif2/export_qif.qif"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
    }

    {
        // Support qif file having Type:Class with description
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGAccountObject la;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/MoneydanceExportExample.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)

            SKGObjectBase::SKGListSKGObjectBase result;
            SKGTESTERROR(QStringLiteral("DOC.getObjects"),
                         document1.getObjects(QStringLiteral("account"), QStringLiteral("t_name='Wells Fargo:Checking'"), result),
                         true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 1)
            if (result.count() != 0) {
                la = result.at(0);
            }
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_QIF"), err)
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif2/export_all.qif"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_QIF"), err)
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif2/export_la.qif"));
            QMap<QString, QString> params;
            params[QStringLiteral("uuid_of_selected_accounts_or_operations")] = la.getUniqueID();
            exp1.setExportParameters(params);
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
    }

    {
        // Default account
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/LA-1234567@ing.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("MODIF"), err)
            SKGObjectBase::SKGListSKGObjectBase result;
            SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("bank"), QStringLiteral("t_name='ing'"), result), true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 1)

            SKGBankObject bank(result.at(0));
            SKGTESTERROR(QStringLiteral("BANK.setName"), bank.setName(QStringLiteral("ING Direct")), true)
            SKGTESTERROR(QStringLiteral("BANK.save"), bank.save(), true)

            SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("account"), QStringLiteral("t_number='1234567'"), result), true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 1)

            SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("account"), QStringLiteral("t_name='LA'"), result), true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 1)

            SKGAccountObject acc(result.at(0));
            SKGTESTERROR(QStringLiteral("ACC.setName"), acc.setName(QStringLiteral("Livret A")), true)
            SKGTESTERROR(QStringLiteral("ACC.save"), acc.save(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/LA-1234567@ing.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        document1.dump(DUMPACCOUNT);
        {
            SKGObjectBase::SKGListSKGObjectBase result;
            SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("account"), QStringLiteral("t_number='1234567'"), result), true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 1)
        }
    }

    {
        // Bug import date
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/date_money.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Autorepair
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/autorepair.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("autorepair")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-400"))
        }
    }

    {
        // Error
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/error.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Split and transfer
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/split_and_transfer.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("Joint Checking")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"),
                    SKGServices::doubleToString(account.getCurrentAmount()),
                    QStringLiteral("-1181.25")) // -600 if sum of splits
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("2012 Acadia Loan")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"),
                    SKGServices::doubleToString(account.getCurrentAmount()),
                    QStringLiteral("1162.5")) // 581.25 if sum of splits
        }
    }

    {
        // Split and transfer 2
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/split_and_transfer_2.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("Joint Checking")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("2714.31"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("Credit Union")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("400"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("TSP")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("437.88"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("FSA")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("88.26"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("TSP - Roth")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("250"))
        }
    }

    {
        // 400724
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/400724.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    {
        // 402330
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/402330.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    {
        // 403725
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/403725.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        SKGObjectBase::SKGListSKGObjectBase result;
        SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("refund"), QString(), result), true)
        SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 2)
    }

    {
        // 406243
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/406243.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("406243")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("794"))
        }
    }

    {
        // 406243
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/406271.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGTrackerObject tracker(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), tracker.setName(QStringLiteral("Jane")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), tracker.load(), true)
            SKGTEST(QStringLiteral("QIF:getComment"), tracker.getComment(), QStringLiteral("expenses that Jane incurs"))
        }
    }

    {
        // 406266
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/406266.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGUnitObject unit(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), unit.setName(QStringLiteral("ACME CORP")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), unit.load(), true)
            SKGTEST(QStringLiteral("QIF:getSymbol"), unit.getSymbol(), QStringLiteral("ACMW"))
            SKGTEST(QStringLiteral("QIF:getSymbol"), static_cast<unsigned int>(unit.getType()), static_cast<unsigned int>(SKGUnitObject::SHARE))
        }
    }

    {
        // 406270
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/406270.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        SKGObjectBase::SKGListSKGObjectBase result;
        SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("unit"), QString(), result), true)
        SKGTEST(QStringLiteral("DOC.getObjects.count"), static_cast<unsigned int>(result.count()), static_cast<unsigned int>(1))
    }

    {
        // BUG
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/bug.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    {
        // 489784
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGAccountObject account;
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:setNumber"), bank.setNumber(QStringLiteral("0003")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("489784")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account.setType(SKGAccountObject::CREDITCARD), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/489784.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account.getType()), static_cast<unsigned int>(SKGAccountObject::CREDITCARD))
    }

    {
        // 493062
        SKGDocumentBank document1;
        SKGError err;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/493062.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        SKGObjectBase::SKGListSKGObjectBase result;
        SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("refund"), QStringLiteral("1=1"), result), true)
        SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 0)
    }

    {
        // 513589
        SKGDocumentBank document1;
        SKGError err;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif2/513589.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    // End test
    SKGENDTEST()
}
