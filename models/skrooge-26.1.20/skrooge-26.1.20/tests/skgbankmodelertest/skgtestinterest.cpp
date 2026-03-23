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
        // Test interest document
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T1"), err)

            // Creation bank1
            SKGBankObject bank1(&document1);
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank1.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank1.save(), true)

            // Creation account1
            SKGAccountObject account1;
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank1.addAccount(account1), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account1.setName(QStringLiteral("Livre A")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account1.save(), true)

            // Add interest parameters
            SKGInterestObject interest1;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addInterest"), account1.addInterest(interest1), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setDate"), interest1.setDate(QDate::currentDate()), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setRate"), interest1.setRate(1.25), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setIncomeValueDateMode"), interest1.setIncomeValueDateMode(SKGInterestObject::J1), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setExpenditueValueDateMode"), interest1.setExpenditueValueDateMode(SKGInterestObject::J2), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setInterestComputationMode"), interest1.setInterestComputationMode(SKGInterestObject::DAYS360), true)

            SKGTEST(QStringLiteral("INTEREST:getRate"), interest1.getRate(), 1.25)
            SKGTEST(QStringLiteral("INTEREST:getIncomeValueDateMode"),
                    static_cast<unsigned int>(interest1.getIncomeValueDateMode()),
                    static_cast<unsigned int>(SKGInterestObject::J1))
            SKGTEST(QStringLiteral("INTEREST:getExpenditueValueDateMode"),
                    static_cast<unsigned int>(interest1.getExpenditueValueDateMode()),
                    static_cast<unsigned int>(SKGInterestObject::J2))
            SKGTEST(QStringLiteral("INTEREST:getInterestComputationMode"),
                    static_cast<unsigned int>(interest1.getInterestComputationMode()),
                    static_cast<unsigned int>(SKGInterestObject::DAYS360))
            SKGTESTERROR(QStringLiteral("INTEREST:save"), interest1.save(), true)

            SKGInterestObject interest11(static_cast<SKGObjectBase>(interest1));
            SKGInterestObject interest12 = interest1;
            SKGInterestObject interest13(SKGObjectBase(&document1, QStringLiteral("xxx"), interest1.getID()));
            SKGInterestObject interest14;
            interest14 = static_cast<SKGObjectBase>(interest1);
        }

        // Test interest computation
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)
        {
            QDate firstday = QDate::currentDate();
            firstday = firstday.addMonths(1 - firstday.month());
            firstday = firstday.addDays(1 - firstday.day());

            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BANK_T1"), err)

            // Creation bank1
            SKGBankObject bank1(&document1);
            SKGTESTERROR(QStringLiteral("BANK:setName"), bank1.setName(QStringLiteral("CREDIT COOP")), true)
            SKGTESTERROR(QStringLiteral("BANK:save"), bank1.save(), true)

            // Creation account1
            SKGAccountObject account1;
            SKGTESTERROR(QStringLiteral("BANK:addAccount"), bank1.addAccount(account1), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:setName"), account1.setName(QStringLiteral("Livre A")), true)
            SKGTESTERROR(QStringLiteral("ACCOUNT:save"), account1.save(), true)

            // Creation unit
            SKGUnitObject unit_euro(&document1);
            SKGTESTERROR(QStringLiteral("UNIT:setName"), unit_euro.setName(QStringLiteral("euro")), true)
            SKGTESTERROR(QStringLiteral("UNIT:save"), unit_euro.save(), true)

            // Creation unitvalue
            SKGUnitValueObject unit_euro_val1;
            SKGTESTERROR(QStringLiteral("UNIT:addUnitValue"), unit_euro.addUnitValue(unit_euro_val1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setQuantity"), unit_euro_val1.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:setDate"), unit_euro_val1.setDate(firstday), true)
            SKGTESTERROR(QStringLiteral("UNITVALUE:save"), unit_euro_val1.save(), true)

            // Add interest parameters
            SKGInterestObject interest1;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addInterest"), account1.addInterest(interest1), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setDate"), interest1.setDate(firstday.addDays(-10)), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setRate"), interest1.setRate(4), true)
            SKGTESTERROR(QStringLiteral("INTEREST:save"), interest1.save(), true)

            SKGInterestObject interest2;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addInterest"), account1.addInterest(interest2), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setDate"), interest2.setDate(firstday.addMonths(1)), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setRate"), interest2.setRate(2.5), true)
            SKGTESTERROR(QStringLiteral("INTEREST:save"), interest2.save(), true)

            SKGInterestObject interest3;
            SKGTESTERROR(QStringLiteral("ACCOUNT:addInterest"), account1.addInterest(interest3), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setDate"), interest3.setDate(firstday.addMonths(4)), true)
            SKGTESTERROR(QStringLiteral("INTEREST:setRate"), interest3.setRate(1.75), true)
            SKGTESTERROR(QStringLiteral("INTEREST:save"), interest3.save(), true)

            // Add transactions
            {
                SKGOperationObject op_1;
                SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(op_1), true)
                SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(firstday.addDays(-1)), true)
                SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.setUnit(unit_euro), true)
                SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
                SKGSubOperationObject subop_1;
                SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), true)
                SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop_1.setQuantity(100), true)
                SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)
            }
            {
                SKGOperationObject op_1;
                SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(op_1), true)
                SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(firstday.addDays(14)), true)
                SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.setUnit(unit_euro), true)
                SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
                SKGSubOperationObject subop_1;
                SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), true)
                SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop_1.setQuantity(100), true)
                SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)
            }
            {
                SKGOperationObject op_1;
                SKGTESTERROR(QStringLiteral("ACCOUNT:addOperation"), account1.addOperation(op_1), true)
                SKGTESTERROR(QStringLiteral("OPE:setDate"), op_1.setDate(firstday.addMonths(5).addDays(28)), true)
                SKGTESTERROR(QStringLiteral("OPE:setUnit"), op_1.setUnit(unit_euro), true)
                SKGTESTERROR(QStringLiteral("OPE:save"), op_1.save(), true)
                SKGSubOperationObject subop_1;
                SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op_1.addSubOperation(subop_1), true)
                SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop_1.setQuantity(-100), true)
                SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop_1.save(), true)
            }

            SKGAccountObject::SKGInterestItemList oInterestList;
            double oInterests = 0;
            SKGTESTERROR(QStringLiteral("ACCOUNT:getInterestItems"), account1.getInterestItems(oInterestList, oInterests), true)

            int nb = oInterestList.count();
            for (int i = 0; i < nb; ++i) {
                SKGAccountObject::SKGInterestItem item = oInterestList.at(i);
                SKGTRACE << "[" << item.object.getDisplayName() << "]: " << SKGServices::dateToSqlString(item.date) << " "
                         << SKGServices::dateToSqlString(item.valueDate) << " " << item.amount << " " << item.coef << " " << item.rate << " "
                         << item.annualInterest << " " << item.accruedInterest << Qt::endl;
            }
            SKGTEST(QStringLiteral("INTEREST:oInterestList.count"), oInterestList.count(), 5)
            SKGTESTBOOL("INTEREST:oInterestList.at(1).date", (oInterestList.at(0).date == firstday), true)
            SKGTESTBOOL("INTEREST:oInterestList.at(2).date", (oInterestList.at(1).date == firstday.addDays(14)), true)
            SKGTESTBOOL("INTEREST:oInterestList.at(3).date", (oInterestList.at(2).date == firstday.addMonths(1)), true)
            SKGTESTBOOL("INTEREST:oInterestList.at(6).date", (oInterestList.at(3).date == firstday.addMonths(4)), true)
            SKGTESTBOOL("INTEREST:oInterestList.at(7).date", (oInterestList.at(4).date == firstday.addMonths(5).addDays(28)), true)
            SKGTEST(QStringLiteral("INTEREST:oInterests"), SKGServices::doubleToString(oInterests), QStringLiteral("3.135416667"))
        }
    }

    // End test
    SKGENDTEST()
}
