/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGNAMEDOBJECT_H
#define SKGNAMEDOBJECT_H
/** @file
 * This file defines classes SKGNamedObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgdefine.h"
#include "skgerror.h"
#include "skgobjectbase.h"

/**
 * This class is a named object base.
 * This is a generic way to manipulate objects with name.
 */
class SKGBASEMODELER_EXPORT SKGNamedObject : public SKGObjectBase
{
    /**
     * Name of the object
     */
    Q_PROPERTY(QString name READ getName WRITE setName USER true) // clazy:exclude=qproperty-without-notify

public:
    /**
     * Return the object (@p oObject) corresponding to a name (@p iName ).
     * If more than one objects are returned by the query, then an error is generated
     * If 0 object is returned by the query, then an error is generated
     * @param iDocument the document where to search
     * @param iTable the table where to search
     * @param iName name of the object in @p iTable
     * @param oObject the result
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError getObjectByName(SKGDocument *iDocument, const QString &iTable, const QString &iName, SKGObjectBase &oObject);

    /**
     * Default constructor
     */
    explicit SKGNamedObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iTable the table of the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGNamedObject(SKGDocument *iDocument, const QString &iTable = QString(), int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGNamedObject(const SKGNamedObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGNamedObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGNamedObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGNamedObject &operator=(const SKGNamedObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGNamedObject();

    /**
     * Set the name of this object
     * @param iName the name
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setName(const QString &iName);

    /**
     * Get the name of this object
     * @return the name
     */
    virtual QString getName() const;

protected:
    /**
     * Get where clause needed to identify objects.
     * For this class, the whereclause is based on name
     * @return the where clause
     */
    QString getWhereclauseId() const override;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGNamedObject, Q_MOVABLE_TYPE);
#endif
