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
#include "skgbankincludes.h"
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

    {
        // Test restore
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("document1.load(nopwd.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/nopwd.skg", QString(), true),
                     true)
        SKGTESTERROR(QStringLiteral("document1.load(pwd.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/pwd.skg", QString(), true),
                     false)
        SKGTESTERROR(QStringLiteral("document1.load(pwd.skg, a)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/pwd.skg", QStringLiteral("a"), true),
                     false) // Since 2.33.0, no more supported

        QFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/.nopwd.skg.wrk").remove();
        SKGTESTERROR(QStringLiteral("document1.load(nopwd.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/nopwd.skg", QString(), true),
                     true)
    }

    {
        // Test load file
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("document1.load(sqlite_no_pwd.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/sqlite_no_pwd.skg", QString(), true),
                     true)
        SKGTESTERROR(QStringLiteral("document1.load(sqlite_pwd_ABC.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/sqlite_pwd_ABC.skg", QStringLiteral("ABC"), true),
                     false) // Since 2.33.0, no more supported
        SKGTESTERROR(QStringLiteral("document1.load(sqlcipher_no_pwd.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/sqlcipher_no_pwd.skg", QString(), true),
                     true)
        SKGTESTERROR(QStringLiteral("document1.load(sqlcipher_pwd_ABC.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/sqlcipher_pwd_ABC.skg", QStringLiteral("ABC"), true),
                     false) // Since 2.33.0, no more supported
    }

    {
        // Test load (sqlite mode) + save + load
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("document1.load(nopwd.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/nopwd.skg", QString(), true),
                     true)
        SKGTESTERROR(QStringLiteral("document1.saveas(nopwd2.skg)"),
                     document1.saveAs(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/nopwd2.skg", true),
                     true)
        SKGTESTERROR(QStringLiteral("document1.load(nopwd2.skg)"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestrestore/nopwd2.skg", QString(), true),
                     true)
    }

    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("DOC:load"),
                     document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % QStringLiteral("skgtestrestore/recovery.skg"), QString(), true),
                     true)
    }
    // End test
    SKGENDTEST()
}
