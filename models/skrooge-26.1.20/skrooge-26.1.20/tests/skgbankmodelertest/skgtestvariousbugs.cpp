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
#include "skgdefine.h"
#include "skgimportexportmanager.h"
#include "skgimportplugin.h"
#include "skgreportbank.h"
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
    {
        // Test bug 324649
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/324649.skg"), true)

        // Check
        {
            auto *report = qobject_cast<SKGReportBank *>(document1.getReport());
            report->setPeriod(QStringLiteral("2013-09"));

            QVariantList values = report->getBankTable();
            SKGTEST(QStringLiteral("324649:getCurrentAmount"), values.count(), 4)
            SKGTEST(QStringLiteral("324649:title"), values[0].toList()[0].toString(), QStringLiteral("sum"))
            SKGTEST(QStringLiteral("324649:10"), values[1].toList()[2].toString(), QStringLiteral("20"))
            SKGTEST(QStringLiteral("324649:20"), values[2].toList()[2].toString(), QStringLiteral("10"))
            SKGTEST(QStringLiteral("324649:30"), values[3].toList()[2].toString(), QStringLiteral("30"))
        }
    }

    {
        // Test bug 324972
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/324972_1.csv"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_CSV"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/324972_2.csv"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)

            SKGDocument::SKGMessageList messages;
            SKGTESTERROR(QStringLiteral("DOC.getMessages"), document1.getMessages(document1.getCurrentTransaction(), messages, true), true)
            bool test = false;
            for (const auto &msg : std::as_const(messages)) {
                SKGTRACE << "Message:" << msg.Text << Qt::endl;
                if (msg.Text.contains(QStringLiteral("last imported one"))) {
                    test = true;
                }
            }
            SKGTEST(QStringLiteral("message.last imported one"), static_cast<unsigned int>(test), 1)
        }

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("ACCOUNT.getObjectByName"),
                     SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("My account"), account),
                     true)
        SKGTESTERROR(QStringLiteral("ACCOUNT.load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), SKGServices::doubleToString(account.getCurrentAmount()), QStringLiteral("1000"))
    }

    {
        // Test import Weboob
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/interests_and_shares.skg"),
                     true)

        // Check
        {
            SKGAccountObject act(&document1);
            SKGTESTERROR(QStringLiteral("account.setName()"), act.setName(QStringLiteral("TEST")), true)
            SKGTESTERROR(QStringLiteral("account.load()"), act.load(), true)

            SKGAccountObject::SKGInterestItemList oInterestList;
            double oInterests;
            SKGTESTERROR(QStringLiteral("account.getInterestItems()"), act.getInterestItems(oInterestList, oInterests, 2013), true)
            SKGTEST(QStringLiteral("oInterestList"), oInterestList.count(), 4)
            SKGTEST(QStringLiteral("oInterests"), static_cast<unsigned int>(oInterests > 3.20 && oInterests < 3.22), 1)

            SKGTESTERROR(QStringLiteral("account.getInterestItems()"), act.getInterestItems(oInterestList, oInterests, 2014), true)
        }
    }

    {
        // Test open invalid files
        SKGDocumentBank document1;
        SKGTEST(QStringLiteral("document1.load(interests_and_shares.skg,notneededpassword)"),
                document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/interests_and_shares.skg", QStringLiteral("notneededpassword"))
                    .getReturnCode(),
                0)
        SKGTEST(QStringLiteral("document1.load(invalid.skg)"),
                document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/invalid.skg").getReturnCode(),
                ERR_CORRUPTION)
        SKGTEST(QStringLiteral("document1.load(invalid_protected.skg)"),
                document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/invalid_protected.skg").getReturnCode(),
                ERR_ABORT) // Since 2.33.0, no more supported
        SKGTEST(QStringLiteral("document1.load(protected.skg)"),
                document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/protected.skg").getReturnCode(),
                ERR_ABORT) // Since 2.33.0, no more supported
        SKGTEST(QStringLiteral("document1.load(protected.skg, pwd)"),
                document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/protected.skg", QStringLiteral("pwd")).getReturnCode(),
                ERR_ABORT) // Since 2.33.0, no more supported

        SKGTEST(QStringLiteral("document1.load(forrecovery.skg)"),
                document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestvariousbugs/forrecovery.skg").getReturnCode(),
                ERR_CORRUPTION)
        QString recoveredFile;
        SKGTESTERROR(QStringLiteral("document1.recover(forrecovery.skg)"),
                     document1.recover(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestvariousbugs/forrecovery.skg", QString(), recoveredFile),
                     true)
        SKGTEST(QStringLiteral("document1.load(forrecovery_recovered.skg)"), document1.load(recoveredFile).getReturnCode(), 0)
    }

    {
        // Test 329568
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestvariousbugs/329568.skg"), true)

        // Check
        {
            SKGAccountObject act(&document1);
            SKGTESTERROR(QStringLiteral("account.setName()"), act.setName(QStringLiteral("T")), true)
            SKGTESTERROR(QStringLiteral("account.load()"), act.load(), true)

            SKGAccountObject::SKGInterestItemList oInterestList;
            double oInterests;
            SKGTESTERROR(QStringLiteral("account.getInterestItems()"), act.getInterestItems(oInterestList, oInterests, 2014), true)
            SKGTEST(QStringLiteral("oInterestList"), oInterestList.count(), 4)
            SKGTEST(QStringLiteral("oInterests"), static_cast<unsigned int>(oInterests > 8.20 && oInterests < 8.22), 1)
        }

        {
            auto imp1 = new SKGImportPlugin();
            SKGTESTERROR(QStringLiteral("SKGImportPlugin.importFile"), imp1->importFile(), false)
            SKGTESTERROR(QStringLiteral("SKGImportPlugin.exportFile"), imp1->exportFile(), false)
            imp1->getMimeTypeFilter();
            delete imp1;
        }
    }
    // End test
    SKGENDTEST()
}
