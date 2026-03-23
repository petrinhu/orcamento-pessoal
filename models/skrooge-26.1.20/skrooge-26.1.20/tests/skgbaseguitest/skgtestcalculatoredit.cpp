/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGCalculatorEdit component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestcalculatoredit.h"

#include <qtestevent.h>
#include <qtestkeyboard.h>

#include "skgcalculatoredit.h"

void SKGTESTCalculatorEdit::TestValueCALCULATOR_data()
{
    QTest::addColumn<QTestEventList>("events");
    QTest::addColumn<double>("expected");

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("5.1"));
        QTest::newRow("simple positive value") << list << 5.1;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-7.2"));
        QTest::newRow("simple negative value") << list << -7.2;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("10+5"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter +") << list << 15.0;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("3.5*3"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter *") << list << 10.5;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("5/2"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter /") << list << 2.5;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-5*2/4-6.2+3.1+4.5"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter complex operation") << list << -1.1;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("3,5*3"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter ,") << list << 10.5;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("3 024,25"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter 3 024,25") << list << 3024.25;
    }
}

void SKGTESTCalculatorEdit::TestValueCALCULATOR()
{
    QFETCH(QTestEventList, events);
    QFETCH(double, expected);

    SKGCalculatorEdit calculator(nullptr);
    calculator.setMode(SKGCalculatorEdit::CALCULATOR);
    events.simulate(&calculator);

    calculator.valid();
    calculator.formula();
    calculator.addParameterValue(QStringLiteral("a"), 5.1);

    QCOMPARE(calculator.value(), expected);
}

void SKGTESTCalculatorEdit::TestValueEXPRESSION_data()
{
    QTest::addColumn<QTestEventList>("events");
    QTest::addColumn<bool>("valid");
    QTest::addColumn<double>("expected");

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("abc"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("invalid expression") << list << false << 0.0;
    }
    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("1e"));
        list.addKeyClick(Qt::Key_Return);
        list.addKeyClicks(QStringLiteral("1"));
        QTest::newRow("valid expression after invalid one") << list << true << 10.0;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("5.1"));
        QTest::newRow("simple positive value") << list << true << 5.1;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-7.2"));
        QTest::newRow("simple negative value") << list << true << -7.2;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("10+5"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter +") << list << true << 15.0;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("3.5*3"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter *") << list << true << 10.5;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("5/2"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter /") << list << true << 2.5;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-5*2/4-6.2+3.1+4.5"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter complex operation") << list << true << -1.1;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("3,5*3"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter ,") << list << true << 10.5;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("3 024,25"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter 3 024,25") << list << true << 3024.25;
    }

    {
        QLocale::setDefault(QLocale::German);
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-3 024,25"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter -3 024,25") << list << true << -3024.25;
    }
}

void SKGTESTCalculatorEdit::TestValueEXPRESSION()
{
    QFETCH(QTestEventList, events);
    QFETCH(bool, valid);
    QFETCH(double, expected);

    SKGCalculatorEdit calculator(nullptr);
    calculator.setMode(SKGCalculatorEdit::EXPRESSION);
    events.simulate(&calculator);

    QCOMPARE(calculator.valid(), valid);
    QCOMPARE(calculator.value(), expected);
}

void SKGTESTCalculatorEdit::TestString_data()
{
    QTest::addColumn<QTestEventList>("events");
    QTest::addColumn<QString>("expected");
    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("3,5*3"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("enter , as string") << list << "10.5";
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("5a"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("invalid expression") << list << "5a";
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-7.2"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("simple negative value") << list << "-7.2";
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("+7.2"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("simple positive value") << list << "+7.2";
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-1.84*4-0.46*2-1.89-3.7"));
        list.addKeyClick(Qt::Key_Return);
        QTest::newRow("Bug 261318") << list << "-13.87";
    }
}

void SKGTESTCalculatorEdit::TestString()
{
    QFETCH(QTestEventList, events);
    QFETCH(QString, expected);

    SKGCalculatorEdit calculator(nullptr);
    calculator.setMode(SKGCalculatorEdit::EXPRESSION);
    events.simulate(&calculator);

    QCOMPARE(calculator.text(), expected);
}

void SKGTESTCalculatorEdit::TestSign_data()
{
    QTest::addColumn<QTestEventList>("events");
    QTest::addColumn<int>("expected");

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("5.1"));
        QTest::newRow("simple positive value") << list << 0;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("-7.2"));
        QTest::newRow("simple negative value") << list << -1;
    }

    {
        QTestEventList list;
        list.addKeyClicks(QStringLiteral("+7.2"));
        QTest::newRow("simple positive value") << list << 1;
    }
}

void SKGTESTCalculatorEdit::TestSign()
{
    QFETCH(QTestEventList, events);
    QFETCH(int, expected);

    SKGCalculatorEdit calculator(nullptr);
    calculator.setMode(SKGCalculatorEdit::CALCULATOR);
    events.simulate(&calculator);

    QCOMPARE(calculator.sign(), expected);
}

QTEST_MAIN(SKGTESTCalculatorEdit)
