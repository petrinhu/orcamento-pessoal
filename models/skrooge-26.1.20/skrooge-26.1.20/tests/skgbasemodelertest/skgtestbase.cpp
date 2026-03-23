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
#include "skgtestmacro.h"

/**
 * The main function of the unit test
 * @param argc the number of arguments
 * @param argv the list of arguments
 */
void test_getPeriodWhereClause(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"),
            SKGServices::getPeriodWhereClause(QStringLiteral("2014"), QStringLiteral("my_date"), QStringLiteral("<")),
            QStringLiteral("(STRFTIME('%Y',my_date)<'2014' OR my_date='0000-00-00')"))
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"),
            SKGServices::getPeriodWhereClause(QStringLiteral("2014")),
            QStringLiteral("STRFTIME('%Y',d_date)='2014'"))
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"),
            SKGServices::getPeriodWhereClause(QStringLiteral("2014-02")),
            QStringLiteral("STRFTIME('%Y-%m',d_date)='2014-02'"))
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"),
            SKGServices::getPeriodWhereClause(QStringLiteral("2014-10")),
            QStringLiteral("STRFTIME('%Y-%m',d_date)='2014-10'"))
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"),
            SKGServices::getPeriodWhereClause(QStringLiteral("2014-Q2")),
            QStringLiteral("STRFTIME('%Y',d_date)||'-Q'||(CASE WHEN STRFTIME('%m',d_date)<='03' THEN '1' WHEN STRFTIME('%m',d_date)<='06' THEN '2' WHEN "
                           "STRFTIME('%m',d_date)<='09' THEN '3' ELSE '4' END)='2014-Q2'"))
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"),
            SKGServices::getPeriodWhereClause(QStringLiteral("2014-S2")),
            QStringLiteral("STRFTIME('%Y',d_date)||'-S'||(CASE WHEN STRFTIME('%m',d_date)<='06' THEN '1' ELSE '2' END)='2014-S2'"))
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"), SKGServices::getPeriodWhereClause(QStringLiteral("INVALID STRING")), QStringLiteral("1=0"))
    SKGTEST(QStringLiteral("CONV:getPeriodWhereClause"), SKGServices::getPeriodWhereClause(QStringLiteral("ALL")), QStringLiteral("1=1"))
}

void test_getNeighboringPeriod(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("2014")), QStringLiteral("2013"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-10")), QStringLiteral("2014-09"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-01")), QStringLiteral("2013-12"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-Q2")), QStringLiteral("2014-Q1"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-Q1")), QStringLiteral("2013-Q4"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-S2")), QStringLiteral("2014-S1"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-S1")), QStringLiteral("2013-S2"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("INVALID STRING")), QStringLiteral("1=0"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod"), SKGServices::getNeighboringPeriod(QStringLiteral("ALL")), QStringLiteral("1=0"))

    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014"), 1), QStringLiteral("2015"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-10"), 1), QStringLiteral("2014-11"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-01"), 1), QStringLiteral("2014-02"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-Q4"), 1), QStringLiteral("2015-Q1"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-Q2"), 1), QStringLiteral("2014-Q3"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-Q1"), 1), QStringLiteral("2014-Q2"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-S2"), 1), QStringLiteral("2015-S1"))
    SKGTEST(QStringLiteral("CONV:getNeighboringPeriod +1"), SKGServices::getNeighboringPeriod(QStringLiteral("2014-S1"), 1), QStringLiteral("2014-S2"))
}

void test_periodToDate(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014")).toString(), QDate(2014, 12, 31).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-S1")).toString(), QDate(2014, 6, 30).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-S2")).toString(), QDate(2014, 12, 31).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-Q1")).toString(), QDate(2014, 3, 31).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-Q2")).toString(), QDate(2014, 6, 30).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-Q3")).toString(), QDate(2014, 9, 30).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-Q4")).toString(), QDate(2014, 12, 31).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-01")).toString(), QDate(2014, 1, 31).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("2014-07")).toString(), QDate(2014, 7, 31).toString())
    SKGTEST(QStringLiteral("CONV:periodToDate"), SKGServices::periodToDate(QStringLiteral("ALL")).toString(), QDate::currentDate().toString())
}

