/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBUDGETRULEOBJECTT_H
#define SKGBUDGETRULEOBJECTT_H
/** @file
 * This file defines classes SKGBudgetRuleObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgcategoryobject.h"
#include "skgerror.h"
#include "skgobjectbase.h"

class SKGDocumentBank;

/**
 * This class is a budget rule
 */
class SKGBANKMODELER_EXPORT SKGBudgetRuleObject final : public SKGObjectBase
{
public:
    /**
     * Condition
     */
    enum Condition {
        NEGATIVE = -1,
        ALL = 0,
        POSITIVE = 1
    };
    /**
     * Condition
     */
    Q_ENUM(Condition)

    /**
     * Mode
     */
    enum Mode {
        NEXT,
        CURRENT,
        YEAR
    };
    /**
     * Mode
     */
    Q_ENUM(Mode)

    /**
     * Default constructor
     */
    explicit SKGBudgetRuleObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGBudgetRuleObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Destructor
     */
    virtual ~SKGBudgetRuleObject();

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGBudgetRuleObject(const SKGBudgetRuleObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGBudgetRuleObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGBudgetRuleObject &operator=(const SKGObjectBase &iObject);

    /**
     * Enable / disable condition on year of the budget
     * @param iEnable condition
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError enableYearCondition(bool iEnable);

    /**
     * To know if condition on year is enabled or disabled
     * @return condition
     */
    bool isYearConditionEnabled() const;

    /**
     * Set year of the condition of the rule
     * @param iYear the year (0=all year)
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setBudgetYear(int iYear);

    /**
     * Get year of the condition of the rule
     * @return year of the condition of the rule
     */
    int getBudgetYear() const;

    /**
     * Enable / disable condition on month of the budget
     * @param iEnable condition
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError enableMonthCondition(bool iEnable);

    /**
     * To know if condition on month is enabled or disabled
     * @return condition
     */
    bool isMonthConditionEnabled() const;

    /**
     * Set month of the condition of the rule
     * @param iMonth the month
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setBudgetMonth(int iMonth);

    /**
     * Get month of the condition of the rule
     * @return month of the budget
     */
    int getBudgetMonth() const;

    /**
     * Enable / disable condition on category of the budget
     * @param iEnable condition
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError enableCategoryCondition(bool iEnable);

    /**
     * To know if condition on category is enabled or disabled
     * @return condition
     */
    bool isCategoryConditionEnabled() const;

    /**
     * Set the category of the condition of the rule
     * @param iCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setBudgetCategory(const SKGCategoryObject &iCategory);

    /**
     * Remove the category of the condition of the rule
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError removeBudgetCategory();

    /**
     * Get the category of the condition of the rule
     * @param oCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getBudgetCategory(SKGCategoryObject &oCategory) const;

    /**
     * Set the condition when the rule must be applied
     * @param iCondition the condition
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setCondition(SKGBudgetRuleObject::Condition iCondition);

    /**
     * Get the condition when the rule must be applied
     * @return the condition
     */
    SKGBudgetRuleObject::Condition getCondition() const;

    /**
     * Set the quantity to transfer
     * @param iQuantity quantity
     * @param iAbsolute true means "an amount in primary unit", false means "a percentage"
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setQuantity(double iQuantity, bool iAbsolute);

    /**
     * Get quantity to transfer
     * @return quantity to transfer
     */
    double getQuantity() const;

    /**
     * To know if the quantity is an amount or a percentage
     * @return true means "an amount in primary unit", false means "a percentage"
     */
    bool isAbolute() const;

    /**
     * Enable / disable transfer change the category of the budget
     * @param iEnable condition
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError enableCategoryChange(bool iEnable);

    /**
     * To know if transfer change the category
     * @return transfer change the category
     */
    bool isCategoryChangeEnabled() const;

    /**
     * Set the transfer to apply
     * @param iMode the mode (NEXT=same category but for following budget, CURRENT=same period but for another category, YEAR=period of same year)
     * @param iCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setTransfer(SKGBudgetRuleObject::Mode iMode, const SKGCategoryObject &iCategory = SKGCategoryObject());

    /**
     * Get the mode of the transfer
     * @return the mode
     */
    SKGBudgetRuleObject::Mode getTransferMode() const;

    /**
     * Get the category of the transfer of the rule
     * @param oCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getTransferCategory(SKGCategoryObject &oCategory) const;

    /**
     * Set the order for the rule
     * @param iOrder the order. (-1 means "at the end")
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setOrder(double iOrder);

    /**
     * Get the order for the rule
     * @return the order
     */
    double getOrder() const;

    /**
     * Process all rules
     * @param iDocument the document containing the object
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError processAllRules(SKGDocumentBank *iDocument);
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGBudgetRuleObject, Q_MOVABLE_TYPE);
#endif
