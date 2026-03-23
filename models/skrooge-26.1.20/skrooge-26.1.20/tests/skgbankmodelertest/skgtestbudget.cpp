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
        // Import
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "skgtestbudget/budget.skg"), true)
        SKGError err;
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BUDGET_CREATION"), err)

            SKGTESTERROR(QStringLiteral("BUDGET.createAutomaticBudget"), SKGBudgetObject::createAutomaticBudget(&document1, 2010, 2010, true, true), true)
            SKGTESTERROR(QStringLiteral("BUDGET.balanceBudget"), SKGBudgetObject::balanceBudget(&document1, 2010), true)
        }
        {
            // Scope of the transaction
            SKGBEGINTRANSACTION(document1, QStringLiteral("BUDGETRULE_CREATION"), err)

            SKGBudgetRuleObject br(&document1);

            SKGTESTERROR(QStringLiteral("BUDGETRULE.enableYearCondition"), br.enableYearCondition(true), true)
            SKGTESTBOOL("BUDGETRULE.isYearConditionEnabled", br.isYearConditionEnabled(), true)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.enableYearCondition"), br.enableYearCondition(false), true)
            SKGTESTBOOL("BUDGETRULE.isYearConditionEnabled", br.isYearConditionEnabled(), false)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.setBudgetYear"), br.setBudgetYear(2010), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getBudgetYear"), br.getBudgetYear(), 2010)

            SKGTESTERROR(QStringLiteral("BUDGETRULE.enableMonthCondition"), br.enableMonthCondition(true), true)
            SKGTESTBOOL("BUDGETRULE.isMonthConditionEnabled", br.isMonthConditionEnabled(), true)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.enableMonthCondition"), br.enableMonthCondition(false), true)
            SKGTESTBOOL("BUDGETRULE.isMonthConditionEnabled", br.isMonthConditionEnabled(), false)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.setBudgetMonth"), br.setBudgetMonth(10), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getBudgetMonth"), br.getBudgetMonth(), 10)

            SKGTESTERROR(QStringLiteral("BUDGETRULE.setOrder"), br.setOrder(1.0), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getOrder"), br.getOrder(), 1.0)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.setOrder"), br.setOrder(-1), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getOrder"), br.getOrder(), 1.0)

            SKGTESTERROR(QStringLiteral("BUDGETRULE.enableCategoryCondition"), br.enableCategoryCondition(true), true)
            SKGTESTBOOL("BUDGETRULE.isCategoryConditionEnabled", br.isCategoryConditionEnabled(), true)
            SKGCategoryObject cat;
            SKGTESTERROR(QStringLiteral("BUDGETRULE.enableCategoryChange"), br.enableCategoryChange(br.isCategoryChangeEnabled()), true)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.getBudgetCategory"), br.getBudgetCategory(cat), false)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.createPathCategory"),
                         SKGCategoryObject::createPathCategory(&document1, QStringLiteral("category_55 > category_57"), cat),
                         true)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.removeBudgetCategory"), br.removeBudgetCategory(), true)
            SKGTESTERROR(QStringLiteral("BUDGETRULE.setBudgetCategory"), br.setBudgetCategory(cat), true)

            SKGTESTERROR(QStringLiteral("BUDGETRULE.setCondition"), br.setCondition(SKGBudgetRuleObject::NEGATIVE), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getCondition"),
                    static_cast<unsigned int>(br.getCondition()),
                    static_cast<unsigned int>(SKGBudgetRuleObject::NEGATIVE))
            SKGTESTERROR(QStringLiteral("BUDGETRULE.setCondition"), br.setCondition(SKGBudgetRuleObject::POSITIVE), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getCondition"),
                    static_cast<unsigned int>(br.getCondition()),
                    static_cast<unsigned int>(SKGBudgetRuleObject::POSITIVE))
            SKGTESTERROR(QStringLiteral("BUDGETRULE.setCondition"), br.setCondition(SKGBudgetRuleObject::ALL), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getCondition"),
                    static_cast<unsigned int>(br.getCondition()),
                    static_cast<unsigned int>(SKGBudgetRuleObject::ALL))

            SKGTESTERROR(QStringLiteral("BUDGETRULE.setQuantity"), br.setQuantity(100, false), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getQuantity"), br.getQuantity(), 100)
            SKGTESTBOOL("BUDGETRULE.isAbolute", br.isAbolute(), false)

            SKGTESTERROR(QStringLiteral("BUDGETRULE.setTransfer"), br.setTransfer(SKGBudgetRuleObject::CURRENT), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getTransferMode"),
                    static_cast<unsigned int>(br.getTransferMode()),
                    static_cast<unsigned int>(SKGBudgetRuleObject::CURRENT))
            SKGTESTERROR(QStringLiteral("BUDGETRULE.setTransfer"), br.setTransfer(SKGBudgetRuleObject::NEXT), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getTransferMode"),
                    static_cast<unsigned int>(br.getTransferMode()),
                    static_cast<unsigned int>(SKGBudgetRuleObject::NEXT))

            SKGTESTERROR(QStringLiteral("BUDGETRULE.save"), br.save(), true)

            SKGBudgetRuleObject br2 = br;
            SKGBudgetRuleObject br3(br);
            SKGBudgetRuleObject br4(static_cast<SKGObjectBase>(br));
            SKGBudgetRuleObject br5(SKGObjectBase(&document1, QStringLiteral("xxx"), br.getID()));

            SKGTESTERROR(QStringLiteral("BUDGETRULE.processAllRules"), SKGBudgetRuleObject::processAllRules(&document1), true)

            SKGTESTERROR(QStringLiteral("BUDGETRULE.setTransfer"), br.setTransfer(SKGBudgetRuleObject::YEAR), true)
            SKGTEST(QStringLiteral("BUDGETRULE.getTransferMode"),
                    static_cast<unsigned int>(br.getTransferMode()),
                    static_cast<unsigned int>(SKGBudgetRuleObject::YEAR))
            SKGTESTERROR(QStringLiteral("BUDGETRULE.save"), br.save(), true)

            SKGTESTERROR(QStringLiteral("BUDGETRULE.processAllRules"), SKGBudgetRuleObject::processAllRules(&document1), true)
        }
        SKGTESTERROR(QStringLiteral("document1.saveAs()"),
                     document1.saveAs(SKGTest::getTestPath(QStringLiteral("OUT")) % "skgtestbudget/budget.skg", true),
                     true)
    }

    // ============================================================================
    {
        // Import
        SKGDocumentBank document1;
        SKGTESTERROR(QStringLiteral("document1.load()"), document1.load(SKGTest::getTestPath(QStringLiteral("IN")) % "skgtestbudget/320323.skg"), true)
        SKGError err;
        {// Scope of the transaction
         SKGBEGINTRANSACTION(document1, QStringLiteral("BUDGET_PROCESS"), err)

             SKGTESTERROR(QStringLiteral("BUDGETRULE.processAllRules"), SKGBudgetRuleObject::processAllRules(&document1), true)} document1.dump(DUMPBUDGET);
        bool check = false;
        document1.existObjects(QStringLiteral("v_budget_display"),
                               QStringLiteral("t_PERIOD='2013-02' AND t_CATEGORY='Alimentation' AND f_budgeted_modified=300"),
                               check);
        SKGTESTBOOL("BUDGETRULE.Alimentation 2013-02 300", check, true)

        document1.existObjects(QStringLiteral("v_budget_display"),
                               QStringLiteral("t_PERIOD='2013-02' AND t_CATEGORY='Loisirs' AND f_budgeted_modified=2100"),
                               check);
        SKGTESTBOOL("BUDGETRULE.Loisirs 2013-02 2100", check, true)

        SKGObjectBase bo;
        SKGTESTERROR(QStringLiteral("document1.getObject()"),
                     document1.getObject(QStringLiteral("v_budget_display"),
                                         QStringLiteral("t_PERIOD='2013-02' AND t_CATEGORY='Loisirs' AND f_budgeted_modified=2100"),
                                         bo),
                     true)
        SKGBudgetObject b(bo);
        SKGTEST(QStringLiteral("BUDGET.getBudgetedAmount"), b.getBudgetedAmount(), 300)
        SKGCategoryObject cat;
        SKGTESTERROR(QStringLiteral("BUDGET.getCategory"), b.getCategory(cat), true)
        SKGTEST(QStringLiteral("BUDGET.getModificationReasons"),
                b.getModificationReasons().remove(QLatin1Char(',')),
                QStringLiteral("Transfer of -1800 from ' 2013-01 2000.0' to 'Loisirs 2013-02 300.0' due to the rule 'All 100.0% Next Loisirs'"))
        SKGTESTERROR(QStringLiteral("BUDGET.removeCategory"), b.removeCategory(), true)
        SKGTESTERROR(QStringLiteral("BUDGET.enableSubCategoriesInclusion"), b.enableSubCategoriesInclusion(true), true)
        SKGTESTBOOL(QStringLiteral("BUDGET.isSubCategoriesInclusionEnabled"), b.isSubCategoriesInclusionEnabled(), true)
        SKGTESTERROR(QStringLiteral("BUDGET.enableSubCategoriesInclusion"), b.enableSubCategoriesInclusion(false), true)
        SKGTESTBOOL(QStringLiteral("BUDGET.isSubCategoriesInclusionEnabled"), b.isSubCategoriesInclusionEnabled(), false)

        SKGBudgetObject bu;
    }

    // End test
    SKGENDTEST()
}