void test_partialStringToDate(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("CONV:partialStringToDate"), SKGServices::partialStringToDate(QStringLiteral("INVALID"), true).toString(), QString())

    QDate currentMonth10 = QDate::currentDate();
    currentMonth10 = currentMonth10.addDays(10 - QDate::currentDate().day());
    QDate current0102(QDate::currentDate().year(), 2, 1);
    QDate current3112(QDate::currentDate().year(), 12, 31);
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("10"), true).toString(),
            (currentMonth10 <= QDate::currentDate() ? currentMonth10 : currentMonth10.addMonths(-1)).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2"), true).toString(),
            (current0102 <= QDate::currentDate() ? current0102 : current0102.addYears(-1)).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("31/12"), true).toString(),
            (current3112 <= QDate::currentDate() ? current3112 : current3112.addYears(-1)).toString())

    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/14"), true).toString(),
            QDate(2014, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/99"), true).toString(),
            QDate(1999, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/014"), true).toString(),
            QDate(2014, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/199"), true).toString(),
            QDate(1199, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/1014"), true).toString(),
            QDate(1014, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/2222"), true).toString(),
            QDate(2222, 2, 1).toString())

    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("10"), false).toString(),
            (currentMonth10 >= QDate::currentDate() ? currentMonth10 : currentMonth10.addMonths(1)).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2"), false).toString(),
            (current0102 >= QDate::currentDate() ? current0102 : current0102.addYears(1)).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/14"), false).toString(),
            QDate(2114, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/99"), false).toString(),
            QDate(2099, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/014"), false).toString(),
            QDate(3014, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/199"), false).toString(),
            QDate(2199, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/1014"), false).toString(),
            QDate(1014, 2, 1).toString())
    SKGTEST(QStringLiteral("CONV:partialStringToDate"),
            SKGServices::partialStringToDate(QStringLiteral("1/2/2222"), false).toString(),
            QDate(2222, 2, 1).toString())
}

void test_conversions(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2013, 03, 05), QStringLiteral("D")), QStringLiteral("2013-03-05"))
    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2013, 01, 01), QStringLiteral("W")), QStringLiteral("2013-W01"))
    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2013, 03, 05), QStringLiteral("W")), QStringLiteral("2013-W10"))
    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2013, 03, 05), QStringLiteral("M")), QStringLiteral("2013-03"))
    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2013, 03, 05), QStringLiteral("Q")), QStringLiteral("2013-Q1"))
    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2013, 03, 05), QStringLiteral("S")), QStringLiteral("2013-S1"))
    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2013, 03, 05), QStringLiteral("Y")), QStringLiteral("2013"))

    SKGTEST(QStringLiteral("CONV:dateToPeriod"), SKGServices::dateToPeriod(QDate(2019, 12, 31), QStringLiteral("W")), QStringLiteral("2020-W01"))

    SKGTEST(QStringLiteral("CONV:intToString"), SKGServices::intToString(10), QStringLiteral("10"))
    SKGTEST(QStringLiteral("CONV:intToString"), SKGServices::intToString(5490990004), QStringLiteral("5490990004"))
    SKGTEST(QStringLiteral("CONV:stringToInt"), SKGServices::stringToInt(QStringLiteral("56")), 56)
    SKGTEST(QStringLiteral("CONV:stringToInt"), SKGServices::stringToInt(QStringLiteral("HELLO")), 0)
    SKGTEST(QStringLiteral("CONV:stringToInt"), SKGServices::stringToInt(QStringLiteral("5HELLO")), 0)
    SKGTEST(QStringLiteral("CONV:stringToInt"), SKGServices::stringToInt(QStringLiteral("5490990004")), 5490990004)
    SKGTEST(QStringLiteral("CONV:doubleToString"), SKGServices::doubleToString(10), QStringLiteral("10"))
    SKGTEST(QStringLiteral("CONV:doubleToString"), SKGServices::doubleToString(5.3), QStringLiteral("5.3"))
    SKGTEST(QStringLiteral("CONV:doubleToString"), SKGServices::doubleToString(11111115.33), QStringLiteral("11111115.33"))
    SKGTEST(QStringLiteral("CONV:doubleToString"), SKGServices::doubleToString(119999.11), QStringLiteral("119999.11"))
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("10")) - 10, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("5.3")) - 5.3, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("11111115.33")) - 11111115.33, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("25,000.00")) - 25000.00, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("1.307,40")) - 1307.40, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("2 150,10")) - 2150.10, 0)

    QLocale::setDefault(QLocale::German);
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("107.75500000")) - 107.755, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("107.75500000")) - 107.755, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("11.000"), true), 11000)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("11.000"), false), 11.0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("241,53")), 241.53)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("1,234")), 1.234)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("1,234"), true), 1.234)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("1.234"), true), 1234)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("1.234"), false), 1.234)

    QLocale::setDefault(QLocale::C);
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("-$8.35")) + 8.35, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("8.35€")) - 8.35, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("1234.56e-02")) - 12.3456, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("31238/100")) - 312.38, 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("31238/abc")), 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("nan")), 0)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("inf")), 1e300)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("-inf")), -1e300)
    SKGTEST(QStringLiteral("CONV:stringToDouble"), SKGServices::stringToDouble(QStringLiteral("00000000194065")), 194065)
    SKGServices::timeToString(QDateTime());
    SKGTEST(QStringLiteral("CONV:stringToTime"),
            SKGServices::timeToString(SKGServices::stringToTime(QStringLiteral("1970-07-16"))),
            QStringLiteral("1970-07-16 00:00:00"))
    SKGTEST(QStringLiteral("CONV:stringToTime"),
            SKGServices::timeToString(SKGServices::stringToTime(QStringLiteral("2008-04-20"))),
            QStringLiteral("2008-04-20 00:00:00"))
    SKGTEST(QStringLiteral("CONV:stringToTime"),
            SKGServices::dateToSqlString(SKGServices::stringToTime(QStringLiteral("1970-07-16"))),
            QStringLiteral("1970-07-16"))
    SKGTEST(QStringLiteral("CONV:stringToTime"),
            SKGServices::dateToSqlString(SKGServices::stringToTime(QStringLiteral("2008-04-20"))),
            QStringLiteral("2008-04-20"))
    SKGTEST(QStringLiteral("CONV:stringToDate"),
            SKGServices::dateToSqlString(SKGServices::stringToDate(QStringLiteral("1970-07-16"))),
            QStringLiteral("1970-07-16"))
    SKGTEST(QStringLiteral("CONV:stringToDate"),
            SKGServices::dateToSqlString(SKGServices::stringToDate(QStringLiteral("2008-04-20"))),
            QStringLiteral("2008-04-20"))

    SKGTESTBOOL("CONV:SKGServices::getMicroTime", (SKGServices::getMicroTime() > 0), true)

    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("20080525"), QStringLiteral("YYYYMMDD")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("2008-05-25"), QStringLiteral("YYYY-MM-DD")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("2008-05-25 00:00:00"), QStringLiteral("YYYY-MM-DD")),
            QStringLiteral("2008-05-25"))

    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("05/25/08"), QStringLiteral("MM/DD/YY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("05/25/78"), QStringLiteral("MM/DD/YY")),
            QStringLiteral("1978-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("05-25-08"), QStringLiteral("MM-DD-YY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("05-25-78"), QStringLiteral("MM-DD-YY")),
            QStringLiteral("1978-05-25"))

    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5/25/08"), QStringLiteral("MM/DD/YY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5/25/78"), QStringLiteral("MM/DD/YY")),
            QStringLiteral("1978-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5-25-08"), QStringLiteral("MM-DD-YY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5-6-08"), QStringLiteral("MM-DD-YY")),
            QStringLiteral("2008-05-06"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5-25-78"), QStringLiteral("MM-DD-YY")),
            QStringLiteral("1978-05-25"))

    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("25/05/08"), QStringLiteral("DD/MM/YY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("25/05/78"), QStringLiteral("DD/MM/YY")),
            QStringLiteral("1978-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("25-05-08"), QStringLiteral("DD-MM-YY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("25-05-78"), QStringLiteral("DD-MM-YY")),
            QStringLiteral("1978-05-25"))

    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("25/05/2008"), QStringLiteral("DD/MM/YYYY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("25-05-2008"), QStringLiteral("DD-MM-YYYY")),
            QStringLiteral("2008-05-25"))

    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5/25/2008"), QStringLiteral("MM/DD/YYYY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5-25-2008"), QStringLiteral("MM-DD-YYYY")),
            QStringLiteral("2008-05-25"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5-6-2008"), QStringLiteral("MM-DD-YYYY")),
            QStringLiteral("2008-05-06"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("5-6-8"), QStringLiteral("MM-DD-YYYY")),
            QStringLiteral("2008-05-06"))

    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("3.21.01"), QStringLiteral("MM-DD-YY")),
            QStringLiteral("2001-03-21"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("1/ 1' 6"), QStringLiteral("DD-MM-YY")),
            QStringLiteral("2006-01-01"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("6/ 1/94"), QStringLiteral("DD-MM-YY")),
            QStringLiteral("1994-01-06"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("21/12'2001"), QStringLiteral("DD-MM-YYYY")),
            QStringLiteral("2001-12-21"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("21122001"), QStringLiteral("DDMMYYYY")),
            QStringLiteral("2001-12-21"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("12212001"), QStringLiteral("MMDDYYYY")),
            QStringLiteral("2001-12-21"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("010203"), QStringLiteral("MMDDYY")),
            QStringLiteral("2003-01-02"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("010203"), QStringLiteral("DDMMYY")),
            QStringLiteral("2003-02-01"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("3/9/04"), QStringLiteral("DD-MM-YY")),
            QStringLiteral("2004-09-03"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("31Dec2005"), QStringLiteral("DDMMMYYYY")),
            QStringLiteral("2005-12-31"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("31-Dec-2005"), QStringLiteral("DD-MMM-YYYY")),
            QStringLiteral("2005-12-31"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("31/Dec/2005"), QStringLiteral("DD/MMM/YYYY")),
            QStringLiteral("2005-12-31"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("31Dec05"), QStringLiteral("DDMMMYY")),
            QStringLiteral("2005-12-31"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("31-Dec-05"), QStringLiteral("DD-MMM-YY")),
            QStringLiteral("2005-12-31"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("31/Dec/05"), QStringLiteral("DD/MMM/YY")),
            QStringLiteral("2005-12-31"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("31DEC2005"), QStringLiteral("DDMMMYYYY")),
            QStringLiteral("2005-12-31"))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("INVALIDDATE"), QStringLiteral("DD-MM-YY")),
            SKGServices::dateToSqlString(QDateTime::currentDateTime()))
    SKGTEST(QStringLiteral("STR:dateToSqlString"),
            SKGServices::dateToSqlString(QStringLiteral("02.01.2015"), QStringLiteral("DD-MM-YYYY")),
            QStringLiteral("2015-01-02"))
}

