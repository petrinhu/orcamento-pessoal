/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGPeriodEdit component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestperiodedit.h"

#include "skgperiodedit.h"

void SKGTESTPeriodEdit::TestPeriods_data()
{
    QTest::addColumn<int>("period");
    QTest::addColumn<int>("interval");
    QTest::addColumn<int>("value");
    QTest::addColumn<QDate>("date");
    QTest::addColumn<QDate>("expectedBeginDate");
    QTest::addColumn<QDate>("expectedEndDate");

    QTest::newRow("CURRENT DAY") << static_cast<int>(SKGPeriodEdit::CURRENT) << static_cast<int>(SKGPeriodEdit::DAY) << 0 << QDate(2014, 12, 3)
                                 << QDate(2014, 12, 3) << QDate(2014, 12, 3);
    QTest::newRow("CURRENT WEEK") << static_cast<int>(SKGPeriodEdit::CURRENT) << static_cast<int>(SKGPeriodEdit::WEEK) << 0 << QDate(2014, 12, 3)
                                  << QDate(2014, 12, 1) << QDate(2014, 12, 7);
    QTest::newRow("CURRENT MONTH") << static_cast<int>(SKGPeriodEdit::CURRENT) << static_cast<int>(SKGPeriodEdit::MONTH) << 0 << QDate(2014, 12, 3)
                                   << QDate(2014, 12, 1) << QDate(2014, 12, 31);
    QTest::newRow("CURRENT QUARTER") << static_cast<int>(SKGPeriodEdit::CURRENT) << static_cast<int>(SKGPeriodEdit::QUARTER) << 0 << QDate(2014, 12, 3)
                                     << QDate(2014, 10, 1) << QDate(2014, 12, 31);
    QTest::newRow("CURRENT SEMESTER") << static_cast<int>(SKGPeriodEdit::CURRENT) << static_cast<int>(SKGPeriodEdit::SEMESTER) << 0 << QDate(2014, 12, 3)
                                      << QDate(2014, 7, 1) << QDate(2014, 12, 31);
    QTest::newRow("CURRENT YEAR") << static_cast<int>(SKGPeriodEdit::CURRENT) << static_cast<int>(SKGPeriodEdit::YEAR) << 0 << QDate(2014, 12, 3)
                                  << QDate(2014, 1, 1) << QDate(2014, 12, 31);

    QTest::newRow("PREVIOUS DAY") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::DAY) << 1 << QDate(2014, 12, 3)
                                  << QDate(2014, 12, 2) << QDate(2014, 12, 2);
    QTest::newRow("PREVIOUS WEEK") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::WEEK) << 1 << QDate(2014, 12, 3)
                                   << QDate(2014, 11, 24) << QDate(2014, 11, 30);
    QTest::newRow("PREVIOUS MONTH") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::MONTH) << 1 << QDate(2014, 12, 3)
                                    << QDate(2014, 11, 1) << QDate(2014, 11, 30);
    QTest::newRow("PREVIOUS QUARTER") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::QUARTER) << 1 << QDate(2014, 12, 3)
                                      << QDate(2014, 7, 1) << QDate(2014, 9, 30);
    QTest::newRow("PREVIOUS SEMESTER") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::SEMESTER) << 1 << QDate(2014, 12, 3)
                                       << QDate(2014, 1, 1) << QDate(2014, 6, 30);
    QTest::newRow("PREVIOUS YEAR") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::YEAR) << 1 << QDate(2014, 12, 3)
                                   << QDate(2013, 1, 1) << QDate(2013, 12, 31);

    QTest::newRow("LAST DAY") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::DAY) << 1 << QDate(2014, 12, 3) << QDate(2014, 12, 3)
                              << QDate(2014, 12, 3);
    QTest::newRow("LAST WEEK") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::WEEK) << 1 << QDate(2014, 12, 3)
                               << QDate(2014, 11, 27) << QDate(2014, 12, 3);
    QTest::newRow("LAST MONTH") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::MONTH) << 1 << QDate(2014, 12, 3)
                                << QDate(2014, 11, 4) << QDate(2014, 12, 3);
    QTest::newRow("LAST QUARTER") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::QUARTER) << 1 << QDate(2014, 12, 3)
                                  << QDate(2014, 9, 4) << QDate(2014, 12, 3);
    QTest::newRow("LAST SEMESTER") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::SEMESTER) << 1 << QDate(2014, 12, 3)
                                   << QDate(2014, 6, 4) << QDate(2014, 12, 3);
    QTest::newRow("LAST YEAR") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::YEAR) << 1 << QDate(2014, 12, 3)
                               << QDate(2013, 12, 4) << QDate(2014, 12, 3);

    QTest::newRow("PREVIOUS 2 DAY") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::DAY) << 2 << QDate(2014, 12, 3)
                                    << QDate(2014, 12, 1) << QDate(2014, 12, 2);
    QTest::newRow("PREVIOUS 2 WEEK") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::WEEK) << 2 << QDate(2014, 12, 3)
                                     << QDate(2014, 11, 17) << QDate(2014, 11, 30);
    QTest::newRow("PREVIOUS 2 MONTH") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::MONTH) << 2 << QDate(2014, 12, 3)
                                      << QDate(2014, 10, 1) << QDate(2014, 11, 30);
    QTest::newRow("PREVIOUS 2 QUARTER") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::QUARTER) << 2 << QDate(2014, 12, 3)
                                        << QDate(2014, 4, 1) << QDate(2014, 9, 30);
    QTest::newRow("PREVIOUS 2 SEMESTER") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::SEMESTER) << 2 << QDate(2014, 12, 3)
                                         << QDate(2013, 7, 1) << QDate(2014, 6, 30);
    QTest::newRow("PREVIOUS 2 YEAR") << static_cast<int>(SKGPeriodEdit::PREVIOUS) << static_cast<int>(SKGPeriodEdit::YEAR) << 2 << QDate(2014, 12, 3)
                                     << QDate(2012, 1, 1) << QDate(2013, 12, 31);

    QTest::newRow("LAST 2 DAY") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::DAY) << 2 << QDate(2014, 12, 3)
                                << QDate(2014, 12, 2) << QDate(2014, 12, 3);
    QTest::newRow("LAST 2 WEEK") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::WEEK) << 2 << QDate(2014, 12, 3)
                                 << QDate(2014, 11, 20) << QDate(2014, 12, 3);
    QTest::newRow("LAST 2 MONTH") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::MONTH) << 2 << QDate(2014, 12, 3)
                                  << QDate(2014, 10, 4) << QDate(2014, 12, 3);
    QTest::newRow("LAST 2 QUARTER") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::QUARTER) << 2 << QDate(2014, 12, 3)
                                    << QDate(2014, 6, 4) << QDate(2014, 12, 3);
    QTest::newRow("LAST 2 SEMESTER") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::SEMESTER) << 2 << QDate(2014, 12, 3)
                                     << QDate(2013, 12, 4) << QDate(2014, 12, 3);
    QTest::newRow("LAST 2 YEAR") << static_cast<int>(SKGPeriodEdit::LAST) << static_cast<int>(SKGPeriodEdit::YEAR) << 2 << QDate(2014, 12, 3)
                                 << QDate(2012, 12, 4) << QDate(2014, 12, 3);

    QTest::newRow("TIMELINE 12 DAY") << static_cast<int>(SKGPeriodEdit::TIMELINE) << static_cast<int>(SKGPeriodEdit::DAY) << 12 << QDate(2014, 12, 3)
                                     << QDate(2014, 12, 3) << QDate(2014, 12, 3);
    QTest::newRow("TIMELINE 12 WEEK") << static_cast<int>(SKGPeriodEdit::TIMELINE) << static_cast<int>(SKGPeriodEdit::WEEK) << 12 << QDate(2014, 12, 3)
                                      << QDate(2014, 12, 1) << QDate(2014, 12, 7);
    QTest::newRow("TIMELINE 12 MONTH") << static_cast<int>(SKGPeriodEdit::TIMELINE) << static_cast<int>(SKGPeriodEdit::MONTH) << 12 << QDate(2014, 12, 3)
                                       << QDate(2014, 12, 1) << QDate(2014, 12, 31);
    QTest::newRow("TIMELINE 12 QUARTER") << static_cast<int>(SKGPeriodEdit::TIMELINE) << static_cast<int>(SKGPeriodEdit::QUARTER) << 12 << QDate(2014, 12, 3)
                                         << QDate(2014, 10, 1) << QDate(2014, 12, 31);
    QTest::newRow("TIMELINE 12 SEMESTER") << static_cast<int>(SKGPeriodEdit::TIMELINE) << static_cast<int>(SKGPeriodEdit::SEMESTER) << 12 << QDate(2014, 12, 3)
                                          << QDate(2014, 7, 1) << QDate(2014, 12, 31);
    QTest::newRow("TIMELINE 12 YEAR") << static_cast<int>(SKGPeriodEdit::TIMELINE) << static_cast<int>(SKGPeriodEdit::YEAR) << 12 << QDate(2014, 12, 3)
                                      << QDate(2014, 1, 1) << QDate(2014, 12, 31);
}

void SKGTESTPeriodEdit::TestPeriods()
{
    QFETCH(int, period);
    QFETCH(int, interval);
    QFETCH(int, value);
    QFETCH(QDate, date);

    QFETCH(QDate, expectedBeginDate);
    QFETCH(QDate, expectedEndDate);

    QDate oBeginDate;
    QDate oEndDate;
    SKGPeriodEdit::getDates(static_cast<SKGPeriodEdit::PeriodMode>(period),
                            static_cast<SKGPeriodEdit::PeriodInterval>(interval),
                            value,
                            oBeginDate,
                            oEndDate,
                            date);

    QCOMPARE(oBeginDate, expectedBeginDate);
    QCOMPARE(oEndDate, expectedEndDate);
}

QTEST_MAIN(SKGTESTPeriodEdit)
