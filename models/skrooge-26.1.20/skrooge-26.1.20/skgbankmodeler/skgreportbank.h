/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGREPORTBANK_H
#define SKGREPORTBANK_H
/** @file
 * A report class for bank document
 *
 * @author Stephane MANKOWSKI
 */
#include <qvariant.h>

#include "skgbankmodeler_export.h"
#include "skgerror.h"
#include "skgreport.h"
#include "skgunitobject.h"

class SKGDocument;
/**
 * A report class for bank document
 */
class SKGBANKMODELER_EXPORT SKGReportBank : public SKGReport
{
    Q_OBJECT
    /**
     * The budget table
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList budget_table READ getBudgetTable NOTIFY changed2)

    /**
     * The unit table
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList unit_table READ getUnitTable NOTIFY changed2)

    /**
     * The portfolio
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList portfolio READ getPortfolio NOTIFY changed2)

    /**
     * The account table
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList account_table READ getAccountTable NOTIFY changed2)

    /**
     * The bank table
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList bank_table READ getBankTable NOTIFY changed2)

    /**
     * The alarms
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList alarms READ getAlarms NOTIFY changed2)

    /**
     * The interests
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList interests READ getInterests NOTIFY changed2)

    /**
     * The scheduled transactions
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList scheduled_operations READ getScheduledOperations NOTIFY changed2)

    /**
     * The main categories of the period
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList categories_period READ getMainCategoriesForPeriod NOTIFY changed2)

    /**
     * The main categories of the previous period
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList categories_previous_period READ getMainCategoriesForPreviousPeriod NOTIFY changed2)

    /**
     * The main categories of the period (for compatibility)
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList categories_month READ getMainCategoriesForPeriod NOTIFY changed2)

    /**
     * The main categories of the previous period (for compatibility)
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList categories_previous_month READ getMainCategoriesForPreviousPeriod NOTIFY changed2)

    /**
     * The income versus expenditure
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantList income_vs_expenditure READ getIncomeVsExpenditure NOTIFY changed2)

    /**
     * The net worth
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(double networth READ getNetWorth NOTIFY changed2)

    /**
     * The annual spending
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(double annual_spending READ getAnnualSpending NOTIFY changed2)

    /**
     * The personal finance score
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(double personal_finance_score READ getPersonalFinanceScore NOTIFY changed2)

    /**
     * The personal finance score details
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QVariantMap personal_finance_score_details READ getPersonalFinanceScoreDetails NOTIFY changed2)

    /**
     * The main categories variations
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QStringList categories_variations READ get5MainCategoriesVariation NOTIFY changed2)

    /**
     * The main categories variations which are issues (expenditure increasing or incomes decreasing
     * WARNING: Notification is launched only when cleanCache or setPeriod are called. So do not forget to connect cleanCache with
     * SKGDocument::transactionSuccessfullyEnded
     */
    Q_PROPERTY(QStringList categories_variations_issues READ get5MainCategoriesVariationIssue NOTIFY changed2)

public:
    /**
     * Default Constructor
     */
    explicit SKGReportBank(SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    virtual ~SKGReportBank() override;

    /**
     * Get the budget table
     * @return the budget table
     */
    Q_INVOKABLE virtual QVariantList getBudgetTable();

    /**
     * Get the unit table
     * @return the unit table
     */
    Q_INVOKABLE virtual QVariantList getUnitTable();

    /**
     * Get the portfolio
     * @return the portfolio
     */
    Q_INVOKABLE virtual QVariantList getPortfolio();

    /**
     * Get the account table
     * @return the account table
     */
    Q_INVOKABLE virtual QVariantList getAccountTable();

    /**
     * Get the bank table
     * @return the bank table
     */
    Q_INVOKABLE virtual QVariantList getBankTable();

    /**
     * Get the scheduled transactions
     * Parameters supported:
     *    scheduled_operation_days_max: number max of days displayed (default: 30)
     * @return the scheduled transactions
     */
    Q_INVOKABLE virtual QVariantList getScheduledOperations();

    /**
     * Get the main categories of the period
     * @return the main categories of the period
     */
    Q_INVOKABLE virtual QVariantList getMainCategoriesForPeriod();

    /**
     * Get the main categories of the previous period
     * @return the main categories of the previous period
     */
    Q_INVOKABLE virtual QVariantList getMainCategoriesForPreviousPeriod();

    /**
     * Get the main categories variations
     * @return the main categories variations
     */
    Q_INVOKABLE virtual QStringList get5MainCategoriesVariation();

    /**
     * Get the main categories variations which are issues (expenditure increasing or incomes decreasing)
     * @return the main categories variations
     */
    Q_INVOKABLE virtual QStringList get5MainCategoriesVariationIssue();

    /**
     * Get the income versus expenditure
     * @param iOnSubOperation the computation is done on sub operation
     * @param iGrouped the computation includes grouped (sub) operation
     * @param iTransfer the computation includes Transfers
     * @param iTracker the computation includes tracked transactions
     * @param iWhereClause1 the period where clause (if empty then use the period of the report)
     * @param iWhereClause2 the period where clause (if empty then use the previous period of the report)
     * @return the income versus expenditure
     */
    Q_INVOKABLE virtual QVariantList getIncomeVsExpenditure(bool iOnSubOperation = true,
                                                            bool iGrouped = true,
                                                            bool iTransfer = false,
                                                            bool iTracker = true,
                                                            const QString &iWhereClause1 = QString(),
                                                            const QString &iWhereClause2 = QString());

    /**
     * Get the net worth
     * @param iTransfer the computation includes Transfers
     * @param iTracker the computation includes tracked transactions
     * @return the net worth
     */
    Q_INVOKABLE virtual double getNetWorth(bool iTransfer = false, bool iTracker = true);

    /**
     * Get the annual spending
     * @param iTransfer the computation includes Transfers
     * @param iTracker the computation includes tracked transactions
     * @return the annual spending
     */
    Q_INVOKABLE virtual double getAnnualSpending(bool iTransfer = false, bool iTracker = true);

    /**
     * Get the personal finance score (https://jlyblog.wordpress.com/2013/10/13/the-new-score-that-outweighs-your-credit-score/)
     * @param iTransfer the computation includes Transfers
     * @param iTracker the computation includes tracked transactions
     * @return the personal finance score
     */
    Q_INVOKABLE virtual double getPersonalFinanceScore(bool iTransfer = false, bool iTracker = true);

    /**
     * Get the personal finance score (https://jlyblog.wordpress.com/2013/10/13/the-new-score-that-outweighs-your-credit-score/)
     * @param iTransfer the computation includes Transfers
     * @param iTracker the computation includes tracked transactions
     * @return the personal finance score + (success, warning or danger) + advice string + color
     */
    Q_INVOKABLE virtual QVariantMap getPersonalFinanceScoreDetails(bool iTransfer = false, bool iTracker = true);

    /**
     * Get the alarms
     * @return the alarms
     */
    Q_INVOKABLE virtual QVariantList getAlarms();

    /**
     * Get the interests
     * @return the interests
     */
    Q_INVOKABLE virtual QVariantList getInterests();

Q_SIGNALS:
    /**
     * Emitted when the report changed
     */
    void changed2();

protected:
    /**
     * Enrich the grantlee mapping
     * @param iMapping the mapping
     */
    void addItemsInMapping(QVariantHash &iMapping) override;

private:
    Q_DISABLE_COPY(SKGReportBank)

    struct unitValues {
        SKGUnitObject unit;
        double initalAmount{};
        double purchaseAmount{};
        double currentAmount{};
        double quantity{};
    };
};

/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGReportBank, Q_COMPLEX_TYPE);
#endif // SKGREPORTBANK_H
