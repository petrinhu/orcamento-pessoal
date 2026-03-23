/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSUBOPERATIONOBJECT_H
#define SKGSUBOPERATIONOBJECT_H
/** @file
 * This file defines classes SKGSubOperationObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgobjectbase.h"
class SKGOperationObject;
class SKGCategoryObject;
class SKGTrackerObject;

/**
 * This class manages suboperation object
 */
class SKGBANKMODELER_EXPORT SKGSubOperationObject final : public SKGObjectBase
{
public:
    /**
     * Default constructor
     */
    explicit SKGSubOperationObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGSubOperationObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGSubOperationObject(const SKGSubOperationObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGSubOperationObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGSubOperationObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGSubOperationObject &operator=(const SKGSubOperationObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGSubOperationObject();

    /**
     * Set date of this subtransaction
     * @param iDate the date
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError setDate(QDate iDate);

    /**
     * Get date of this subtransaction
     * @return the date
     */
    QDate getDate() const;

    /**
     * Set the order of suboperation
     * @param iOrder the order
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setOrder(int iOrder);

    /**
     * Get order of this subtransaction
     * @return the order
     */
    int getOrder() const;

    /**
     * Set the comment of suboperation
     * @param iComment the comment
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setComment(const QString &iComment);

    /**
     * Get the comment of this subtransaction
     * @return the comment
     */
    QString getComment() const;

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
     * Set the category
     * @param iCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setCategory(const SKGCategoryObject &iCategory);

    /**
     * Get the category
     * @param oCategory the category
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getCategory(SKGCategoryObject &oCategory) const;

    /**
     * Set the tracker
     * @param iTracker the tracker
     * @param iForce force the change of the tracker even if closed
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setTracker(const SKGTrackerObject &iTracker, bool iForce = false);

    /**
     * Get the tracker
     * @param oTracker the tracker
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getTracker(SKGTrackerObject &oTracker) const;

    /**
     * Set the quantity of the subtransaction
     * @param iValue the value
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setQuantity(double iValue);

    /**
     * Get the quantity of the subtransaction
     * @return the value
     */
    double getQuantity() const;

    /**
     * Set the formula
     * @param iFormula the formula
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setFormula(const QString &iFormula);

    /**
     * Get the formula
     * @return the formula
     */
    QString getFormula() const;
};

/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGSubOperationObject, Q_MOVABLE_TYPE);
#endif
