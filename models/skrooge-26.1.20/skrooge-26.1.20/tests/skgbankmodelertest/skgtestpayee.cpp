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

    // ============================================================================
    {
        // Test bank document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGBankObject bank(&document1);
        SKGAccountObject account;
        SKGUnitObject unit_euro(&document1);
        SKGUnitValueObject unit_euro_val1;
        QDate d1 = QDate::currentDate().addMonths(-6);
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

            // Creation operation
            SKGOperationObject op;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account.addOperation(op), true)
            SKGTESTERROR(QStringLiteral("OPE:setMode"), op.setMode(QStringLiteral("cheque")), true)
            SKGTESTERROR(QStringLiteral("OPE:setDate"), op.setDate(d1), true)
            SKGTESTERROR(QStringLiteral("OPE:setUnit"), op.setUnit(unit_euro), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op.save(), true)

            SKGPayeeObject payee(&document1);
            SKGPayeeObject payeeCopy(payee);
            SKGPayeeObject payeeCopy2(static_cast<SKGObjectBase>(payee));
            SKGTESTERROR(QStringLiteral("REF:setName"), payee.setName(QStringLiteral("payee")), true)
            SKGTESTERROR(QStringLiteral("REF:setAddress"), payee.setAddress(QStringLiteral("address")), true)
            SKGTESTBOOL("REF:isBookmarked", payee.isBookmarked(), false)
            SKGTESTERROR(QStringLiteral("REF:bookmark"), payee.bookmark(true), true)
            SKGTESTBOOL("REF:isBookmarked", payee.isBookmarked(), true)

            SKGTESTBOOL("REF:isClosed", payee.isClosed(), false)
            SKGTESTERROR(QStringLiteral("REF:setClosed"), payee.setClosed(true), true)
            SKGTESTBOOL("REF:isClosed", payee.isClosed(), true)

            SKGTESTERROR(QStringLiteral("REF:save"), payee.save(), true)
            SKGTEST(QStringLiteral("REF:getName"), payee.getName(), QStringLiteral("payee"))
            SKGTEST(QStringLiteral("REF:getAddress"), payee.getAddress(), QStringLiteral("address"))

            SKGTESTERROR(QStringLiteral("OPE:setPayee"), op.setPayee(payee), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), op.save(), true)
            SKGPayeeObject payee2;
            SKGTESTERROR(QStringLiteral("OPE:getPayee"), op.getPayee(payee2), true)
            SKGTESTBOOL("OPE:compare", (payee == payee2), true)

            SKGCategoryObject cat;
            SKGTESTERROR(QStringLiteral("REF:getCategory"), payee.getCategory(cat), false)

            SKGTESTERROR(QStringLiteral("CAT:createPathCategory"), SKGCategoryObject::createPathCategory(&document1, QStringLiteral("A"), cat), true)
            SKGTESTERROR(QStringLiteral("REF:setCategory"), payee.setCategory(cat), true)
            SKGTESTERROR(QStringLiteral("REF:getCategory"), payee.getCategory(cat), true)

            SKGTESTERROR(QStringLiteral("REF:save"), payee.save(), true)
            SKGTESTERROR(QStringLiteral("REF:load"), payee.load(), true)

            SKGPayeeObject pay;
            SKGTESTERROR(QStringLiteral("REF:createPayee"), SKGPayeeObject::createPayee(&document1, QStringLiteral("pay"), pay, true), true)

            SKGTEST(QStringLiteral("DOC:getCategoryForPayee"), document1.getCategoryForPayee(QStringLiteral("payee")), QStringLiteral("A"))

            // Merge
            SKGTESTERROR(QStringLiteral("CAT:merge"), payee2.merge(pay), true)
        }
    }

    // End test
    SKGENDTEST()
}