void test_nbWorkingDays(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("SKGServices::nbWorkingDays"), SKGServices::nbWorkingDays(QDate(2010, 9, 3), QDate(2010, 9, 6)), 1)
    SKGTEST(QStringLiteral("SKGServices::nbWorkingDays"), SKGServices::nbWorkingDays(QDate(2010, 9, 6), QDate(2010, 9, 3)), 1)
    SKGTEST(QStringLiteral("SKGServices::nbWorkingDays"), SKGServices::nbWorkingDays(QDate(2010, 9, 3), QDate(2010, 9, 3)), 1)
    SKGTEST(QStringLiteral("SKGServices::nbWorkingDays"), SKGServices::nbWorkingDays(QDate(2010, 9, 1), QDate(2010, 9, 3)), 2)
    SKGTEST(QStringLiteral("SKGServices::nbWorkingDays"), SKGServices::nbWorkingDays(QDate(2010, 9, 1), QDate(2010, 9, 8)), 5)
}

void test_getnext(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("SKGServices::getNextString"), SKGServices::getNextString(QStringLiteral("12345")), QStringLiteral("12346"))
    SKGTEST(QStringLiteral("SKGServices::getNextString"), SKGServices::getNextString(QStringLiteral("9")), QStringLiteral("10"))
}

void test_errors(int &nberror, int &nbcheck, bool showonlyfailures)
{
    {
        SKGTraces::cleanProfilingStatistics();

        SKGError err;
        SKGTEST(QStringLiteral("ERR:Default RC"), err.getReturnCode(), 0)
        SKGTEST(QStringLiteral("ERR:Default RC"), err.getReturnCode(), 0)
        SKGTESTBOOL("ERR:isWarning", err.isWarning(), false)
        SKGTESTBOOL("ERR:isSucceeded", err.isSucceeded(), true)
        SKGTESTBOOL("ERR:isFailed", err.isFailed(), false)
        SKGTESTBOOL("ERR:!", !err, true)
        SKGTESTBOOL("ERR:operator bool", err, false)
        SKGTESTBOOL("ERR:getPreviousError", (err.getPreviousError() == nullptr), true)

        SKGTEST(QStringLiteral("ERR:Default message"), err.getMessage(), QString())
        err.setReturnCode(10);
        err.setMessage(QStringLiteral("Invalid parameter"));
        SKGTEST(QStringLiteral("ERR:getHistoricalSize"), err.getHistoricalSize(), 0)
        SKGTEST(QStringLiteral("ERR:RC 10"), err.getReturnCode(), 10)
        SKGTEST(QStringLiteral("ERR:MSG Invalid parameter"), err.getMessage(), QStringLiteral("Invalid parameter"))
        err.addError(11, QStringLiteral("Message 11"));
        SKGTESTBOOL("ERR:isWarning", err.isWarning(), false)
        SKGTESTBOOL("ERR:isSucceeded", err.isSucceeded(), false)
        SKGTESTBOOL("ERR:isFailed", err.isFailed(), true)
        SKGTESTBOOL("ERR:!", !err, false)
        SKGTESTBOOL("ERR:operator bool", err, true)

        SKGTEST(QStringLiteral("ERR:getHistoricalSize"), err.getHistoricalSize(), 1)
        err.addError(-12, QStringLiteral("Message 12"));
        SKGTESTBOOL("ERR:isWarning", err.isWarning(), true)
        SKGTESTBOOL("ERR:isSucceeded", err.isSucceeded(), true)
        SKGTESTBOOL("ERR:isFailed", err.isFailed(), false)
        SKGTESTBOOL("ERR:!", !err, true)
        SKGTESTBOOL("ERR:operator bool", err, false)
        SKGTEST(QStringLiteral("ERR:getHistoricalSize"), err.getHistoricalSize(), 2)
        SKGTEST(QStringLiteral("ERR:getFullMessageWithHistorical"),
                err.getFullMessageWithHistorical(),
                QStringLiteral("[WAR--12]: Message 12\n[ERR-11]: Message 11\n[ERR-10]: Invalid parameter"))
    }

    {
        SKGTraces::cleanProfilingStatistics();

        SKGError err;
        err.setReturnCode(10).setMessage(QStringLiteral("Invalid parameter")).addError(11, QStringLiteral("Message 11"));
        SKGTEST(QStringLiteral("ERR:getHistoricalSize"), err.getHistoricalSize(), 1)
        SKGTEST(QStringLiteral("ERR:RC 10"), err.getReturnCode(), 11)
        SKGTEST(QStringLiteral("ERR:MSG Message 11"), err.getMessage(), QStringLiteral("Message 11"))
        err.setProperty(QStringLiteral("ABC"));
        SKGTEST(QStringLiteral("ERR:Property ABC"), err.getProperty(), QStringLiteral("ABC"))
    }
}

