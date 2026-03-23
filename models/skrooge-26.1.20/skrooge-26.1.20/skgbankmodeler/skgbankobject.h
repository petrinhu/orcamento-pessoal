/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBANKOBJECT_H
#define SKGBANKOBJECT_H
/** @file
 * This file defines classes SKGBankObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgnamedobject.h"

class SKGAccountObject;
/**
 * This class manages bank object
 */
class SKGBANKMODELER_EXPORT SKGBankObject final : public SKGNamedObject
{
public:
    /**
     * Default constructor
     */
    explicit SKGBankObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGBankObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGBankObject(const SKGBankObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGBankObject(const SKGNamedObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGBankObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGBankObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGBankObject &operator=(const SKGBankObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGBankObject();

    /**
     * Add an account
     * @param oAccount the created account
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError addAccount(SKGAccountObject &oAccount);

    /**
     * Get accounts
     * @param oAccountList the list of accounts in this bank
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getAccounts(SKGListSKGObjectBase &oAccountList) const;

    /**
     * Set the number of the bank
     * @param iNumber the number
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setNumber(const QString &iNumber);

    /**
     * Get the number of the bank
     * @return the number
     */
    QString getNumber() const;

    /**
     * Set the icon of the bank
     * @param iIcon the icon
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setIcon(const QString &iIcon);

    /**
     * Get the icon of the bank
     * @return the number
     */
    QString getIcon() const;

    /**
     * Get the current amount
     * @return the current amount
     */
    double getCurrentAmount() const;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGBankObject, Q_MOVABLE_TYPE);

#endif
