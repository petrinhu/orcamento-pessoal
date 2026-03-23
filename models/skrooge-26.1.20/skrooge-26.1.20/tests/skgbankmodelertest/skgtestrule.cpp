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
#include "skgimportexportmanager.h"
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

    // ============================================================================
    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.initialize()"), document1.initialize(), true)

        SKGError err;
        // Initialisation document
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1, QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/385366.ofx"));
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }

        // Check static methods
        {
            auto list = SKGRuleObject::getListOfOperators(SKGServices::TEXT, SKGRuleObject::SEARCH);
            list << SKGRuleObject::getListOfOperators(SKGServices::TEXT, SKGRuleObject::UPDATE);
            list << SKGRuleObject::getListOfOperators(SKGServices::INTEGER, SKGRuleObject::SEARCH);
            list << SKGRuleObject::getListOfOperators(SKGServices::INTEGER, SKGRuleObject::UPDATE);
            list << SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::SEARCH);
            list << SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::UPDATE);
            list << SKGRuleObject::getListOfOperators(SKGServices::DATE, SKGRuleObject::SEARCH);
            list << SKGRuleObject::getListOfOperators(SKGServices::DATE, SKGRuleObject::UPDATE);
            list << SKGRuleObject::getListOfOperators(SKGServices::TEXT, SKGRuleObject::ALARM);
            list << SKGRuleObject::getListOfOperators(SKGServices::TEXT, SKGRuleObject::APPLYTEMPLATE);
            for (const auto &i : std::as_const(list)) {
                SKGRuleObject::getDisplayForOperator(i, QString(), QString(), QString());
                SKGRuleObject::getToolTipForOperator(i);
            }
            SKGRuleObject::getListOfOperators(SKGServices::INTEGER, SKGRuleObject::SEARCH);
            SKGRuleObject::getListOfOperators(SKGServices::FLOAT, SKGRuleObject::SEARCH);
            SKGRuleObject::getListOfOperators(SKGServices::DATE, SKGRuleObject::SEARCH);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::SEARCH);
            SKGRuleObject::getListOfOperators(SKGServices::TRISTATE, SKGRuleObject::SEARCH);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::SEARCH);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::SEARCH);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::SEARCH);

            SKGRuleObject::getListOfOperators(SKGServices::TEXT, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::INTEGER, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::FLOAT, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::DATE, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::TRISTATE, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::UPDATE);
            SKGRuleObject::getListOfOperators(SKGServices::BOOL, SKGRuleObject::UPDATE);

            SKGRuleObject::getListOfOperators(SKGServices::TEXT, SKGRuleObject::ALARM);

            SKGRuleObject::getListOfOperators(SKGServices::TEXT, SKGRuleObject::APPLYTEMPLATE);
        }

        // Rule creation
        SKGRuleObject rule1;
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("RULE_1"), err)

            // Create rule
            rule1 = SKGRuleObject(&document1);
            QString searchDef = QStringLiteral(
                "<element> <!--OR-->"
                "<element>  <!--AND-->"
                "<element attribute=\"t_comment\" operator=\"#ATT#='#V1S#'\" value=\"REINVESTMENT\"/>"
                "</element>"
                "</element>");
            SKGTESTERROR(QStringLiteral("RULE:setXMLSearchDefinition"), rule1.setXMLSearchDefinition(searchDef), true)
            SKGTEST(QStringLiteral("RULE:getXMLSearchDefinition"), rule1.getXMLSearchDefinition(), searchDef)

            QString actionDef = QStringLiteral(
                "<element> <!--OR-->"
                "<element>  <!--AND-->"
                "<element attribute=\"t_mode\" operator=\"#ATT#='#V1#'\" value=\"modified\" />"
                "</element>"
                "</element>");
            SKGTESTERROR(QStringLiteral("RULE:setXMLActionDefinition"), rule1.setXMLActionDefinition(actionDef), true)
            SKGTEST(QStringLiteral("RULE:getXMLActionDefinition"), rule1.getXMLActionDefinition(), actionDef)

            SKGTESTERROR(QStringLiteral("RULE:setSearchDescription"), rule1.setSearchDescription(QStringLiteral("search description")), true)
            SKGTEST(QStringLiteral("RULE:getSearchDescription"), rule1.getSearchDescription(), QStringLiteral("search description"))
            SKGTESTERROR(QStringLiteral("RULE:setActionDescription"), rule1.setActionDescription(QStringLiteral("action description")), true)
            SKGTEST(QStringLiteral("RULE:getActionDescription"), rule1.getActionDescription(), QStringLiteral("action description"))

            SKGTESTERROR(QStringLiteral("RULE:setActionType"), rule1.setActionType(SKGRuleObject::SEARCH), true)
            SKGTESTERROR(QStringLiteral("RULE:setActionType"), rule1.setActionType(SKGRuleObject::APPLYTEMPLATE), true)
            SKGTESTERROR(QStringLiteral("RULE:setActionType"), rule1.setActionType(SKGRuleObject::UPDATE), true)
            SKGTEST(QStringLiteral("RULE:getActionType"), static_cast<unsigned int>(rule1.getActionType()), static_cast<unsigned int>(SKGRuleObject::UPDATE))

            SKGTESTERROR(QStringLiteral("RULE:setOrder"), rule1.setOrder(-1), true)
            SKGTEST(QStringLiteral("RULE:getOrder"), rule1.getOrder(), 1)

            SKGTESTBOOL("RULE:setOrder", rule1.isBookmarked(), false)
            SKGTESTERROR(QStringLiteral("RULE:getOrder"), rule1.bookmark(true), true)
            SKGTESTBOOL("RULE:setOrder", rule1.isBookmarked(), true)

            SKGTESTERROR(QStringLiteral("UNIT:save"), rule1.save(), true)
            SKGTESTERROR(QStringLiteral("UNIT:load"), rule1.load(), true)

            rule1.getDisplayName();

            SKGRuleObject rule1bis = SKGRuleObject(rule1);
            SKGRuleObject rule1ter = SKGRuleObject(static_cast<SKGObjectBase>(rule1));
            SKGRuleObject rule4;
            rule4 = static_cast<SKGObjectBase>(rule1);
            SKGRuleObject rule5(SKGObjectBase(&document1, QStringLiteral("xxx"), rule1.getID()));
        }

        // Rule creation (ALARM)
        SKGRuleObject alarm1;
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("ALARM_1"), err)

            // Create rule
            alarm1 = SKGRuleObject(&document1);
            QString searchDef = QStringLiteral(
                "<element> <!--OR-->"
                "<element>  <!--AND-->"
                "<element attribute=\"t_comment\" operator=\"#ATT#='#V1S#'\" value=\"REINVESTMENT\"/>"
                "</element>"
                "</element>");
            SKGTESTERROR(QStringLiteral("RULE:setXMLSearchDefinition"), alarm1.setXMLSearchDefinition(searchDef), true)
            SKGTEST(QStringLiteral("RULE:getXMLSearchDefinition"), alarm1.getXMLSearchDefinition(), searchDef)

            QString actionDef = QStringLiteral(
                "<element> <!--OR-->"
                "<element>  <!--AND-->"
                "<element attribute=\"f_REALCURRENTAMOUNT\" operator=\"ABS(TOTAL(#ATT#))#OP##V1#,ABS(TOTAL(#ATT#)), #V1#, '#V2S#'\" value=\"100\" "
                "value2=\"Take care!\"operator2=\">=\"/>"
                "</element>"
                "</element>");
            SKGTESTERROR(QStringLiteral("RULE:setXMLActionDefinition"), alarm1.setXMLActionDefinition(actionDef), true)
            SKGTEST(QStringLiteral("RULE:getXMLActionDefinition"), alarm1.getXMLActionDefinition(), actionDef)

            SKGTESTERROR(QStringLiteral("RULE:setSearchDescription"), alarm1.setSearchDescription(QStringLiteral("search description")), true)
            SKGTEST(QStringLiteral("RULE:getSearchDescription"), alarm1.getSearchDescription(), QStringLiteral("search description"))
            SKGTESTERROR(QStringLiteral("RULE:setActionDescription"), alarm1.setActionDescription(QStringLiteral("action description")), true)
            SKGTEST(QStringLiteral("RULE:getActionDescription"), alarm1.getActionDescription(), QStringLiteral("action description"))

            SKGTESTERROR(QStringLiteral("RULE:setActionType"), alarm1.setActionType(SKGRuleObject::ALARM), true)
            SKGTEST(QStringLiteral("RULE:getActionType"), static_cast<unsigned int>(alarm1.getActionType()), static_cast<unsigned int>(SKGRuleObject::ALARM))

            SKGTESTERROR(QStringLiteral("RULE:setOrder"), alarm1.setOrder(-1), true)
            SKGTEST(QStringLiteral("RULE:getOrder"), alarm1.getOrder(), 2)

            SKGTESTERROR(QStringLiteral("UNIT:save"), alarm1.save(), true)
            SKGTESTERROR(QStringLiteral("UNIT:load"), alarm1.load(), true)

            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), alarm1.getAlarmInfo().Message, QStringLiteral("Take care!"))
            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), static_cast<unsigned int>(alarm1.getAlarmInfo().Raised), static_cast<unsigned int>(true))
            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), alarm1.getAlarmInfo().Amount, 121.41)
            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), alarm1.getAlarmInfo().Limit, 100)
        }

        // Process rule1
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("PROCESS_1"), err)
            SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(), true)
            SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(SKGRuleObject::IMPORTING), true)
            SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(SKGRuleObject::IMPORTEDNOTVALIDATE), true)
            SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(SKGRuleObject::IMPORTED), true)
            SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(SKGRuleObject::NOTCHECKED), true)
        }

        // Rule creation (BUG:345974)
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("ALARM_2"), err)

            // Create rule
            alarm1 = SKGRuleObject(&document1);
            QString searchDef = QStringLiteral(
                "<element> <!--OR-->"
                "<element>  <!--AND-->"
                "<element attribute=\"t_comment\" operator=\"#ATT#='#V1S#'\" value=\"REINVESTMENT\"/>"
                "</element>"
                "</element>");
            SKGTESTERROR(QStringLiteral("RULE:setXMLSearchDefinition"), alarm1.setXMLSearchDefinition(searchDef), true)
            SKGTEST(QStringLiteral("RULE:getXMLSearchDefinition"), alarm1.getXMLSearchDefinition(), searchDef)

            QString actionDef = QStringLiteral(
                "<element> <!--OR-->"
                "<element>  <!--AND-->"
                "<element attribute=\"f_REALCURRENTAMOUNT\" operator=\"ABS(TOTAL(#ATT#))#OP##V1#,ABS(TOTAL(#ATT#)), #V1#, '#V2S#'\" value=\"200\" "
                "value2=\"Take care!\"operator2=\">=\"/>"
                "</element>"
                "</element>");
            SKGTESTERROR(QStringLiteral("RULE:setXMLActionDefinition"), alarm1.setXMLActionDefinition(actionDef), true)
            SKGTEST(QStringLiteral("RULE:getXMLActionDefinition"), alarm1.getXMLActionDefinition(), actionDef)

            SKGTESTERROR(QStringLiteral("RULE:setSearchDescription"), alarm1.setSearchDescription(QStringLiteral("search description")), true)
            SKGTEST(QStringLiteral("RULE:getSearchDescription"), alarm1.getSearchDescription(), QStringLiteral("search description"))
            SKGTESTERROR(QStringLiteral("RULE:setActionDescription"), alarm1.setActionDescription(QStringLiteral("action description")), true)
            SKGTEST(QStringLiteral("RULE:getActionDescription"), alarm1.getActionDescription(), QStringLiteral("action description"))

            SKGTESTERROR(QStringLiteral("RULE:setActionType"), alarm1.setActionType(SKGRuleObject::ALARM), true)
            SKGTEST(QStringLiteral("RULE:getActionType"), static_cast<unsigned int>(alarm1.getActionType()), static_cast<unsigned int>(SKGRuleObject::ALARM))

            SKGTESTERROR(QStringLiteral("UNIT:save"), alarm1.save(), true)
            SKGTESTERROR(QStringLiteral("UNIT:load"), alarm1.load(), true)

            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), alarm1.getAlarmInfo().Message, QStringLiteral("Take care!"))
            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), static_cast<unsigned int>(alarm1.getAlarmInfo().Raised), static_cast<unsigned int>(false))
            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), alarm1.getAlarmInfo().Amount, 121.41)
            SKGTEST(QStringLiteral("RULE:getAlarmInfo"), alarm1.getAlarmInfo().Limit, 200)
        }

        // Process rule1
        {SKGBEGINTRANSACTION(document1, QStringLiteral("PROCESS_1"), err) SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(), true)
             SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(SKGRuleObject::IMPORTING), true)
                 SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(SKGRuleObject::IMPORTEDNOTVALIDATE), true)
                     SKGTESTERROR(QStringLiteral("RULE:execute"), rule1.execute(SKGRuleObject::IMPORTED), true)}

        document1.dump(DUMPOPERATION | DUMPACCOUNT);
    }

    // ============================================================================
    {
        // Test import OFX skrooge
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "skgtestrule/file.skg"), true)

        // Process rules
        SKGError err;
        {
            SKGBEGINTRANSACTION(document1, QStringLiteral("PROCESS_1"), err)
            SKGObjectBase::SKGListSKGObjectBase rules;
            SKGTESTERROR(QStringLiteral("RULE:execute"), document1.getObjects(QStringLiteral("rule"), QString(), rules), true)
            for (int i = 0; i < rules.count(); ++i) {
                SKGRuleObject rule(rules.at(i));
                SKGTESTERROR(QStringLiteral("RULE:execute"), rule.execute(), true)
            }
        }

        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("IMPORT_OFX"), err)
            SKGImportExportManager imp1(&document1,
                                        QUrl::fromLocalFile(SKGTest::getTestPath(QStringLiteral("IN")) % "/skgtestimportofx/ofx_spec201_stmtrs_example.ofx"));
            imp1.setAutomaticApplyRules(true);
            SKGTESTERROR(QStringLiteral("imp1.importFile"), imp1.importFile(), true)
        }
    }

    // End test
    SKGENDTEST()
}
