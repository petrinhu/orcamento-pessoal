/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGDateEdit component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestdateedit.h"

#include <qtestevent.h>
#include <qtestkeyboard.h>

#include <klocalizedstring.h>

#include "skgdateedit.h"

void SKGTESTDateEdit::Test_data()
{
    KLocalizedString::setApplicationDomain("skrooge");

    QTest::addColumn<QTestEventList>("events");
    QTest::addColumn<QDate>("expected");

    // Day
    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_Up);
        list.addKeyClick(Qt::Key_Up);
        QTest::newRow("++") << list << QDate(1970, 7, 18);
    }

    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_Up);
        list.addKeyClick(Qt::Key_Down);
        QTest::newRow("+-") << list << QDate(1970, 7, 16);
    }

    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_Down);
        list.addKeyClick(Qt::Key_Down);
        QTest::newRow("--") << list << QDate(1970, 7, 14);
    }

    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_Down);
        list.addKeyClick(Qt::Key_Up);
        QTest::newRow("-+") << list << QDate(1970, 7, 16);
    }

    // Month
    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_PageUp);
        list.addKeyClick(Qt::Key_PageUp);
        QTest::newRow("++ctrl") << list << QDate(1970, 9, 16);
    }

    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_PageUp);
        list.addKeyClick(Qt::Key_PageDown);
        QTest::newRow("+-ctrl") << list << QDate(1970, 7, 16);
    }

    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_PageDown);
        list.addKeyClick(Qt::Key_PageDown);
        QTest::newRow("--ctrl") << list << QDate(1970, 5, 16);
    }

    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_PageDown);
        list.addKeyClick(Qt::Key_PageUp);
        QTest::newRow("-+ctrl") << list << QDate(1970, 7, 16);
    }

    // Today
    {
        QTestEventList list;
        list.addKeyClick(Qt::Key_Equal);
        QTest::newRow("=") << list << QDate::currentDate();
    }
}

void SKGTESTDateEdit::Test()
{
    QFETCH(QTestEventList, events);
    QFETCH(QDate, expected);

    SKGDateEdit dateEditor(nullptr);
    dateEditor.setDate(QDate(1970, 7, 16));
    events.simulate(&dateEditor);
    dateEditor.mode();

    QCOMPARE(dateEditor.date(), expected);
}

QTEST_MAIN(SKGTESTDateEdit)
