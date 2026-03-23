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
#include <qstringliteral.h>
#include <utility>

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
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("missingfile.ofx")));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/ofx_spec160_stmtrs_example.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        {
            // To check double import
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/ofx_spec160_stmtrs_example.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("999988"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("200.29"))
    }

    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/ofx_spec201_stmtrs_example.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("999988"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("200.29"))
    }

    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/t1.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1,
                                                     QStringLiteral("v_account"),
                                                     QStringLiteral("Investment account 12345 at broker ameritrade.com"),
                                                     account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1672.84"))
    }

    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/385366.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1,
                                                     QStringLiteral("v_account"),
                                                     QStringLiteral("Investment account 209830947 at broker fidelity.com"),
                                                     account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("0"))
    }

    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/430130.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("1234567L123"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-8238.77"))
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), account.getAttribute(QStringLiteral("f_importbalance")), QStringLiteral("-8238.77"))
    }

    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/ca_remi.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }
        QStringList oResult;
        SKGTESTERROR(QStringLiteral("ACCOUNT:getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("account"), QStringLiteral("t_name"), oResult),
                     true)
        SKGTEST(QStringLiteral("ACCOUNT:oResult.size"), oResult.size(), 5)

        /*SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"), SKGNamedObject::getObjectByName(&document1,QStringLiteral("v_account"),
        QStringLiteral("1234567L123"), account), true) SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"),SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("3366.86"))*/
    }

    {
        // Test import OFX with initial balance
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/initial_balance.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("40080030367683"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("5036.46"))
    }

    {
        // Test import OFX with initial balance after rename of account
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Create account
            SKGBEGINTRANSACTION(document1, QStringLiteral("CREATE_ACCOUNT"), err)
            SKGTESTERROR(QStringLiteral("DOC.addOrModifyAccount"),
                         document1.addOrModifyAccount(QStringLiteral("COURANT"), QStringLiteral("111111"), QStringLiteral("BANK")),
                         true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/bug_statement.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("COURANT"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("7645.86"))
    }

    {
        // Test BUG 234771
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Create an account without number
            SKGBEGINTRANSACTION(document1, QStringLiteral("CREATE_ACCOUNT"), err)
            SKGTESTERROR(QStringLiteral("DOC.addOrModifyAccount"),
                         document1.addOrModifyAccount(QStringLiteral("COURANT"), QString(), QStringLiteral("BANK")),
                         true)
        }
        {
            // Create an account without number
            SKGBEGINTRANSACTION(document1, QStringLiteral("CREATE_ACCOUNT"), err)
            SKGTESTERROR(QStringLiteral("DOC.addOrModifyAccount"),
                         document1.addOrModifyAccount(QStringLiteral("COURANT2"), QString(), QStringLiteral("BANK")),
                         true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/234771.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            // document1.dump ( DUMPOPERATION|DUMPACCOUNT );
        }

        // Test BUG 319706
        bool existMode = false;
        SKGTESTERROR(QStringLiteral("doc.existObjects"), document1.existObjects(QStringLiteral("operation"), QStringLiteral("t_mode<>''"), existMode), true)
        SKGTESTBOOL("doc.existMode", existMode, true)
    }

    {
        // Test UTF-8 - 284843
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/284843.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        int nb = 0;
        SKGTESTERROR(QStringLiteral("PAYEE.getNbObjects"),
                     document1.getNbObjects(QStringLiteral("v_payee"), "t_name='" % QStringLiteral("カ－ド") % "'", nb),
                     true)
        SKGTEST(QStringLiteral("PAYEE.getNbObjects"), nb, 1)
        document1.dump(DUMPPAYEE);
    }

    {
        // Test UTF-8
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/UTF8.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test 255133
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/exception.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/missing_file.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), false)
        }
    }

    {
        // Test export OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGImportExportManager exp(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportofx/export.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.exportFile"), exp.exportFile(), false)
        }
    }

    {
        // Test 336320
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/336320.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        int nb1 = 0;
        SKGTESTERROR(QStringLiteral("PAYEE.getNbObjects"), document1.getNbObjects(QStringLiteral("operation"), QString(), nb1), true)

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/336320.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        int nb2 = 0;
        SKGTESTERROR(QStringLiteral("PAYEE.getNbObjects"), document1.getNbObjects(QStringLiteral("operation"), QString(), nb2), true)

        SKGTEST(QStringLiteral("PAYEE.getNbObjects"), nb1, nb2)
    }

    {
        // Test handling of debit amount with wrong sign
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/debit_pos.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        bool existDebitPos = false;
        SKGTESTERROR(QStringLiteral("doc.existObjects"),
                     document1.existObjects(QStringLiteral("v_operation"), QStringLiteral("t_mode='Debit' and f_currentamount>0"), existDebitPos),
                     true)
        SKGTESTBOOL("doc.existDebitPos", existDebitPos, false)
    }

    {
        // Test import OFX with missing id on transactions (Theo Raves)
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/mut.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("11111111000"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-12.34"))
    }

    {
        // Test import OFX from GNUCASH fr Android
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(
                &document1,
                QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/20150818_133327_gnucash_export.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
    }

    {
        // Test import OFX from GNUCASH fr Android
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/FEE.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("1234567L123"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-18294.69"))
    }

    {
        // 406321
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/406321.skg"), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/406321.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        // 406741
        int nb2 = 0;
        SKGTESTERROR(QStringLiteral("imp1.getNbObjects"), document1.getNbObjects(QStringLiteral("operation"), QStringLiteral("d_date>'2000-01-01'"), nb2), true)
        SKGTEST(QStringLiteral("OPERATION:nb"), SKGServices::intToString(nb2), QStringLiteral("5"))
    }

    {
        // 406321
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/406321.skg"), true)

        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/406321_2.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
    }

    {
        // 412494
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/412494.qfx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            SKGDocument::SKGMessageList messages;
            SKGTESTERROR(QStringLiteral("imp1.getMessages"), document1.getMessages(document1.getCurrentTransaction(), messages, true), true)
            bool test = false;
            for (const auto &msg : std::as_const(messages)) {
                SKGTRACE << "Message:" << msg.Text << Qt::endl;
                if (msg.Text.contains(QStringLiteral("0 transactions imported")) || msg.Text.contains(QStringLiteral("1 transaction imported"))) {
                    test = true;
                }
            }
            SKGTEST(QStringLiteral("message.0 transactions imported"), static_cast<unsigned int>(test), 1)
        }
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
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/XXXX485.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
            document1.dump(DUMPACCOUNT | DUMPPARAMETERS);
            int nb2 = 0;
            SKGTESTERROR(QStringLiteral("imp1.getNbObjects"), document1.getNbObjects(QStringLiteral("account"), QString(), nb2), true)
            SKGTEST(QStringLiteral("ACCOUNT:nb"), SKGServices::intToString(nb2), QStringLiteral("1"))
        }
    }
    // End test
    SKGENDTEST()
}
