/**********************"*****************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test script.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestmacro.h"
#include <qsqldatabase.h>
#include <qthread.h>
#include <quuid.h>
#include <unistd.h>

void dumpConnections()
{
    const auto conNames = QSqlDatabase::connectionNames();
    for (const auto &conName : conNames) {
        SKGTRACE << "### " << conName << " ###" << Qt::endl;
        auto con = QSqlDatabase::database(conName, false);
        SKGTRACE << "    connectionName=" << con.connectionName() << Qt::endl;
        SKGTRACE << "    connectOptions=" << con.connectOptions() << Qt::endl;
        SKGTRACE << "    isOpen=" << (con.isOpen() ? "Y" : "N") << Qt::endl;
    }
}

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

    QLocale::setDefault(QLocale::C);

    // test class SKGDocument / PARAMETERS
    SKGDocument document1;
    SKGTESTERROR(QStringLiteral("PARAM:initialize"), document1.initialize(), true)
    SKGTESTERROR(QStringLiteral("PARAM:close"), document1.close(), true)
    SKGTESTERROR(QStringLiteral("PARAM:initialize"), document1.initialize(), true)
    SKGTESTERROR(QStringLiteral("PARAM:beginTransaction"), document1.beginTransaction(QStringLiteral("t1")), true)
    SKGTESTERROR(QStringLiteral("PARAM:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")), true)
    SKGTESTERROR(QStringLiteral("PARAM:endTransaction"), document1.endTransaction(true), true)
    SKGTEST(QStringLiteral("PARAM:getCachedValue"), document1.getCachedValue(QStringLiteral("NOTFOUND")), QString())

    SKGDocument document2;
    SKGTESTERROR(QStringLiteral("PARAM:initialize"), document2.initialize(), true)
    SKGTESTERROR(QStringLiteral("PARAM:beginTransaction"), document2.beginTransaction(QStringLiteral("t2")), true)
    SKGTESTERROR(QStringLiteral("PARAM:setParameter"), document2.setParameter(QStringLiteral("ATT2"), QStringLiteral("VAL2")), true)
    SKGTESTERROR(QStringLiteral("PARAM:setParameter"),
                 document2.setParameter(QStringLiteral("ATT3"), QStringLiteral("dates.txt"), SKGTest::getTestPath(QStringLiteral("IN")) % "/dates.txt"),
                 true)
    SKGTESTERROR(QStringLiteral("PARAM:endTransaction"), document2.endTransaction(true), true)

    SKGTEST(QStringLiteral("PARAM:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
    SKGTEST(QStringLiteral("PARAM:getParameter"), document2.getParameter(QStringLiteral("ATT2")), QStringLiteral("VAL2"))

    SKGTEST(QStringLiteral("PARAM:getFileExtension"), document2.getFileExtension(), QStringLiteral("skgc"))

    document1.formatPercentage(1.1, true);
    document1.formatPercentage(1.1, false);
    SKGTEST(QStringLiteral("PARAM:getRealAttribute"), document2.getRealAttribute(QStringLiteral("t_ATT")), QString())
    SKGTEST(QStringLiteral("PARAM:getRealAttribute"), document2.getRealAttribute(QStringLiteral("t_att")), QStringLiteral("t_att"))

    document1.getDatabaseIdentifier();
    document1.getParameters(QStringLiteral("document"), QStringLiteral("t_name like 'ATT%'"));

    // Special SQL command
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSqliteOrder(
                     QStringLiteral("SELECT * FROM (SELECT CAPITALIZE(LOWER(UPPER(WORD('Abc Def', 2)))) AS V) WHERE REGEXP('D.*', V) AND WILDCARD('D*', V)")),
                 true)
    QString result;
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT TOWEEKYEAR('2019-12-31')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-TOWEEKYEAR"), result, QStringLiteral("2020-W01"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT TODATE('07162013', 'MMDDYYYY')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-TODATE"), result, QStringLiteral("2013-07-16"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT TOFORMATTEDDATE('2013-07-16', 'dd-MM-yyyy')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-TODATE"), result, QStringLiteral("16-07-2013"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT TOFORMATTEDDATE('2013-07-16', 'd M yy')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-TODATE"), result, QStringLiteral("16 7 13"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT TODATE('ABCDEFGHIJ', 'MMDDYYYY')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-TODATE"), result, SKGServices::dateToSqlString(QDate::currentDate()))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', 0)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Abc"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', 1)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Abc"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', 2)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Def"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', 3)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Ghi"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', 99)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Ghi"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', -99)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Abc"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', -3)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Abc"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', -2)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Def"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('Abc Def Ghi', -1)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Ghi"))

    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('   Abc    Def   Ghi ', 1)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Abc"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('   Abc    Def   Ghi ', 2)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Def"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('   Abc    Def   Ghi ', 3)"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("Ghi"))

    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"), document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('N:1234', 1)"), result), true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("N"))

    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"), document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT WORD('N:1234', 2)"), result), true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("1234"))

    SKGTESTERROR(
        QStringLiteral("PARAM:executeSqliteOrder"),
        document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT REGEXPCAPTURE('(.*) CARTE 1234.*', 'MyShopName CARTE 12345678 PAIEME', 0)"), result),
        true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("MyShopName CARTE 12345678 PAIEME"))

    SKGTESTERROR(
        QStringLiteral("PARAM:executeSqliteOrder"),
        document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT REGEXPCAPTURE('(.*) CARTE 1234.*', 'MyShopName CARTE 12345678 PAIEME', 1)"), result),
        true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QStringLiteral("MyShopName"))

    SKGTESTERROR(
        QStringLiteral("PARAM:executeSqliteOrder"),
        document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT REGEXPCAPTURE('(.*) CARTE 1234.*', 'MyShopName CARTE 12345678 PAIEME', 12)"), result),
        true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-WORD"), result, QString())

    QMap<QString, QVariant> map;
    map[QStringLiteral(":2")] = "20";
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"), document1.executeSqliteOrder(QStringLiteral("SELECT WORD('Abc Def', :2)"), map, nullptr), true)

    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT TOCURRENCY(1234, 'F')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-1234 F"), result.remove(' '), QStringLiteral("1234.00F"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT TOCURRENCY(-1234, (SELECT 'F'))"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-1234 F"), result.remove(' '), QStringLiteral("-1234.00F"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD((SELECT '2013-03-05'), (SELECT 'D'))"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD D"), result, QStringLiteral("2013-03-05"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2013-03-05', 'W')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD W"), result, QStringLiteral("2013-W10"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2013-03-05', 'M')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD M"), result, QStringLiteral("2013-03"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2013-03-05', 'Q')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD Q"), result, QStringLiteral("2013-Q1"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2013-03-05', 'S')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD S"), result, QStringLiteral("2013-S1"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2013-03-05', 'Y')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD Y"), result, QStringLiteral("2013"))

    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2014-07-16', 'D')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD D"), result, QStringLiteral("2014-07-16"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2014-07-16', 'W')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD W"), result, QStringLiteral("2014-W29"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2014-07-16', 'M')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD M"), result, QStringLiteral("2014-07"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2014-07-16', 'Q')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD Q"), result, QStringLiteral("2014-Q3"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2014-07-16', 'S')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD S"), result, QStringLiteral("2014-S2"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT PERIOD('2014-07-16', 'Y')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-PERIOD Y"), result, QStringLiteral("2014"))

    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"), document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT NEXT('12345')"), result), true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-NEXT"), result, QStringLiteral("12346"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"), document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT NEXT('9')"), result), true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-NEXT"), result, QStringLiteral("10"))
    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"), document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT NEXT('ABC')"), result), true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-NEXT"), result, QString())

    SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                 document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT XOR('HELLO WORLD!', 'KEY')"), result),
                 true)
    SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-XOR"), result, QStringLiteral("# 030015070a791c0a0b070178"))
    for (int i = 1; i < 100; ++i) {
        auto string = QUuid::createUuid().toString();
        SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                     document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT XOR(XOR('") + string + "', 'KEY'), 'KEY')", result),
                     true)
        SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-XOR"), result, string)
    }
    for (double i = -1200.53; i < 5023.25; i = i + 125.54) {
        auto string = SKGServices::doubleToString(i);
        SKGTESTERROR(QStringLiteral("PARAM:executeSqliteOrder"),
                     document1.executeSingleSelectSqliteOrder(QStringLiteral("SELECT XORD(XORD(") + string + ", 'KEY'), 'KEY')", result),
                     true)
        SKGTEST(QStringLiteral("PARAM:executeSqliteOrder-XORD"), result, string)
    }

    SKGTRACE << "####### Before concurrent calls" << Qt::endl;
    dumpConnections();
    int nb = 5;
    {
        SKGTRACE << ">> executeSelectSqliteOrder same order" << Qt::endl;
        QString output;
        double elapse = SKGServices::getMicroTime();
        for (int i = 0; i < nb; ++i) {
            SKGStringListList oResult;
            IFOK(document1.executeSelectSqliteOrder(QStringLiteral("SELECT SLEEP(1)"), oResult))
            {
                output = output + SKGServices::intToString(oResult.count());
            }
        }
        double time = SKGServices::getMicroTime() - elapse;
        SKGTRACE << nb << " x executeSelectSqliteOrder:" << output << "     " << time << " ms" << Qt::endl;
        SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO >=1000"), static_cast<unsigned int>(time >= 1000), static_cast<unsigned int>(true))
        SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO <2000"), static_cast<unsigned int>(time < 2000), static_cast<unsigned int>(true))
        SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder"), output, QStringLiteral("22222"))
    }

    {
        SKGTRACE << ">> concurrentExecuteSelectSqliteOrder same order" << Qt::endl;
        QString output;
        double elapse = SKGServices::getMicroTime();
        for (int i = 0; i < nb; ++i) {
            document1.concurrentExecuteSelectSqliteOrder(QStringLiteral("SELECT SLEEP(1), 2"), [&output](const SKGStringListList &iResult) {
                output = output + SKGServices::intToString(iResult.count());
            });
        }
        double time = SKGServices::getMicroTime() - elapse;
        SKGTRACE << nb << " x concurrentExecuteSelectSqliteOrder:" << output << "     " << (SKGServices::getMicroTime() - elapse) << " ms" << Qt::endl;
        qApp->processEvents(QEventLoop::AllEvents, 500);
        for (int i = 1; i < 100; ++i) {
            QThread::msleep(100);
            qApp->processEvents(QEventLoop::AllEvents, 500);
            time = SKGServices::getMicroTime() - elapse;
            if (output == QStringLiteral("22222")) {
                SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO >=1000"), static_cast<unsigned int>(time >= 1000), static_cast<unsigned int>(true))
                SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO <3500"), static_cast<unsigned int>(time < 3500), static_cast<unsigned int>(true))
                break;
            }
        }

        SKGTRACE << nb << " x concurrentExecuteSelectSqliteOrder:" << output << "     " << time << " ms" << Qt::endl;
        SKGTEST(QStringLiteral("PARAM:concurrentExecuteSelectSqliteOrder"), output, QStringLiteral("22222"))
    }

    {
        SKGTRACE << ">> executeSelectSqliteOrder different orders" << Qt::endl;
        QString output;
        double elapse = SKGServices::getMicroTime();
        for (int i = 0; i < nb; ++i) {
            SKGStringListList oResult;
            IFOK(document1.executeSelectSqliteOrder(QStringLiteral("SELECT SLEEP(1), ") + SKGServices::intToString(1000 + i), oResult))
            {
                output = output + SKGServices::intToString(oResult.count());
            }
        }
        double time = SKGServices::getMicroTime() - elapse;
        SKGTRACE << nb << " x executeSelectSqliteOrder:" << output << "     " << time << " ms" << Qt::endl;
        SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO >5000"), static_cast<unsigned int>(time > 5000), static_cast<unsigned int>(true))
        SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO <6000"), static_cast<unsigned int>(time < 6000), static_cast<unsigned int>(true))
        SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder"), output, QStringLiteral("22222"))
    }

    {
        SKGTRACE << ">> concurrentExecuteSelectSqliteOrder different orders" << Qt::endl;
        QString output;
        double elapse = SKGServices::getMicroTime();
        for (int i = 0; i < nb; ++i) {
            document1.concurrentExecuteSelectSqliteOrder(QStringLiteral("SELECT SLEEP(1), ") + SKGServices::intToString(2000 + i),
                                                         [&output](const SKGStringListList &iResult) {
                                                             output = output + SKGServices::intToString(iResult.count());
                                                         });
        }
        double time = SKGServices::getMicroTime() - elapse;
        SKGTRACE << nb << " x concurrentExecuteSelectSqliteOrder:" << output << "     " << (SKGServices::getMicroTime() - elapse) << " ms" << Qt::endl;
        qApp->processEvents(QEventLoop::AllEvents, 500);
        for (int i = 1; i < 100; ++i) {
            QThread::msleep(100);
            qApp->processEvents(QEventLoop::AllEvents, 500);
            time = SKGServices::getMicroTime() - elapse;
            if (output == QStringLiteral("22222")) {
                SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO >=1000"), static_cast<unsigned int>(time >= 1000), static_cast<unsigned int>(true))
                SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO <3500"), static_cast<unsigned int>(time < 3500), static_cast<unsigned int>(true))
                break;
            }
        }

        SKGTRACE << nb << " x concurrentExecuteSelectSqliteOrder:" << output << "     " << time << " ms" << Qt::endl;
        SKGTEST(QStringLiteral("PARAM:concurrentExecuteSelectSqliteOrder"), output, QStringLiteral("22222"))
    }
    SKGTRACE << "####### Before close" << Qt::endl;
    dumpConnections();
    SKGTESTERROR(QStringLiteral("PARAM:close"), document1.close(), true)
    SKGTRACE << "####### Before initialize" << Qt::endl;
    dumpConnections();
    SKGTESTERROR(QStringLiteral("PARAM:initialize"), document1.initialize(), true)
    SKGTRACE << "####### Before concurrent calls" << Qt::endl;
    dumpConnections();

    {
        SKGTRACE << ">> concurrentExecuteSelectSqliteOrder different orders (in other thread)" << Qt::endl;
        QString output;
        double elapse = SKGServices::getMicroTime();
        for (int i = 0; i < nb; ++i) {
            document1.concurrentExecuteSelectSqliteOrder(
                QStringLiteral("SELECT SLEEP(1), ") + SKGServices::intToString(3000 + i),
                [&output](const SKGStringListList &iResult) {
                    QMutex mutex;
                    mutex.lock();
                    output = output + SKGServices::intToString(iResult.count());
                    mutex.unlock();
                },
                false);
        }
        double time = SKGServices::getMicroTime() - elapse;
        SKGTRACE << nb << " x concurrentExecuteSelectSqliteOrder:" << output << "     " << (SKGServices::getMicroTime() - elapse) << " ms" << Qt::endl;
        for (int i = 1; i < 100; ++i) {
            QThread::msleep(100);
            time = SKGServices::getMicroTime() - elapse;
            if (output == QStringLiteral("22222")) {
                SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO >=1000"), static_cast<unsigned int>(time >= 1000), static_cast<unsigned int>(true))
                SKGTEST(QStringLiteral("PARAM:executeSelectSqliteOrder-PERFO <3500"), static_cast<unsigned int>(time < 3500), static_cast<unsigned int>(true))
                break;
            }
        }

        SKGTRACE << nb << " x concurrentExecuteSelectSqliteOrder:" << output << "     " << time << " ms" << Qt::endl;
        SKGTEST(QStringLiteral("PARAM:concurrentExecuteSelectSqliteOrder"), output, QStringLiteral("22222"))
    }
    SKGTRACE << "####### End" << Qt::endl;
    dumpConnections();

    // End test
    SKGENDTEST()
}
