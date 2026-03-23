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
#include <klocalizedstring.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgtestmacro.h"
#include <utility>

struct test {
    QString fileName;
    QString password;
    QMap<QString, double> expectedAccountAmount;
};

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
        // Test export ledger
        SKGDocumentBank document1;
        SKGError err;
        SKGTESTERROR(QStringLiteral("DOC.load"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportskg/all_types.skg"), true)

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_LEDGE"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestimportledger/test-obfuscated.ledger"));
            SKGTESTERROR(QStringLiteral("LEDGE.exportFile"), imp1.exportFile(), true)
        }
    }

    QVector<test> listTests;
    {
        test t1;
        t1.fileName = QStringLiteral("demo.ledger");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("MasterCard")] = -20;
        accounts[QStringLiteral("Checking")] = -4124;
        accounts[QStringLiteral("Savings")] = -5200;
        accounts[QStringLiteral("Mortgage:Principal")] = -800; // Expected : 200
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("drewr.ledger");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("Checking")] = 1366;
        accounts[QStringLiteral("Checking:Business")] = 30;
        accounts[QStringLiteral("Savings")] = -5200;
        accounts[QStringLiteral("MasterCard")] = -20;
        accounts[QStringLiteral("Mortgage:Principal")] = -800; // Expected : 200
        accounts[QStringLiteral("Tithe")] = -243.6;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("sample.ledger");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("Bank:Checking")] = 1480;
        accounts[QStringLiteral("Brokerage")] = 50;
        accounts[QStringLiteral("MasterCard")] = -70;
        accounts[QStringLiteral("Taxes")] = -2;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("sebastien.ledger");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("CMB:Compte Cheques")] = 3313.19;
        accounts[QStringLiteral("CMB:LIVRET A de Mme")] = 5650.00;
        accounts[QStringLiteral("CMB:LIVRET A de Mr")] = 5650.00;
        accounts[QStringLiteral("Soldes d'Ouvertures")] = -12000.00;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("transfer.ledger");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("Payable:hcoop.net")] = -72355001;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("wow.ledger");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("Tajer")] = 1973592.662; // Expected : "Orb of Deception" 1
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    for (const auto &t : std::as_const(listTests)) {
        // Test import ledger
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("EXPORT_LEDGER"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportledger/" % t.fileName));
            SKGTESTERROR(t.fileName % QStringLiteral(".importFile"), imp1.importFile(), true)
        }

        // document1.dump(DUMPOPERATION | DUMPACCOUNT | DUMPUNIT | DUMPPAYEE | DUMPCATEGORY);

        QStringList keys = t.expectedAccountAmount.keys();
        for (const auto &k : std::as_const(keys)) {
            SKGAccountObject account;
            SKGTESTERROR(t.fileName % QStringLiteral(".getObjectByName(") % k % ")",
                         SKGNamedObject::getObjectByName(&document1, QStringLiteral("v_account"), k, account),
                         true)
            SKGTEST(t.fileName % "." % k % QStringLiteral(".getCurrentAmount"),
                    SKGServices::toCurrencyString(account.getCurrentAmount(), QString(), 2),
                    SKGServices::toCurrencyString(t.expectedAccountAmount[k], QString(), 2))
        }
    }
    // End test
    SKGENDTEST()
}
