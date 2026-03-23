/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTESTMAINPANEL_H
#define SKGTESTMAINPANEL_H
/** @file
 * This file is a test for SKGMainPanel component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qtest.h>

/**
 * A unit test
 */
class SKGTESTMainPanel : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void Test();
};
#endif
