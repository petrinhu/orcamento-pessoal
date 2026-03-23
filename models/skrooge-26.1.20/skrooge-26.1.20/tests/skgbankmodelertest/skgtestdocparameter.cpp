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
#include <utility>

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

    // test class SKGDocument / PARAMETERS
    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("PARAM:initialize"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("PARAM:beginTransaction"), document1.beginTransaction(QStringLiteral("t1")), true)
        SKGTESTERROR(QStringLiteral("PARAM:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")), true)
        SKGTEST(QStringLiteral("PARAM:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
        SKGTESTERROR(QStringLiteral("PARAM:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1UPDATED")), true)
        SKGTEST(QStringLiteral("PARAM:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1UPDATED"))
        SKGTESTERROR(QStringLiteral("PARAM:setParameter"), document1.setParameter(QStringLiteral("ATT2"), QStringLiteral("VAL2")), true)
        SKGTEST(QStringLiteral("PARAM:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1UPDATED"))
        SKGTEST(QStringLiteral("PARAM:getParameter"), document1.getParameter(QStringLiteral("ATT2")), QStringLiteral("VAL2"))
        SKGTEST(QStringLiteral("PARAM:getParameter"), document1.getParameter(QStringLiteral("NOTFOUND")), QString())
        SKGTESTERROR(QStringLiteral("PARAM:setParameter+sql injection"), document1.setParameter(QStringLiteral("'"), QStringLiteral("VAL3")), true)
        SKGTEST(QStringLiteral("PARAM:getParameter+sql injection"), document1.getParameter(QStringLiteral("'")), QStringLiteral("VAL3"))
        SKGTESTERROR(QStringLiteral("PARAM:endTransaction"), document1.endTransaction(true), true)
    }

    // Test parameters on object
    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("PROP:initialize"), document1.initialize(), true)
        SKGTESTERROR(QStringLiteral("PROP:beginTransaction"), document1.beginTransaction(QStringLiteral("t1")), true)
        SKGTESTERROR(QStringLiteral("PROP:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")), true)

        SKGObjectBase obj1(&document1, QStringLiteral("bank"));
        SKGTESTERROR(QStringLiteral("PROP:setAttribute"), obj1.setAttribute(QStringLiteral("t_name"), QStringLiteral("CL")), true)
        SKGTESTERROR(QStringLiteral("PROP:Replace"), obj1.save(), true)

        SKGTESTERROR(QStringLiteral("PROP:setProperty"), obj1.setProperty(QStringLiteral("ATT1"), QStringLiteral("VAL2"), QVariant(145)), true)

        // Check
        SKGTEST(QStringLiteral("PROP:getParameter"), document1.getParameter(QStringLiteral("ATT1")), QStringLiteral("VAL1"))
        SKGTEST(QStringLiteral("PROP:getProperty"), obj1.getProperty(QStringLiteral("ATT1")), QStringLiteral("VAL2"))
        SKGTEST(QStringLiteral("PROP:getProperty"), obj1.getPropertyBlob(QStringLiteral("ATT1")).toInt(), 145)

        QStringList oResult;
        SKGTESTERROR(QStringLiteral("PROP:getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("parameters"), QStringLiteral("t_value"), QStringLiteral("t_value like 'VAL%'"), oResult),
                     true)
        SKGTEST(QStringLiteral("PROP:oResult.size"), oResult.size(), 2)

        // delete cascade
        SKGTESTERROR(QStringLiteral("PROP:Replace"), obj1.remove(), true)
        SKGTESTERROR(QStringLiteral("PROP:getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("parameters"), QStringLiteral("t_value"), QStringLiteral("t_value like 'VAL%'"), oResult),
                     true)
        SKGTEST(QStringLiteral("PROP:oResult.size"), oResult.size(), 1)

        SKGTESTERROR(QStringLiteral("PROP:setProperty"),
                     obj1.setProperty(QStringLiteral("ATT4"), QStringLiteral("VAL4"), SKGTest::getTestPath(QStringLiteral("IN")) % "/dates.txt"),
                     true)
        SKGTESTERROR(QStringLiteral("PROP:setProperty"),
                     obj1.setProperty(QStringLiteral("ATT5"), QStringLiteral("VAL5"), SKGTest::getTestPath(QStringLiteral("IN"))),
                     true)

        {
            // Scope of the transaction
            SKGError err;
            SKGBEGINTRANSACTION(document1, QStringLiteral("T1"), err)

            SKGPropertyObject propAdded1;
            SKGTESTERROR(QStringLiteral("PROP.setProperty"), obj1.setProperty(QStringLiteral("normal"), QStringLiteral("value"), QVariant(), &propAdded1), true)
            SKGTEST(QStringLiteral("PROP.getUrl"), propAdded1.getUrl().toDisplayString(), QString())

            SKGPropertyObject propAdded2;
            SKGTESTERROR(QStringLiteral("PROP.setProperty"),
                         obj1.setProperty(QStringLiteral("url"), QStringLiteral("https://skrooge.org/"), QVariant(), &propAdded2),
                         true)
            SKGTEST(QStringLiteral("PROP.getUrl"), propAdded2.getUrl().toDisplayString(), QStringLiteral("https://skrooge.org/"))

            SKGPropertyObject propAdded3;
            SKGTESTERROR(QStringLiteral("PROP.setProperty"),
                         obj1.setProperty(QStringLiteral("file copied"), SKGTest::getTestPath(QStringLiteral("IN")) % "dates.txt", QVariant(), &propAdded3),
                         true)
            SKGTEST(QStringLiteral("PROP.getUrl"), propAdded3.getUrl().toDisplayString(), "file://" % SKGTest::getTestPath(QStringLiteral("IN")) % "dates.txt")

            SKGPropertyObject propAdded4;
            SKGTESTERROR(QStringLiteral("PROP.setProperty"),
                         obj1.setProperty(QStringLiteral("file moved"), QStringLiteral("test.txt"), QVariant("ABC"), &propAdded4),
                         true)
            SKGTESTBOOL("PROP.getUrl", propAdded4.getUrl().toDisplayString().startsWith(QStringLiteral("file://")), true)
            SKGTESTBOOL("PROP.getUrl", propAdded4.getUrl().toDisplayString().endsWith(QStringLiteral("test.txt")), true)

            SKGTESTBOOL("PROP.getUrl", propAdded4.getUrl(true).toDisplayString().startsWith(QStringLiteral("file://")), true)
            SKGTESTBOOL("PROP.getUrl", propAdded4.getUrl(true).toDisplayString().endsWith(QStringLiteral("test.txt")), true)
        }
    }

    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("DOC:initialize"), document1.initialize(), true)
        QStringList listTables;
        SKGTESTERROR(QStringLiteral("DOC::getTablesList"), document1.getTablesList(listTables), true)
        for (const auto &table : std::as_const(listTables)) {
            document1.getDisplaySchemas(table);
        }
    }

    // End test
    SKGENDTEST()
}
