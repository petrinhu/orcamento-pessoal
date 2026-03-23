/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPROPERTYOBJECT_H
#define SKGPROPERTYOBJECT_H
/** @file
 * This file defines classes SKGPropertyObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgdefine.h"
#include "skgnamedobject.h"

class SKGDocument;
/**
 * This class manages properties on objects
 */
class SKGBASEMODELER_EXPORT SKGPropertyObject final : public SKGNamedObject
{
    /**
     * Value of the property
     */
    Q_PROPERTY(QString value READ getValue WRITE setValue) // clazy:exclude=qproperty-without-notify
    /**
     * Parent identifier of the property
     */
    Q_PROPERTY(QString parentId READ getParentId WRITE setParentId) // clazy:exclude=qproperty-without-notify
public:
    /**
     * Default constructor
     */
    explicit SKGPropertyObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGPropertyObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGPropertyObject(const SKGPropertyObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGPropertyObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGPropertyObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGPropertyObject &operator=(const SKGPropertyObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGPropertyObject();

    /**
     * Set the value of the property
     * @param iValue the value of the property
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setValue(const QString &iValue);

    /**
     * Get the value of the property
     * @return the value of the property
     */
    QString getValue() const;

    /**
     * Set the parent identifier
     * @param iParentId the parent identifier
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setParentId(const QString &iParentId);

    /**
     * Get the parent identifier
     * @return the parent identifier
     */
    QString getParentId() const;

    /**
     * Get the url of the property
     * @param iBuildTemporaryFile to build the temporary file if needed
     * @return the url of the property
     */
    QUrl getUrl(bool iBuildTemporaryFile = false) const;

protected:
    /**
     * Get where clause needed to identify objects.
     * For this class, the whereclause is based on name + t_uuid_parent
     * @return the where clause
     */
    QString getWhereclauseId() const override;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGPropertyObject, Q_MOVABLE_TYPE);
#endif
