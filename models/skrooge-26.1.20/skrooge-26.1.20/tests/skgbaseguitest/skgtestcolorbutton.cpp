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
#include "skgtestcolorbutton.h"
#include "skgcolorbutton.h"

void SKGTESTColorButton::Test()
{
    SKGColorButton color(nullptr);
    color.setText(QStringLiteral("Hello"));
    QCOMPARE(color.text(), QStringLiteral("Hello"));
    color.setColor(Qt::white);
    QCOMPARE(color.color(), QColor(Qt::white));
    color.setDefaultColor(Qt::black);
    QCOMPARE(color.defaultColor(), QColor(Qt::black));
}

QTEST_MAIN(SKGTESTColorButton)
