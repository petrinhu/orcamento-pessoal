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
        // Test import QIF 1
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGAccountObject account;
        SKGUnitObject unit_euro(&document1);
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.qif")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)
            impmissing.getParameterDefaultValue(QStringLiteral("mapping_date"));

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:setNumber"), bank.setNumber(QStringLiteral("0003")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account.setNumber(QStringLiteral("12345P")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setAgencyNumber"), account.setAgencyNumber(QStringLiteral("98765")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setAgencyNumber"), account.setAgencyAddress(QStringLiteral("10 rue Dupon, 31000 TOULOUSE")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(d), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_BP_QIF"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/ref.qif"));
            SKGTESTERROR(QStringLiteral("QIF.setDefaultAccount"), imp1.setDefaultAccount(&account), true)
            SKGTESTERROR(QStringLiteral("QIF.setDefaultUnit"), imp1.setDefaultUnit(&unit_euro), true)
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-935"))

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_BP_QIF"), err)
            SKGImportExportManager imp1(&document1);
            SKGTESTERROR(QStringLiteral("QIF.cleanBankImport"), imp1.cleanBankImport(), true)
            int out = 0;
            SKGTESTERROR(QStringLiteral("QIF.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)

            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/ref.qif"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }

        SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-935"))
    }

    {
        // Test import QIF 2
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGAccountObject account;
        SKGUnitObject unit_euro(&document1);
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("Courant Guillaume")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account.setNumber(QStringLiteral("98765A")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(d), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_BP_QIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/E0269787.qif"));
            SKGTESTERROR(QStringLiteral("QIF.setDefaultAccount"), imp1.setDefaultAccount(&account), true)
            SKGTESTERROR(QStringLiteral("QIF.setDefaultUnit"), imp1.setDefaultUnit(&unit_euro), true)
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            SKGTESTERROR(QStringLiteral("QIF.cleanBankImport"), imp1.cleanBankImport(), true)
            int out = 0;
            SKGTESTERROR(QStringLiteral("QIF.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)

            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/E0269787.qif"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }

        SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-1684.58"))

        // Check import with account retrieved from file
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        unit_euro = SKGUnitObject(&document1);
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)
            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(d), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/E0269787.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            SKGTESTERROR(QStringLiteral("QIF.cleanBankImport"), imp1.cleanBankImport(), true)
            int out = 0;
            SKGTESTERROR(QStringLiteral("QIF.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)

            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/E0269787_bis.qif"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }

        // Check multi import
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/ref.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true) // Double import
            SKGTESTERROR(QStringLiteral("QIF.cleanBankImport"), imp1.cleanBankImport(), true)
            int out = 0;
            SKGTESTERROR(QStringLiteral("QIF.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)
        }

        // Check import with account retrieved from files
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/ref.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            SKGImportExportManager imp2(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/E0269787.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp2.importFile(), true)
            SKGTESTERROR(QStringLiteral("QIF.cleanBankImport"), imp1.cleanBankImport(), true)
            int out = 0;
            SKGTESTERROR(QStringLiteral("QIF.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)

            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/E0269787_ref.qif"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }

        // Check import qif multi accounts
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/E0269787_ref.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            SKGTESTERROR(QStringLiteral("QIF.cleanBankImport"), imp1.cleanBankImport(), true)
            int out = 0;
            SKGTESTERROR(QStringLiteral("QIF.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getDistinctValues"),
                         document1.getDistinctValues(QStringLiteral("account"), QStringLiteral("t_name"), oResult),
                         true)
            SKGTEST(QStringLiteral("ACCOUNT:oResult.size"), oResult.size(), 3)
        }
    }

    {
        // Test import QIF 2
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGAccountObject account;
        SKGUnitObject unit_euro(&document1);
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("ING")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("ING")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account.setNumber(QStringLiteral("ING")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(d), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_ING_QIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/ing.qif"));
            SKGTESTERROR(QStringLiteral("QIF.setDefaultAccount"), imp1.setDefaultAccount(&account), true)
            SKGTESTERROR(QStringLiteral("QIF.setDefaultUnit"), imp1.setDefaultUnit(&unit_euro), true)
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            SKGTESTERROR(QStringLiteral("QIF.cleanBankImport"), imp1.cleanBankImport(), true)
            int out = 0;
            SKGTESTERROR(QStringLiteral("QIF.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)

            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/ing.qif"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
    }

    {
        // Test import QIF MOTO
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_ING_QIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/moto.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("ACTIF Moto")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("26198.77"))
        }
    }

    {
        // Test import QIF MOTO with euro and franc
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)

            SKGUnitObject unit;
            SKGTESTERROR(QStringLiteral("SKGUnitObject::createCurrencyUnit"), SKGUnitObject::createCurrencyUnit(&document1, QStringLiteral("EUR"), unit), true)
            SKGTESTERROR(QStringLiteral("SKGUnitObject::createCurrencyUnit"), SKGUnitObject::createCurrencyUnit(&document1, QStringLiteral("FRF"), unit), true)
            SKGTESTERROR(QStringLiteral("FRANC.addOrModifyUnitValue"),
                         document1.addOrModifyUnitValue(QStringLiteral("French Franc (FRF)"), QDate(1963, 1, 1), 1.0 / 6.55957),
                         true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_ING_QIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/moto.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("ACTIF Moto")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(static_cast<int>(account.getCurrentAmount())), QStringLiteral("5009"))
        }
    }

    {
        // Test import QIF KMM
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMM_QIF"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/kmm-without-category.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-52.36"))
        }
    }

    {
        // Test import QIF KMM with category
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_KMM_QIF"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/kmm-with-category.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-52.36"))
        }
    }

    {
        // Test import QIF REMI in double to check merge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_REMI_QIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/remi_2.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_REMI_QIF"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/remi_2.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("remi 2")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-1208.63"))
        }
    }

    {
        // Test import QIF REMI in double to check merge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_DOUBLE"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/double.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("double")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-243"))
        }
    }

    {
        // Test import bug GNUCash 350286
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/350286.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("350286")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1057.37"))
        }
    }

    {
        // Test import bug GNUCash 393596
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/393596.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("393596")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("12.34"))
        }
    }

    {
        // Test import bug GNUCash 503166
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/503166.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("My Investments")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("20"))
        }
    }

    {
        // Test import bug GNUCash 392707
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/392707.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("392707")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1219.06"))
        }
    }

    {
        // Test import bug GNUCash 373584
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/373584.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("My Investments")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1000000"))
        }
    }

    {
        // Test import bug 199818
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/199818.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("199818")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-345.64"))
        }
    }

    {
        // Test import bug 201316
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/201316.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGStringListList listTmp;
            SKGTESTERROR(QStringLiteral("QIF.executeSelectSqliteOrder"),
                         document1.executeSelectSqliteOrder(
                             QStringLiteral("SELECT TOTAL(f_CURRENTAMOUNT),t_status FROM v_operation_display GROUP BY t_status ORDER BY t_status"),
                             listTmp),
                         true);
            SKGTEST(QStringLiteral("QIF:listTmp.count"), listTmp.count(), 4)
            if (listTmp.count() == 4) {
                SKGTEST(QStringLiteral("QIF:listTmp.at(1).at(0)"), listTmp.at(1).at(0), QStringLiteral("-10"))
                SKGTEST(QStringLiteral("QIF:listTmp.at(2).at(0)"), listTmp.at(2).at(0), QStringLiteral("-100"))
                SKGTEST(QStringLiteral("QIF:listTmp.at(3).at(0)"), listTmp.at(3).at(0), QStringLiteral("-1000"))
            }
        }
    }

    {
        // Test import bug 201451
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/201451.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("liability")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-123.45"))
        }
    }

    {
        // Test import bug 214809
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/BNP_CC_virement.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("BNP CC")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("696.64"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("BNP CEL")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("500"))
        }
    }

    {
        // Test import bug 214851
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/Fortuneo PEA (Caisse).qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)

            SKGImportExportManager imp2(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/Fortuneo PEA.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp2.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("BNP CC")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-3604"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("Fortuneo PEA")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("86.29"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("Fortuneo PEA (Caisse)")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("10319.5"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("Fortuneo Titres (Caisse)")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-6700"))
        }
    }

    {
        // Test import bug 215620
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/Cortal PEA.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)

            SKGImportExportManager imp2(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/EADS.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp2.importFile(), true)
        }
    }

    {
        // Test import bug 216520
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/216520.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("BNP CC")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("11196.64"))
        }
    }

    {
        // Test import investment
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)

            SKGUnitObject unit;
            SKGTESTERROR(QStringLiteral("EUR.setName"), SKGUnitObject::createCurrencyUnit(&document1, QStringLiteral("Euro (EUR)"), unit), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/exp_inv.qif"));
            imp1.setCodec(QStringLiteral("UTF-8"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)

            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/exp_inv.qif"));
            exp1.setCodec(QStringLiteral("UTF-8"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
    }

    {
        // Test transfer qif
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/t2.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("COURANT")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("4767.97"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("CODEVI")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-4767.97"))
        }
    }

    {
        // 233930
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/233930.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Quicken
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/quicken.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
    }

    {
        // 267996
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/267996.qif"));
            SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
        }
        {
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportqif1/267996.csv"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
    }

    {
        // 271708
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)

            {
                SKGImportExportManager imp1(&document1,
                                            QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/271708/10Compte A.qif"));
                SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            }
            {
                SKGImportExportManager imp1(&document1,
                                            QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/271708/20Compte B.qif"));
                SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            }
            {
                SKGImportExportManager imp1(&document1,
                                            QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/271708/30Compte C.qif"));
                SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            }
            {
                SKGImportExportManager imp1(&document1,
                                            QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportqif1/271708/40Compte D.qif"));
                SKGTESTERROR(QStringLiteral("QIF.importFile"), imp1.importFile(), true)
            }
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("10Compte A")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("0"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("20Compte B")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("10"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("30Compte C")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("20"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("QIF.setName"), account.setName(QStringLiteral("40Compte D")), true)
            SKGTESTERROR(QStringLiteral("QIF.load"), account.load(), true)
            SKGTEST(QStringLiteral("QIF:getValue"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("40"))
        }
    }

    // End test
    SKGENDTEST()
} // NOLINT(readability/fn_size)
