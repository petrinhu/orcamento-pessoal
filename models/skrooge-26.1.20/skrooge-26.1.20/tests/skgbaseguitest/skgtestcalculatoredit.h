/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTESTCALCULATOREDIT_H
#define SKGTESTCALCULATOREDIT_H
/** @file
 * This file is a test for SKGCalculatorEdit component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qtest.h>

/**
 * A unit test
 */
class SKGTESTCalculatorEdit : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void TestValueCALCULATOR();
    void TestValueCALCULATOR_data();

    void TestValueEXPRESSION();
    void TestValueEXPRESSION_data();

    void TestString();
    void TestString_data();

    void TestSign();
    void TestSign_data();
};
#endif