void test_getDateFormat(int &nberror, int &nbcheck, bool showonlyfailures)
{
    {
        QStringList dates;
        dates << QStringLiteral("19/08/2008");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("20080819");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("YYYYMMDD"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("10141989");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("MMDDYYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("14101989");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DDMMYYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("05/08/2008") << QStringLiteral("19/08/2008");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("19/ 1' 6");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("21/12'2001");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("6/ 1/94");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("10.29.07");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("MM-DD-YY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("10.05.07");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("2001.10.25");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("YYYY-MM-DD"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("45450116");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("YYYYMMDD"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("7/14' 0") << QStringLiteral("11/30/99");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("MM-DD-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("10/ 8'10");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("7/8/06");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("7/8/06") << QStringLiteral("11/30/99") << QStringLiteral("7/14' 0");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("MM-DD-YYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("3/9/04");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("31Dec2005");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DDMMMYYYY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("31-Dec-05");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MMM-YY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("2008-05-25 00:00:00");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("YYYY-MM-DD"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("2008-05-25 01:02:03");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("YYYY-MM-DD"))
    }
    {
        QStringList dates;
        QFile inputFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/dates.txt");
        if (inputFile.open(QIODevice::ReadOnly)) {
            QTextStream in(&inputFile);
            while (!in.atEnd()) {
                QString l = in.readLine().trimmed();
                if (!l.isEmpty()) {
                    dates << l;
                }
            }

            inputFile.close();
        }
        SKGTEST(QStringLiteral("STR:dats.count"), dates.count(), 2364)
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("MM-DD-YY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("  7/8/06    ") << QString();
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QStringLiteral("DD-MM-YY"))
    }
    {
        QStringList dates;
        dates << QStringLiteral("99999999") << QStringLiteral("9999999999");
        SKGTEST(QStringLiteral("STR:getDateFormat"), SKGServices::getDateFormat(dates), QString())
    }
}

void test_csv(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGTEST(QStringLiteral("STR:stringToCsv"), SKGServices::stringToCsv(QStringLiteral("ABC")), QStringLiteral("\"ABC\""))
    SKGTEST(QStringLiteral("STR:stringToCsv"), SKGServices::stringToCsv(QStringLiteral("ABC;CDE")), QStringLiteral("\"ABC;CDE\""))
    SKGTEST(QStringLiteral("STR:stringToCsv"), SKGServices::stringToCsv(QStringLiteral("AB \"C\" DE")), QStringLiteral("\"AB \"\"C\"\" DE\""))
    SKGTEST(QStringLiteral("STR:stringToCsv"), SKGServices::stringToCsv(QStringLiteral("AB \"C\";DE")), QStringLiteral("\"AB \"\"C\"\";DE\""))

    QStringList parameters =
        SKGServices::splitCSVLine(QStringLiteral("52.33,\"9/28/2010\",52.36,231803,52.33,0.00,+0.15,-,0.00,-,0.00,0.00,0.00,0.00,\"- - "
                                                 "-\",\"-\",-,\"n\",N/A,0,+15.82,+43.33%,9,672,-1.08,-2.02%,-,-,\"51.76 - "
                                                 "52.57\",0.00,0.00,N/A,N/A,N/A,N/A,\"-\",51.91,52.18,-,\"+0.29%\",N/A,N/A,\"N/A\",N/A,\"N/A\",N/A,N/A,N/A,-,N/"
                                                 "A,\"11:35am\",N/A,211524,-,\"36.51 - 53.41\",\"- - +0.29%\",\"Paris\",N/A,\"DASSAULT SYST.\""),
                                  ',');
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 59)

    parameters = SKGServices::splitCSVLine(QStringLiteral("2013-04-02;transfer;\"a2\";'some text 2';-20,13"), ';', true);
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 5)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("2013-04-02"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("transfer"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[2], QStringLiteral("a2"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[3], QStringLiteral("some text 2"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[4], QStringLiteral("-20,13"))

    parameters = SKGServices::splitCSVLine(QStringLiteral("\"A;'B\";'A;\"B'"), ';', true);
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 2)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A;'B"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("A;\"B"))

    SKGTEST(QStringLiteral("STR:stringToHtml"), SKGServices::stringToHtml(QStringLiteral("<hello>&<world>")), QStringLiteral("&lt;hello&gt;&amp;&lt;world&gt;"))
    SKGTEST(QStringLiteral("STR:htmlToString"), SKGServices::htmlToString(QStringLiteral("&lt;hello&gt;&amp;&lt;world&gt;")), QStringLiteral("<hello>&<world>"))

    parameters = SKGServices::splitCSVLine(SKGServices::stringToCsv(QStringLiteral("Y")) % u';' % SKGServices::stringToCsv(QStringLiteral("A;B")));
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 2)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("Y"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("A;B"))

    parameters = SKGServices::splitCSVLine(QStringLiteral("A;\"B;C\";D"));
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 3)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("B;C"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[2], QStringLiteral("D"))

    parameters = SKGServices::splitCSVLine(QStringLiteral("'A';'B'"));
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 2)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("B"))

    parameters = SKGServices::splitCSVLine(QStringLiteral("\"A ' B\",\"C\""));
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 2)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A ' B"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("C"))

    parameters = SKGServices::splitCSVLine(QStringLiteral("'A \" B','C'"));
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 2)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A \" B"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("C"))

    QChar realsep;
    parameters = SKGServices::splitCSVLine(QStringLiteral("\"A\",18,\"B\""), ';', true, &realsep);
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 3)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("18"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[2], QStringLiteral("B"))
    SKGTEST(QStringLiteral("STR:sep"), realsep, ',')

    parameters = SKGServices::splitCSVLine(QStringLiteral("30/05/2008;RETRAIT ESPECES AGENCE;                                ;      100,00;DEBIT;"));
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 6)

    parameters = SKGServices::splitCSVLine(QStringLiteral("A|\"B\";\"C\""), '|', false);
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 2)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("\"B\";\"C\""))

    parameters = SKGServices::splitCSVLine(QStringLiteral("+123 \"-abc def\" \"e:f\" e:f"), ' ', true);
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 4)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("+123"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("-abc def"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[2], QStringLiteral("e:f"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[3], QStringLiteral("e:f"))

    parameters = SKGServices::splitCSVLine(QStringLiteral("A; \"B\"; \"C\" ;\"D\" ;E"), ';', true);
    SKGTEST(QStringLiteral("STR:splitCSVLine count"), parameters.count(), 5)
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[0], QStringLiteral("A"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[1], QStringLiteral("B"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[2], QStringLiteral("C"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[3], QStringLiteral("D"))
    SKGTEST(QStringLiteral("STR:splitCSVLine"), parameters[4], QStringLiteral("E"))

    SKGTEST(QStringLiteral("STR:splitCSVLine words"), SKGServices::splitCSVLine(QStringLiteral("w1 w2"), ' ', true).count(), 2)
    SKGTEST(QStringLiteral("STR:splitCSVLine words"), SKGServices::splitCSVLine(QStringLiteral("\"w1 w1\""), ' ', true).count(), 1)
    SKGTEST(QStringLiteral("STR:splitCSVLine words"), SKGServices::splitCSVLine(QStringLiteral("\"w1 w1\" w2 \"w3 w3 w3\" w4"), ' ', true).count(), 4)

    QChar realseparator;
    SKGTEST(QStringLiteral("STR:splitCSVLine comma"), SKGServices::splitCSVLine(QStringLiteral("date,payee,amount"), ';', true, &realseparator).count(), 1)
    SKGTEST(QStringLiteral("STR:splitCSVLine comma"), realseparator, ';')

    QString t(
        QStringLiteral("Export Format, Date (YYYY-MM-DD as UTC), Time (HH:MM:SS), Merchant, Txn Amount (Funding Card), Txn Currency (Funding Card), Txn Amount "
                       "(Foreign Spend), Txn Currency (Foreign Spend), Card Name, Card Last 4 Digits, Type, Category, Notes"));
    SKGTEST(QStringLiteral("STR:splitCSVLine comma"), SKGServices::splitCSVLine(t, ';', true, &realseparator).count(), 1)
    SKGTEST(QStringLiteral("STR:splitCSVLine comma"), SKGServices::splitCSVLine(t, ',', true, &realseparator).count(), 13)
}

void test_stringToSearchCriterias(int &nberror, int &nbcheck, bool showonlyfailures)
{
    SKGServices::SKGSearchCriteriaList criterias = SKGServices::stringToSearchCriterias(QStringLiteral("abc +def +ghi"));
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias count"), criterias.count(), 3)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[0].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[0].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[0], QStringLiteral("ghi"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[1].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[1].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[1].words[0], QStringLiteral("def"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[2].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[2].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[2].words[0], QStringLiteral("abc"))

    criterias = SKGServices::stringToSearchCriterias(QStringLiteral("-abc +def ghi"));
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias count"), criterias.count(), 2)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[0].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[0].words.count(), 2)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[0], QStringLiteral("def"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[1], QStringLiteral("ghi"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[1].mode, '-')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[1].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[1].words[0], QStringLiteral("abc"))

    criterias = SKGServices::stringToSearchCriterias(QStringLiteral("-abc +def ghi -10"));
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias count"), criterias.count(), 2)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[0].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[0].words.count(), 3)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[0], QStringLiteral("def"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[1], QStringLiteral("ghi"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[2], QStringLiteral("-10"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[1].mode, '-')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[1].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[1].words[0], QStringLiteral("abc"))

    criterias = SKGServices::stringToSearchCriterias(QStringLiteral("-10"));
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias count"), criterias.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[0].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[0].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[0], QStringLiteral("-10"))

    criterias = SKGServices::stringToSearchCriterias(QStringLiteral("-abc"));
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias count"), criterias.count(), 2)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[0].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[0].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[0], QString())
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[1].mode, '-')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[1].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[1].words[0], QStringLiteral("abc"))

    criterias = SKGServices::stringToSearchCriterias(QStringLiteral("\"abc def ghi\" \"123 456\""));
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias count"), criterias.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[0].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[0].words.count(), 2)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[0], QStringLiteral("abc def ghi"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[1], QStringLiteral("123 456"))

    criterias = SKGServices::stringToSearchCriterias(QStringLiteral("\"-payee:abc def : ghi\" +amount:25"));
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias count"), criterias.count(), 2)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[0].mode, '+')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[0].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[0].words[0], QStringLiteral("amount:25"))
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias mode"), criterias[1].mode, '-')
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias words count"), criterias[1].words.count(), 1)
    SKGTEST(QStringLiteral("STR:stringToSearchCriterias word"), criterias[1].words[0], QStringLiteral("payee:abc def : ghi"))

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral(
                "((lower(d_DATEOP) LIKE '%toto%' OR lower(i_number) LIKE '%toto%' OR lower(t_PAYEE) LIKE '%toto%' OR lower(t_bookmarked) LIKE '%toto%'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("i_num:ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((lower(i_number) LIKE '%toto%'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("i_num=1234")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((i_number=1234))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_PAYEE>ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((lower(t_PAYEE)>'toto'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_PAYEE<ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((lower(t_PAYEE)<'toto'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_PAYEE>=ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((lower(t_PAYEE)>='toto'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_PAYEE<=ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((lower(t_PAYEE)<='toto'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_PAYEE=ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((lower(t_PAYEE)='toto'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_PAYEE#^t[o|a]to$")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((REGEXP('^t[o|a]to$',t_PAYEE)))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("+i_number<20 +i_number>30")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((i_number>30)) OR ((i_number<20))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("d_DATEOP>2015-05-04")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("((lower(d_DATEOP)>'2015-05-04'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("notfound:ToTo")),
                                                      QStringList() << QStringLiteral("d_DATEOP") << QStringLiteral("i_number") << QStringLiteral("t_PAYEE")
                                                                    << QStringLiteral("t_bookmarked"),
                                                      nullptr),
            QStringLiteral("(1=0)"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("v1 v2 +v3 -v4 -v5")),
                                                      QStringList() << QStringLiteral("t_comment"),
                                                      nullptr),
            QStringLiteral("((lower(t_comment) LIKE '%v3%')) OR ((lower(t_comment) LIKE '%v1%') AND (lower(t_comment) LIKE '%v2%')) AND NOT((lower(t_comment) "
                           "LIKE '%v4%')) AND NOT((lower(t_comment) LIKE '%v5%'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("v1 v2 +v3 -v4 -v5")),
                                                      QStringList() << QStringLiteral("t_comment"),
                                                      nullptr,
                                                      true),
            QStringLiteral("(((t_comment:v3) or ((t_comment:v1) and (t_comment:v2))) and not (t_comment:v4)) and not (t_comment:v5)"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("-v5")),
                                                      QStringList() << QStringLiteral("t_comment"),
                                                      nullptr),
            QStringLiteral("((lower(t_comment) LIKE '%%')) AND NOT((lower(t_comment) LIKE '%v5%'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("a'b")),
                                                      QStringList() << QStringLiteral("t_comment"),
                                                      nullptr),
            QStringLiteral("((lower(t_comment) LIKE '%a''b%'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("-v5")),
                                                      QStringList() << QStringLiteral("t_comment"),
                                                      nullptr,
                                                      true),
            QStringLiteral("(t_comment:) and not (t_comment:v5)"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral(":ToTo")),
                                                      QStringList() << QStringLiteral("p_prop1") << QStringLiteral("p_prop2"),
                                                      nullptr),
            QStringLiteral("((i_PROPPNAME='prop1' AND (lower(i_PROPVALUE) LIKE '%toto%') OR i_PROPPNAME='prop2' AND (lower(i_PROPVALUE) LIKE '%toto%')))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral(":ToTo")),
                                                      QStringList() << QStringLiteral("p_prop1") << QStringLiteral("p_prop2"),
                                                      nullptr,
                                                      true),
            QStringLiteral("p_prop1:toto or p_prop2:toto"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("#ToTo")),
                                                      QStringList() << QStringLiteral("p_prop1") << QStringLiteral("p_prop2"),
                                                      nullptr),
            QStringLiteral("((i_PROPPNAME='prop1' AND REGEXP('toto',i_PROPVALUE) OR i_PROPPNAME='prop2' AND REGEXP('toto',i_PROPVALUE)))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_att>10")),
                                                      QStringList() << QStringLiteral("t_att") << QStringLiteral("t_att1"),
                                                      nullptr),
            QStringLiteral("((lower(t_att)>'10' OR lower(t_att1)>'10'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_att.>10")),
                                                      QStringList() << QStringLiteral("t_att") << QStringLiteral("t_att1"),
                                                      nullptr),
            QStringLiteral("((lower(t_att)>'10'))"));

    SKGTEST(QStringLiteral("STR:searchCriteriasToWhereClause"),
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(QStringLiteral("t_att.>10")),
                                                      QStringList() << QStringLiteral("t_att") << QStringLiteral("t_att1"),
                                                      nullptr,
                                                      true),
            QStringLiteral("t_att>10"));
}

int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)

    // Test class SKGError
    test_errors(nberror, nbcheck, showonlyfailures);

    // SKGServices
    {
        test_getPeriodWhereClause(nberror, nbcheck, showonlyfailures);

        test_getNeighboringPeriod(nberror, nbcheck, showonlyfailures);

        test_periodToDate(nberror, nbcheck, showonlyfailures);

        test_partialStringToDate(nberror, nbcheck, showonlyfailures);

        test_conversions(nberror, nbcheck, showonlyfailures);

        test_nbWorkingDays(nberror, nbcheck, showonlyfailures);

        test_getDateFormat(nberror, nbcheck, showonlyfailures);

        test_csv(nberror, nbcheck, showonlyfailures);

        test_stringToSearchCriterias(nberror, nbcheck, showonlyfailures);

        test_getnext(nberror, nbcheck, showonlyfailures);

        // Various test
        SKGStringListList table;
        table.push_back(QStringList() << QStringLiteral("Person") << QStringLiteral("Salary") << QStringLiteral("Age"));
        table.push_back(QStringList() << QStringLiteral("John") << QStringLiteral("58000") << QStringLiteral("33"));
        table.push_back(QStringList() << QStringLiteral("Paul") << QStringLiteral("42000") << QStringLiteral("25"));
        table.push_back(QStringList() << QStringLiteral("Alan") << QStringLiteral("65000") << QStringLiteral("41"));

        SKGServices::getBase100Table(table);
        SKGServices::getPercentTable(table, true, false);
        SKGServices::getPercentTable(table, false, true);
        SKGServices::getHistorizedTable(table);

        SKGTEST(QStringLiteral("STR:encodeForUrl"), SKGServices::encodeForUrl(QStringLiteral("abc")), QStringLiteral("abc"))

        QLocale::setDefault(QLocale::C);

        SKGTEST(QStringLiteral("STR:getMajorVersion"), SKGServices::getMajorVersion(QStringLiteral("4.3.12.3")), QStringLiteral("4.3"))
        SKGTEST(QStringLiteral("STR:getMajorVersion"), SKGServices::getMajorVersion(QStringLiteral("4.3.12")), QStringLiteral("4.3"))
        SKGTEST(QStringLiteral("STR:getMajorVersion"), SKGServices::getMajorVersion(QStringLiteral("4.3")), QStringLiteral("4.3"))
        SKGTEST(QStringLiteral("STR:getMajorVersion"), SKGServices::getMajorVersion(QStringLiteral("4")), QStringLiteral("4"))

        SKGTEST(QStringLiteral("STR:toCurrencyString"), SKGServices::toCurrencyString(5.12341234, QStringLiteral("F"), 2).remove(' '), QStringLiteral("5.12F"))
        SKGTEST(QStringLiteral("STR:toCurrencyString"),
                SKGServices::toCurrencyString(-5.12341234, QStringLiteral("F"), 4).remove(' '),
                QStringLiteral("-5.1234F"))

        SKGTEST(QStringLiteral("STR:toPercentageString"), SKGServices::toPercentageString(5.12341234, 2), QStringLiteral("5.12 %"))
        SKGTEST(QStringLiteral("STR:toPercentageString"), SKGServices::toPercentageString(5.12341234, 4), QStringLiteral("5.1234 %"))

        QByteArray tmp;
        SKGTESTERROR(QStringLiteral("STR:downloadToStream"), SKGServices::downloadToStream(QUrl::fromLocalFile(QStringLiteral("notfound")), tmp), false)
        SKGTESTERROR(QStringLiteral("STR:downloadToStream"),
                     SKGServices::downloadToStream(QUrl::fromUserInput(QStringLiteral("https://skrooge.org/skgtestimporturl/test.skg")), tmp),
                     true)

        SKGTEST(QStringLiteral("STR:getFullPathCommandLine"),
                SKGServices::getFullPathCommandLine(QStringLiteral("skrooge-release.py --help")),
                QStringLiteral("skrooge-release.py --help"))

        QLocale::setDefault(QLocale::French);
        SKGTEST(QStringLiteral("STR:toCurrencyString"), SKGServices::toCurrencyString(12345.63, QStringLiteral("F"), 2), QStringLiteral("12 345,63 F"))
    }

    // End test
    SKGENDTEST()
}
