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
        // Test import MNY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)
            SKGImportExportManager impmissing(&document1, QUrl::fromLocalFile(QStringLiteral("/not-existing/missingfile.mny")));
            {
                QMap<QString, QString> params = impmissing.getImportParameters();
                params[QStringLiteral("install_sunriise")] = 'Y';
                impmissing.setImportParameters(params);
            }
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impmissing.importFile(), false)

            SKGImportExportManager impwrong(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny1/wrongfile.mny"));
            {
                QMap<QString, QString> params = impwrong.getImportParameters();
                params[QStringLiteral("install_sunriise")] = 'Y';
                impwrong.setImportParameters(params);
            }
            SKGTESTERROR(QStringLiteral("imp1.importFile"), impwrong.importFile(), false)

            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny1/money2004-pwd-123@ABC!.mny"));
            {
                QMap<QString, QString> params = imp1.getImportParameters();
                params[QStringLiteral("password")] = QStringLiteral("wrong password");
                params[QStringLiteral("install_sunriise")] = 'Y';
                imp1.setImportParameters(params);
            }
            SKGTEST(QStringLiteral("imp1.importFile"), imp1.importFile().getReturnCode(), ERR_ENCRYPTION)
        }
    }

    QVector<test> listTests;
    {
        test t1;
        t1.fileName = QStringLiteral("A B/money2002.mny");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("None Investment (Cash)")] = 0.0;
        accounts[QStringLiteral("Investments to Watch")] = 0.0;
        accounts[QStringLiteral("None Investment")] = 1.49;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("money2001.mny");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("Investments to Watch")] = 0.0;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("money2004-pwd-123@ABC!.mny");
        t1.password = QStringLiteral("123@ABC!");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("Investments to Watch")] = 0.0;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("money2005-pwd-123@ABC!.mny");
        t1.password = QStringLiteral("123@ABC!");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("George's Pension Plan")] = 250.0;
        accounts[QStringLiteral("Investments to Watch")] = 0.0;
        accounts[QStringLiteral("Invoice Sales Ledger")] = 1780.71;
        accounts[QStringLiteral("Stocks and Shares")] = 805.5;
        accounts[QStringLiteral("Stocks and Shares (Cash)")] = -178.0;
        accounts[QStringLiteral("Woodgrove Bank Credit Card")] = 1013.0;
        accounts[QStringLiteral("Woodgrove Bank Current")] = 20280.14;
        accounts[QStringLiteral("Woodgrove Bank Savings")] = 800.0;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }
    {
        test t1;
        t1.fileName = QStringLiteral("money2008-pwd-TEST12345.mny");
        t1.password = QStringLiteral("TEST12345");

        QMap<QString, double> accounts;
        accounts[QStringLiteral("Investments to Watch")] = 0.0;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }

    for (const auto &t : std::as_const(listTests)) {
        // Test import MNY
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny1/" % t.fileName));
            QMap<QString, QString> params = imp1.getImportParameters();
            params[QStringLiteral("password")] = t.password;
            params[QStringLiteral("install_sunriise")] = 'Y';
            imp1.setImportParameters(params);
            SKGTESTERROR(t.fileName % ".importFile", imp1.importFile(), true)
        }

        QStringList keys = t.expectedAccountAmount.keys();
        for (const auto &k : std::as_const(keys)) {
            SKGAccountObject account(&document1);
            SKGTESTERROR(t.fileName % ".setName(QStringLiteral(" % k % "))", account.setName(k), true)
            SKGTESTERROR(t.fileName % ".load(QStringLiteral(" % k % "))", account.load(), true)
            SKGTEST(t.fileName % ".getCurrentAmount(" % k % ")",
                    SKGServices::doubleToString(account.getCurrentAmount()),
                    SKGServices::doubleToString(t.expectedAccountAmount[k]))
        }

        if (t.fileName == QStringLiteral("money2005-pwd-123@ABC!.mny")) {
            bool check = false;
            SKGTESTERROR(t.fileName % ".existObjects", document1.existObjects(QStringLiteral("v_operation"), QStringLiteral("t_TRANSFER='Y'"), check), true)
            SKGTESTBOOL(t.fileName % ".existObjects", check, true)
        }
    }
    // End test
    SKGENDTEST()
}
