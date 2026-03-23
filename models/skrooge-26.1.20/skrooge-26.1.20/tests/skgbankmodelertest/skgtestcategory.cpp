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
        // Test category
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGCategoryObject parent2;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("CAT_T1"), err)

            // Create category 0
            SKGCategoryObject cat0(&document1);
            SKGTESTERROR(QStringLiteral("CAT:setName+invalid name"), cat0.setName(QLatin1Char('a') % OBJECTSEPARATOR % QLatin1Char('b')), false)
            SKGTESTBOOL("CAT:exist", cat0.exist(), false)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat0.save(), false)

            // Create category 1
            SKGCategoryObject cat1(&document1);

            SKGCategoryObject cat1_1;
            SKGTESTERROR(QStringLiteral("CAT:addCategory"), cat1.addCategory(cat1_1), false)
            SKGTESTERROR(QStringLiteral("CAT:setParentCategory"), cat1.setParentCategory(cat1_1), false)

            SKGTESTERROR(QStringLiteral("CAT:setName"), cat1.setName(QStringLiteral("root1")), true)
            SKGTESTBOOL("CAT:getFullName", cat1.isBookmarked(), false)
            SKGTESTERROR(QStringLiteral("CAT:bookmark"), cat1.bookmark(true), true)
            SKGTESTBOOL("CAT:getFullName", cat1.isBookmarked(), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat1.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1.getFullName(), QStringLiteral("root1"))
            SKGTESTERROR(QStringLiteral("CAT:load"), cat1.load(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1.getFullName(), QStringLiteral("root1"))

            SKGTESTBOOL("REF:isClosed", cat1.isClosed(), false)
            SKGTESTERROR(QStringLiteral("REF:setClosed"), cat1.setClosed(true), true)
            SKGTESTBOOL("REF:isClosed", cat1.isClosed(), true)

            SKGObjectBase obj1 = static_cast<SKGObjectBase>(cat1);
            SKGCategoryObject cat11(obj1);

            // Update with bad name
            SKGTESTERROR(QStringLiteral("CAT:setName"), cat1.setName("root1" % OBJECTSEPARATOR % QLatin1Char('A')), false)
            SKGTESTERROR(QStringLiteral("CAT:load"), cat1.load(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1.getFullName(), QStringLiteral("root1"))

            // Create category 1.1
            SKGTESTERROR(QStringLiteral("CAT:addCategory"), cat1.addCategory(cat1_1), true)
            SKGTESTERROR(QStringLiteral("CAT:setName"), cat1_1.setName(QStringLiteral("cat1")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat1_1.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1_1.getFullName(), "root1" % OBJECTSEPARATOR % "cat1")

            // Update cat1_1
            SKGTESTERROR(QStringLiteral("CAT:setName"), cat1_1.setName(QStringLiteral("CAT1")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat1_1.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1_1.getFullName(), "root1" % OBJECTSEPARATOR % "CAT1")

            // Update cat1
            SKGTESTERROR(QStringLiteral("CAT:setName"), cat1.setName(QStringLiteral("ROOT1")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat1.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1.getFullName(), QStringLiteral("ROOT1"))

            SKGTESTERROR(QStringLiteral("CAT:load"), cat1_1.load(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1_1.getFullName(), "ROOT1" % OBJECTSEPARATOR % "CAT1")

            // Create category 1.2
            SKGCategoryObject cat1_2;
            SKGTESTERROR(QStringLiteral("CAT:addCategory"), cat1.addCategory(cat1_2), true)
            SKGTESTERROR(QStringLiteral("CAT:setName"), cat1_2.setName(QStringLiteral("CAT2")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat1_2.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), cat1_2.getFullName(), "ROOT1" % OBJECTSEPARATOR % "CAT2")

            // Create category end
            SKGCategoryObject end1;
            SKGTESTERROR(QStringLiteral("CAT:addCategory"), cat1_1.addCategory(end1), true)
            SKGTESTERROR(QStringLiteral("CAT:setName"), end1.setName(QStringLiteral("END")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), end1.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), end1.getFullName(), "ROOT1" % OBJECTSEPARATOR % "CAT1" % OBJECTSEPARATOR % "END")

            // Create category end
            SKGCategoryObject end2;
            SKGTESTERROR(QStringLiteral("CAT:addCategory"), cat1_2.addCategory(end2), true)
            SKGTESTERROR(QStringLiteral("CAT:setName"), end2.setName(QStringLiteral("END")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), end2.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), end2.getFullName(), "ROOT1" % OBJECTSEPARATOR % "CAT2" % OBJECTSEPARATOR % "END")

            SKGCategoryObject end2_1;
            SKGTESTERROR(QStringLiteral("CAT:addCategory"), end2.addCategory(end2_1), true)
            SKGTESTERROR(QStringLiteral("CAT:setName"), end2_1.setName(QStringLiteral("REALEND")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), end2_1.save(), true)
            SKGTEST(QStringLiteral("CAT:getFullName"),
                    end2_1.getFullName(),
                    "ROOT1" % OBJECTSEPARATOR % "CAT2" % OBJECTSEPARATOR % "END" % OBJECTSEPARATOR % "REALEND")

            // Get parent
            SKGCategoryObject parent1;
            SKGTESTERROR(QStringLiteral("CAT:getParentCategory"), end2.getParentCategory(parent1), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), parent1.getFullName(), "ROOT1" % OBJECTSEPARATOR % "CAT2")

            // Get parent
            SKGTESTERROR(QStringLiteral("CAT:getParentCategory"), parent1.getParentCategory(parent2), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), parent2.getFullName(), QStringLiteral("ROOT1"))

            SKGCategoryObject root;
            SKGTESTERROR(QStringLiteral("CAT:getRootCategory"), end2_1.getRootCategory(root), true)
            SKGTEST(QStringLiteral("CAT:getFullName"), root.getFullName(), QStringLiteral("ROOT1"))

            // Get children
            SKGObjectBase::SKGListSKGObjectBase CategoryList;
            SKGTESTERROR(QStringLiteral("CAT:getCategories"), parent2.getCategories(CategoryList), true)
            SKGTEST(QStringLiteral("CAT:nb categories"), CategoryList.size(), 2)

            // Simple delete
            SKGTESTERROR(QStringLiteral("CAT:delete"), end1.remove(), true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 5)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("CAT_T2"), err)

            // Cascading delete
            SKGTESTERROR(QStringLiteral("CAT:delete"), parent2.remove(), true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 0)
        }

        // Undo
        SKGTESTERROR(QStringLiteral("CAT:undoRedoTransaction"), document1.undoRedoTransaction(), true)
        QStringList oResult;
        SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 5)

        SKGTESTERROR(QStringLiteral("CAT:undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 0)

        // Redo
        SKGTESTERROR(QStringLiteral("CAT:undoRedoTransaction(SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 5)

        SKGTESTERROR(QStringLiteral("CAT:undoRedoTransaction(SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 0)
    }

    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("CAT_T1"), err)

            SKGCategoryObject cat;
            SKGTESTERROR(QStringLiteral("CAT:createPathCategory"), SKGCategoryObject::createPathCategory(&document1, QString(), cat), true)
            SKGTESTERROR(QStringLiteral("CAT:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('A'), cat, true),
                         true)
            SKGTEST(QStringLiteral("CAT:getName"), cat.getName(), QStringLiteral("A"))

            QStringList oResult;
            SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 2)

            SKGTESTERROR(QStringLiteral("CAT:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B'), cat, true, true),
                         true)
            SKGTEST(QStringLiteral("CAT:getName"), cat.getName(), QStringLiteral("B"))
            SKGTESTERROR(QStringLiteral("CAT:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B'), cat, true, true),
                         true)
            SKGTEST(QStringLiteral("CAT:getName"), cat.getName(), QStringLiteral("B (2)"))

            SKGTESTERROR(QStringLiteral("CAT:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QStringLiteral("C"), cat, true, true),
                         true)
            SKGTEST(QStringLiteral("CAT:getName"), cat.getName(), QStringLiteral("C"))
            SKGTESTERROR(QStringLiteral("CAT:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QStringLiteral("C"), cat, true, true),
                         true)
            SKGTEST(QStringLiteral("CAT:getName"), cat.getName(), QStringLiteral("C (2)"))
        }
    }

    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGCategoryObject categoryB;
            SKGTESTERROR(QStringLiteral("NOD:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B'), categoryB),
                         true)
            SKGCategoryObject categoryC;
            SKGTESTERROR(QStringLiteral("NOD:createPathCategory"), SKGCategoryObject::createPathCategory(&document1, QStringLiteral("C"), categoryC), true)
            SKGCategoryObject categoryA;
            SKGTESTERROR(QStringLiteral("NOD:getParentCategory"), categoryB.getParentCategory(categoryA), true)

            SKGTESTERROR(QStringLiteral("NOD:setParentCategory"), categoryA.setParentCategory(categoryB), false)
            SKGTESTERROR(QStringLiteral("NOD:setParentCategory"), categoryA.setParentCategory(categoryA), false)
            SKGTESTERROR(QStringLiteral("NOD:setParentCategory"), categoryB.setParentCategory(categoryB), false)
            SKGTESTERROR(QStringLiteral("NOD:setParentCategory"), categoryC.setParentCategory(categoryB), true)
            SKGTESTERROR(QStringLiteral("NOD:removeParentCategory"), categoryB.removeParentCategory(), true)
        }
    }

    // Bug 245254
    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGCategoryObject categoryB;
            SKGTESTERROR(QStringLiteral("NOD:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B'), categoryB),
                         true)
            SKGCategoryObject categoryC;
            SKGTESTERROR(QStringLiteral("NOD:createPathCategory"), SKGCategoryObject::createPathCategory(&document1, QStringLiteral("C"), categoryC), true)
            SKGCategoryObject categoryA;
            SKGTESTERROR(QStringLiteral("NOD:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QLatin1Char('C') % OBJECTSEPARATOR % QLatin1Char('A'), categoryA),
                         true)

            QStringList oResult;
            SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 4)

            // Merge
            SKGTESTERROR(QStringLiteral("CAT:merge"), categoryB.merge(categoryA), true)
        }
    }

    // Cascading delete
    {
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGCategoryObject categoryA;
            SKGTESTERROR(QStringLiteral("NOD:createPathCategory"), SKGCategoryObject::createPathCategory(&document1, QStringLiteral("A"), categoryA), true)
            SKGCategoryObject categoryC;
            SKGTESTERROR(QStringLiteral("NOD:createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1,
                                                               QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B') % OBJECTSEPARATOR % QLatin1Char('C'),
                                                               categoryC),
                         true)

            QStringList oResult;
            SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 3)

            // Delete
            SKGTESTERROR(QStringLiteral("CAT:remove"), categoryA.remove(), true)

            SKGTESTERROR(QStringLiteral("CAT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("category"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("CAT:oResult.size"), oResult.size(), 0)
        }
    }
    // End test
    SKGENDTEST()
}
