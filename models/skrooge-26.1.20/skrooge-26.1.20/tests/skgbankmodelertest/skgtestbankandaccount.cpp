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
    // ============================================================================
    // Init
    {
        // Test bank1 document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T1"), err)

            // Creation unit
            SKGUnitObject unit_euro(&document1);
            SKGTESTERROR(QStringLiteral("BANK:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("BANK:setSymbol"), unit_euro.setSymbol(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("BANK:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("BANK:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("BANK:setDate"), unit_euro_val1.setDate(QDate::currentDate()), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), unit_euro_val1.save(), true)

            // Creation bank1
            SKGBankObject bank1(&document1);
            SKGAccountObject account1;
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank1.addAccount(account1), false)

            SKGTESTERROR(QStringLiteral("BANK:setName"), bank1.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:setIcon"), bank1.setIcon(QStringLiteral("credit cooperatif")), true)
            SKGTESTERROR(QStringLiteral("BANK:setNumber"), bank1.setNumber(QStringLiteral("111111")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank1.save(), true)
            SKGTEST(QStringLiteral("BANK:getIcon"), bank1.getIcon(), QStringLiteral("credit cooperatif"))
            SKGTEST(QStringLiteral("BANK:getNumber"), bank1.getNumber(), QStringLiteral("111111"))

            SKGBankObject bank11(bank1); // For coverage
            SKGTESTBOOL("BANK:comparison", (bank11 == bank1), true)
            SKGBankObject bank12 = bank1; // For coverage
            SKGTESTBOOL("BANK:comparison", (bank12 == bank1), true)
            SKGBankObject bank13((SKGNamedObject(bank1))); // For coverage
            SKGTESTBOOL("BANK:comparison", (bank12 == SKGNamedObject(bank1)), true)
            SKGNamedObject bank1n(bank1);
            SKGBankObject bank14(bank1n); // For coverage

            SKGBankObject bank15(SKGObjectBase(bank1.getDocument(), QStringLiteral("xxx"), bank1.getID())); // For coverage
            SKGBankObject bank16(SKGNamedObject(bank1.getDocument(), QStringLiteral("xxx"), bank1.getID())); // For coverage

            // Creation account1
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank1.addAccount(account1), true)

            SKGOperationObject op;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(op), false)

            SKGInterestObject it;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addInterest(it), false)

            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account1.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account1.setNumber(QStringLiteral("12345P")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setAgencyNumber"), account1.setAgencyNumber(QStringLiteral("99999")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setAgencyAddress"), account1.setAgencyAddress(QStringLiteral("10 rue Albert CAMUS\n31000 TOULOUSE")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::CURRENT), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setComment"), account1.setComment(QStringLiteral("bla bla")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setMaxLimitAmount"), account1.setMaxLimitAmount(15000.0), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setMinLimitAmount"), account1.setMinLimitAmount(-500.0), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:maxLimitAmountEnabled"), account1.maxLimitAmountEnabled(true), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:minLimitAmountEnabled"), account1.minLimitAmountEnabled(false), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account1.save(), true)
            SKGTEST(QStringLiteral("ACCOUNT:getNumber"), account1.getNumber(), QStringLiteral("12345P"))
            SKGTEST(QStringLiteral("ACCOUNT:getAgencyNumber"), account1.getAgencyNumber(), QStringLiteral("99999"))
            SKGTEST(QStringLiteral("ACCOUNT:getAgencyAddress"), account1.getAgencyAddress(), QStringLiteral("10 rue Albert CAMUS\n31000 TOULOUSE"))
            SKGTEST(QStringLiteral("ACCOUNT:getMaxLimitAmount"), account1.getMaxLimitAmount(), 15000.0)
            SKGTEST(QStringLiteral("ACCOUNT:getMinLimitAmount"), account1.getMinLimitAmount(), -500.0)

            SKGTESTERROR(QStringLiteral("ACCOUNT:setMaxLimitAmount"), account1.setMaxLimitAmount(-1000.0), true)
            SKGTEST(QStringLiteral("ACCOUNT:getMaxLimitAmount"), account1.getMaxLimitAmount(), -1000.0)
            SKGTEST(QStringLiteral("ACCOUNT:getMinLimitAmount"), account1.getMinLimitAmount(), -1000.0)

            SKGTESTBOOL("ACCOUNT:isMaxLimitAmountEnabled", account1.isMaxLimitAmountEnabled(), true)
            SKGTESTBOOL("ACCOUNT:isMinLimitAmountEnabled", account1.isMinLimitAmountEnabled(), false)

            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::CURRENT))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::CREDITCARD), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::CREDITCARD))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::ASSETS), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::ASSETS))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::INVESTMENT), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::INVESTMENT))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::WALLET), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::WALLET))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::LOAN), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::LOAN))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::SAVING), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::SAVING))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::OTHER), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::OTHER))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setType"), account1.setType(SKGAccountObject::PENSION), true)
            SKGTEST(QStringLiteral("ACCOUNT:getType"), static_cast<unsigned int>(account1.getType()), static_cast<unsigned int>(SKGAccountObject::PENSION))

            SKGTEST(QStringLiteral("ACCOUNT:getComment"), account1.getComment(), QStringLiteral("bla bla"))

            QDate n = QDate::currentDate();
            SKGTESTERROR(QStringLiteral("ACCOUNT:setReconciliationDate"), account1.setReconciliationDate(n), true)
            SKGTEST(QStringLiteral("ACCOUNT:getReconciliationDate"),
                    SKGServices::dateToSqlString(account1.getReconciliationDate()),
                    SKGServices::dateToSqlString(n))
            SKGTESTERROR(QStringLiteral("ACCOUNT:setReconciliationBalance"), account1.setReconciliationBalance(125), true)
            SKGTEST(QStringLiteral("ACCOUNT:getReconciliationBalance"),
                    SKGServices::doubleToString(account1.getReconciliationBalance()),
                    SKGServices::doubleToString(125))

            SKGTESTERROR(QStringLiteral("ACCOUNT:setInitialBalance"), account1.setInitialBalance(125, unit_euro), true)
            double init = 0;
            SKGUnitObject unit;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getInitialBalance"), account1.getInitialBalance(init, unit), true)
            SKGTESTBOOL("ACCOUNT:getInitialBalance", (unit == unit_euro), true)
            SKGTEST(QStringLiteral("ACCOUNT:getInitialBalance"), init, 125)

            SKGAccountObject account11(account1); // For coverage
            SKGTESTBOOL("BANK:comparison", (account11 == account1), true)
            SKGAccountObject account12(static_cast<SKGNamedObject>(account1)); // For coverage
            SKGTESTBOOL("BANK:comparison", (account12 == account1), true)
            SKGAccountObject account13(static_cast<SKGObjectBase>(account1)); // For coverage
            SKGTESTBOOL("BANK:comparison", (account13 == account1), true)

            SKGAccountObject account14(SKGObjectBase(account1.getDocument(), QStringLiteral("xxx"), account1.getID())); // For coverage
            SKGAccountObject account15(SKGNamedObject(account1.getDocument(), QStringLiteral("xxx"), account1.getID())); // For coverage

            // Creation bank2
            SKGBankObject bank2(&document1);
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank2.setName(QStringLiteral("NEF")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank2.save(), true)

            // Creation account2
            SKGAccountObject account2;
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank2.addAccount(account2), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account2.setName(QStringLiteral("Courant steph")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account2.setNumber(QStringLiteral("98765A")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account2.save(false), false)

            SKGAccountObject account3;
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank2.addAccount(account3), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account3.setName(QStringLiteral("Courant vero")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setNumber"), account3.setNumber(QStringLiteral("98765A")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account3.save(), true)

            QStringList oResult;
            SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"), document1.getDistinctValues(QStringLiteral("bank"), QStringLiteral("t_name"), oResult), true)
            SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 2)

            SKGTESTERROR(QStringLiteral("BANK:getDistinctValues"),
                         document1.getDistinctValues(QStringLiteral("account"), QStringLiteral("t_name"), oResult),
                         true)
            SKGTEST(QStringLiteral("BANK:oResult.size"), oResult.size(), 2)

            SKGObjectBase::SKGListSKGObjectBase oAccountList;
            SKGTESTERROR(QStringLiteral("BANK:getAccounts"), bank1.getAccounts(oAccountList), true)
            SKGTEST(QStringLiteral("ACCOUNT:count"), oAccountList.count(), 1)

            SKGTESTERROR(QStringLiteral("BANK:getAccounts"), bank2.getAccounts(oAccountList), true)
            SKGTEST(QStringLiteral("ACCOUNT:count"), oAccountList.count(), 1)

            // Modification account1
            SKGBankObject tmpBank;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getBank"), account3.getBank(tmpBank), true)
            SKGTESTBOOL("BANK:tmpBank==bank2", (tmpBank == bank2), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setBank"), account3.setBank(bank1), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:getBank"), account3.getBank(tmpBank), true)
            SKGTESTBOOL("BANK:tmpBank==bank2", (tmpBank == bank1), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account3.save(), true)

            SKGTESTERROR(QStringLiteral("BANK:getAccounts"), bank1.getAccounts(oAccountList), true)
            SKGTEST(QStringLiteral("ACCOUNT:count"), oAccountList.count(), 2)

            SKGTESTERROR(QStringLiteral("BANK:getAccounts"), bank2.getAccounts(oAccountList), true)
            SKGTEST(QStringLiteral("ACCOUNT:count"), oAccountList.count(), 0)

            SKGTESTBOOL("ACCOUNT:isBookmarked", account3.isBookmarked(), false)
            SKGTESTERROR(QStringLiteral("ACCOUNT:bookmark"), account3.bookmark(true), true)
            SKGTESTBOOL("ACCOUNT:isBookmarked", account3.isBookmarked(), true)

            SKGTESTBOOL("ACCOUNT:isClosed", account3.isClosed(), false)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setClosed"), account3.setClosed(true), true)
            SKGTESTBOOL("ACCOUNT:isClosed", account3.isClosed(), true)

            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account3.save(), true)

            // Merge
            SKGTESTERROR(QStringLiteral("ACCOUNT:merge"), account2.merge(account3, true), true)

            // interest
            SKGObjectBase::SKGListSKGObjectBase oInterestList;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getInterests"), account2.getInterests(oInterestList), true)

            SKGInterestObject oInterest;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getInterests"), account2.getInterest(QDate::currentDate(), oInterest), false)

            SKGObjectBase account4(&document1, QStringLiteral("account"), account1.getID());
            SKGTESTERROR(QStringLiteral("ACCOUNT:load"), account4.load(), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setAttribute"), account4.setAttribute(QStringLiteral("t_BANK"), QStringLiteral("bankname")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account4.save(), true)
            SKGTESTERROR(QStringLiteral("BANK:load"), bank1.load(), true)
            SKGTEST(QStringLiteral("BANK:getName"), bank1.getName(), QStringLiteral("bankname"))

            SKGTESTERROR(QStringLiteral("DOC:dump"), document1.dump(DUMPACCOUNT), true)
        }
    }

    // End test
    SKGENDTEST()
}
