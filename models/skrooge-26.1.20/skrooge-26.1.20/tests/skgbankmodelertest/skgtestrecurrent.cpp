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
#include "skgservices.h"
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

    struct TestData {
        bool sourceOperationIsTemplate;
        bool recurrentOperationIsTemplate;
    };

    std::vector<TestData> testDataSet{
        {false, false},
        {false, true},
        {true, true},
    };

    // ============================================================================
    for (const auto &testData : testDataSet) {
        const auto sourceOperationIsTemplate = testData.sourceOperationIsTemplate;
        const auto recurrentOperationIsTemplate = testData.recurrentOperationIsTemplate;
        const bool createTemplateForRecurrent = !sourceOperationIsTemplate && recurrentOperationIsTemplate;
        SKGTRACE << "####### Test data set:" << Qt::endl;
        SKGTRACE << "####### sourceOperationIsTemplate=" << (sourceOperationIsTemplate ? "TRUE" : "FALSE") << Qt::endl;
        SKGTRACE << "####### recurrentOperationIsTemplate=" << (recurrentOperationIsTemplate ? "TRUE" : "FALSE") << Qt::endl;
        SKGTRACE << "####### createTemplateForRecurrent=" << (createTemplateForRecurrent ? "TRUE" : "FALSE") << Qt::endl;

        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGAccountObject account;
        SKGUnitObject unit_euro(&document1);
        SKGUnitValueObject unit_euro_val1;
        QDate d1 = QDate::currentDate().addMonths(-6);
        QDate d2 = QDate::currentDate().addMonths(-4);
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
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(d1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            const auto createOperation = [&](SKGOperationObject &op, QDate operationDate, const QString &comment, bool isTemplate) {
                SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op), true)

                SKGTESTERROR(QStringLiteral("OP:setTemplate"), op.setTemplate(isTemplate), true)
                SKGTESTERROR(QStringLiteral("OP:setMode"), op.setMode(QStringLiteral("cheque")), true)
                SKGTESTERROR(QStringLiteral("OP:setComment"), op.setComment(comment), true)
                SKGTESTERROR(QStringLiteral("OP:setDate"), op.setDate(operationDate), true)
                SKGTESTERROR(QStringLiteral("OP:setUnit"), op.setUnit(unit_euro), true)
                SKGTESTERROR(QStringLiteral("OP:save"), op.save(), true)
            };

            const auto addSuboperations = [&](SKGOperationObject &op) {
                {
                    SKGSubOperationObject subop;
                    SKGTESTERROR(QStringLiteral("OP:addSubOperation"), op.addSubOperation(subop), true)
                    SKGTESTERROR(QStringLiteral("SUBOP:setQuantity"), subop.setQuantity(8.5), true)
                    SKGTESTERROR(QStringLiteral("SUBOP:save"), subop.save(), true)
                }
                {
                    SKGSubOperationObject subop;
                    SKGTESTERROR(QStringLiteral("OP:addSubOperation"), op.addSubOperation(subop), true)
                    SKGTESTERROR(QStringLiteral("SUBOP:setQuantity"), subop.setQuantity(10), true)
                    SKGTESTERROR(QStringLiteral("SUBOP:save"), subop.save(), true)
                }
            };

            SKGOperationObject op1;
            SKGOperationObject templateOp1;

            SKGTRACE << "####### Test recurrent with an unsaved operation" << Qt::endl;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op1), true)

            SKGObjectBase::SKGListSKGObjectBase recups;
            SKGTESTERROR(QStringLiteral("OP:getRecurrentOperations"), op1.getRecurrentOperations(recups), false)
            SKGTEST(QStringLiteral("RECOP:recops.count"), recups.count(), 0)
            SKGTEST(QStringLiteral("OP:getRecurrentOperation"), op1.getRecurrentOperation(), 0)

            SKGRecurrentOperationObject recuope1;
            SKGTESTERROR(QStringLiteral("OP:addRecurrentOperation"), op1.addRecurrentOperation(recuope1), false)
            SKGTEST(QStringLiteral("OP:getRecurrentOperation"), op1.getRecurrentOperation(), 0)

            const auto addGroupedOperation = [&](SKGOperationObject &op, QDate operationDate, const QString &comment) {
                SKGTESTERROR(QStringLiteral("OP:load"), op.load(), true)
                {
                    SKGOperationObject op2;
                    SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op2), true)
                    SKGTESTERROR(QStringLiteral("OP:setMode"), op2.setMode(QStringLiteral("cheque")), true)
                    SKGTESTERROR(QStringLiteral("OP:setComment"), op2.setComment(comment), true)
                    SKGTESTERROR(QStringLiteral("OP:setDate"), op2.setDate(operationDate), true)
                    SKGTESTERROR(QStringLiteral("OP:setUnit"), op2.setUnit(unit_euro), true)
                    SKGTESTERROR(QStringLiteral("OP:save"), op2.save(), true)

                    SKGSubOperationObject subop;
                    SKGTESTERROR(QStringLiteral("OP:addSubOperation"), op2.addSubOperation(subop), true)
                    SKGTESTERROR(QStringLiteral("SUBOP:setQuantity"), subop.setQuantity(8.5), true)
                    SKGTESTERROR(QStringLiteral("SUBOP:save"), subop.save(), true)

                    SKGTESTERROR(QStringLiteral("OP:save"), op.setGroupOperation(op2), true)
                    SKGTESTERROR(QStringLiteral("OP:save"), op.save(), true)
                }
            };

            const auto createRecurrentOperation = [&](SKGRecurrentOperationObject &recuope, SKGOperationObject &sourceOp, SKGOperationObject &templateOp) {
                SKGOperationObject op = sourceOp;
                if (createTemplateForRecurrent) {
                    IFOKDO(err, op.duplicate(templateOp, op.getDate(), true))
                    op = templateOp;
                }

                SKGTESTERROR(QStringLiteral("OP:addRecurrentOperation"), op.addRecurrentOperation(recuope), true)
                SKGTESTERROR(QStringLiteral("RECOP:setPeriodIncrement"), recuope.setPeriodIncrement(2), true)
                SKGTESTERROR(QStringLiteral("RECOP:setPeriodUnit"), recuope.setPeriodUnit(SKGRecurrentOperationObject::MONTH), true)
                SKGTESTERROR(QStringLiteral("RECOP:setAutoWriteDays"), recuope.setAutoWriteDays(6), true)
                SKGTESTERROR(QStringLiteral("RECOP:autoWriteEnabled"), recuope.autoWriteEnabled(true), true)
                SKGTESTERROR(QStringLiteral("RECOP:setWarnDays"), recuope.setWarnDays(10), true)
                SKGTESTERROR(QStringLiteral("RECOP:warnEnabled"), recuope.warnEnabled(true), true)
                SKGTESTERROR(QStringLiteral("RECOP:setTimeLimit"), recuope.setTimeLimit(d2), true)
                SKGTESTERROR(QStringLiteral("RECOP:timeLimit"), recuope.timeLimit(true), true)
                SKGTEST(QStringLiteral("OP:getRecurrentOperation"), op.getRecurrentOperation(), 0)
                SKGTESTERROR(QStringLiteral("RECOP:save"), recuope.save(), true)

                if (createTemplateForRecurrent) {
                    SKGTESTERROR(QStringLiteral("RECOP:save"),
                                 sourceOp.setAttribute(QStringLiteral("r_recurrentoperation_id"), SKGServices::intToString(recuope.getID())),
                                 true)
                    SKGTESTERROR(QStringLiteral("OP:save"), sourceOp.save(), true)
                }

                SKGTESTBOOL(QStringLiteral("RECOP:isTemplate"), recuope.isTemplate(), recurrentOperationIsTemplate)
            };

            SKGTRACE << "####### Test recurrent with a saved operation" << Qt::endl;
            createOperation(op1, d1, QStringLiteral("10 tickets"), sourceOperationIsTemplate);
            addSuboperations(op1);
            addGroupedOperation(op1, d1, QStringLiteral("10 tickets"));

            createRecurrentOperation(recuope1, op1, templateOp1);
            SKGTEST(QStringLiteral("OP:getRecurrentOperation"), op1.getRecurrentOperation(), recuope1.getID())
            SKGTESTBOOL(QStringLiteral("OP:exist"), templateOp1.exist(), createTemplateForRecurrent)

            SKGRecurrentOperationObject recuope2(recuope1);
            SKGRecurrentOperationObject recuope3(SKGObjectBase(recuope1.getDocument(), QStringLiteral("xxx"), recuope1.getID()));
            SKGRecurrentOperationObject recuope4(SKGNamedObject(recuope1.getDocument(), QStringLiteral("xxx"), recuope1.getID()));

            SKGObjectBase::SKGListSKGObjectBase recops;
            SKGTESTERROR(QStringLiteral("OP:getRecurrentOperations"), op1.getRecurrentOperations(recops), true)
            SKGTEST(QStringLiteral("RECOP:recops.count"), recops.count(), createTemplateForRecurrent ? 0 : 1)
            if (createTemplateForRecurrent) {
                SKGTESTERROR(QStringLiteral("OP:getRecurrentOperations"), templateOp1.getRecurrentOperations(recops), true)
                SKGTEST(QStringLiteral("RECOP:recops.count"), recops.count(), 1)
            }

            SKGTEST(QStringLiteral("RECOP:getPeriodIncrement"), recuope1.getPeriodIncrement(), 2)
            SKGTEST(QStringLiteral("RECOP:getPeriodUnit"),
                    static_cast<unsigned int>(recuope1.getPeriodUnit()),
                    static_cast<unsigned int>(SKGRecurrentOperationObject::MONTH))
            SKGTEST(QStringLiteral("RECOP:getAutoWriteDays"), recuope1.getAutoWriteDays(), 6)
            SKGTESTBOOL("RECOP:isAutoWriteEnabled", recuope1.isAutoWriteEnabled(), true)
            SKGTEST(QStringLiteral("RECOP:getWarnDays"), recuope1.getWarnDays(), 10)
            SKGTESTBOOL("RECOP:isWarnEnabled", recuope1.isWarnEnabled(), true)
            SKGTEST(QStringLiteral("RECOP:getTimeLimit"), recuope1.getTimeLimit(), 3)
            SKGTESTBOOL("RECOP:hasTimeLimit", recuope1.hasTimeLimit(), true)
            SKGTEST(QStringLiteral("RECOP:getDate"), recuope1.getDate().toString(), d1.toString())

            SKGOperationObject ope2;
            SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope1.getParentOperation(ope2), true)
            SKGTESTBOOL("RECOP:op1==ope2", (op1 == ope2), !createTemplateForRecurrent)
            SKGTESTBOOL("RECOP:templateOp1==ope2", (templateOp1 == ope2), createTemplateForRecurrent)
            SKGTEST(QStringLiteral("OP:getRecurrentOperation"), ope2.getRecurrentOperation(), recuope1.getID())

            const auto testProcess = [&](SKGRecurrentOperationObject &recuope, int resultingOperationsCount) {
                int nbi = 0;
                SKGTESTERROR(QStringLiteral("RECOP:process"), recuope.process(nbi), true)
                SKGTEST(QStringLiteral("RECOP:nbi"), nbi, resultingOperationsCount)
                SKGOperationObject opeP;
                SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(opeP), true)
                SKGTESTBOOL("RECOP:op1!=opeP", (op1 != opeP), !sourceOperationIsTemplate)
                SKGTEST(QStringLiteral("OP:getRecurrentOperation"), opeP.getRecurrentOperation(), recuope.getID())

                SKGObjectBase::SKGListSKGObjectBase recopsP;
                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recops.count"), recopsP.count(), resultingOperationsCount + (createTemplateForRecurrent ? 1 : 0))
                for (const auto &opr : std::as_const(recopsP)) {
                    SKGTEST(QStringLiteral("OP:getAttribute"),
                            opr.getAttribute(QStringLiteral("r_recurrentoperation_id")),
                            SKGServices::intToString(recuope.getID()))
                }

                SKGTESTERROR(QStringLiteral("RECOP:SKGRecurrentOperationObject::process"), SKGRecurrentOperationObject::process(&document1, nbi), true)
            };

            SKGTRACE << "####### Process a schedule" << Qt::endl;
            testProcess(recuope1, 3);

            SKGTESTERROR(QStringLiteral("RECOP:setPeriodIncrement"), recuope1.setPeriodIncrement(2), true)
            SKGTESTERROR(QStringLiteral("RECOP:setPeriodUnit"), recuope1.setPeriodUnit(SKGRecurrentOperationObject::DAY), true)
            recuope1.getNextDate();
            SKGTESTERROR(QStringLiteral("RECOP:setPeriodUnit"), recuope1.setPeriodUnit(SKGRecurrentOperationObject::WEEK), true)
            recuope1.getNextDate();
            SKGTESTERROR(QStringLiteral("RECOP:setPeriodUnit"), recuope1.setPeriodUnit(SKGRecurrentOperationObject::MONTH), true)
            recuope1.getNextDate();
            SKGTESTERROR(QStringLiteral("RECOP:setPeriodUnit"), recuope1.setPeriodUnit(SKGRecurrentOperationObject::YEAR), true)
            recuope1.getNextDate();

            const auto testAssign = [&](SKGRecurrentOperationObject &recuope, QDate operationDate, const QString &comment, int resultingOperationsCount) {
                SKGOperationObject opA;
                SKGTESTERROR(QStringLiteral("OP:duplicate"), op1.duplicate(opA, operationDate, false), true)
                SKGTESTERROR(QStringLiteral("OP:setComment"), opA.setComment(comment), true)
                SKGTESTERROR(QStringLiteral("OP:duplicate"), opA.setAttribute(QStringLiteral("r_recurrentoperation_id"), QStringLiteral("0")), true)
                SKGTESTERROR(QStringLiteral("OP:save"), opA.save(), true)

                // Update comment for all grouped transactions
                SKGObjectBase::SKGListSKGObjectBase goupops;
                SKGTESTERROR(QStringLiteral("OP:getGroupedOperations"), opA.getGroupedOperations(goupops), true)
                for (int i = 0; !err && i < goupops.count(); ++i) {
                    SKGOperationObject groupop(goupops.at(i));
                    SKGTESTERROR(QStringLiteral("OP:setComment"), groupop.setComment(comment), true)
                    SKGTESTERROR(QStringLiteral("OP:save"), groupop.save(), true)
                }

                auto expectedRecurrentId = recuope.getID();
                if (!recurrentOperationIsTemplate) {
                    SKGOperationObject parentOp;
                    SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)
                    if (operationDate >= parentOp.getDate()) {
                        // This transaction should become a parent for a non-template schedule
                        expectedRecurrentId = 0;
                    }
                }
                SKGTESTERROR(QStringLiteral("OP:setRecurrentOperation"), opA.setRecurrentOperation(recuope.getID()), true)
                SKGTEST(QStringLiteral("OP:getAttribute"),
                        opA.getAttribute(QStringLiteral("r_recurrentoperation_id")),
                        SKGServices::intToString(expectedRecurrentId))

                SKGObjectBase::SKGListSKGObjectBase recopsP;
                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recops.count"), recopsP.count(), resultingOperationsCount + (createTemplateForRecurrent ? 1 : 0))
                for (const auto &opr : std::as_const(recopsP)) {
                    SKGTEST(QStringLiteral("OP:getAttribute"),
                            opr.getAttribute(QStringLiteral("r_recurrentoperation_id")),
                            SKGServices::intToString(recuope.getID()))
                }
            };

            const auto removeOperation = [&](SKGOperationObject &op) {
                const auto removeSubOp = [&](SKGSubOperationObject &subop) {
                    SKGTESTERROR(QStringLiteral("SUBOP:remove"), subop.remove(false), true)
                };
                const auto removeOpAndSubops = [&](SKGOperationObject &op) {
                    SKGObjectBase::SKGListSKGObjectBase subops;
                    SKGTESTERROR(QStringLiteral("OP:getSubOperations"), op.getSubOperations(subops), true)
                    for (int i = 0; !err && i < subops.count(); ++i) {
                        SKGSubOperationObject subop(subops.at(i));
                        removeSubOp(subop);
                    }
                    SKGTESTERROR(QStringLiteral("OP:remove"), op.remove(false), true)
                };
                const auto removeOp = [&](SKGOperationObject &op) {
                    if (op.isInGroup()) {
                        SKGObjectBase::SKGListSKGObjectBase groupops;
                        SKGTESTERROR(QStringLiteral("OP:getGroupedOperations"), op.getGroupedOperations(groupops), true)
                        for (int i = 0; !err && i < groupops.count(); ++i) {
                            SKGOperationObject groupop(groupops.at(i));
                            removeOpAndSubops(groupop);
                        }
                    } else {
                        removeOpAndSubops(op);
                    }
                };

                removeOp(op);
            };

            const auto removeAllScheduled = [&](SKGRecurrentOperationObject &recuope) {
                SKGObjectBase::SKGListSKGObjectBase recops;
                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recops), true)
                for (int i = 0; !err && i < recops.count(); ++i) {
                    SKGOperationObject op(recops.at(i));
                    removeOperation(op);
                }

                recuope.load();
                SKGOperationObject parentOp;
                SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)

                SKGTESTERROR(QStringLiteral("RECOP:remove"), recuope.remove(false), true)

                removeOperation(parentOp);
            };

            const auto testDocumentIsEmpty = [&]() {
                bool exists = false;
                SKGTESTERROR(QStringLiteral("DOC:existObjects"), document1.existObjects(QStringLiteral("operation"), QString(), exists), true)
                SKGTESTBOOL(QStringLiteral("DOC:operation.exists"), exists, false)
                SKGTESTERROR(QStringLiteral("DOC:existObjects"), document1.existObjects(QStringLiteral("recurrentoperation"), QString(), exists), true)
                SKGTESTBOOL(QStringLiteral("DOC:recurrentoperation.exists"), exists, false)
                SKGTESTERROR(QStringLiteral("DOC:existObjects"), document1.existObjects(QStringLiteral("suboperation"), QString(), exists), true)
                SKGTESTBOOL(QStringLiteral("DOC:recurrentoperation.suboperation"), exists, false)
            };

            QDate start_date, end_date;

            SKGTRACE << "####### Assign a schedule to a preceeding operation" << Qt::endl;
            start_date = d1.addMonths(-3);
            testAssign(recuope1, start_date, QStringLiteral("10 tickets (old)"), 4);

            SKGTRACE << "####### Assign a schedule to a succeeding operation" << Qt::endl;
            end_date = d2.addMonths(2);
            testAssign(recuope1, end_date, QStringLiteral("10 tickets (new)"), 5);

            SKGTRACE << "####### Remove all scheduled transactions" << Qt::endl;
            removeAllScheduled(recuope1);
            testDocumentIsEmpty();

            SKGTRACE << "####### Unassign last transaction from a schedule" << Qt::endl;
            {
                SKGOperationObject op;
                createOperation(op, d1, QStringLiteral("20 tickets"), sourceOperationIsTemplate);

                SKGRecurrentOperationObject recuope;
                SKGOperationObject templateOp;
                createRecurrentOperation(recuope, op, templateOp);

                const bool shouldFail = sourceOperationIsTemplate && recurrentOperationIsTemplate;
                SKGTESTERROR(QStringLiteral("OP:setRecurrentOperation"), op.setRecurrentOperation(0), !shouldFail)
                if (!shouldFail) {
                    SKGTEST(QStringLiteral("OP:getAttribute"), op.getAttribute(QStringLiteral("r_recurrentoperation_id")), QStringLiteral("0"))
                }

                recuope.load();
                SKGTESTBOOL(QStringLiteral("RECOP:exist"), recuope.exist(), recurrentOperationIsTemplate)

                if (createTemplateForRecurrent) {
                    SKGOperationObject parentOp;
                    SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)
                    SKGTESTBOOL(QStringLiteral("RECOP:exist"), parentOp.exist(), true)
                    SKGTESTBOOL("RECOP:templateOp==parentOp", (templateOp == parentOp), true)
                }

                removeAllScheduled(recuope);
                removeOperation(op);

                testDocumentIsEmpty();
            }

            SKGTRACE << "####### Remove last transaction from a schedule" << Qt::endl;
            {
                SKGOperationObject op;
                createOperation(op, d1, QStringLiteral("20 tickets"), sourceOperationIsTemplate);

                SKGRecurrentOperationObject recuope;
                SKGOperationObject templateOp;
                createRecurrentOperation(recuope, op, templateOp);

                removeOperation(op);

                recuope.load();
                SKGTESTBOOL(QStringLiteral("RECOP:exist"), recuope.exist(), createTemplateForRecurrent)

                if (createTemplateForRecurrent) {
                    SKGOperationObject parentOp;
                    SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)
                    SKGTESTBOOL(QStringLiteral("RECOP:exist"), parentOp.exist(), createTemplateForRecurrent)
                    SKGTESTBOOL("RECOP:templateOp==parentOp", (templateOp == parentOp), true)
                    removeAllScheduled(recuope);
                }

                testDocumentIsEmpty();
            }

            SKGTRACE << "####### Unassign non-last transaction from a schedule" << Qt::endl;
            {
                SKGOperationObject op1;
                createOperation(op1, d1, QStringLiteral("20 tickets"), sourceOperationIsTemplate);

                SKGRecurrentOperationObject recuope;
                SKGOperationObject templateOp;
                createRecurrentOperation(recuope, op1, templateOp);

                SKGOperationObject op2;
                createOperation(op2, d2, QStringLiteral("15 tickets"), false);
                SKGTESTERROR(QStringLiteral("OP:setRecurrentOperation"), op2.setRecurrentOperation(recuope.getID()), true)

                SKGObjectBase::SKGListSKGObjectBase recopsP;
                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recopsP.count"), recopsP.count(), createTemplateForRecurrent ? 2 : 1)
                SKGOperationObject firstOp(recopsP.first());
                SKGTESTERROR(QStringLiteral("OP:setRecurrentOperation"), firstOp.setRecurrentOperation(0), true)
                SKGTEST(QStringLiteral("OP:getAttribute"), firstOp.getAttribute(QStringLiteral("r_recurrentoperation_id")), QStringLiteral("0"))

                recuope.load();
                SKGTEST(QStringLiteral("RECOP:exist"), static_cast<unsigned int>(recuope.exist()), static_cast<unsigned int>(true))

                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recops.count"), recopsP.count(), createTemplateForRecurrent ? 1 : 0)
                for (const auto &opr : std::as_const(recopsP)) {
                    SKGTEST(QStringLiteral("OP:getAttribute"),
                            opr.getAttribute(QStringLiteral("r_recurrentoperation_id")),
                            SKGServices::intToString(recuope.getID()))
                }

                SKGOperationObject parentOp;
                SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)
                SKGTEST(QStringLiteral("RECOP:exist"), static_cast<unsigned int>(parentOp.exist()), static_cast<unsigned int>(true))
                SKGTESTBOOL("RECOP:templateOp==parentOp", (templateOp == parentOp), createTemplateForRecurrent)
                SKGTEST(QStringLiteral("OP:getRecurrentOperation"), parentOp.getRecurrentOperation(), recuope.getID())

                removeAllScheduled(recuope);
                removeOperation(firstOp);
                testDocumentIsEmpty();
            }

            SKGTRACE << "####### Remove non-last transaction from a schedule" << Qt::endl;
            {
                SKGOperationObject op1;
                createOperation(op1, d1, QStringLiteral("20 tickets"), sourceOperationIsTemplate);

                SKGRecurrentOperationObject recuope;
                SKGOperationObject templateOp;
                createRecurrentOperation(recuope, op1, templateOp);

                SKGOperationObject op2;
                createOperation(op2, d2, QStringLiteral("15 tickets"), false);
                SKGTESTERROR(QStringLiteral("OP:setRecurrentOperation"), op2.setRecurrentOperation(recuope.getID()), true)

                SKGObjectBase::SKGListSKGObjectBase recopsP;
                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recopsP.count"), recopsP.count(), createTemplateForRecurrent ? 2 : 1)
                SKGOperationObject firstOp(recopsP.first());
                removeOperation(firstOp);

                recuope.load();
                SKGTEST(QStringLiteral("RECOP:exist"), static_cast<unsigned int>(recuope.exist()), static_cast<unsigned int>(true))

                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recops.count"), recopsP.count(), createTemplateForRecurrent ? 1 : 0)
                for (const auto &opr : std::as_const(recopsP)) {
                    SKGTEST(QStringLiteral("OP:getAttribute"),
                            opr.getAttribute(QStringLiteral("r_recurrentoperation_id")),
                            SKGServices::intToString(recuope.getID()))
                }

                SKGOperationObject parentOp;
                SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)
                SKGTEST(QStringLiteral("RECOP:exist"), static_cast<unsigned int>(parentOp.exist()), static_cast<unsigned int>(true))
                SKGTESTBOOL("RECOP:templateOp==parentOp", (templateOp == parentOp), createTemplateForRecurrent)
                SKGTEST(QStringLiteral("OP:getRecurrentOperation"), parentOp.getRecurrentOperation(), recuope.getID())

                removeAllScheduled(recuope);
                testDocumentIsEmpty();
            }

            SKGTRACE << "####### Toggle template mode for a schedule with a single operation" << Qt::endl;
            {
                SKGOperationObject op;
                createOperation(op, d1, QStringLiteral("20 tickets"), sourceOperationIsTemplate);

                SKGRecurrentOperationObject recuope;
                SKGOperationObject templateOp;
                createRecurrentOperation(recuope, op, templateOp);

                const bool newTemplateState = !recurrentOperationIsTemplate;
                const bool shouldFail = sourceOperationIsTemplate && recurrentOperationIsTemplate;
                SKGTESTERROR(QStringLiteral("RECOP:setTemplate"), recuope.setTemplate(newTemplateState), !shouldFail)
                if (!shouldFail) {
                    SKGOperationObject parentOp;
                    SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)
                    SKGTEST(QStringLiteral("RECOP:exist"), static_cast<unsigned int>(parentOp.exist()), static_cast<unsigned int>(true))
                    SKGTESTBOOL("RECOP:parentOp.idTemplate", parentOp.isTemplate(), newTemplateState)
                }

                SKGObjectBase::SKGListSKGObjectBase recopsP;
                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recopsP.count"), recopsP.count(), !!newTemplateState && !sourceOperationIsTemplate ? 1 : 0)

                removeAllScheduled(recuope);
                testDocumentIsEmpty();
            }

            SKGTRACE << "####### Toggle template mode for a schedule with several transactions" << Qt::endl;
            {
                SKGOperationObject op1;
                createOperation(op1, d1, QStringLiteral("20 tickets"), sourceOperationIsTemplate);

                SKGRecurrentOperationObject recuope;
                SKGOperationObject templateOp;
                createRecurrentOperation(recuope, op1, templateOp);

                SKGOperationObject op2;
                createOperation(op2, d2, QStringLiteral("15 tickets"), false);
                SKGTESTERROR(QStringLiteral("OP:setRecurrentOperation"), op2.setRecurrentOperation(recuope.getID()), true)

                const bool newTemplateState = !recurrentOperationIsTemplate;
                recuope.load();
                SKGTESTERROR(QStringLiteral("RECOP:setTemplate"), recuope.setTemplate(newTemplateState), true)
                SKGOperationObject parentOp;
                SKGTESTERROR(QStringLiteral("RECOP:getParentOperation"), recuope.getParentOperation(parentOp), true)
                SKGTEST(QStringLiteral("RECOP:exist"), static_cast<unsigned int>(parentOp.exist()), static_cast<unsigned int>(true))
                SKGTESTBOOL("RECOP:parentOp.idTemplate", parentOp.isTemplate(), newTemplateState)

                SKGObjectBase::SKGListSKGObjectBase recopsP;
                SKGTESTERROR(QStringLiteral("RECOP:getRecurredOperations"), recuope.getRecurredOperations(recopsP), true)
                SKGTEST(QStringLiteral("RECOP:recopsP.count"), recopsP.count(), !sourceOperationIsTemplate ? (!!newTemplateState ? 2 : 1) : 0)

                removeAllScheduled(recuope);
                testDocumentIsEmpty();
            }
        }

        // 492287
        {
            SKGError err;
            SKGDocumentBank document1;
            SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestreccurent/492287.skg"), true)
            {
            }
        }
    }

    // 491382
    {
        struct TestDataAdapter {
            QDate previousDate;
            int increment;
            SKGRecurrentOperationObject::Adapter adapter;
            QDate expectedNextDate;
        };

        std::vector<TestDataAdapter> testDataAdapterSet{
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NONE, QDate(2025, 10, 18)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_MONDAY, QDate(2025, 10, 13)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_TUESDAY, QDate(2025, 10, 14)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_WEDNESDAY, QDate(2025, 10, 15)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_THURSDAY, QDate(2025, 10, 16)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_FRIDAY, QDate(2025, 10, 17)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_SATURDAY, QDate(2025, 10, 18)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_SUNDAY, QDate(2025, 10, 12)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_MONDAY, QDate(2025, 10, 20)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_TUESDAY, QDate(2025, 10, 21)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_WEDNESDAY, QDate(2025, 10, 15)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_THURSDAY, QDate(2025, 10, 16)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_FRIDAY, QDate(2025, 10, 17)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_SATURDAY, QDate(2025, 10, 18)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_SUNDAY, QDate(2025, 10, 19)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_MONDAY, QDate(2025, 10, 20)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_TUESDAY, QDate(2025, 10, 21)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_WEDNESDAY, QDate(2025, 10, 22)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_THURSDAY, QDate(2025, 10, 23)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_FRIDAY, QDate(2025, 10, 24)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_SATURDAY, QDate(2025, 10, 18)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_SUNDAY, QDate(2025, 10, 19)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::PREVIOUS_OPEN_DAY, QDate(2025, 10, 17)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::CLOSEST_OPEN_DAY, QDate(2025, 10, 17)},
            {QDate(2025, 10, 11), 7, SKGRecurrentOperationObject::NEXT_OPEN_DAY, QDate(2025, 10, 20)},
            {QDate(2025, 10, 11), 1, SKGRecurrentOperationObject::PREVIOUS_MONDAY, QDate(2025, 10, 12)}, // Adapter can not be applied
            {QDate(2025, 10, 1), 31, SKGRecurrentOperationObject::FIRST_OPEN_DAY_OF_MONTH, QDate(2025, 11, 3)},
        };

        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        for (const auto &t : testDataAdapterSet) {
            SKGRecurrentOperationObject recuope(&document1, 1);
            recuope.setPeriodIncrement(t.increment);
            recuope.setPeriodUnit(SKGRecurrentOperationObject::DAY);
            recuope.setDate(t.previousDate);
            recuope.setAdapter(t.adapter);
            SKGTEST(QStringLiteral("RECOP:getNextDate ") % SKGServices::intToString(t.adapter), recuope.getNextDate().toString(), t.expectedNextDate.toString())
        }

        SKGRecurrentOperationObject recuope(&document1, 1);
        recuope.setPeriodIncrement(1);
        recuope.setPeriodUnit(SKGRecurrentOperationObject::MONTH);
        recuope.setDate(QDate(2025, 2, 1));
        recuope.setAdapter(SKGRecurrentOperationObject::NEXT_OPEN_DAY);
        SKGTEST(QStringLiteral("RECOP:getNextDate "), recuope.getNextDate().toString(), QDate(2025, 3, 3).toString())
    }
    // End test
    SKGENDTEST()
}
