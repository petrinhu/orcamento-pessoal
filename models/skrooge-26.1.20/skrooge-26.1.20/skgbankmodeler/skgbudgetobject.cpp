/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGBudgetObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbudgetobject.h"

#include <klocalizedstring.h>

#include "skgbudgetruleobject.h"
#include "skgcategoryobject.h"
#include "skgdefine.h"
#include "skgdocumentbank.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGBudgetObject::SKGBudgetObject()
    : SKGBudgetObject(nullptr)
{
}

SKGBudgetObject::SKGBudgetObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_budget"), iID)
{
}

SKGBudgetObject::~SKGBudgetObject() = default;

SKGBudgetObject::SKGBudgetObject(const SKGBudgetObject &iObject) = default;

SKGBudgetObject::SKGBudgetObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("budget")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_budget"), iObject.getID());
    }
}

SKGBudgetObject &SKGBudgetObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

QString SKGBudgetObject::getWhereclauseId() const
{
    // Could we use the id
    QString output = SKGObjectBase::getWhereclauseId();
    if (output.isEmpty()) {
        QString y = getAttribute(QStringLiteral("i_year"));
        if (!y.isEmpty()) {
            output = "i_year=" % y;
        }

        QString m = getAttribute(QStringLiteral("i_month"));
        if (!m.isEmpty()) {
            if (!output.isEmpty()) {
                output = output % " AND ";
            }
            output = output % "i_month=" % m;
        }

        QString r = getAttribute(QStringLiteral("rc_category_id"));
        if (!output.isEmpty()) {
            output = output % " AND ";
        }
        output = output % "rc_category_id=" % (r.isEmpty() ? QStringLiteral("0") : std::move(r));
    }
    return output;
}

SKGError SKGBudgetObject::setBudgetedAmount(double iAmount)
{
    SKGError err = setAttribute(QStringLiteral("f_budgeted"), SKGServices::doubleToString(iAmount));
    IFOKDO(err, setAttribute(QStringLiteral("f_budgeted_modified"), SKGServices::doubleToString(iAmount)))
    IFOKDO(err, setAttribute(QStringLiteral("t_modification_reasons"), QString()))
    return err;
}

double SKGBudgetObject::getBudgetedAmount() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_budgeted")));
}

double SKGBudgetObject::getBudgetedModifiedAmount() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_budgeted_modified")));
}

QString SKGBudgetObject::getModificationReasons() const
{
    return getAttribute(QStringLiteral("t_modification_reasons"));
}

double SKGBudgetObject::getDelta() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_DELTABEFORETRANSFER")));
}

SKGError SKGBudgetObject::setYear(int iYear)
{
    return setAttribute(QStringLiteral("i_year"), SKGServices::intToString(iYear));
}

int SKGBudgetObject::getYear() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_year")));
}

SKGError SKGBudgetObject::setMonth(int iMonth)
{
    return setAttribute(QStringLiteral("i_month"), SKGServices::intToString(iMonth));
}

int SKGBudgetObject::getMonth() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_month")));
}

SKGError SKGBudgetObject::setCategory(const SKGCategoryObject &iCategory)
{
    return setAttribute(QStringLiteral("rc_category_id"), SKGServices::intToString(iCategory.getID()));
}

SKGError SKGBudgetObject::getCategory(SKGCategoryObject &oCategory) const
{
    return getDocument()->getObject(QStringLiteral("v_category"), "id=" % getAttribute(QStringLiteral("rc_category_id")), oCategory);
}

