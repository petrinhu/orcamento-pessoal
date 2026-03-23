/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr


 * SPDX-License-Identifier: GPL-3.0-or-later


 ***************************************************************************/
/** @file
 * This file is a test for SKGComboBox component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestcombobox.h"

#include <qtestkeyboard.h>

#include "skgcombobox.h"

void SKGTESTComboBox::Test()
{
    SKGComboBox combo;
    QVERIFY(combo.text() == QString());
    combo.setText(QStringLiteral("Hello"));
    QVERIFY(combo.text() == QStringLiteral("Hello"));

    combo.setEditable(false);
    QTest::keyClicks(&combo, QStringLiteral("ABCD"));
    QCOMPARE(combo.text(), QStringLiteral("Hello"));

    combo.setEditable(true);
    QTest::keyClicks(&combo, QStringLiteral("ABCD"));
    QCOMPARE(combo.text(), QStringLiteral("HelloABCD"));

    combo.setPalette(combo.palette());
}

QTEST_MAIN(SKGTESTComboBox)
