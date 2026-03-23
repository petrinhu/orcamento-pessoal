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

    QDate now = QDate::currentDate();

    {
        // Test import SKGImportExportManager::CSV skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.csv")));
            impmissing.setAutomaticApplyRules(true);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/skrooge_partial.csv"));
            SKGTEST(QStringLiteral("imp1:getImportMimeTypeFilter"), imp1.getImportMimeTypeFilter().split('\n').count(), 2);
            SKGTEST(QStringLiteral("imp1:getExportMimeTypeFilter"), imp1.getExportMimeTypeFilter().split('\n').count(), 2);

            SKGImportExportManager generic;
            SKGTEST(QStringLiteral("SKGImportExportManager().getImportMimeTypeFilter"), generic.getImportMimeTypeFilter().split('\n').count(), 19);
            SKGTEST(QStringLiteral("SKGImportExportManager().getExportMimeTypeFilter"), generic.getExportMimeTypeFilter().split('\n').count(), 11);
            imp1.setCodec(QString());
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("Courant steph"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-935"))
        SKGTEST(QStringLiteral("document1:getCategoryForPayee"),
                document1.getCategoryForPayee(QStringLiteral("Anthony Hopkins"), false),
                QStringLiteral("Entertain > Movie"))
        SKGTEST(QStringLiteral("document1:getCategoryForPayee"), document1.getCategoryForPayee(QStringLiteral("NOT FOUND")), QString())
    }

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

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account.setNumber(QStringLiteral("12345P")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            // Date;Libelle;Libelle complementaire;Montant;Sens;Numero de cheque
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/coopanet.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_header")] = 'N';
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|comment||amount|sign|number");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("QIF.setDefaultAccount"), imp1.setDefaultAccount(&account), true)
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-680.28"))
    }

    {
        // Test import bankperfect
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGAccountObject account1;
        SKGAccountObject account2;
        SKGUnitObject unit_euro(&document1);
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_INIT"), err)

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account1), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account1.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account1.setNumber(QStringLiteral("12345P")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account1.save(), true)

            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account2), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account2.setName(QStringLiteral("PEL")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account2.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_BP_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/bankperfect.csv"));
            SKGTESTERROR(QStringLiteral("QIF.setDefaultAccount"), imp1.setDefaultAccount(&account1), true)
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_BP_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/bankperfect2.csv"));
            SKGTESTERROR(QStringLiteral("QIF.setDefaultAccount"), imp1.setDefaultAccount(&account2), true)
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)

            int NbOperationsMerged = 0;
            SKGTESTERROR(QStringLiteral("imp1.findAndGroupTransfers"), imp1.findAndGroupTransfers(NbOperationsMerged), true)
            SKGTEST(QStringLiteral("imp1:NbOperationsMerged"), NbOperationsMerged, 6)
        }
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account1.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account1.getCurrentAmount()), QStringLiteral("2624.071111"))

        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account2.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account2.getCurrentAmount()), QStringLiteral("1500"))

        SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportcsv/bankperfect.csv"));
        SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"), exp1.exportFile(), true)

        {
            SKGStringListList oTable;
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QStringLiteral("d_DATEMONTH"),
                                                       QStringLiteral("t_CATEGORY"),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            QStringList dump = SKGServices::tableToDump(oTable, SKGServices::DUMP_TEXT);
            int nbl = dump.count();
            for (int i = 0; i < nbl; ++i) {
                SKGTRACE << dump.at(i) << Qt::endl;
            }
        }
        {
            SKGStringListList oTable;
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QString(),
                                                       QStringLiteral("t_CATEGORY"),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            QStringList dump = SKGServices::tableToDump(oTable, SKGServices::DUMP_TEXT);
            int nbl = dump.count();
            for (int i = 0; i < nbl; ++i) {
                SKGTRACE << dump.at(i) << Qt::endl;
            }
        }
        {
            SKGStringListList oTable;
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QStringLiteral("d_DATEWEEK"),
                                                       QString(),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QStringLiteral("d_DATEQUARTER"),
                                                       QString(),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QStringLiteral("d_DATESEMESTER"),
                                                       QString(),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QStringLiteral("d_DATEYEAR"),
                                                       QString(),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QStringLiteral("d_date"),
                                                       QString(),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"),
                         document1.getConsolidatedView(QStringLiteral("v_operation_display"),
                                                       QStringLiteral("d_DATEMONTH"),
                                                       QString(),
                                                       QStringLiteral("f_CURRENTAMOUNT"),
                                                       QStringLiteral("SUM"),
                                                       QString(),
                                                       oTable),
                         true)
            QStringList dump = SKGServices::tableToDump(oTable, SKGServices::DUMP_TEXT);
            int nbl = dump.count();
            for (int i = 0; i < nbl; ++i) {
                SKGTRACE << dump.at(i) << Qt::endl;
            }
        }
    }

    {
        // Test import skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_SKROOGE_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportcsv/bankperfect.csv"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportcsv/bankperfect2.csv"));
        SKGTESTERROR(QStringLiteral("SKGImportExportManager::CSV.exportFile"), exp1.exportFile(), true)
    }

    {
        // Test import skrooge+optimization
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OPTIM"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/bankperfect.csv"));
            SKGTESTERROR(QStringLiteral("OPTIM.importFile"), imp1.importFile(), true)
        }

        SKGTESTERROR(QStringLiteral("OPTIM.undoRedoTransaction"), document1.undoRedoTransaction(SKGDocument::UNDOLASTSAVE), true)
    }

    {
        // Test import skrooge in double to check merge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/remi_1.csv"));
            SKGTESTERROR(QStringLiteral("OPTIM.importFile"), imp1.importFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/remi_1.csv"));
            SKGTESTERROR(QStringLiteral("OPTIM.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.setName(QStringLiteral("remi 1")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-767.26"))
    }
    {
        // Test import 2638120
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/XXXXXXMxxxXXXXXXX.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.setName(QStringLiteral("XXXXXXMxxxXXXXXXX")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("8114.26"))
    }
    {
        // Test import 206894
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/206894.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.setName(QStringLiteral("206894")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-2986.39"))
    }
    {
        // Test import 397055
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/397055.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.setName(QStringLiteral("397055")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("50"))
    }
    {
        // Test import with tabulation
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/mutual fund.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.setName(QStringLiteral("Janus Twenty Fund")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("24.51428572"))
    }

    {
        // Test import shares with original amount
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/209705.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/209705_2.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1);

            int out = 0;
            SKGTESTERROR(QStringLiteral("CSV.findAndGroupTransfers"), imp1.findAndGroupTransfers(out), true)
            SKGTEST(QStringLiteral("CSV:nb"), out, 2)
        }

        SKGObjectBase::SKGListSKGObjectBase grouped;
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getObjects(QStringLiteral("operation"), QStringLiteral("i_group_id!=0"), grouped), true)
        SKGTEST(QStringLiteral("CSV:grouped.count"), grouped.count(), 2)
    }

    {
        // Test import transactions split and grouped
        SKGAccountObject la;
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/compte.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)

            SKGObjectBase::SKGListSKGObjectBase banks;
            SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getObjects(QStringLiteral("bank"), QString(), banks), true)
            SKGBankObject bank(banks.at(0));
            bank.setName(QStringLiteral("bp"));
            bank.save();
        }

        SKGObjectBase::SKGListSKGObjectBase grouped;
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getObjects(QStringLiteral("operation"), QString(), grouped), true)
        SKGTEST(QStringLiteral("CSV:grouped.count"), grouped.count(), 4)
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getObjects(QStringLiteral("operation"), QStringLiteral("i_group_id!=0"), grouped), true)
        SKGTEST(QStringLiteral("CSV:grouped.count"), grouped.count(), 2)

        SKGObjectBase::SKGListSKGObjectBase result;
        SKGTESTERROR(QStringLiteral("DOC.getObjects"), document1.getObjects(QStringLiteral("account"), QStringLiteral("t_name='PEL'"), result), true)
        SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 1)
        if (result.count() != 0) {
            la = result.at(0);
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_CSV"), err)
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportcsv/export_all.csv"));
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_CSV"), err)
            SKGImportExportManager exp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportcsv/export_la.csv"));
            QMap<QString, QString> params;
            params[QStringLiteral("uuid_of_selected_accounts_or_operations")] = la.getUniqueID();
            exp1.setExportParameters(params);
            SKGTESTERROR(QStringLiteral("QIF.exportFile"), exp1.exportFile(), true)
        }
    }

    {
        // Test import mmex
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/mmex.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("CATEGORY.getNbObjects"),
                     document1.getNbObjects(QStringLiteral("category"), QStringLiteral("t_fullname='Alimentation > Restaurant'"), nb),
                     true)
        SKGTEST(QStringLiteral("CATEGORY:nb"), nb, 1)

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("mmex")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1418.44"))
    }
    {
        // Test import mmex
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/mmex_no_header.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|comment|sign|amount|category");
            parameters[QStringLiteral("automatic_search_header")] = 'N';
            parameters[QStringLiteral("header_position")] = '0';
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("mmex no header")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1418.44"))
    }
    {
        // 263263
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/263263.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("mapping_debit")] = QStringLiteral("kreditrente");
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date||number|sign|comment|amount|amount");
            parameters[QStringLiteral("automatic_search_header")] = 'N';
            parameters[QStringLiteral("header_position")] = '1';
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("263263")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("800.09"))
    }
    {
        // CREDIT-DEBIT
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/credit-debit.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("credit debit")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1500"))
    }
    {
        // BACKSLASHES
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/backslashes.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("backslashes")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1000"))
    }
    {
        // MULTILINE
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/multiline.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("multiline")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("3000"))
    }

    {
        // MULTILINE
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/non_numerical_amount.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("non numerical amount")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-119.56"))
    }

    {
        // Test import 320112
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/320112.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|mode|payee|comment|amount");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("320112")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("6.13"))
    }

    {
        // Test import date DDMMMYYYY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/date_DDMMMYYYY.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("date DDMMMYYYY")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getAmount"), SKGServices::doubleToString(account.getAmount(QDate(2004, 12, 31))), QStringLiteral("35"))
    }

    {
        // Test import separator tab
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/tabs.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import separator comma
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/commas.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }
    }

    {
        // BUG 406488
        //
        // "Date","Type","Number","Payee","Withdrawal (-)","Amount"
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/406488.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|mode|number|payee|amount|amount");
            imp1.setImportParameters(parameters);
            SKGError err = imp1.importFile();
            SKGTESTERROR(QStringLiteral("CSV.importFile"), err, false)
            SKGTEST(QStringLiteral("CSV:error message"), err.getMessage(), QStringLiteral("Invalid number of columns in line 2. Expected 6. Found 1."))
        }
    }

    {
        // Test import separator comma
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/349961.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }
        int nb = 0;
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getNbObjects(QStringLiteral("operation"), QString(), nb), true)
        SKGTEST(QStringLiteral("CSV:nb transactions"), nb, 1)
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getNbObjects(QStringLiteral("suboperation"), QStringLiteral("d_date='2015-07-07'"), nb), true)
        SKGTEST(QStringLiteral("CSV:nb subtransactions 2015-07-07"), nb, 1)
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getNbObjects(QStringLiteral("suboperation"), QStringLiteral("d_date='2015-07-08'"), nb), true)
        SKGTEST(QStringLiteral("CSV:nb subtransactions 2015-07-08"), nb, 1)
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getNbObjects(QStringLiteral("suboperation"), QStringLiteral("d_date='2015-07-09'"), nb), true)
        SKGTEST(QStringLiteral("CSV:nb subtransactions 2015-07-09"), nb, 1)
    }

    {
        // Test import separator comma
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/362231.csv"));
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }
        int nb = 0;
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getNbObjects(QStringLiteral("operation"), QStringLiteral("d_date!='0000-00-00'"), nb), true)
        SKGTEST(QStringLiteral("CSV:nb operation"), nb, 1)
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getNbObjects(QStringLiteral("suboperation"), QStringLiteral("d_date='2016-05-15'"), nb), true)
        SKGTEST(QStringLiteral("CSV:nb subtransactions 2016-05-15"), nb, 1)
        SKGTESTERROR(QStringLiteral("CSV.getObjects"), document1.getNbObjects(QStringLiteral("suboperation"), QStringLiteral("d_date='2016-05-20'"), nb), true)
        SKGTEST(QStringLiteral("CSV:nb subtransactions 2016-05-20"), nb, 1)
    }

    {
        // 381562
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
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account.setNumber(QStringLiteral("DE00 1234 5678 9012 3456 78")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            // Date;Libelle;Libelle complementaire;Montant;Sens;Numero de cheque
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/DE00 1234 5678 9012 3456 78.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_header")] = 'N';
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|comment||amount|sign|number");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-680.28"))
    }

    {
        // Test 411958
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/411958.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("mapping_date")] = QStringLiteral("^Buchungstag");
            parameters[QStringLiteral("mapping_payee")] = QStringLiteral("^payee|^tiers|^.*Zahlungspflichtiger");
            parameters[QStringLiteral("mapping_comment")] = QStringLiteral("^comment|^libell?|^d?tail|^info|^Vorgang.*");
            parameters[QStringLiteral("mapping_amount")] = QStringLiteral("^value|^amount|^valeur|^montant|^credit|^debit|^Umsatz");
            parameters[QStringLiteral("mapping_account")] = QStringLiteral("^Konto");
            parameters[QStringLiteral("mapping_sign")] = QStringLiteral("^sign|^sens");
            parameters[QStringLiteral("mapping_unit")] = QStringLiteral("^Währung");
            parameters[QStringLiteral("mapping_debit")] = QStringLiteral("^S");
            parameters[QStringLiteral("automatic_search_columns")] = 'Y';
            parameters[QStringLiteral("automatic_search_header")] = 'Y';
            imp1.setImportParameters(parameters);

            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("411958")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("658.88"))
    }

    {
        // Test import with footer
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/footer.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|mode|payee|amount|");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account(&document1);
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.setName(QStringLiteral("footer")), true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-2"))
    }

    {
        // Test import separator comma
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/commas_2.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("header_position")] = QStringLiteral("1");
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("|date||payee|amount||||mode");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import BUG 420557
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/420557_1.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("header_position")] = QStringLiteral("1");
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|account|idgroup|idtransaction|value|unit");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("OPERATION.getNbObjects"), document1.getNbObjects(QStringLiteral("operation"), QStringLiteral("i_group_id=1"), nb), true)
        SKGTEST(QStringLiteral("OPERATION:nb"), nb, 2)
    }

    {
        // Test import BUG 420557
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/420557_2.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("header_position")] = QStringLiteral("1");
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("date|account|idgroup|idtransaction|value|unit");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("OPERATION.getNbObjects"), document1.getNbObjects(QStringLiteral("operation"), QStringLiteral("i_group_id=1"), nb), true)
        SKGTEST(QStringLiteral("OPERATION:nb"), nb, 2)
    }

    {
        // 421302
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Create an account without number
            SKGBEGINTRANSACTION(document1, QStringLiteral("CREATE_ACCOUNT"), err)
            SKGTESTERROR(QStringLiteral("DOC.addOrModifyAccount"),
                         document1.addOrModifyAccount(QStringLiteral("COURANT"), QStringLiteral("5823485"), QStringLiteral("BANK")),
                         true)
            SKGObjectBase account;
            SKGTESTERROR(QStringLiteral("DOC.getObject"), document1.getObject(QStringLiteral("v_account"), QStringLiteral("t_name='COURANT'"), account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.setProperty"), account.setProperty(QStringLiteral("alias"), QStringLiteral("XXXX485")), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/XXXX485.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("header_position")] = QStringLiteral("1");
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("|date||payee|amount||||mode");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)
            int nb2 = 0;
            SKGTESTERROR(QStringLiteral("imp1.getNbObjects"), document1.getNbObjects(QStringLiteral("account"), QString(), nb2), true)
            SKGTEST(QStringLiteral("ACCOUNT:nb"), SKGServices::intToString(nb2), QStringLiteral("1"))
        }
    }

    {
        // 494159
        QLocale::setDefault(QLocale::German);

        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/494159.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("header_position")] = QStringLiteral("1");
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("|date||payee|||||amount");
            imp1.setImportParameters(parameters);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)

            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.setName(QStringLiteral("494159")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-12468.93"))
        }
    }

    {
        // 492495
        QLocale::setDefault(QLocale::German);

        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/492495.skg"), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportcsv/494159.csv"));
            QMap<QString, QString> parameters = imp1.getImportParameters();
            parameters[QStringLiteral("automatic_search_columns")] = 'N';
            parameters[QStringLiteral("header_position")] = QStringLiteral("1");
            parameters[QStringLiteral("columns_positions")] = QStringLiteral("|date||payee|||||amount");
            imp1.setImportParameters(parameters);
            imp1.setAutomaticApplyRules(true);
            SKGTESTERROR(QStringLiteral("CSV.importFile"), imp1.importFile(), true)

            document1.dump(DUMPOPERATION | DUMPACCOUNT);
            int nb = 0;
            SKGTESTERROR(QStringLiteral("imp1.getNbObjects"), document1.getNbObjects(QStringLiteral("account"), QString(), nb), true)
            SKGTEST(QStringLiteral("ACCOUNT:nb"), SKGServices::intToString(nb), QStringLiteral("2"))

            SKGTESTERROR(QStringLiteral("imp1.getNbObjects"), document1.getNbObjects(QStringLiteral("bank"), QString(), nb), true)
            SKGTEST(QStringLiteral("BANK:nb"), SKGServices::intToString(nb), QStringLiteral("1"))
        }
    }
    // End test
    SKGENDTEST()
} // NOLINT(readability/fn_size)