SKGError SKGBudgetObject::enableSubCategoriesInclusion(bool iEnable)
{
    return setAttribute(QStringLiteral("t_including_subcategories"), iEnable ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGBudgetObject::isSubCategoriesInclusionEnabled() const
{
    return (getAttribute(QStringLiteral("t_including_subcategories")) == QStringLiteral("Y"));
}

SKGError SKGBudgetObject::removeCategory()
{
    return setAttribute(QStringLiteral("rc_category_id"), QStringLiteral("0"));
}

SKGError SKGBudgetObject::process()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Main values
    int m = getMonth();
    int y = getYear();
    double transferred = SKGServices::stringToDouble(getAttribute(QStringLiteral("f_transferred")));

    // Get budgets rules ordered
    SKGObjectBase::SKGListSKGObjectBase budgetsRules;
    QString sql = "(t_year_condition='N' OR i_year=" % SKGServices::intToString(y) % ") AND "
                  "(t_month_condition='N' OR i_month=" % SKGServices::intToString(m) % ") AND "
                  "(t_category_condition='N' OR rc_category_id=" % getAttribute(QStringLiteral("rc_category_id")) % ") "
                  "ORDER BY i_ORDER ASC";
    err = getDocument()->getObjects(QStringLiteral("v_budgetrule"), sql, budgetsRules);

    int nb = budgetsRules.count();
    if (!err && (nb != 0)) {
        err = getDocument()->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Apply budget rules"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGBudgetRuleObject rule(budgetsRules.at(i));

            // Do we have something to do
            SKGBudgetRuleObject::Condition cond = rule.getCondition();
            double delta = getDelta();
            double quantity = rule.getQuantity();
            if (delta != 0.0) {
                if (cond == SKGBudgetRuleObject::ALL || (delta < 0 && cond == SKGBudgetRuleObject::NEGATIVE)
                    || (delta > 0 && cond == SKGBudgetRuleObject::POSITIVE)) {
                    // Compute value to transfer
                    double value = (rule.isAbolute() ? (cond == SKGBudgetRuleObject::NEGATIVE ? qMax(delta, quantity) : qMin(delta, quantity))
                                                     : quantity * (delta - transferred) / 100.0);

                    // Get impacted budget
                    SKGBudgetObject impactedBudget = *this;
                    impactedBudget.resetID();

                    SKGBudgetRuleObject::Mode mode = rule.getTransferMode();
                    if (mode == SKGBudgetRuleObject::NEXT) {
                        // Next
                        int mi = m;
                        int yi = y;
                        if (mi == 0) {
                            // Yearly budget
                            ++yi;
                        } else {
                            // Monthly budget
                            ++mi;
                            if (mi == 13) {
                                mi = 1;
                                ++yi;
                            }
                        }
                        IFOKDO(err, impactedBudget.setYear(yi))
                        IFOKDO(err, impactedBudget.setMonth(mi))
                    } else if (mode == SKGBudgetRuleObject::YEAR) {
                        // Year
                        IFOKDO(err, impactedBudget.setYear(y))
                        IFOKDO(err, impactedBudget.setMonth(0))
                    }

                    // Change category
                    if (!err && rule.isCategoryChangeEnabled()) {
                        SKGCategoryObject transferCategory;
                        rule.getTransferCategory(transferCategory);
                        err = impactedBudget.setCategory(transferCategory);
                    }

                    IFOK(err)
                    {
                        if (impactedBudget.exist()) {
                            err = impactedBudget.load();
                            QString newBudget = SKGServices::doubleToString(impactedBudget.getBudgetedModifiedAmount() - value);
                            IFOKDO(err, impactedBudget.setAttribute(QStringLiteral("f_budgeted_modified"), newBudget))
                            QString reasons = impactedBudget.getAttribute(QStringLiteral("t_modification_reasons"));
                            if (!reasons.isEmpty()) {
                                reasons += '\n';
                            }
                            reasons += i18nc("Message",
                                             "Transfer of %1 from '%2' to '%3' due to the rule '%4'",
                                             value,
                                             getDisplayName(),
                                             impactedBudget.getDisplayName(),
                                             rule.getDisplayName());
                            IFOKDO(err, impactedBudget.setAttribute(QStringLiteral("t_modification_reasons"), reasons))
                            IFOKDO(err, impactedBudget.save())

                            transferred += value;
                            IFOKDO(err, setAttribute(QStringLiteral("f_transferred"), SKGServices::doubleToString(transferred)))
                            IFOKDO(err, save())
                        } else {
                            getDocument()->sendMessage(i18nc("",
                                                             "Impossible to apply the rule '%1' for budget '%2' because the impacted budget does not exist",
                                                             rule.getDisplayName(),
                                                             this->getDisplayName()),
                                                       SKGDocument::Warning);
                        }
                    }
                }
            }
            IFOKDO(err, getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(getDocument(), err)
    }

    return err;
}

SKGError SKGBudgetObject::createAutomaticBudget(SKGDocumentBank *iDocument, int iYear, int iBaseYear, bool iUseScheduledOperation, bool iRemovePreviousBudget)
{
    Q_UNUSED(iUseScheduledOperation)
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    QString baseYear = SKGServices::intToString(iBaseYear);
    int fistMonth = 0;
    if (iDocument != nullptr) {
        SKGStringListList listTmp;
        err = iDocument->executeSelectSqliteOrder("SELECT MIN(STRFTIME('%m', d_date)) FROM operation WHERE i_group_id = 0 AND STRFTIME('%Y', d_date) = '"
                                                      % baseYear % "' AND t_template='N'",
                                                  listTmp);
        if (listTmp.count() == 2) {
            fistMonth = SKGServices::stringToInt(listTmp.at(1).at(0));
        }
    }
    if (!err && (iDocument != nullptr)) {
        SKGStringListList listTmp;
        QString sql = "SELECT t_REALCATEGORY, COUNT(TOT),"
                      "100*COUNT(TOT)/((CASE WHEN STRFTIME('%Y', date('now', 'localtime'))<>'" % baseYear % "' THEN 12 ELSE STRFTIME('%m', date('now', 'localtime'))-1 END)-" %
                      SKGServices::intToString(fistMonth) % "+1) AS CPOUR,"
                      "ROUND(TOTAL(TOT)/COUNT(TOT)), MAX(MONTH), TOTAL(TOT) FROM ("
                      "SELECT t_REALCATEGORY, STRFTIME('%m', d_date) AS MONTH, TOTAL(f_REALCURRENTAMOUNT) AS TOT "
                      "FROM v_suboperation_consolidated WHERE i_group_id = 0 AND d_DATEYEAR = '" % baseYear % "' AND d_DATEMONTH<STRFTIME('%Y-%m', date('now', 'localtime')) "
                      "GROUP BY t_REALCATEGORY, d_DATEMONTH"
                      ") GROUP BY t_REALCATEGORY ORDER BY COUNT(TOT) DESC, (MAX(TOT)-MIN(TOT))/ABS(ROUND(TOTAL(TOT)/COUNT(TOT))) ASC, ROUND(TOTAL(TOT)/COUNT(TOT)) ASC";
        err = iDocument->executeSelectSqliteOrder(sql, listTmp);

        // SELECT r.d_date,r.i_period_increment,r.t_period_unit, r.i_nb_times, r. t_times, r.t_CATEGORY, r.f_CURRENTAMOUNT  FROM v_recurrentoperation_display r
        // WHERE r.t_TRANSFER='N'

        /*double sumBudgeted = 0;
        double sumOps = 0;*/

        int nbval = listTmp.count();
        if (!err) {
            int step = 0;
            err = iDocument->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Create automatic budget"),
                                              nbval - 1 + 1 + (iRemovePreviousBudget ? 1 : 0));

            // Remove previous budgets
            if (iRemovePreviousBudget) {
                IFOKDO(err, iDocument->executeSqliteOrder("DELETE FROM budget WHERE i_year=" % SKGServices::intToString(iYear)))
                ++step;
                IFOKDO(err, iDocument->stepForward(step))
            }

            // Create budgets
            for (int i = 1; !err && i < nbval; ++i) { // Ignore header
                // Get values
                QString catName = listTmp.at(i).at(0);
                int count = SKGServices::stringToInt(listTmp.at(i).at(1));
                int countPercent = SKGServices::stringToInt(listTmp.at(i).at(2));
                double amount = SKGServices::stringToDouble(listTmp.at(i).at(3));
                int month = SKGServices::stringToInt(listTmp.at(i).at(4));
                // sumOps += SKGServices::stringToDouble(listTmp.at(i).at(5));

                if (!catName.isEmpty() && (countPercent > 85 || count == 1)) {
                    SKGCategoryObject cat;
                    err = iDocument->getObject(QStringLiteral("v_category"), "t_fullname = '" % SKGServices::stringToSqlString(catName) % u'\'', cat);
                    for (int m = fistMonth; !err && m <= (count == 1 ? fistMonth : 12); ++m) {
                        SKGBudgetObject budget(iDocument);
                        err = budget.setBudgetedAmount(amount);
                        IFOKDO(err, budget.setYear(iYear))
                        IFOKDO(err, budget.setMonth(count == 1 ? month : m))
                        IFOKDO(err, budget.setCategory(cat))
                        IFOKDO(err, budget.save())

                        // sumBudgeted += amount;
                    }
                }
                ++step;
                IFOKDO(err, iDocument->stepForward(step))
            }

            // Analyze
            IFOKDO(err, iDocument->executeSqliteOrder(QStringLiteral("ANALYZE")))
            ++step;
            IFOKDO(err, iDocument->stepForward(step))

            SKGENDTRANSACTION(iDocument, err)
        }
    }
    return err;
}

SKGError SKGBudgetObject::balanceBudget(SKGDocumentBank *iDocument, int iYear, int iMonth, bool iBalanceYear)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (iDocument != nullptr) {
        err = iDocument->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Balance budgets"), 2);

        // Monthly balancing
        if (!err && iMonth != -1) {
            for (int m = (iMonth == 0 ? 1 : std::as_const(iMonth)); !err && m <= (iMonth == 0 ? 12 : iMonth); ++m) {
                SKGStringListList listTmp;
                err = iDocument->executeSelectSqliteOrder("SELECT TOTAL(f_budgeted) FROM budget WHERE i_year=" % SKGServices::intToString(iYear)
                                                              % " AND i_month=" % SKGServices::intToString(m) % " AND rc_category_id<>0",
                                                          listTmp);
                if (!err && listTmp.count() == 2) {
                    SKGBudgetObject budget(iDocument);
                    double amount = -SKGServices::stringToDouble(listTmp.at(1).at(0));
                    err = budget.setBudgetedAmount(amount);
                    IFOKDO(err, budget.setYear(iYear))
                    IFOKDO(err, budget.setMonth(m))
                    IFOKDO(err, budget.save())
                }
            }
        }
        IFOKDO(err, iDocument->stepForward(1))

        // Annual balancing
        if (!err && iBalanceYear) {
            SKGStringListList listTmp;
            err = iDocument->executeSelectSqliteOrder("SELECT TOTAL(f_budgeted) FROM budget WHERE i_year=" % SKGServices::intToString(iYear)
                                                          % " AND (i_month<>0 OR rc_category_id<>0)",
                                                      listTmp);
            if (!err && listTmp.count() == 2) {
                SKGBudgetObject budget(iDocument);
                double amount = -SKGServices::stringToDouble(listTmp.at(1).at(0));
                err = budget.setBudgetedAmount(amount);
                IFOKDO(err, budget.setYear(iYear))
                IFOKDO(err, budget.setMonth(0))
                IFOKDO(err, budget.save())
            }
        }
        IFOKDO(err, iDocument->stepForward(2))

        SKGENDTRANSACTION(iDocument, err)
    }
    return err;
}
