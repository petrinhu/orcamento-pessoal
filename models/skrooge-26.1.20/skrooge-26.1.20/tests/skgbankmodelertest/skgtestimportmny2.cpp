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

class SKGTestImportMny2
{
public:
    /**
     * To check the progress
     */
    static QString previousProgress;

    /**
     * To test progress
     * @param iPos the current position
     * @return 0
     */
    static int progress1(int iPos, qint64 iTime, const QString &iName, void * /*iData*/)
    {
        if (SKGTestImportMny2::previousProgress != iName) {
            SKGTRACE << iPos << "-" << iTime << ":" << iName << Qt::endl;
            SKGTestImportMny2::previousProgress = iName;
        }
        return 0;
    }
};

QString SKGTestImportMny2::previousProgress = QString();

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

    QVector<test> listTests;
    {
        test t1;
        t1.fileName = QStringLiteral("sunset-sample-5-pwd-12@a!.mny");
        t1.password = QStringLiteral("12@a!");

        QMap<QString, double> accounts;
        // TODO(Stephane MANKOWSKI): accounts["Charlie & May’s Joint Inv (Cash)"] = -10004.15;
        accounts[QStringLiteral("Charlie's 401(k)")] = 24749.18;
        accounts[QStringLiteral("Charlie's 401(k) (Contributions)")] = 13192.68;
        accounts[QStringLiteral("Commodities")] = 8745.0000;
        accounts[QStringLiteral("Commodities (Cash)")] = 255.0;
        accounts[QStringLiteral("ETF Brokerage Account")] = 1560.0;
        accounts[QStringLiteral("ETF Brokerage Account (Cash)")] = 2310.0;
        accounts[QStringLiteral("Escrow Account")] = 28100.0;
        accounts[QStringLiteral("Home Loan")] = -149122.08;
        accounts[QStringLiteral("Investments to Watch")] = 110.25;
        accounts[QStringLiteral("Previous card (No longer used)")] = -984.25;
        accounts[QStringLiteral("Primary Residence")] = 355000.0;
        accounts[QStringLiteral("WoodGrove Finance Stock Options")] = 45000;
        accounts[QStringLiteral("WoodGrove Finance Stock Options (Cash)")] = 0.0;
        accounts[QStringLiteral("Woodgrove Bank Checking")] = 22871.06;
        accounts[QStringLiteral("Woodgrove Bank Credit Card")] = 19305.74;
        accounts[QStringLiteral("Woodgrove Bank Savings")] = 22946.30;
        accounts[QStringLiteral("Woodgrove Bond Account")] = 62160.85;
        accounts[QStringLiteral("Woodgrove Bond Account (Cash)")] = -57916.20;
        accounts[QStringLiteral("Woodgrove Investments")] = 254176.78;
        accounts[QStringLiteral("Woodgrove Investments (Cash)")] = -1594.62;
        accounts[QStringLiteral("Woodgrove Platinum Card")] = -836.0;
        t1.expectedAccountAmount = accounts;
        listTests << t1;
    }

    for (const auto &t : std::as_const(listTests)) {
        // Test import MNY
        SKGTRACE << "Filename:" << t.fileName << Qt::endl;
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGTRACE << "SKG_SQLITE_LAST_VERSION:" << document1.getParameter(QStringLiteral("SKG_SQLITE_LAST_VERSION")) << Qt::endl;
        SKGTESTERROR(QStringLiteral("document1.setProgressCallback"), document1.setProgressCallback(&SKGTestImportMny2::progress1, nullptr), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_MNY"), err)

            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportmny2/" % t.fileName));
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
    }
    // End test
    SKGENDTEST()
}
