/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGRECURRENTOPERATIONOBJECT_H
#define SKGRECURRENTOPERATIONOBJECT_H
/** @file
 * This file defines classes SKGRecurrentOperationObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgobjectbase.h"
class SKGOperationObject;
class SKGDocumentBank;

/**
 * This class manages recurrent transaction object
 */
class SKGBANKMODELER_EXPORT SKGRecurrentOperationObject final : public SKGObjectBase
{
public:
    /**
     * This enumerate defines the adaptation of the date
     */
    enum Adapter {
        NONE = 0,
        PREVIOUS_MONDAY = 1,
        PREVIOUS_TUESDAY = 2,
        PREVIOUS_WEDNESDAY = 3,
        PREVIOUS_THURSDAY = 4,
        PREVIOUS_FRIDAY = 5,
        PREVIOUS_SATURDAY = 6,
        PREVIOUS_SUNDAY = 7,
        CLOSEST_MONDAY = 8,
        CLOSEST_TUESDAY = 9,
        CLOSEST_WEDNESDAY = 10,
        CLOSEST_THURSDAY = 11,
        CLOSEST_FRIDAY = 12,
        CLOSEST_SATURDAY = 13,
        CLOSEST_SUNDAY = 14,
        NEXT_MONDAY = 15,
        NEXT_TUESDAY = 16,
        NEXT_WEDNESDAY = 17,
        NEXT_THURSDAY = 18,
        NEXT_FRIDAY = 19,
        NEXT_SATURDAY = 20,
        NEXT_SUNDAY = 21,
        PREVIOUS_OPEN_DAY = 22,
        CLOSEST_OPEN_DAY = 23,
        NEXT_OPEN_DAY = 24,
        FIRST_OPEN_DAY_OF_MONTH = 25,
    };
    Q_ENUM(Adapter)

    /**
     * This enumerate defines the period unit
     */
    enum PeriodUnit {
        DAY = 0, /**< day */
        WEEK = 1, /**< week */
        MONTH = 2, /**< month */
        YEAR = 3 /**< year */
    };
    /**
     * This enumerate defines the period unit
     */
    Q_ENUM(PeriodUnit)

    /**
     * Default constructor
     */
    explicit SKGRecurrentOperationObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGRecurrentOperationObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGRecurrentOperationObject(const SKGObjectBase &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGRecurrentOperationObject(const SKGRecurrentOperationObject &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGRecurrentOperationObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGRecurrentOperationObject &operator=(const SKGRecurrentOperationObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGRecurrentOperationObject();

    /**
     * Get the parent operation
     * @param oOperation the parent operation
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getParentOperation(SKGOperationObject &oOperation) const;

    /**
     * Set the parent operation
     * @param iOperation the parent operation
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setParentOperation(const SKGOperationObject &iOperation);

    /**
     * Set the template mode of this recurrent operation
     * @param iTemplate the template status: true or false
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setTemplate(bool iTemplate);

    /**
     * Get the template mode of this recurrent operation
     * @return an object managing the error
     *   @see SKGError
     */
    bool isTemplate() const;

    /**
     * Set the increment
     * @param iIncrement the number of @see setPeriodUnit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setPeriodIncrement(int iIncrement);

    /**
     * Get the increment
     * @return the number
     */
    int getPeriodIncrement() const;

    /**
     * Get the period unit of this recurrent operation
     * @return the unit
     */
    SKGRecurrentOperationObject::PeriodUnit getPeriodUnit() const;

    /**
     * Set the period unit of this recurrent operation
     * @param iPeriod the period unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setPeriodUnit(SKGRecurrentOperationObject::PeriodUnit iPeriod);

    /**
     * Get the adapter of this recurrent operation
     * @return the adapter
     */
    SKGRecurrentOperationObject::Adapter getAdapter() const;

    /**
     * Set the adapter of this recurrent operation
     * @param iAdapter the adapter
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setAdapter(SKGRecurrentOperationObject::Adapter iAdapter);

    /**
     * Set the number of days before term to create operation
     * @param iDays the number of days
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setAutoWriteDays(int iDays);

    /**
     * Get the number of days before term to create operation
     * @return the number of days
     */
    int getAutoWriteDays() const;

    /**
     * Set the number of days before term to warn user
     * @param iDays the number of days
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setWarnDays(int iDays);

    /**
     * Get the number of days before term to warn user
     * @return the number of days
     */
    int getWarnDays() const;

    /**
     * Set date of this recurrent operation
     * @param iDate the date
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError setDate(QDate iDate);

    /**
     * Get date of this recurrent operation
     * @return the date
     */
    QDate getDate() const;

    /**
     * Get next date of this recurrent operation
     * @return the date
     */
    QDate getNextDate() const;

    /**
     * Get all transactions created by this recurrent operation
     * @param oOperations all transactions
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getRecurredOperations(SKGListSKGObjectBase &oOperations) const;

    /**
     * To warn or not the end user
     * @param iWarn the warn: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError warnEnabled(bool iWarn);

    /**
     * To know if the end user is warned or not
     * @return an object managing the error
     *   @see SKGError
     */
    bool isWarnEnabled() const;

    /**
     * To activate or not the auto write mode
     * @param iAutoWrite auto write mode: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError autoWriteEnabled(bool iAutoWrite);

    /**
     * To know if auto write mode is enabled or not
     * @return an object managing the error
     *   @see SKGError
     */
    bool isAutoWriteEnabled() const;

    /**
     * To know if a time limit is enabled or not
     * @return an object managing the error
     *   @see SKGError
     */
    bool hasTimeLimit() const;

    /**
     * To enable / disable a time limit
     * @param iTimeLimit the time limit: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError timeLimit(bool iTimeLimit);

    /**
     * Set the time limit
     * @param iTimeLimit the number of times transaction will be inserted
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setTimeLimit(int iTimeLimit);

    /**
     * Set the time limit
     * @param iLastDate the last date of the transaction will be inserted. setDate, setPeriodIncrement and setPeriodUnit must be used before.
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError setTimeLimit(QDate iLastDate);

    /**
     * Get the number of times transaction will be inserted
     * @return the number of times
     */
    int getTimeLimit() const;

    /**
     * Warn and/or create transactions for this recurrent operation
     * @param oNbInserted number of transactions inserted
     * @param iForce to force the insertion even if autowrite is not enable
     * @param iDate date limit for insertion
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError process(int &oNbInserted, bool iForce = false, QDate iDate = QDate::currentDate());

    /**
     * Warn and/or create transactions for all recurrent transactions of the document
     * @param iDocument the document containing the object
     * @param oNbInserted number of transactions inserted
     * @param iForce to force the insertion even if autowrite is not enable*
     * @param iDate date limit for insertion
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    static SKGError process(SKGDocumentBank *iDocument, int &oNbInserted, bool iForce = false, QDate iDate = QDate::currentDate());
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGRecurrentOperationObject, Q_MOVABLE_TYPE);
#endif
