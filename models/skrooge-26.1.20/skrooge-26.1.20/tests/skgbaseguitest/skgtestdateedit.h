/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTESTDATEEDIT_H
#define SKGTESTDATEEDIT_H
/** @file
 * This file is a test for SKGDateEdit component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qtest.h>

/**
 * A unit test
 */
class SKGTESTDateEdit : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void Test();
    void Test_data();
};
#endif
