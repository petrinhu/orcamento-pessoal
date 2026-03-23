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
#include <qfile.h>

#include "skgdocumentbank.h"
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
        // Test load old version of files
        {
            SKGDocumentBank document1;
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestmigration/version_0.1.skg"), true)
        }
        {
            SKGDocumentBank document1;
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestmigration/version_0.3.skg"), true)
        }
        {
            SKGDocumentBank document1;
            SKGTESTERROR(QStringLiteral("DOC:load"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestmigration/version_1.12.skg"), true)
        }
    }

    // End test
    SKGENDTEST()
}
