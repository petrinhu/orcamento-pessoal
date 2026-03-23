/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPAYEEOBJECT_H
#define SKGPAYEEOBJECT_H
/** @file
 * This file defines classes SKGPayeeObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgnamedobject.h"
class SKGDocumentBank;
class SKGCategoryObject;

/**
 * This class manages payee object
 */
class SKGBANKMODELER_EXPORT SKGPayeeObject final : public SKGNamedObject
{
public:
    /**
     * Default constructor
     */
    explicit SKGPayeeObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGPayeeObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGPayeeObject(const SKGPayeeObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGPayeeObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGPayeeObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGPayeeObject &operator=(const SKGPayeeObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGPayeeObject();

    /**
     * Create a payee if needed and return it
     * @param iDocument the document where to create
     * @param iName the name
     * @param oPayee the payee
     * @param iSendPopupMessageOnCreation to send a creation message if the payee is created
     * @return an object managing the error.
     *   @see SKGError
     */
    static SKGError createPayee(SKGDocumentBank *iDocument, const QString &iName, SKGPayeeObject &oPayee, bool iSendPopupMessageOnCreation = false);
    /**
     * Get all transactions of this payee
     * @param oOperations all transactions of this payee
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getOperations(SKGListSKGObjectBase &oOperations) const;

    /**
     * Set the address of payee
     * @param iAddress the address
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setAddress(const QString &iAddress);

    /**
     * Get the address of this payee
     * @return the address
     */
    QString getAddress() const;

    /**
     * To set the closed attribute of a payee
     * @param iClosed the closed attribute: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setClosed(bool iClosed);

    /**
     * To know if the payee has been closed or not
     * @return an object managing the error
     *   @see SKGError
     */
    virtual bool isClosed() const;

    /**
     * To bookmark or not a payee
     * @param iBookmark the bookmark: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError bookmark(bool iBookmark);

    /**
     * To know if the payee is bookmarked
     * @return an object managing the error
     *   @see SKGError
     */
    bool isBookmarked() const;

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
     * Merge iPayee in current payee
     * @param iPayee the payee. All transactions will be transferred into this payee. The payee will be removed
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError merge(const SKGPayeeObject &iPayee);
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGPayeeObject, Q_MOVABLE_TYPE);

#endif
