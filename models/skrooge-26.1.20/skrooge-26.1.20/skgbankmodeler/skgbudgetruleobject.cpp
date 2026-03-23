/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGBudgetRuleObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbudgetruleobject.h"

#include <klocalizedstring.h>

#include "skgbudgetobject.h"
#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGBudgetRuleObject::SKGBudgetRuleObject()
    : SKGBudgetRuleObject(nullptr)
{
}

SKGBudgetRuleObject::SKGBudgetRuleObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_budgetrule"), iID)
{
}

SKGBudgetRuleObject::~SKGBudgetRuleObject() = default;

SKGBudgetRuleObject::SKGBudgetRuleObject(const SKGBudgetRuleObject &iObject) = default;

SKGBudgetRuleObject::SKGBudgetRuleObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("budgetrule")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_budgetrule"), iObject.getID());
    }
}

SKGBudgetRuleObject &SKGBudgetRuleObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGBudgetRuleObject::enableYearCondition(bool iEnable)
{
    return setAttribute(QStringLiteral("t_year_condition"), iEnable ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGBudgetRuleObject::isYearConditionEnabled() const
{
    return (getAttribute(QStringLiteral("t_year_condition")) == QStringLiteral("Y"));
}

SKGError SKGBudgetRuleObject::enableMonthCondition(bool iEnable)
{
    return setAttribute(QStringLiteral("t_month_condition"), iEnable ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGBudgetRuleObject::isMonthConditionEnabled() const
{
    return (getAttribute(QStringLiteral("t_month_condition")) == QStringLiteral("Y"));
}

SKGError SKGBudgetRuleObject::enableCategoryCondition(bool iEnable)
{
    return setAttribute(QStringLiteral("t_category_condition"), iEnable ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGBudgetRuleObject::isCategoryConditionEnabled() const
{
    return (getAttribute(QStringLiteral("t_category_condition")) == QStringLiteral("Y"));
}

SKGError SKGBudgetRuleObject::setBudgetYear(int iYear)
{
    return setAttribute(QStringLiteral("i_year"), SKGServices::intToString(iYear));
}

int SKGBudgetRuleObject::getBudgetYear() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_year")));
}

SKGError SKGBudgetRuleObject::setBudgetMonth(int iMonth)
{
    return setAttribute(QStringLiteral("i_month"), SKGServices::intToString(iMonth));
}

int SKGBudgetRuleObject::getBudgetMonth() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_month")));
}

SKGError SKGBudgetRuleObject::setBudgetCategory(const SKGCategoryObject &iCategory)
{
    return setAttribute(QStringLiteral("rc_category_id"), SKGServices::intToString(iCategory.getID()));
}

SKGError SKGBudgetRuleObject::getBudgetCategory(SKGCategoryObject &oCategory) const
{
    return getDocument()->getObject(QStringLiteral("v_category"), "id=" % getAttribute(QStringLiteral("rc_category_id")), oCategory);
}

SKGError SKGBudgetRuleObject::removeBudgetCategory()
{
    return setAttribute(QStringLiteral("rc_category_id"), QStringLiteral("0"));
}

SKGError SKGBudgetRuleObject::setCondition(SKGBudgetRuleObject::Condition iCondition)
{
    return setAttribute(QStringLiteral("i_condition"), SKGServices::intToString(static_cast<int>(iCondition)));
}

SKGBudgetRuleObject::Condition SKGBudgetRuleObject::getCondition() const
{
    return static_cast<SKGBudgetRuleObject::Condition>(SKGServices::stringToInt(getAttribute(QStringLiteral("i_condition"))));
}

SKGError SKGBudgetRuleObject::setQuantity(double iQuantity, bool iAbsolute)
{
    SKGError err = setAttribute(QStringLiteral("f_quantity"), SKGServices::doubleToString(iQuantity));
    IFOKDO(err, setAttribute(QStringLiteral("t_absolute"), iAbsolute ? QStringLiteral("Y") : QStringLiteral("N")))
    return err;
}

double SKGBudgetRuleObject::getQuantity() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_quantity")));
}

bool SKGBudgetRuleObject::isAbolute() const
{
    return getAttribute(QStringLiteral("t_absolute")) != QStringLiteral("N");
}

SKGError SKGBudgetRuleObject::enableCategoryChange(bool iEnable)
{
    return setAttribute(QStringLiteral("t_category_target"), iEnable ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGBudgetRuleObject::isCategoryChangeEnabled() const
{
    return getAttribute(QStringLiteral("t_category_target")) == QStringLiteral("Y");
}

SKGError SKGBudgetRuleObject::setTransfer(SKGBudgetRuleObject::Mode iMode, const SKGCategoryObject &iCategory)
{
    SKGError err = setAttribute(QStringLiteral("t_rule"), iMode == NEXT ? QStringLiteral("N") : iMode == CURRENT ? QStringLiteral("C") : QStringLiteral("Y"));
    IFOKDO(err, setAttribute(QStringLiteral("rc_category_id_target"), SKGServices::intToString(iCategory.getID())))
    return err;
}

SKGBudgetRuleObject::Mode SKGBudgetRuleObject::getTransferMode() const
{
    return (getAttribute(QStringLiteral("t_rule")) == QStringLiteral("N") ? NEXT
                                                                          : (getAttribute(QStringLiteral("t_rule")) == QStringLiteral("C") ? CURRENT : YEAR));
}

SKGError SKGBudgetRuleObject::getTransferCategory(SKGCategoryObject &oCategory) const
{
    return getDocument()->getObject(QStringLiteral("v_category"), "id=" % getAttribute(QStringLiteral("rc_category_id_target")), oCategory);
}

SKGError SKGBudgetRuleObject::processAllRules(SKGDocumentBank *iDocument)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (iDocument != nullptr) {
        // Initialize
        err = iDocument->executeSqliteOrder(QStringLiteral("UPDATE budget SET f_budgeted_modified=f_budgeted  WHERE f_budgeted_modified!=f_budgeted"));
        IFOKDO(err, iDocument->executeSqliteOrder(QStringLiteral("UPDATE budget SET f_transferred=0  WHERE f_transferred!=0")))

        // Get budgets ordered by date
        SKGObjectBase::SKGListSKGObjectBase budgets;
        IFOKDO(err,
               iDocument->getObjects(
                   QStringLiteral("vm_budget_tmp"),
                   QStringLiteral("length(t_RULES)>0 "
                                  "AND (t_PERIOD<=STRFTIME('%Y-%m', date('now', 'localtime')) OR t_PERIOD=STRFTIME('%Y', date('now', 'localtime'))) "
                                  "ORDER BY t_PERIOD, id"),
                   budgets));
        int nb = budgets.count();
        if (!err && nb > 0) {
            err = iDocument->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Apply rules"), nb);
            for (int i = 0; !err && i < nb; ++i) {
                SKGBudgetObject bud(budgets.at(i));
                err = bud.load(); // Reload to be sure that delta has been updated
                IFOKDO(err, bud.process())

                IFOKDO(err, iDocument->stepForward(i + 1))
            }

            IFOKDO(err, iDocument->setParameter(QStringLiteral("SKG_LAST_BUDGET_PROCESSING"), QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))))

            SKGENDTRANSACTION(iDocument, err)
        }
    }

    return err;
}

SKGError SKGBudgetRuleObject::setOrder(double iOrder)
{
    SKGError err;
    double order = iOrder;
    if (order == -1) {
        order = 1;
        SKGStringListList result;
        err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT max(f_sortorder) from budgetrule"), result);
        if (!err && result.count() == 2) {
            order = SKGServices::stringToDouble(result.at(1).at(0)) + 1;
        }
    }
    IFOKDO(err, setAttribute(QStringLiteral("f_sortorder"), SKGServices::doubleToString(order)))
    return err;
}

double SKGBudgetRuleObject::getOrder() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_sortorder")));
}
