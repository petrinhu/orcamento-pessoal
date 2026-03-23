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

            SKGSubOperationObject subop1;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op.addSubOperation(subop1), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop1.setQuantity(21), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop1.save(), true)
            SKGSubOperationObject subop2;
            SKGTESTERROR(QStringLiteral("OPE:addSubOperation"), op.addSubOperation(subop2), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:setQuantity"), subop2.setQuantity(1), true)
            SKGTESTERROR(QStringLiteral("SUBOPE:save"), subop2.save(), true)

            SKGTrackerObject tracker(&document1);
            SKGTrackerObject trackerCopy(tracker);
            SKGTrackerObject trackerCopy2(static_cast<SKGObjectBase>(tracker));
            SKGTESTERROR(QStringLiteral("REF:setName"), tracker.setName(QStringLiteral("tracker")), true)
            SKGTESTERROR(QStringLiteral("REF:setComment"), tracker.setComment(QStringLiteral("comment")), true)
            SKGTESTERROR(QStringLiteral("REF:save"), tracker.save(), true)
            SKGTEST(QStringLiteral("REF:getName"), tracker.getName(), QStringLiteral("tracker"))
            SKGTEST(QStringLiteral("REF:getComment"), tracker.getComment(), QStringLiteral("comment"))

            SKGTESTBOOL("REF:isClosed", tracker.isClosed(), false)
            SKGTESTERROR(QStringLiteral("OPE:setClosed"), tracker.setClosed(true), true)
            SKGTESTBOOL("REF:isClosed", tracker.isClosed(), true)
            SKGTESTERROR(QStringLiteral("OPE:setClosed"), tracker.setClosed(false), true)
            SKGTESTBOOL("REF:isClosed", tracker.isClosed(), false)

            SKGTESTERROR(QStringLiteral("OPE:setTracker"), subop1.setTracker(tracker), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), subop1.save(), true)

            SKGTESTERROR(QStringLiteral("OPE:setClosed"), tracker.setClosed(true), true)
            SKGTESTERROR(QStringLiteral("OPE:save"), tracker.save(), true)

            // BUG 250350 vvv
            SKGTESTERROR(QStringLiteral("OPE:setTracker"), subop1.setTracker(tracker), true)
            SKGTESTERROR(QStringLiteral("OPE:setTracker"), subop1.setTracker(tracker), true)

            SKGTrackerObject trackerclose(&document1);
            SKGTESTERROR(QStringLiteral("REF:setName"), trackerclose.setName(QStringLiteral("trackerclose")), true)
            SKGTESTERROR(QStringLiteral("OPE:setClosed"), trackerclose.setClosed(true), true)
            SKGTESTERROR(QStringLiteral("REF:save"), trackerclose.save(), true)
            SKGTESTERROR(QStringLiteral("OPE:setTracker"), subop1.setTracker(trackerclose), false)

            SKGTrackerObject trackeropen(&document1);
            SKGTESTERROR(QStringLiteral("REF:setName"), trackeropen.setName(QStringLiteral("trackeropen")), true)
            SKGTESTERROR(QStringLiteral("OPE:setClosed"), trackerclose.setClosed(true), true)
            SKGTESTERROR(QStringLiteral("OPE:setTracker"), subop1.setTracker(trackeropen), false)
            // BUG 250350 ^^^

            SKGTrackerObject tracker2;
            SKGTESTERROR(QStringLiteral("OPE:getTracker"), subop1.getTracker(tracker2), true)
            SKGTESTBOOL("OPE:compare", (tracker == tracker2), true)

            SKGTESTERROR(QStringLiteral("REF:load"), tracker.load(), true)
            SKGTEST(QStringLiteral("REF:getCurrentAmount"), tracker.getCurrentAmount(), 21)
            SKGObjectBase::SKGListSKGObjectBase oSubOperations;
            SKGTESTERROR(QStringLiteral("REF:getSubOperations"), tracker.getSubOperations(oSubOperations), true)
            SKGTEST(QStringLiteral("REF:compare"), oSubOperations.count(), 1)

            SKGTrackerObject track;
            SKGTESTERROR(QStringLiteral("REF:createTracker"), SKGTrackerObject::createTracker(&document1, QStringLiteral("track"), track, true), true)

            SKGTESTERROR(QStringLiteral("OPE:setClosed"), tracker.setClosed(false), true)
            SKGTESTERROR(QStringLiteral("REF:save"), tracker.save(), true)
            SKGTESTERROR(QStringLiteral("REF:merge"), track.merge(tracker), true)

            SKGTrackerObject trackBlank;
            SKGTESTERROR(QStringLiteral("REF:createTracker"), SKGTrackerObject::createTracker(&document1, QString(), trackBlank, true), true)
        }
    }

    // End test
    SKGENDTEST()
}
