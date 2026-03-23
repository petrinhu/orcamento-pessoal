/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNITVALUEOBJECT_H
#define SKGUNITVALUEOBJECT_H
/** @file
 * This file defines classes SKGUnitValueObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgerror.h"
#include "skgobjectbase.h"

class SKGUnitObject;

/**
 * This class is a value at a time for a unit
 */
class SKGBANKMODELER_EXPORT SKGUnitValueObject final : public SKGObjectBase
{
public:
    /**
     * Default constructor
     */
    explicit SKGUnitValueObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGUnitValueObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGUnitValueObject(const SKGUnitValueObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGUnitValueObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGUnitValueObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGUnitValueObject &operator=(const SKGUnitValueObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGUnitValueObject();

    /**
     * Set the quantity for the date of this unit
     * @param iValue the quantity
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setQuantity(double iValue);

    /**
     * Get the quantity for the date of this unit
     * @return the quantity
     */
    double getQuantity() const;

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
     * Get the parent unit
     * @param oUnit the parent unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getUnit(SKGUnitObject &oUnit) const;

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
Q_DECLARE_TYPEINFO(SKGUnitValueObject, Q_MOVABLE_TYPE);
#endif
