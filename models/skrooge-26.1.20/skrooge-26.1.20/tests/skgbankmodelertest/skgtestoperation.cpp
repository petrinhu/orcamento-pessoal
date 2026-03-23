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

    QString filename = SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestoperation.skg";
    QDate now = QDate::currentDate();
    QDate tomorrow = QDate::currentDate().addDays(+1);
    // ============================================================================
    {
        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGUnitValueObject unit_euro_val1;
        SKGBankObject bank(&document1);
        SKGUnitObject unit_euro(&document1);
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

            SKGOperationObject op_1;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op_1), false)
            SKGTESTERROR(QStringLiteral("OP:setParentAccount"), op_1.setParentAccount(account), false)

            SKGTESTERROR(QStringLiteral("ACCOUNT:setClosed"), account.setClosed(true), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation transaction ==> failed
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op_1), false)

            // Reopen account
            SKGTESTERROR(QStringLiteral("ACCOUNT:setClosed"), account.setClosed(false), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            // Creation transaction ==> OK
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op_1), true)

            SKGSubOperationObject subop_1;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), false)

            SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.setUnit(unit_euro), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)

            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)

            SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)

            SKGTEST(QStringLiteral("OPE:getDate"), SKGServices::dateToSqlString(op_1.getDate()), SKGServices::dateToSqlString(now))
            SKGTEST(QStringLiteral("SUBOPE:getDate"), SKGServices::dateToSqlString(subop_1.getDate()), SKGServices::dateToSqlString(now))

            SKGTESTERROR(QStringLiteral("SUBOPE:setDate"), subop_1.setDate(tomorrow), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)

            SKGTEST(QStringLiteral("SUBOPE:getDate"), SKGServices::dateToSqlString(subop_1.getDate()), SKGServices::dateToSqlString(tomorrow))

            SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(tomorrow), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:load"), subop_1.load(), true)

            SKGTEST(QStringLiteral("OPE:getDate"), SKGServices::dateToSqlString(op_1.getDate()), SKGServices::dateToSqlString(tomorrow))
            SKGTEST(QStringLiteral("SUBOPE:getDate"), SKGServices::dateToSqlString(subop_1.getDate()), SKGServices::dateToSqlString(tomorrow.addDays(+1)))
        }
    }

    // ============================================================================
    {
        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGUnitValueObject unit_euro_val1;
        SKGBankObject bank(&document1);
        SKGUnitObject unit_euro(&document1);
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
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            // Creation categories
            SKGCategoryObject cat_1(&document1);
            SKGTESTERROR(QStringLiteral("CAT:setName"), cat_1.setName(QStringLiteral("transport")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat_1.save(), true)

            SKGCategoryObject cat_2;
            SKGTESTERROR(QStringLiteral("CAT:addCategory"), cat_1.addCategory(cat_2), true)
            SKGTESTERROR(QStringLiteral("CAT:setName"), cat_2.setName(QStringLiteral("train")), true)
            SKGTESTERROR(QStringLiteral("CAT:save"), cat_2.save(), true)

            // Creation operation
            SKGOperationObject mainOperation;
            double balance = 0;
            for (int i = 1; i <= 10; ++i) {
                SKGOperationObject op_1;
                SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op_1), true)
                SKGTESTERROR(QStringLiteral("OPE:setTemplate"), op_1.setTemplate(true), true)
                SKGTESTBOOL("OPE:isTemplate", op_1.isTemplate(), true)
                SKGTESTERROR(QStringLiteral("OPE:setTemplate"), op_1.setTemplate(false), true)
                SKGTESTBOOL("OPE:isTemplate", op_1.isTemplate(), false)
                SKGTESTERROR(QStringLiteral("OPE:setNumber"), op_1.setNumber(SKGServices::intToString(1000 + i)), true)
                SKGTESTERROR(QStringLiteral("OPE:setMode"), op_1.setMode(QStringLiteral("cheque")), true)
                SKGTESTERROR(QStringLiteral("OPE:setComment"), op_1.setComment(QStringLiteral("10 tickets")), true)
                SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(now), true)
                SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.setUnit(unit_euro), true)
                SKGUnitObject u;
                SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.getUnit(u), true)
                SKGTESTERROR(QStringLiteral("OPE:bookmark"), op_1.bookmark(i % 2 == 0), true)
                SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
                if (i == 1) {
                    mainOperation = op_1;
                    SKGTESTERROR(QStringLiteral("OPE:setGroupOperation"), mainOperation.setGroupOperation(mainOperation), true)
                    SKGTESTERROR(QStringLiteral("OPE:save"), mainOperation.save(), true)
                } else {
                    if (!op_1.isBookmarked()) {
                        SKGTESTERROR(QStringLiteral("OPE:setGroupOperation"), op_1.setGroupOperation(mainOperation), true)
                    }
                    SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
                }

                // Creation suboperation
                for (int j = 1; j <= 5; ++j) {
                    SKGSubOperationObject subop_1;
                    SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), true)
                    SKGTESTERROR(QStringLiteral("SUBOPE:setCategory"), subop_1.setCategory(cat_2), true)
                    SKGTESTERROR(QStringLiteral("SUBOPE:setComment"), subop_1.setComment(QStringLiteral("subop")), true)
                    SKGTEST(QStringLiteral("SUBOPE:getComment"), subop_1.getComment(), QStringLiteral("subop"))
                    SKGTESTERROR(QStringLiteral("OPE:setFormula"), subop_1.setFormula(QStringLiteral("total*0.196")), true)
                    SKGTEST(QStringLiteral("SUBOPE:getFormula"), subop_1.getFormula(), QStringLiteral("total*0.196"))
                    SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop_1.setQuantity(i * j), true)
                    SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)

                    SKGTESTERROR(QStringLiteral("SUBOPE:load"), subop_1.load(), true)
                    SKGOperationObject opget;
                    SKGTESTERROR(QStringLiteral("SUBOPE:getParentOperation"), subop_1.getParentOperation(opget), true)
                    SKGTESTBOOL("OPE:comparison", (opget == op_1), true)

                    SKGSubOperationObject subop_12 = subop_1;
                    SKGSubOperationObject subop_13((SKGObjectBase(subop_1)));
                    SKGSubOperationObject subop_14 = SKGSubOperationObject(subop_1);

                    SKGObjectBase subop_base = subop_1;
                    SKGSubOperationObject subop_15;
                    subop_15 = subop_base;
                }

                // Checks
                SKGTESTERROR(QStringLiteral("OPE:load"), op_1.load(), true)
                SKGTEST(QStringLiteral("OPE:getCurrentAmount"), op_1.getCurrentAmount(), i * 15)
                balance += i * 15;
                SKGTEST(QStringLiteral("OPE:getNbSubOperations"), op_1.getNbSubOperations(), 5)
                SKGTEST(QStringLiteral("OPE:getBalance"), op_1.getBalance(), balance)
            }

            SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account.load(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), account.getCurrentAmount(), 55 * 15)
            SKGTEST(QStringLiteral("ACCOUNT:getAmount"), account.getAmount(QDate::currentDate()), 55 * 15)

            SKGUnitObject getUnit;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getUnit"), account.getUnit(getUnit), true)
            SKGTESTBOOL("ACCOUNT:unit", (getUnit == unit_euro), true)

            SKGTESTERROR(QStringLiteral("OPE:load"), mainOperation.load(), true)
            SKGObjectBase::SKGListSKGObjectBase oGroupedOperations;
            SKGTESTERROR(QStringLiteral("OPE:getGroupedOperations"), mainOperation.getGroupedOperations(oGroupedOperations), true)
            SKGTEST(QStringLiteral("OPE:oGroupedOperations"), oGroupedOperations.size(), 2) // Normal
            SKGTESTBOOL("OPE:isInGroup", mainOperation.isInGroup(), true) // Normal
            SKGOperationObject mainOpe;
            SKGTESTERROR(QStringLiteral("OPE:getGroupedOperations"), mainOperation.getGroupOperation(mainOpe), true)
            SKGTESTBOOL("OPE:isImported", mainOperation.isImported(), false)
            SKGTESTBOOL("OPE:isTransfer", mainOperation.isTransfer(mainOpe), false)

            SKGTEST(QStringLiteral("OPE:getAmount"), mainOpe.getAmount(QDate::currentDate()), 135)

            SKGAccountObject acc;
            SKGTESTERROR(QStringLiteral("OPE:getParentAccount"), mainOperation.getParentAccount(acc), true)
            SKGTESTBOOL("OPE:account==acc", (account == acc), true)
            SKGTEST(QStringLiteral("OPE:getImportID"), mainOperation.getImportID(), QString())

            SKGObjectBase::SKGListSKGObjectBase oSubOperations;
            SKGTESTERROR(QStringLiteral("OPE:getSubOperations"), mainOperation.getSubOperations(oSubOperations), true)
            SKGTEST(QStringLiteral("OPE:oSubOperations"), oSubOperations.size(), 5)

            SKGTESTERROR(QStringLiteral("BANK:load"), bank.load(), true)
        } // A commit is done here because the scope is close

        {// Scope of the transaction
         SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T2"), err)

             SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(2.5), true)
                 SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

                     SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account.load(), true)
                         SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), account.getCurrentAmount(), 55 * 15 * 2.5)

                             SKGTESTERROR(QStringLiteral("BANK:load"), bank.load(), true)
                                 SKGTEST(QStringLiteral("BANK:getCurrentAmount"), bank.getCurrentAmount(), 55 * 15 * 2.5)}

        QFile(filename)
            .remove();
        SKGTESTERROR(QStringLiteral("DOC:saveAs"), document1.saveAs(filename), true)

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T3"), err)

            // Test foreign key constrain
            SKGTESTERROR(QStringLiteral("UNIT:remove"), unit_euro.remove(), false)
        }
    }

    // ============================================================================
    {
        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load"), document1.load(filename), true)

        SKGAccountObject account;
        SKGTESTERROR(QStringLiteral("SKGAccountObject::getObjectByName"),
                     SKGAccountObject::getObjectByName(&document1, QStringLiteral("v_account"), QStringLiteral("Courant steph"), account),
                     true)

        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), account.getCurrentAmount(), 55 * 15 * 2.5)

        SKGBankObject bank;
        SKGTESTERROR(QStringLiteral("SKGBankObject::getObjectByName"),
                     SKGBankObject::getObjectByName(&document1, QStringLiteral("v_bank"), QStringLiteral("CREDIT COOP"), bank),
                     true)

        SKGTEST(QStringLiteral("BANK:getCurrentAmount"), bank.getCurrentAmount(), 55 * 15 * 2.5)

        SKGTESTERROR(QStringLiteral("BANK:undoRedoTransaction"), document1.undoRedoTransaction(), true)

        SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account.load(), true)
        SKGTEST(QStringLiteral("ACCOUNT:getCurrentAmount"), account.getCurrentAmount(), 55 * 15)
        SKGTESTERROR(QStringLiteral("BANK:load"), bank.load(), true)
        SKGTEST(QStringLiteral("BANK:getCurrentAmount"), bank.getCurrentAmount(), 55 * 15)

        // delete cascade
        SKGError err;
        {// Scope of the transaction
         SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T2"), err)

             SKGTESTERROR(QStringLiteral("BANK:delete"), bank.remove(), true)} QStringList oResult;
        SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"), document1.getDistinctValues(QStringLiteral("bank"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 0)
        SKGTESTERROR(QStringLiteral("ACCOUNT:getDistinctValues"), document1.getDistinctValues(QStringLiteral("account"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("ACCOUNT:oResult.size"), oResult.size(), 0)
        SKGTESTERROR(QStringLiteral("OPE:getDistinctValues"), document1.getDistinctValues(QStringLiteral("operation"), QStringLiteral("id"), oResult), true)
        SKGTEST(QStringLiteral("OPE:oResult.size"), oResult.size(), 0)
        SKGTESTERROR(QStringLiteral("SUBOPE:getDistinctValues"),
                     document1.getDistinctValues(QStringLiteral("suboperation"), QStringLiteral("id"), oResult),
                     true)
        SKGTEST(QStringLiteral("SUBOPE:oResult.size"), oResult.size(), 0)

        SKGTESTERROR(QStringLiteral("BANK:undoRedoTransaction"), document1.undoRedoTransaction(), true)
        SKGTESTERROR(QStringLiteral("BANK:undoRedoTransaction(SKGDocument::REDO)"), document1.undoRedoTransaction(SKGDocument::REDO), true)

        QFile(filename).remove();
    }

    // ============================================================================
    {
        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGUnitValueObject unit_euro_val1;
        SKGBankObject bank(&document1);
        SKGUnitObject unit_euro(&document1);
        SKGAccountObject account1;
        SKGAccountObject account2;
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T1"), err)

            // Creation bank
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank.save(), true)

            // Creation account
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account1), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account1.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::CURRENT), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account1.save(), true)

            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank.addAccount(account2), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account2.setName(QStringLiteral("Loan")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account2.setType(SKGAccountObject::CURRENT), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account2.save(), true)

            // Creation unit
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            // Creation transactions
            SKGOperationObject op_1;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(op_1), true)
            SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(now), true)
            SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.setUnit(unit_euro), true)
            SKGTESTERROR(QStringLiteral("OPE:setNumber"), op_1.setNumber(QStringLiteral("5490990004")), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
            SKGTEST(QStringLiteral("OPE:getNumber"), op_1.getNumber(), QStringLiteral("5490990004"))

            SKGSubOperationObject subop_1;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop_1.setQuantity(10), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)

            SKGOperationObject op_2;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account2.addOperation(op_2), true)
            SKGTESTERROR(QStringLiteral("OPE:setDate"), op_2.setDate(now), true)
            SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_2.setUnit(unit_euro), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op_2.save(), true)

            SKGSubOperationObject subop_2;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_2.addSubOperation(subop_2), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop_2.setQuantity(-10), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_2.save(), true)

            SKGTESTERROR(QStringLiteral("OPE:setGroupOperation"), op_1.setGroupOperation(op_2), true)
            SKGTESTERROR(QStringLiteral("OPE:setStatus"), op_1.setStatus(SKGOperationObject::MARKED), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)

            SKGOperationObject tmp;
            SKGOperationObject tmp2;
            SKGTESTBOOL("OPE:isTransfer", op_1.isTransfer(tmp), true)
            SKGTESTBOOL("OPE:isTransfer", tmp.isTransfer(tmp2), true)
            SKGTESTBOOL("OPE:equal", (tmp == op_2), true)

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account2.setType(SKGAccountObject::LOAN), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account2.save(), true)

            SKGTESTERROR(QStringLiteral("OPE:load"), op_1.load(), true)
            SKGTESTBOOL("OPE:isTransfer", op_1.isTransfer(tmp), false)
            SKGTESTBOOL("OPE:isTransfer", tmp.isTransfer(tmp2), false)
            SKGTESTBOOL("OPE:equal", (tmp == op_2), true)

            SKGTESTERROR(QStringLiteral("OPE:mergeSuboperations"), op_1.mergeSuboperations(op_2), true)

            SKGTESTERROR(QStringLiteral("ACCOUNT:setLinkedAccount"), account1.setLinkedAccount(account2), true)
            SKGAccountObject account2_2;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getLinkedAccount"), account1.getLinkedAccount(account2_2), true)
            SKGTESTBOOL("OPE:equal", (account2 == account2_2), true)

            SKGObjectBase::SKGListSKGObjectBase accounts;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getLinkedByAccounts"), account2.getLinkedByAccounts(accounts), true)

            SKGTESTERROR(QStringLiteral("ACCOUNT:transferDeferredOperations"), account1.transferDeferredOperations(account2), true)

            // Creation template
            SKGOperationObject template1;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(template1), true)
            SKGTESTERROR(QStringLiteral("OPE:setDate"), template1.setDate(now.addDays(-5)), true)
            SKGTESTERROR(QStringLiteral("OPE:setUnit"), template1.setUnit(unit_euro), true)
            SKGTESTERROR(QStringLiteral("OPE:setComment"), template1.setComment(QStringLiteral("Comment op")), true)
            SKGTESTERROR(QStringLiteral("OPE:setTemplate"), template1.setTemplate(true), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), template1.save(), true)

            SKGSubOperationObject template1_1;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), template1.addSubOperation(template1_1), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setDate"), template1_1.setDate(now.addDays(-4)), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), template1_1.setQuantity(10), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setComment"), template1.setComment(QStringLiteral("Comment sub op 1")), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), template1_1.save(), true)

            SKGSubOperationObject template1_2;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), template1.addSubOperation(template1_2), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setDate"), template1_2.setDate(now.addDays(-3)), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), template1_2.setQuantity(90), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setComment"), template1_2.setComment(QStringLiteral("Comment sub op 2")), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), template1_2.save(), true)

            // Creation op
            SKGOperationObject opbasic;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(opbasic), true)
            SKGTESTERROR(QStringLiteral("OPE:setDate"), opbasic.setDate(now), true)
            SKGTESTERROR(QStringLiteral("OPE:setUnit"), opbasic.setUnit(unit_euro), true)
            SKGTESTERROR(QStringLiteral("OPE:setComment"), opbasic.setComment(QStringLiteral("Comment op2")), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), opbasic.save(), true)

            SKGSubOperationObject opbasic_1;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), opbasic.addSubOperation(opbasic_1), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), opbasic_1.setQuantity(200), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), opbasic_1.save(), true)

            document1.dump(DUMPOPERATION);

            SKGTESTERROR(QStringLiteral("SUBOPE:load"), template1.load(), true)
            SKGTEST(QStringLiteral("OPE:getCurrentAmount"), template1.getCurrentAmount(), 100)
            SKGTEST(QStringLiteral("OPE:getNbSubOperations"), template1.getNbSubOperations(), 2)

            SKGTESTERROR(QStringLiteral("SUBOPE:load"), opbasic.load(), true)
            SKGTEST(QStringLiteral("OPE:getCurrentAmount"), opbasic.getCurrentAmount(), 200)
            SKGTEST(QStringLiteral("OPE:getNbSubOperations"), opbasic.getNbSubOperations(), 1)

            // Apply the template
            SKGOperationObject op;
            SKGTESTERROR(QStringLiteral("OPE:duplicate"), template1.duplicate(op), true)
            SKGTESTERROR(QStringLiteral("OPE:load"), op.load(), true)
            SKGTEST(QStringLiteral("OPE:d_createdate"),
                    static_cast<unsigned int>(op.getAttribute(QStringLiteral("d_createdate")) != QString()),
                    static_cast<unsigned int>(true))

            SKGTESTERROR(QStringLiteral("OPE:setProperty"), op.setProperty(QStringLiteral("a"), QStringLiteral("1")), true)
            SKGTESTERROR(QStringLiteral("OPE:setProperty"), opbasic.setProperty(QStringLiteral("a"), QStringLiteral("2")), true)

            SKGTESTERROR(QStringLiteral("OPE:mergeAttribute"), op.mergeAttribute(opbasic, SKGOperationObject::PROPORTIONAL), true)
            document1.dump(DUMPOPERATION);

            SKGTEST(QStringLiteral("OPE:getCurrentAmount"), template1.getCurrentAmount(), 100)
            SKGTEST(QStringLiteral("OPE:getNbSubOperations"), template1.getNbSubOperations(), 2)

            SKGTEST(QStringLiteral("OPE:getCurrentAmount"), op.getCurrentAmount(), 200)
            SKGTEST(QStringLiteral("OPE:getNbSubOperations"), op.getNbSubOperations(), 2)
        }
    }

    // End test
    SKGENDTEST()
}
