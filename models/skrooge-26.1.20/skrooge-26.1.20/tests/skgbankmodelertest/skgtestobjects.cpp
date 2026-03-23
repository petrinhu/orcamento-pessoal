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

    QDate now = QDate::currentDate();

    {
        // Test objectbase
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("v_node.unknown")), QStringLiteral("v_node.unknown"))
            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("t_comment")), QStringLiteral("Comment"))
            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("p_Test")), QStringLiteral("Test"))
            SKGTEST(QStringLiteral("DOC:getDisplay"), document1.getDisplay(QStringLiteral("operation.p_Test")), QStringLiteral("operation.Test"))

            SKGServices::SKGAttributesList undoredoAttributes;
            SKGTESTERROR(QStringLiteral("OBJBASE:getAttributesDescription)"),
                         document1.getAttributesDescription(QStringLiteral("doctransaction"), undoredoAttributes),
                         true)
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributes.count"), undoredoAttributes.count(), 7)
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributes.name"), undoredoAttributes[0].name, QStringLiteral("id"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributes.type"),
                    static_cast<unsigned int>(undoredoAttributes[0].type),
                    static_cast<unsigned int>(SKGServices::ID))
            SKGTESTBOOL("OBJBASE:undoredoAttributes.notnull", undoredoAttributes[0].notnull, false)
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributes.defaultvalue"), undoredoAttributes[0].defaultvalue, QString())
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributes.name"), undoredoAttributes[2].name, QStringLiteral("t_mode"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributes.type"),
                    static_cast<unsigned int>(undoredoAttributes[2].type),
                    static_cast<unsigned int>(SKGServices::TEXT))
            SKGTESTBOOL("OBJBASE:undoredoAttributes.notnull", undoredoAttributes[2].notnull, true)
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributes.defaultvalue"), undoredoAttributes[2].defaultvalue, QStringLiteral("'U'"))
            /*
            | cid | name         | t_type       | notnull | dflt_value | pk |
            | 0   | id           | INTEGER    | 99      |            | 1  |
            | 1   | name         | TEXT       | 99      |            | 0  |
            | 2   | t_mode         | VARCHAR(1) | 0       | 'U'        | 0  |
            | 3   | d_date | DATE       | 99      |            | 0  |
            | 4   | t_savestep     | VARCHAR(1) | 0       | 'N'        | 0  |
            | 5   | parent       | INTEGER    | 0       |            | 0  |
            */

            QStringList undoredoAttributeNames;
            SKGTESTERROR(QStringLiteral("OBJBASE:getAttributesList)"),
                         document1.getAttributesList(QStringLiteral("doctransaction"), undoredoAttributeNames),
                         true)
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames.count"), undoredoAttributeNames.count(), 7)
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames"), undoredoAttributeNames[0], QStringLiteral("id"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames"), undoredoAttributeNames[1], QStringLiteral("t_name"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames"), undoredoAttributeNames[2], QStringLiteral("t_mode"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames"), undoredoAttributeNames[3], QStringLiteral("d_date"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames"), undoredoAttributeNames[4], QStringLiteral("t_savestep"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames"), undoredoAttributeNames[5], QStringLiteral("t_refreshviews"))
            SKGTEST(QStringLiteral("OBJBASE:undoredoAttributeNames"), undoredoAttributeNames[6], QStringLiteral("i_parent"))

            SKGBEGINTRANSACTION(document1, QStringLiteral("OBJBASE 1"), err)
            SKGObjectBase obj1(&document1, QStringLiteral("bank"));
            SKGTESTERROR(QStringLiteral("OBJBASE:setAttribute"), obj1.setAttribute(QStringLiteral("t_name"), QStringLiteral("CL")), true)
            SKGTESTERROR(QStringLiteral("OBJBASE:save"), obj1.save(), true)

            SKGObjectBase obj2(&document1, QStringLiteral("bank"), obj1.getID());
            SKGTESTERROR(QStringLiteral("OBJBASE:load"), obj2.load(), true)
            SKGTEST(QStringLiteral("OBJBASE:getAttribute"), obj2.getAttribute(QStringLiteral("t_name")), QStringLiteral("CL"))
            SKGTESTERROR(QStringLiteral("OBJBASE:setAttribute"), obj2.setAttribute(QStringLiteral("t_name"), QStringLiteral("CC")), true)
            SKGTESTERROR(QStringLiteral("OBJBASE:save"), obj2.save(), true)

            SKGObjectBase obj3(&document1, QStringLiteral("v_bank"), obj2.getID());
            SKGTESTERROR(QStringLiteral("OBJBASE:load"), obj3.load(), true)
            SKGTEST(QStringLiteral("OBJBASE:getAttribute"), obj3.getAttribute(QStringLiteral("t_name")), QStringLiteral("CC"))
            SKGTEST(QStringLiteral("OBJBASE:getTable"), obj3.getTable(), QStringLiteral("v_bank"))
            SKGTESTERROR(QStringLiteral("OBJBASE:save"), obj3.save(), true)

            SKGObjectBase obj4(&document1);
            SKGTESTBOOL("OBJBASE:Operator =", (obj2 == obj1), true)
            SKGTESTBOOL("OBJBASE:Operator =", (obj3 == obj1), true)
            SKGTESTBOOL("OBJBASE:Operator =", (obj4 == obj1), false)

            SKGTESTBOOL("OBJBASE:Operator !=", (obj2 != obj1), false)
            SKGTESTBOOL("OBJBASE:Operator !=", (obj3 != obj1), false)
            SKGTESTBOOL("OBJBASE:Operator !=", (obj4 != obj1), true)

            SKGQStringQStringMap att = obj3.getAttributes();
            int nb = obj3.getNbAttributes();
            SKGTEST(QStringLiteral("OBJBASE:getNbAttributes"), obj3.getNbAttributes(), att.count())
            for (int i = 0; i < nb; ++i) {
                SKGTRACE << i << ":" << obj3.getAttribute(SKGServices::intToString(i)) << Qt::endl;
            }
            SKGTEST(QStringLiteral("OBJBASE:getAttribute"), obj3.getAttribute(QStringLiteral("2")), QStringLiteral("CC"))
            SKGTESTBOOL("OBJBASE:exist", obj3.exist(), true)
            SKGTESTERROR(QStringLiteral("OBJBASE:dump"), obj3.dump(), true)

            SKGTEST(QStringLiteral("OBJBASE:getID"), obj3.getID(), 1)
            SKGTESTERROR(QStringLiteral("OBJBASE:resetID"), obj3.resetID(), true)
            SKGTEST(QStringLiteral("OBJBASE:getID"), obj3.getID(), 0)

            SKGNamedObject tmp1(obj3);
            const SKGNamedObject &tmp2 = tmp1;
            SKGTEST(QStringLiteral("OBJBASE:getAttribute"), tmp2.getAttribute(QStringLiteral("t_name")), QStringLiteral("CC"))

            // Test error
            SKGObjectBase notFound;
            SKGTESTERROR(QStringLiteral("OBJBASE:getObject"),
                         document1.getObject(QStringLiteral("v_bank"), QStringLiteral("t_name='NOTFOUND'"), notFound),
                         false)
            SKGTESTERROR(QStringLiteral("OBJBASE:getObject"), document1.getObject(QStringLiteral("parameters"), QStringLiteral("'"), notFound), false)
            SKGTESTERROR(QStringLiteral("OBJBASE:getObject"), document1.getObject(QStringLiteral("v_bank"), 9999, notFound), false)
        }
    }
    // ============================================================================
    // ============================================================================
    {
        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        SKGTEST(QStringLiteral("BANK:getNbTransaction"), document1.getNbTransaction(), 0)
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T1"), err)

            // The code here
            IFOK(err)
            {
                SKGTEST(QStringLiteral("BANK:getDepthTransaction"), document1.getDepthTransaction(), 1)

                // Example; transaction succeeded
                SKGTESTERROR(QStringLiteral("BANK:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")), true)
                SKGTESTERROR(QStringLiteral("BANK:setParameter"), document1.setParameter(QStringLiteral("ATT2"), QStringLiteral("VAL2")), true)
                SKGTESTERROR(QStringLiteral("BANK:setParameter"), document1.setParameter(QStringLiteral("ATT3"), QStringLiteral("VAL2")), true)

                QStringList oResult;
                SKGTESTERROR(
                    QStringLiteral("BANK:getDistinctValues"),
                    document1.getDistinctValues(QStringLiteral("parameters"), QStringLiteral("t_value"), QStringLiteral("t_value like 'VAL%'"), oResult),
                    true)
                SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 2)

                // Test account
                SKGTESTERROR(QStringLiteral("BANK:addOrModifyAccount"),
                             document1.addOrModifyAccount(QStringLiteral("Courant"), QStringLiteral("123"), QStringLiteral("LCL")),
                             true)
                SKGTESTERROR(QStringLiteral("BANK:addOrModifyAccount"),
                             document1.addOrModifyAccount(QStringLiteral("Livret A"), QStringLiteral("456"), QStringLiteral("CREDIT COOP")),
                             true)
                SKGTESTERROR(QStringLiteral("BANK:addOrModifyAccount"),
                             document1.addOrModifyAccount(QStringLiteral("Codevi"), QStringLiteral("789"), QStringLiteral("NEF")),
                             true)
                SKGTESTERROR(QStringLiteral("BANK:addOrModifyAccount"),
                             document1.addOrModifyAccount(QStringLiteral("PEA"), QStringLiteral("ABC"), QStringLiteral("CA")),
                             true)
                SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"),
                             document1.getDistinctValues(QStringLiteral("account"), QStringLiteral("t_name"), oResult),
                             true)
                SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 4)

                // Test units
                SKGTESTERROR(QStringLiteral("BANK:addOrModifyUnitValue"), document1.addOrModifyUnitValue(QStringLiteral("E"), now, 1), true)
                SKGTESTERROR(QStringLiteral("BANK:addOrModifyUnitValue"),
                             document1.addOrModifyUnitValue(QStringLiteral("$"), now, 1.6),
                             true) // Oups, it is an error
                SKGTESTERROR(QStringLiteral("BANK:addOrModifyUnitValue"),
                             document1.addOrModifyUnitValue(QStringLiteral("$"), now, 0.625),
                             true) // it is better now
                SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"),
                             document1.getDistinctValues(QStringLiteral("unitvalue"), QStringLiteral("rd_unit_id"), oResult),
                             true)
                SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 2)
            }
        } // A commit is done here because the scope is close

        QStringList oResult;
        SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"), document1.getDistinctValues(QStringLiteral("unit"), QStringLiteral("t_name"), oResult), true)
        SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 2)
        SKGTESTERROR(QStringLiteral("BANK:undoRedoTransaction(BANK_T1, SKGDocument::UNDO)"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"), document1.getDistinctValues(QStringLiteral("unit"), QStringLiteral("t_name"), oResult), true)
        SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 0)
        SKGTESTERROR(QStringLiteral("BANK:undoRedoTransaction(BANK_T1, SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)
        SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"), document1.getDistinctValues(QStringLiteral("unit"), QStringLiteral("t_name"), oResult), true)
        SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 2)

        SKGTESTERROR(QStringLiteral("BANK:err"), err, true)
    }

    // End test
    SKGENDTEST()
}
