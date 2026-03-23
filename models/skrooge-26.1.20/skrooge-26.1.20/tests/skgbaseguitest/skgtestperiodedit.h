/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef skgtestperiodedit_h
#define skgtestperiodedit_h
/** @file
 * This file is a test for SKGPeriodEdit component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qtest.h>

/**
 * A unit test
 */
class SKGTESTPeriodEdit : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void TestPeriods();
    void TestPeriods_data();
};
#endif
