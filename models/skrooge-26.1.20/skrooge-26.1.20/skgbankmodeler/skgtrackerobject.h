/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTRACKEROBJECT_H
#define SKGTRACKEROBJECT_H
/** @file
 * This file defines classes SKGTrackerObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgnamedobject.h"
class SKGDocumentBank;

/**
 * This class manages tracker object
 */
class SKGBANKMODELER_EXPORT SKGTrackerObject final : public SKGNamedObject
{
public:
    /**
     * Default constructor
     */
    explicit SKGTrackerObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGTrackerObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGTrackerObject(const SKGTrackerObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGTrackerObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGTrackerObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGTrackerObject &operator=(const SKGTrackerObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGTrackerObject();

    /**
     * Create a tracker if needed and return it
     * @param iDocument the document where to create
     * @param iName the name
     * @param oTracker the tracker
     * @param iSendPopupMessageOnCreation to send a creation message if the tracker is created
     * @return an object managing the error.
     *   @see SKGError
     */
    static SKGError createTracker(SKGDocumentBank *iDocument, const QString &iName, SKGTrackerObject &oTracker, bool iSendPopupMessageOnCreation = false);
    /**
     * Get all subtransactions of this tracker
     * @param oSubOperations all subtransactions of this transaction
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getSubOperations(SKGListSKGObjectBase &oSubOperations) const;

    /**
     * Set the comment of tracker
     * @param iComment the comment
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setComment(const QString &iComment);

    /**
     * Get the comment of this tracker
     * @return the comment
     */
    QString getComment() const;

    /**
     * To set the closed attribute of this tracker
     * @param iClosed the closed attribute: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setClosed(bool iClosed);

    /**
     * To know if the tracker has been closed or not
     * @return an object managing the error
     *   @see SKGError
     */
    bool isClosed() const;

    /**
     * Get the current amount
     * @return the current amount
     */
    double getCurrentAmount() const;

    /**
     * Merge iTracker in current tracker
     * @param iTracker the tracker. All transactions will be transferred into this tracker. The tracker will be removed
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError merge(const SKGTrackerObject &iTracker);
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGTrackerObject, Q_MOVABLE_TYPE);

#endif
