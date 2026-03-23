/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGINTERESTOBJECT_H
#define SKGINTERESTOBJECT_H
/** @file
 * This file defines classes SKGInterestObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgaccountobject.h"
#include "skgbankmodeler_export.h"
#include "skgerror.h"
#include "skgobjectbase.h"

class SKGInterestObject;

/**
 * This class is interest option for an account
 */
class SKGBANKMODELER_EXPORT SKGInterestObject final : public SKGObjectBase
{
public:
    /**
     * This enumerate defines how to compute value date
     */
    enum ValueDateMode {
        FIFTEEN = 0,
        J0,
        J1,
        J2,
        J3,
        J4,
        J5
    };
    /**
     * This enumerate defines how to compute value date
     */
    Q_ENUM(ValueDateMode)

    /**
     * This enumerate defines how to compute interest
     */
    enum InterestMode {
        FIFTEEN24 = 0,
        DAYS360,
        DAYS365
    };
    /**
     * This enumerate defines how to compute interest
     */
    Q_ENUM(InterestMode)

    /**
     * Default constructor
     */
    explicit SKGInterestObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGInterestObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGInterestObject(const SKGInterestObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGInterestObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGInterestObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGInterestObject &operator=(const SKGInterestObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGInterestObject();

    /**
     * Get the parent account
     * @param oAccount the parent account
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getAccount(SKGAccountObject &oAccount) const;

    /**
     * Set the parent account
     * @param iAccount the parent account
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setAccount(const SKGAccountObject &iAccount);

    /**
     * Set the quantity for the date of this unit
     * @param iValue the quantity
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setRate(double iValue);

    /**
     * Get the quantity for the date of this unit
     * @return the quantity
     */
    double getRate() const;

    /**
     * Set date of this value
     * @param iDate the date
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError setDate(QDate iDate);

    /**
     * Get date of this value
     * @return the date
     */
    QDate getDate() const;

    /**
     * Set income value date mode
     * @param iMode the mode
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setIncomeValueDateMode(SKGInterestObject::ValueDateMode iMode);

    /**
     * Get income value date mode
     * @return the income value date mode
     */
    SKGInterestObject::ValueDateMode getIncomeValueDateMode() const;

    /**
     * Set expenditue value date mode
     * @param iMode the mode
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setExpenditueValueDateMode(SKGInterestObject::ValueDateMode iMode);

    /**
     * Get expenditue value date mode
     * @return the expenditue value date mode
     */
    SKGInterestObject::ValueDateMode getExpenditueValueDateMode() const;

    /**
     * Set interest computation mode
     * @param iMode the mode
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setInterestComputationMode(SKGInterestObject::InterestMode iMode);

    /**
     * Get interest computation mode
     * @return the interest computation mode
     */
    SKGInterestObject::InterestMode getInterestComputationMode() const;

protected:
    /**
     * Get where clause needed to identify objects.
     * For this class, the whereclause is based on date + unit
     * @return the where clause
     */
    QString getWhereclauseId() const override;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGInterestObject, Q_MOVABLE_TYPE);
#endif
