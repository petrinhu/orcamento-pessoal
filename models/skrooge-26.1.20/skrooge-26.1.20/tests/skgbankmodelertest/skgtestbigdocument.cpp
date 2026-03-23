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

#include "skgbankincludes.h"
#include "skgtestmacro.h"
#include "skgtraces.h"

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

    // ============================================================================
    // Init
    QString filename = SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestbigdocument/skgtestbigdocument.skg";

    {
        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGUnitValueObject unit_euro_val1;
        SKGBankObject bank(&document1);
        SKGAccountObject account;
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T1"), err)

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account.setNumber(QStringLiteral("12345P")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGUnitObject unit_euro(&document1);
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            // Creation categories
            auto cats = new SKGCategoryObject[30];
            for (int i = 0; i < 10; ++i) {
                cats[i] = SKGCategoryObject(&document1);
                SKGTESTERROR(QStringLiteral("CAT:setName"), cats[i].setName("cat_" % SKGServices::intToString(i)), true)
                SKGTESTERROR(QStringLiteral("CAT:save"), cats[i].save(), true)

                for (int j = 0; j < 2; ++j) {
                    int indexSubCat = 10 * (j + 1) + i;
                    SKGTESTERROR(QStringLiteral("CAT:addCategory"), cats[i].addCategory(cats[indexSubCat]), true)
                    SKGTESTERROR(QStringLiteral("CAT:setName"),
                                 cats[indexSubCat].setName("cat_" % SKGServices::intToString(i) % QLatin1Char('_') % SKGServices::intToString(j)),
                                 true)
                    SKGTESTERROR(QStringLiteral("CAT:save"), cats[indexSubCat].save(), true)
                }
            }

            // Creation payees
            auto payees = new SKGPayeeObject[10];
            for (int i = 0; i < 10; ++i) {
                payees[i] = SKGPayeeObject(&document1);
                SKGTESTERROR(QStringLiteral("PAY:setName"), payees[i].setName("pay_" % SKGServices::intToString(i)), true)
                SKGTESTERROR(QStringLiteral("PAY:save"), payees[i].save(), true)
            }

            // Mode
            auto modes = new QString[5];
            modes[0] = QStringLiteral("cheque");
            modes[1] = QStringLiteral("carte");
            modes[2] = QStringLiteral("tip");
            modes[3] = QStringLiteral("virement");
            modes[4] = QStringLiteral("espece");

            // Comments
            auto comments = new QString[3];
            comments[0] = QStringLiteral("bla bla");
            comments[1] = QStringLiteral("hello world");
            comments[2] = QStringLiteral("youpi");

            // Creation operation
            SKGOperationObject mainOperation;
            for (int i = 1; i <= 365 * 10; ++i) {
                SKGOperationObject op_1;
                SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op_1), true)
                SKGTESTERROR(QStringLiteral("OPE:setNumber"), op_1.setNumber(SKGServices::intToString(1000 + i)), true)
                SKGTESTERROR(QStringLiteral("OPE:setMode"), op_1.setMode(modes[i % 5]), true)
                SKGTESTERROR(QStringLiteral("OPE:setComment"), op_1.setComment(comments[i % 3]), true)
                SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(now.addDays(-i)), true)
                SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.setUnit(unit_euro), true)
                SKGTESTERROR(QStringLiteral("OPE:setStatus"),
                             op_1.setStatus((i < 20 ? SKGOperationObject::NONE : (i < 40 ? SKGOperationObject::MARKED : SKGOperationObject::CHECKED))),
                             true)
                SKGTESTERROR(QStringLiteral("OPE:bookmark"), op_1.bookmark(i % 2 == 0), true)
                SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
                if (i == 1) {
                    mainOperation = op_1;
                    mainOperation.setGroupOperation(mainOperation);
                    SKGTESTERROR(QStringLiteral("OPE:save"), mainOperation.save(), true)
                } else {
                    if (!op_1.isBookmarked()) {
                        op_1.setGroupOperation(mainOperation);
                    }
                    SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
                }

                // Creation suboperation
                for (int j = 1; j <= 2; ++j) {
                    SKGSubOperationObject subop_1;
                    SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), true)
                    SKGTESTERROR(QStringLiteral("SUBOPE:setCategory"), subop_1.setCategory(cats[i % 30]), true)
                    SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop_1.setQuantity(((i * j) % 60) - 10), true)
                    SKGTESTERROR(QStringLiteral("SUBOPE:setOrder"), subop_1.setOrder(i), true)
                    SKGTEST(QStringLiteral("SUBOPE:getOrder"), subop_1.getOrder(), i)
                    SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)
                }
            }

            cats[0].merge(cats[1]);
            payees[0].merge(payees[1]);

            // Delete
            delete[] cats;
            delete[] modes;
            delete[] comments;
            delete[] payees;
        } // A commit is done here because the scope is close

        SKGTESTERROR(QStringLiteral("ACCOUNT:getCurrentAmount"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), account.getCurrentAmount(), 140165)

        QFile(filename).remove();
        SKGTESTERROR(QStringLiteral("DOC:saveAs"), document1.saveAs(filename), true)
    }

    // ============================================================================
    {
        SKGTraces::SKGPerfo = true;
        SKGTRACEIN(0, "openTest")

        // Test bank document
        SKGDocumentBank document1;
        {
            SKGTRACEIN(0, "openTest-Load")
            SKGTESTERROR(QStringLiteral("document1.load"),
                         document1.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestbigdocument/skgtestbigdocument.skg"),
                         true)
        }

        {
            SKGTRACEIN(0, "openTest-Get")
            SKGAccountObject account;
            SKGTESTERROR(QStringLiteral("SKGAccountObject::getObjectByName"),
                         SKGAccountObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("Courant steph"), account),
                         true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), account.getCurrentAmount(), 140165)

            SKGAccountObject::SKGListSKGObjectBase objects;
            SKGTESTERROR(QStringLiteral("SKGAccountObject::getObjects"), document1.getObjects(QStringLiteral("v_operation"), QString(), objects), true)

            int nbobj = 0;
            SKGTESTERROR(QStringLiteral("SKGAccountObject::getNbObjects"), document1.getNbObjects(QStringLiteral("v_operation"), QString(), nbobj), true)
        }

        {
            SKGTRACEIN(0, "openTest-Save")
            SKGTESTERROR(QStringLiteral("document1.save"), document1.save(), true)
        }
    }

    // End test
    SKGENDTEST()
}
