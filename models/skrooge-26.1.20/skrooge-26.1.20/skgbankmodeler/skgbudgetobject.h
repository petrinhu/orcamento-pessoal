/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBUDGETOBJECTT_H
#define SKGBUDGETOBJECTT_H
/** @file
 * This file defines classes SKGBudgetObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgerror.h"
#include "skgobjectbase.h"

class SKGCategoryObject;
class SKGDocumentBank;

/**
 * This class is a budget
 */
class SKGBANKMODELER_EXPORT SKGBudgetObject final : public SKGObjectBase
{
public:
    /**
     * Default constructor
     */
    explicit SKGBudgetObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGBudgetObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Destructor
     */
    virtual ~SKGBudgetObject();

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGBudgetObject(const SKGBudgetObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGBudgetObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGBudgetObject &operator=(const SKGObjectBase &iObject);

    /**
     * Set year of the budget
     * @param iYear the year
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setYear(int iYear);

    /**
     * Get year of the budget
     * @return year of the budget
     */
    int getYear() const;

    /**
     * Set month of the budget
     * @param iMonth the month
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setMonth(int iMonth);

    /**
     * Get month of the budget
     * @return month of the budget
     */
    int getMonth() const;

    /**
     * Set the category
     * @param iCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setCategory(const SKGCategoryObject &iCategory);

    /**
     * Remove the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError removeCategory();

    /**
     * Get the category
     * @param oCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getCategory(SKGCategoryObject &oCategory) const;

    /**
     * Enable / disable the inclusion of sub categories
     * @param iEnable condition
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError enableSubCategoriesInclusion(bool iEnable);

    /**
     * To know if sub categories inclusion is enabled or disabled
     * @return condition
     */
    bool isSubCategoriesInclusionEnabled() const;

    /**
     * Set budgeted amount of the budget
     * @param iAmount the budgeted amount
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setBudgetedAmount(double iAmount);

    /**
     * Get budgeted amount of the budget
     * @return budgeted amount of the budget
     */
    double getBudgetedAmount() const;

    /**
     * Get budgeted modified amount of the budget
     * @return budgeted modified amount of the budget
     */
    double getBudgetedModifiedAmount() const;

    /**
     * Get the text explaining the reasons of the modification
     * @return the text explaining the reasons of the modification
     */
    QString getModificationReasons() const;

    /**
     * Get delta
     * @return delta
     */
    double getDelta() const;

    /**
     * Process all rules
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError process();

    /**
     * Create automatically budget items based on existing transactions
     * @param iDocument the document where to create*
     * @param iYear year of budget
     * @param iBaseYear year of the base for computation
     * @param iUseScheduledOperation use the scheduled transactions for a more accurate creation
     * @param iRemovePreviousBudget remove existing budget for @param iYear
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError createAutomaticBudget(SKGDocumentBank *iDocument, int iYear, int iBaseYear, bool iUseScheduledOperation, bool iRemovePreviousBudget);

    /**
     * Create automatically budget items based on existing transactions
     * @param iDocument the document where to create*
     * @param iYear year of budget
     * @param iMonth month of the budget. 0 to balance all months. -1 to balance any month
     * @param iBalanceYear to balance the year
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError balanceBudget(SKGDocumentBank *iDocument, int iYear, int iMonth = 0, bool iBalanceYear = true);

private:
    QString getWhereclauseId() const override;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGBudgetObject, Q_MOVABLE_TYPE);
#endif
