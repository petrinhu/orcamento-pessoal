/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test script.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestmacro.h"

/**
 * The main function of the unit test
 * @param argc the number of arguments
 * @param argv the list of arguments
 */
int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)
    QString filenameInput1 = SKGTest::getTestPath(QStringLiteral("IN")) % "/all_plugins.skg";
    QString filenameOutput1 = SKGTest::getTestPath(QStringLiteral("OUT")) % "/all_plugins_encrypted.skg";
    QString filenameOutput2 = SKGTest::getTestPath(QStringLiteral("OUT")) % "/all_plugins_decrypted.skg";

    bool mode;
    SKGTESTERROR(QStringLiteral("DOC:cryptFile"),
                 SKGServices::cryptFile(QStringLiteral("notfound"), filenameOutput1, QStringLiteral("password"), true, QStringLiteral("SKROOGE"), mode),
                 false)
    SKGTESTERROR(QStringLiteral("DOC:cryptFile"),
                 SKGServices::cryptFile(filenameInput1, filenameOutput1, QStringLiteral("password"), true, QStringLiteral("SKROOGE"), mode),
                 true)
    SKGTESTERROR(QStringLiteral("DOC:cryptFile"),
                 SKGServices::cryptFile(filenameOutput1, filenameOutput2, QStringLiteral("password"), false, QStringLiteral("SKROOGE"), mode),
                 true)
    SKGTESTERROR(QStringLiteral("DOC:cryptFile"),
                 SKGServices::cryptFile(QStringLiteral("notfound"), filenameOutput1, QStringLiteral("password"), false, QStringLiteral("SKROOGE"), mode),
                 false)

    // End test
    SKGENDTEST()
}
