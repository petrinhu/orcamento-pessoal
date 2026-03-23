/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTESTDEBUGPLUGIN_H
#define SKGTESTDEBUGPLUGIN_H
/** @file
 * This file is a test for SKGDebugPlugin component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qtest.h>

/**
 * A unit test
 */
class SKGTESTDebugPlugin : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void TestPlugin();
};
#endif
