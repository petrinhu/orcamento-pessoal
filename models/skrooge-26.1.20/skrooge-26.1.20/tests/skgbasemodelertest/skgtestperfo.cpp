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

    // Test inserts
    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("INSERT"), err)

            for (int i = 0; i < 10000; ++i) {
                QString v = QString::number(i);
                SKGTESTERROR(QStringLiteral("PARAM:setParameter"), document1.setParameter(v, v), true)
                SKGTEST(QStringLiteral("PARAM:getParameter"), document1.getParameter(v), v)
            }
        }
    }
    // End test
    SKGENDTEST()
}
