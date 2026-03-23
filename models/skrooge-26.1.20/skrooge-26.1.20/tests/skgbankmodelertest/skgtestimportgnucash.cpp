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
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("/not-existing/missingfile.uncompressed")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/test_data.uncompressed"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Accounts Receivable")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("231.11"))
        }
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/all.uncompressed"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("COMPTE COURANT")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-700"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("COMPTE EPARGNE")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("250"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Actif")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-420"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("TITRE")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("520"))
        }
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/initial_balance.uncompressed"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("100"))
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("GNUCASH:getNbObjects(unit, PRIMARY)"),
                     document1.getNbObjects(QStringLiteral("unit"), QStringLiteral("t_type='1'"), nb),
                     true)
        SKGTEST(QStringLiteral("GNUCASH:getNbObjects(unit, PRIMARY)"), nb, 1)
        SKGTESTERROR(QStringLiteral("GNUCASH:getNbObjects(unit, SHARE)"),
                     document1.getNbObjects(QStringLiteral("unit"), QStringLiteral("t_type='S'"), nb),
                     true)
        SKGTEST(QStringLiteral("GNUCASH:getNbObjects(unit, SHARE)"), nb, 13)
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/transfer-CPP-vers-PEE.uncompressed"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("750"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CEL Donald")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("150"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("PEEs")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("190"))
        }

        {
            int nb = 0;
            SKGTESTERROR(QStringLiteral("GNUCASH:getNbObjects"), document1.getNbObjects(QStringLiteral("category"), QString(), nb), true)
            SKGTEST(QStringLiteral("GNUCASH:getNbObjects"), nb, 27)
        }
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/transfer-PEE-vers-CCP.uncompressed"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1140"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CEL Donald")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("150"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("PEEs")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("100"))
        }

        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)
            SKGCategoryObject cat;
            SKGTESTERROR(QStringLiteral("GNUCASH.createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QStringLiteral("Depenses > Frais bancaires"), cat),
                         true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(cat.getCurrentAmount()), QStringLiteral("-10"))
        }

        SKGTESTERROR(QStringLiteral("DOC:dump"), document1.dump(DUMPCATEGORY), true)
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/interet-revenue-frais.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("750"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CEL Donald")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("150"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("PEEs")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("640"))
        }
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/interet-revenue-frais-emprunt.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("CCP")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("250"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Emprunts")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-99620"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Maison")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("100000"))
        }
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/rapprochement-incorrect.uncompressed"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            int nb = 0;
            SKGTESTERROR(QStringLiteral("GNUCASH:getNbObjects"), document1.getNbObjects(QStringLiteral("operation"), QStringLiteral("t_status='Y'"), nb), true)
            SKGTEST(QStringLiteral("GNUCASH:getNbObjects"), nb, 7)
        }
    }

    {
        // Test import GNUCASH
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/scheduled-action.uncompressed"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGObjectBase::SKGListSKGObjectBase recurrentoperations;
            SKGTESTERROR(QStringLiteral("GNUCASH:SKGListSKGObjectBase"),
                         document1.getObjects(QStringLiteral("recurrentoperation"), QString(), recurrentoperations),
                         true)
            SKGTEST(QStringLiteral("GNUCASH:nb"), recurrentoperations.count(), 1)
            if (recurrentoperations.count() == 1) {
                SKGRecurrentOperationObject recu(recurrentoperations.at(0));
                SKGTEST(QStringLiteral("GNUCASH:getDate"), recu.getDate().toString(), QDate(2010, 03, 02).toString())
                SKGTEST(QStringLiteral("GNUCASH:getAutoWriteDays"), recu.getAutoWriteDays(), 10)
                SKGTEST(QStringLiteral("GNUCASH:getPeriodIncrement"), recu.getPeriodIncrement(), 1)
                SKGTEST(QStringLiteral("GNUCASH:getPeriodUnit"),
                        static_cast<unsigned int>(recu.getPeriodUnit()),
                        static_cast<unsigned int>(SKGRecurrentOperationObject::MONTH))
                SKGTEST(QStringLiteral("GNUCASH:getWarnDays"), recu.getWarnDays(), 0)
                SKGTESTBOOL("GNUCASH:isAutoWriteEnabled", recu.isAutoWriteEnabled(), false)
                SKGTESTBOOL("GNUCASH:isWarnEnabled", recu.isWarnEnabled(), false)
                SKGTESTBOOL("GNUCASH:hasTimeLimit", recu.hasTimeLimit(), false)
            }
        }
    }

    {
        // Test import GNUCASH - bug 228808
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/228808.gnc"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGObjectBase::SKGListSKGObjectBase recurrentoperations;
            SKGTESTERROR(QStringLiteral("GNUCASH:SKGListSKGObjectBase"),
                         document1.getObjects(QStringLiteral("recurrentoperation"), QStringLiteral("1=1 ORDER BY d_date"), recurrentoperations),
                         true)
            SKGTEST(QStringLiteral("GNUCASH:nb"), recurrentoperations.count(), 2)
            if (recurrentoperations.count() == 2) {
                SKGRecurrentOperationObject recu(recurrentoperations.at(0));
                SKGOperationObject op;
                SKGTESTERROR(QStringLiteral("GNUCASH:getParentOperation"), recu.getParentOperation(op), true)
                SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), op.getCurrentAmount(), -50)

                recu = recurrentoperations.at(1);
                SKGTESTERROR(QStringLiteral("GNUCASH:getParentOperation"), recu.getParentOperation(op), true)
                SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), op.getCurrentAmount(), -18.75)
            }
        }
    }

    {
        // Test import GNUCASH - bug 228904
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/228904.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGObjectBase::SKGListSKGObjectBase recurrentoperations;
            SKGTESTERROR(QStringLiteral("GNUCASH:SKGListSKGObjectBase"),
                         document1.getObjects(QStringLiteral("recurrentoperation"), QStringLiteral("1=1 ORDER BY d_date"), recurrentoperations),
                         true)
            SKGTEST(QStringLiteral("GNUCASH:nb"), recurrentoperations.count(), 3)
        }
    }

    {
        // Test import 228901
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/228901.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Compte emprunt")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-9476.01"))
        }

        // Test second import
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/228901.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import 234608
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/234608.gnc"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import 234597
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/234597.gnc"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }
        {
            int nb = 0;
            SKGTESTERROR(QStringLiteral("GNUCASH:getNbObjects"), document1.getNbObjects(QStringLiteral("account"), QString(), nb), true)
            SKGTEST(QStringLiteral("GNUCASH:getNbObjects"), nb, 11)
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Compte cheques")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-260"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Compte cheques (2)")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-40"))
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Argent du porte-monnaie")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("0"))
        }
    }

    {
        // Test import 243738
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/243738.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import without book
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/without_book.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import 302388
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/302388.gnc"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import 325174
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/325174.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("GNUCASH:getNbObjects(account)"), document1.getNbObjects(QStringLiteral("account"), QString(), nb), true)
        SKGTEST(QStringLiteral("GNUCASH:getNbObjects(account)"), nb, 11)
    }

    {
        // Test import wallet
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/wallet.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Espece")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGBankObject bank;
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.getBank(bank), true)
            SKGTEST(QStringLiteral("GNUCASH:getName"), bank.getName(), QString())
        }
        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Courant")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGBankObject bank;
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.getBank(bank), true)
            SKGTEST(QStringLiteral("GNUCASH:getName"), bank.getName(), QStringLiteral("GNUCASH"))
        }
    }

    {
        // 407257
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_GNUCASH"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportgnucash/407257.gnucash"));
            SKGTESTERROR(QStringLiteral("GNUCASH.importFile"), imp1.importFile(), true)
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("AccountName (Nom de compte)")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
            SKGTEST(QStringLiteral("GNUCASH:getNumber"), account.getNumber(), QStringLiteral("123_AccountCode(CodeCompte)"))
            SKGTEST(QStringLiteral("GNUCASH:getComment"), account.getComment(), QStringLiteral("DescriptionAccount(Compte123)"))

            SKGUnitObject unit;
            SKGTESTERROR(QStringLiteral("GNUCASH.getUnit"), account.getUnit(unit), true)
            SKGTEST(QStringLiteral("GNUCASH:getName"), unit.getName(), QStringLiteral("Canadian Dollar (CAD)"))
        }

        {
            SKGAccountObject account(&document1);
            SKGTESTERROR(QStringLiteral("GNUCASH.setName"), account.setName(QStringLiteral("Account2Name(no transaction)")), true)
            SKGTESTERROR(QStringLiteral("GNUCASH.load"), account.load(), true)
        }
    }
    // End test
    SKGENDTEST()
}
