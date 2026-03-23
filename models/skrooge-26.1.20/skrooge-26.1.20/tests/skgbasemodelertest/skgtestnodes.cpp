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

    {
        // Test node
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGNodeObject parent2;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            // Create node 0
            SKGNodeObject nod0(&document1);
            SKGTESTERROR(QStringLiteral("NOD:setName+invalid name"), nod0.setName(QLatin1Char('a') % OBJECTSEPARATOR % QLatin1Char('b')), false)
            SKGTESTERROR(QStringLiteral("NOD:save"), nod0.save(), false)

            // Create node 1
            SKGNodeObject nod1(&document1);
            SKGTESTERROR(QStringLiteral("NOD:setName"), nod1.setName(QStringLiteral("root1")), true)
            SKGTESTERROR(QStringLiteral("NOD:setData"), nod1.setData(QStringLiteral("data1")), true)
            SKGTESTERROR(QStringLiteral("NOD:setOrder"), nod1.setOrder(12), true)
            SKGTESTERROR(QStringLiteral("NOD:setAutoStart"), nod1.setAutoStart(true), true)
            SKGTESTBOOL("NOD:exist", nod1.exist(), false)
            SKGTESTERROR(QStringLiteral("NOD:save"), nod1.save(), true)
            SKGTESTBOOL("NOD:exist", nod1.exist(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), nod1.getFullName(), QStringLiteral("root1"))
            SKGTEST(QStringLiteral("NOD:getData"), nod1.getData(), QStringLiteral("data1"))
            SKGTEST(QStringLiteral("NOD:getOrder"), nod1.getOrder(), 12)
            SKGTESTBOOL("NOD:isFolder", nod1.isFolder(), false)
            SKGTESTBOOL("NOD:isAutoStart", nod1.isAutoStart(), true)
            nod1.getIcon();

            // Update with bad name
            SKGTESTERROR(QStringLiteral("NOD:setName"), nod1.setName("root1" % OBJECTSEPARATOR % QLatin1Char('A')), false)
            SKGTESTERROR(QStringLiteral("NOD:load"), nod1.load(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), nod1.getFullName(), QStringLiteral("root1"))

            // Create node 1.1
            SKGNodeObject nod1_1;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), nod1.addNode(nod1_1), true)
            SKGTESTERROR(QStringLiteral("NOD:setName"), nod1_1.setName(QStringLiteral("nod1")), true)
            SKGTESTERROR(QStringLiteral("NOD:setOrder"), nod1_1.setOrder(1), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), nod1_1.save(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), nod1_1.getFullName(), "root1" % OBJECTSEPARATOR % "nod1")

            // Update nod1_1
            SKGTESTERROR(QStringLiteral("NOD:setName"), nod1_1.setName(QStringLiteral("NODE1")), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), nod1_1.save(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), nod1_1.getFullName(), "root1" % OBJECTSEPARATOR % "NODE1")

            // Update nod1
            SKGTESTERROR(QStringLiteral("NOD:setName"), nod1.setName(QStringLiteral("ROOT1")), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), nod1.save(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), nod1.getFullName(), QStringLiteral("ROOT1"))

            SKGTESTERROR(QStringLiteral("NOD:load"), nod1_1.load(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), nod1_1.getFullName(), "ROOT1" % OBJECTSEPARATOR % "NODE1")

            // Create node 1.2
            SKGNodeObject nod1_2;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), nod1.addNode(nod1_2), true)
            SKGTESTERROR(QStringLiteral("NOD:setName"), nod1_2.setName(QStringLiteral("NODE2")), true)
            SKGTESTERROR(QStringLiteral("NOD:setOrder"), nod1_2.setOrder(2), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), nod1_2.save(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), nod1_2.getFullName(), "ROOT1" % OBJECTSEPARATOR % "NODE2")
            SKGTESTBOOL("NOD:<", (nod1_1 < nod1_2), true)
            SKGTESTBOOL("NOD:>", (nod1_1 > nod1_2), false)
            SKGTESTBOOL("NOD:<", (nod1_1 < nod1_1), false)
            SKGTESTBOOL("NOD:>", (nod1_1 > nod1_1), false)

            // Create node end
            SKGNodeObject end1;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), nod1_1.addNode(end1), true)
            SKGTESTERROR(QStringLiteral("NOD:setName"), end1.setName(QStringLiteral("END")), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), end1.save(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), end1.getFullName(), "ROOT1" % OBJECTSEPARATOR % "NODE1" % OBJECTSEPARATOR % "END")

            // Create node end
            SKGNodeObject end2;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), nod1_2.addNode(end2), true)
            SKGTESTERROR(QStringLiteral("NOD:setName"), end2.setName(QStringLiteral("END")), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), end2.save(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), end2.getFullName(), "ROOT1" % OBJECTSEPARATOR % "NODE2" % OBJECTSEPARATOR % "END")

            SKGNodeObject end2_1;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), end2.addNode(end2_1), true)
            SKGTESTERROR(QStringLiteral("NOD:setName"), end2_1.setName(QStringLiteral("REALEND")), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), end2_1.save(), true)
            SKGTEST(QStringLiteral("NOD:getFullName"),
                    end2_1.getFullName(),
                    "ROOT1" % OBJECTSEPARATOR % "NODE2" % OBJECTSEPARATOR % "END" % OBJECTSEPARATOR % "REALEND")

            // Get parent
            SKGNodeObject parent1;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), end2.getParentNode(parent1), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), parent1.getFullName(), "ROOT1" % OBJECTSEPARATOR % "NODE2")

            // Get parent
            SKGTESTERROR(QStringLiteral("NOD:addNode"), parent1.getParentNode(parent2), true)
            SKGTEST(QStringLiteral("NOD:getFullName"), parent2.getFullName(), QStringLiteral("ROOT1"))

            // Get children
            SKGObjectBase::SKGListSKGObjectBase NodeList;
            SKGTESTERROR(QStringLiteral("NOD:getNodes"), parent2.getNodes(NodeList), true)
            SKGTEST(QStringLiteral("NOD:nb nodegories"), NodeList.size(), 2)

            // Simple delete
            SKGTESTERROR(QStringLiteral("NOD:delete"), end1.remove(), true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 5)

            SKGNodeObject nodeROOT1BC;
            SKGTESTERROR(
                QStringLiteral("NOD:addNode"),
                SKGNodeObject::createPathNode(&document1, "ROOT1" % OBJECTSEPARATOR % QLatin1Char('B') % OBJECTSEPARATOR % QLatin1Char('C'), nodeROOT1BC),
                true)

            SKGNodeObject nodeROOT1DE;
            SKGTESTERROR(
                QStringLiteral("NOD:addNode"),
                SKGNodeObject::createPathNode(&document1, "ROOT1" % OBJECTSEPARATOR % QLatin1Char('D') % OBJECTSEPARATOR % QLatin1Char('E'), nodeROOT1DE),
                true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T2"), err)

            // Cascading delete
            SKGTESTERROR(QStringLiteral("NOD:delete"), parent2.remove(), true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 0)
        }

        // Undo
        SKGTESTERROR(QStringLiteral("NOD:undoRedoTransaction"), document1.undoRedoTransaction(), true)
        QStringList oResult;
        SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 9)

        SKGTESTERROR(QStringLiteral("NOD:undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 0)

        // Redo
        SKGTESTERROR(QStringLiteral("NOD:undoRedoTransaction(SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 9)

        SKGTESTERROR(QStringLiteral("NOD:undoRedoTransaction(SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 0)
    }

    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject node;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % "B1" % OBJECTSEPARATOR % QLatin1Char('C'), node),
                         true)
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % "B1" % OBJECTSEPARATOR % QLatin1Char('D'), node),
                         true)
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % "B2" % OBJECTSEPARATOR % QLatin1Char('E'), node),
                         true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 6)
        }
    }

    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject node;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % "B1" % OBJECTSEPARATOR % QLatin1Char('C'), node),
                         true)
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % "B2" % OBJECTSEPARATOR % QLatin1Char('C'), node),
                         true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 5)
        }
    }

    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject node;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('A'), node),
                         true)
            QStringList oResult;
            SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 2)
        }
    }

    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject nodeB;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B'), nodeB),
                         true)
            SKGNodeObject nodeC;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"), SKGNodeObject::createPathNode(&document1, QStringLiteral("C"), nodeC), true)
            SKGNodeObject nodeA;
            SKGTESTERROR(QStringLiteral("NOD:getParentNode"), nodeB.getParentNode(nodeA), true)

            SKGNamedObject nameB = static_cast<SKGNamedObject>(nodeB); // For coverage
            SKGNodeObject nodeC2(static_cast<SKGObjectBase>(nodeC)); // For coverage

            SKGTESTERROR(QStringLiteral("NOD:setParentNode"), nodeA.setParentNode(nodeB), false)
            SKGTESTERROR(QStringLiteral("NOD:setParentNode"), nodeA.setParentNode(nodeA), false)
            SKGTESTERROR(QStringLiteral("NOD:setParentNode"), nodeB.setParentNode(nodeB), false)
            SKGTESTERROR(QStringLiteral("NOD:setParentNode"), nodeC.setParentNode(nodeB), true)
            SKGTESTERROR(QStringLiteral("NOD:removeParentNode"), nodeB.removeParentNode(), true)
        }
    }

    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject nodeB1;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B'), nodeB1, true),
                         true)
            SKGNodeObject nodeB2;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"),
                         SKGNodeObject::createPathNode(&document1, QLatin1Char('A') % OBJECTSEPARATOR % QLatin1Char('B'), nodeB2, true),
                         true)
            document1.dump(DUMPALL);

            QStringList oResult;
            SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 3)
        }
    }

    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject nodeB1;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"), SKGNodeObject::createPathNode(&document1, QStringLiteral("B"), nodeB1, true), true)
            SKGNodeObject nodeB2;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"), SKGNodeObject::createPathNode(&document1, QStringLiteral("B"), nodeB2, true), true)

            QStringList oResult;
            SKGTESTERROR(QStringLiteral("NOD:getDistinctValues"), document1.getDistinctValues(QStringLiteral("node"), QStringLiteral("id"), oResult), true)
            SKGTEST(QStringLiteral("NOD:oResult.size"), oResult.size(), 2)
        }
    }

    // Test properties
    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject node1;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"), SKGNodeObject::createPathNode(&document1, QStringLiteral("1"), node1, true), true)
            SKGNodeObject node2;
            SKGTESTERROR(QStringLiteral("NOD:createPathNode"), SKGNodeObject::createPathNode(&document1, QStringLiteral("2"), node2, true), true)

            SKGTESTERROR(QStringLiteral("NOD:setProperty"), node1.setProperty(QStringLiteral("prop"), QStringLiteral("val1")), true)
            SKGTESTERROR(QStringLiteral("NOD:setProperty"), node2.setProperty(QStringLiteral("prop"), QStringLiteral("val2")), true)

            SKGTEST(QStringLiteral("NOD:getProperty"), node1.getProperty(QStringLiteral("prop")), QStringLiteral("val1"))
            SKGTEST(QStringLiteral("NOD:getProperty"), node2.getProperty(QStringLiteral("prop")), QStringLiteral("val2"))

            SKGTESTERROR(QStringLiteral("NOD:setProperty"), node1.setProperty(QStringLiteral("prop"), QStringLiteral("val3")), true)

            SKGTEST(QStringLiteral("NOD:getProperty"), node1.getProperty(QStringLiteral("prop")), QStringLiteral("val3"))
            SKGTEST(QStringLiteral("NOD:getProperty"), node2.getProperty(QStringLiteral("prop")), QStringLiteral("val2"))
        }
    }

    // Test setAttribute
    {
        SKGDocument document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("NODE_T1"), err)

            SKGNodeObject node1;
            SKGTESTERROR(QStringLiteral("NOD:save"), node1.save(), false)
            SKGTESTERROR(QStringLiteral("NOD:remove"), node1.remove(), false)

            SKGNodeObject node2;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), node1.addNode(node2), false)
            SKGTESTERROR(QStringLiteral("NOD:setParentNode"), node1.setParentNode(node2), false)

            SKGTESTERROR(QStringLiteral("NOD:createPathNode"), SKGNodeObject::createPathNode(&document1, QStringLiteral("ABC DEF "), node1, true), true)

            SKGTESTERROR(QStringLiteral("NOD:setAttribute"),
                         node1.setAttribute(QStringLiteral("t_name"), QStringLiteral("=") + i18nc("Key word to modify a string into a field", "lower")),
                         true)
            SKGTEST(QStringLiteral("NOD:getAttribute"), node1.getAttribute(QStringLiteral("t_name")), QStringLiteral("abc def "))

            SKGTESTERROR(QStringLiteral("NOD:setAttribute"),
                         node1.setAttribute(QStringLiteral("t_name"), QStringLiteral("=") + i18nc("Key word to modify a string into a field", "upper")),
                         true)
            SKGTEST(QStringLiteral("NOD:getAttribute"), node1.getAttribute(QStringLiteral("t_name")), QStringLiteral("ABC DEF "))

            SKGTESTERROR(QStringLiteral("NOD:setAttribute"),
                         node1.setAttribute(QStringLiteral("t_name"), QStringLiteral("=") + i18nc("Key word to modify a string into a field", "capwords")),
                         true)
            SKGTEST(QStringLiteral("NOD:getAttribute"), node1.getAttribute(QStringLiteral("t_name")), QStringLiteral("Abc Def "))

            SKGTESTERROR(QStringLiteral("NOD:setAttribute"),
                         node1.setAttribute(QStringLiteral("t_name"), QStringLiteral("=") + i18nc("Key word to modify a string into a field", "capitalize")),
                         true)
            SKGTEST(QStringLiteral("NOD:getAttribute"), node1.getAttribute(QStringLiteral("t_name")), QStringLiteral("Abc def "))

            SKGTESTERROR(QStringLiteral("NOD:setAttribute"),
                         node1.setAttribute(QStringLiteral("t_name"), QStringLiteral("=") + i18nc("Key word to modify a string into a field", "trim")),
                         true)
            SKGTEST(QStringLiteral("NOD:getAttribute"), node1.getAttribute(QStringLiteral("t_name")), QStringLiteral("Abc def"))

            SKGTESTERROR(QStringLiteral("NOD:setProperty"), node1.setProperty(QStringLiteral("prop"), QStringLiteral("prop on node 1")), true)
            SKGTEST(QStringLiteral("NOD:getProperty"), node1.getProperty(QStringLiteral("prop")), QStringLiteral("prop on node 1"))
            SKGTEST(QStringLiteral("NOD:getAttribute"), node1.getAttribute(QStringLiteral("p_prop")), QStringLiteral("prop on node 1"))

            SKGTESTERROR(QStringLiteral("NOD:save"), node1.save(), true)

            SKGTESTERROR(QStringLiteral("NOD:addNode"), node1.addNode(node2), true)
            SKGTESTERROR(QStringLiteral("NOD:setAttribute"), node2.setAttribute(QStringLiteral("t_name"), QStringLiteral("node2")), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), node2.save(), true)

            SKGTEST(QStringLiteral("NOD:getAttribute"), node2.getAttribute(QStringLiteral("rd_node_id.(v_node)t_name")), QStringLiteral("Abc def"))
            SKGTEST(QStringLiteral("NOD:getAttribute"), node2.getAttribute(QStringLiteral("rd_node_id.(v_node)p_prop")), QStringLiteral("prop on node 1"))

            SKGTEST(QStringLiteral("NOD:getAttribute"), node2.getAttribute(QStringLiteral("rd_node_id.t_name")), QStringLiteral("Abc def"))
            SKGTEST(QStringLiteral("NOD:getAttribute"), node2.getAttribute(QStringLiteral("rd_node_id.p_prop")), QStringLiteral("prop on node 1"))

            SKGNodeObject node3;
            SKGTESTERROR(QStringLiteral("NOD:addNode"), node2.addNode(node3), true)
            SKGTESTERROR(QStringLiteral("NOD:setAttribute"), node3.setAttribute(QStringLiteral("t_name"), QStringLiteral("node3")), true)
            SKGTESTERROR(QStringLiteral("NOD:save"), node3.save(), true)

            SKGTEST(QStringLiteral("NOD:getAttribute"), node3.getAttribute(QStringLiteral("rd_node_id.(v_node)t_name")), QStringLiteral("node2"))
            SKGTEST(QStringLiteral("NOD:getAttribute"), node3.getAttribute(QStringLiteral("rd_node_id.(v_node)p_prop")), QString())

            SKGTEST(QStringLiteral("NOD:getAttribute"), node3.getAttribute(QStringLiteral("rd_node_id.t_name")), QStringLiteral("node2"))
            SKGTEST(QStringLiteral("NOD:getAttribute"), node3.getAttribute(QStringLiteral("rd_node_id.p_prop")), QString())

            SKGTEST(QStringLiteral("NOD:getAttribute"),
                    node3.getAttribute(QStringLiteral("rd_node_id.(v_node)rd_node_id.(v_node)t_name")),
                    QStringLiteral("Abc def"))
            SKGTEST(QStringLiteral("NOD:getAttribute"),
                    node3.getAttribute(QStringLiteral("rd_node_id.(v_node)rd_node_id.(v_node)p_prop")),
                    QStringLiteral("prop on node 1"))

            SKGTEST(QStringLiteral("NOD:getAttribute"), node3.getAttribute(QStringLiteral("rd_node_id.rd_node_id.t_name")), QStringLiteral("Abc def"))
            SKGTEST(QStringLiteral("NOD:getAttribute"), node3.getAttribute(QStringLiteral("rd_node_id.rd_node_id.p_prop")), QStringLiteral("prop on node 1"))
        }
    }
    // End test
    SKGENDTEST()
}
