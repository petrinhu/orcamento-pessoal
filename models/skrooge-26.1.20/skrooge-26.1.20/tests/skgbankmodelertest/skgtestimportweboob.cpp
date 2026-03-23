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
    QString path = qgetenv("PATH");
    QString in = qgetenv("IN");

    {
        // Test import Weboob
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportbackend/fake1/fake1.skg"),
                     true)
        SKGError err;
        {
            SKGTESTBOOL("PUTENV.fake1", qputenv("PATH", (in + "/skgtestimportbackend/fake1/:" + path).toLatin1()), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Check
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("12345"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1523.99"))
            SKGTEST(QStringLiteral("ACCOUNT:getAmount"), SKGServices::doubleToString(account.getAmount(QDate(2013, 5, 1))), QStringLiteral("1471.22"))

            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("47896"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-30"))
            SKGTEST(QStringLiteral("ACCOUNT:getAmount"), SKGServices::doubleToString(account.getAmount(QDate(2013, 5, 1))), QStringLiteral("0"))

            SKGObjectBase::SKGListSKGObjectBase result;
            SKGTESTERROR(QStringLiteral("DOC.getObjects"),
                         document1.getObjects(QStringLiteral("v_operation"), QStringLiteral("t_comment='SNCF'"), result),
                         true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 2)

            SKGTESTERROR(QStringLiteral("DOC.getObjects"),
                         document1.getObjects(QStringLiteral("v_operation"), QStringLiteral("d_date='2013-05-26'"), result),
                         true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 1)
        }
    }

    {
        // BUG: 320716
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            SKGTESTBOOL("PUTENV.320716_1", qputenv("PATH", (in + "/skgtestimportbackend/320716_1/:" + path).toLatin1()), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Check
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("12345"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("100"))
        }

        {
            SKGTESTBOOL("PUTENV.320716_1", qputenv("PATH", (in + "/skgtestimportbackend/320716_2/:" + path).toLatin1()), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Check
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("12345"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("0"))
        }
    }

    {
        // Test error
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            SKGTESTBOOL("PUTENV.fake1", qputenv("PATH", (in + "/skgtestimportbackend/error1/:" + path).toLatin1()), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTEST(QStringLiteral("WEBOOB.importFile"), imp1.importFile().getReturnCode(), ERR_FAIL)
        }
    }

    {
        // Test error
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            SKGTESTBOOL("PUTENV.fake1", qputenv("PATH", (in + "/skgtestimportbackend/error2/:" + path).toLatin1()), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTEST(QStringLiteral("WEBOOB.importFile"), imp1.importFile().getReturnCode(), ERR_FAIL)
        }
    }

    {
        // Test error
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            SKGTESTBOOL("PUTENV.fake1", qputenv("PATH", (in + "/skgtestimportbackend/error3/:" + path).toLatin1()), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            document1.sendMessage(QStringLiteral("Hello"));

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTEST(QStringLiteral("WEBOOB.importFile"), imp1.importFile().getReturnCode(), ERR_FAIL)
        }
    }

    {
        // Test error
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            SKGTESTBOOL("PUTENV.fake1", qputenv("PATH", (in + "/skgtestimportbackend/error4/:" + path).toLatin1()), true)

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTEST(QStringLiteral("WEBOOB.importFile"), imp1.importFile().getReturnCode(), ERR_FAIL)
        }
    }

    {
        // Better account selection
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("CREATE ACCOUNT"), err)

            // Creation the bank
            SKGBankObject bank(&document1);
            SKGTESTERROR(QStringLiteral("BANK.setName"), bank.setName(QStringLiteral("caisse-epargne")), true)
            SKGTESTERROR(QStringLiteral("BANK.setNumber"), bank.setNumber(QStringLiteral("13135")), true)
            SKGTESTERROR(QStringLiteral("BANK.save"), bank.save(), true)

            // Creation the account
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("BANK.addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("test")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.setNumber"), account.setNumber(QStringLiteral("123456789")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.setAgencyNumber"), account.setAgencyNumber(QStringLiteral("00080")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.save"), account.save(), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/betterselection/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Check
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("test"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-147994.82"))
        }
    }

    {
        // Better account selection
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/joint_accounts/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Check
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("12345"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1000"))
        }
    }

    {
        // STAMM
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("CREATE ACCOUNT"), err)

            // Creation the bank
            SKGBankObject bank(&document1);
            SKGTESTERROR(QStringLiteral("BANK.setName"), bank.setName(QStringLiteral("ldlc")), true)
            SKGTESTERROR(QStringLiteral("BANK.save"), bank.save(), true)

            // Creation the accounts
            {
                SKGAccountObject account;
                SKGTESTERROR(QStringLiteral("BANK.addAccount"), bank.addAccount(account), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("acc1")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setNumber"), account.setNumber(QStringLiteral("1234567A")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setAgencyNumber"), account.setAgencyNumber(QStringLiteral("00080")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.save"), account.save(), true)
            }
            {
                SKGAccountObject account;
                SKGTESTERROR(QStringLiteral("BANK.addAccount"), bank.addAccount(account), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("acc2")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setNumber"), account.setNumber(QStringLiteral("987654321")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setAgencyNumber"), account.setAgencyNumber(QStringLiteral("00080")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.save"), account.save(), true)
            }
            {
                SKGAccountObject account;
                SKGTESTERROR(QStringLiteral("BANK.addAccount"), bank.addAccount(account), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setName"), account.setName(QStringLiteral("acc3")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setNumber"), account.setNumber(QStringLiteral("1111111A")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.setAgencyNumber"), account.setAgencyNumber(QStringLiteral("00080")), true)
                SKGTESTERROR(QStringLiteral("ACCOUNT.save"), account.save(), true)
            }
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/stamm/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Checks
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("acc1"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-50"))
        }
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("acc2"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-50"))
        }
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("acc3"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-50"))
        }

        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("1234567W089"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-50"))
        }

        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("MYACCOUNTNAME"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-50"))
        }
    }

    {
        // Avoid merge of account due to same name
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/merge/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Checks
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("CPT"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-50"))
        }
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("CPT2"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-50"))
        }
    }

    {
        // Avoid merge of account due to same name
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/kevin/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Checks
        QStringList accounts;
        accounts << QStringLiteral("COMPTE 1") << QStringLiteral("COMPTE 2") << QStringLiteral("LIVRET 1") << QStringLiteral("LIVRET 2")
                 << QStringLiteral("EPARGNE 1") << QStringLiteral("EPARGNE 2") << QStringLiteral("EPARGNE 3");
        for (const auto &acc : std::as_const(accounts)) {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), acc, account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-10"))
        }
    }

    {
        // Avoid merge of account due to same name
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/double/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Checks
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("CPT"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-100"))
        }
    }

    {
        // Avoid merge of account due to same name
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/397055/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Checks
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("LEO"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("-20"))
        }
    }

    {
        // rdate Not available
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/397611/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Checks
        {
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("LEO"), account),
                         true)
            SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("598.8"))

            SKGObjectBase::SKGListSKGObjectBase result;
            SKGTESTERROR(QStringLiteral("DOC.getObjects"),
                         document1.getObjects(QStringLiteral("v_operation"), QStringLiteral("d_date!='0000-00-00'"), result),
                         true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 3)
        }
    }

    {
        // Double transferts
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_WEBOOB"), err)

            SKGTESTBOOL("PUTENV.weboob", qputenv("PATH", (in + "/skgtestimportbackend/double_transferts/:" + path).toLatin1()), true)

            SKGImportExportManager imp1(&document1, QUrl(QStringLiteral(".weboob")));
            SKGTESTERROR(QStringLiteral("WEBOOB.importFile"), imp1.importFile(), true)
        }

        // Checks
        {
            SKGObjectBase::SKGListSKGObjectBase result;
            SKGTESTERROR(QStringLiteral("DOC.getObjects"),
                         document1.getObjects(QStringLiteral("v_operation"), QStringLiteral("d_date!='0000-00-00'"), result),
                         true)
            SKGTEST(QStringLiteral("DOC.getObjects.count"), result.count(), 4)
        }
    }
    // End test
    SKGENDTEST()
}
