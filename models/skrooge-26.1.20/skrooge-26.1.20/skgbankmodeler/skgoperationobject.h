/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOPERATIONOBJECT_H
#define SKGOPERATIONOBJECT_H
/** @file
 * This file defines classes SKGOperationObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgobjectbase.h"
class SKGAccountObject;
class SKGUnitObject;
class SKGSubOperationObject;
class SKGRecurrentOperationObject;
class SKGPayeeObject;

/**
 * This class manages transaction object
 */
class SKGBANKMODELER_EXPORT SKGOperationObject final : public SKGObjectBase
{
public:
    /**
     * This enumerate defines status for transactions
     */
    enum OperationStatus {
        NONE, /**< no status */
        MARKED, /**< marked */
        CHECKED /**< checked */
    };
    /**
     * This enumerate defines status for transactions
     */
    Q_ENUM(OperationStatus)

    /**
     * This enumerate defines the alignment amount mode
     */
    enum AmountAlignmentMode {
        DEFAULT, /**< Default */
        PROPORTIONAL, /**< Proportional */
        ADDSUBOPERATION /**< Add sub transaction */
    };
    /**
     * This enumerate defines the alignment amount mode
     */
    Q_ENUM(AmountAlignmentMode)

    /**
     * Default constructor
     */
    explicit SKGOperationObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGOperationObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGOperationObject(const SKGObjectBase &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */

    SKGOperationObject(const SKGOperationObject &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGOperationObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGOperationObject &operator=(const SKGOperationObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGOperationObject();

    /**
     * Duplicate current transaction including subtransactions and grouped transactions
     * @param oOperation the created operation
     * @param iDate date(s) for new transaction(s)
     * @param iTemplateMode the template mode for new transaction(s)
     * @return an object managing the error.
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError duplicate(SKGOperationObject &oOperation, QDate iDate = QDate::currentDate(), bool iTemplateMode = false) const;

    /**
     * Get the parent account
     * @param oAccount the parent account
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getParentAccount(SKGAccountObject &oAccount) const;

    /**
     * Set the parent account
     * @param iAccount the parent account
     * @param iForce force the creation even if the account is closed
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setParentAccount(const SKGAccountObject &iAccount, bool iForce = false);

    /**
     * Set the mode of operation
     * @param iNumber the number
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setNumber(const QString &iNumber);

    /**
     * Get the number of this transaction
     * @return the number
     */
    QString getNumber() const;

    /**
     * Set the mode of operation
     * @param iMode the mode
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setMode(const QString &iMode);

    /**
     * Get the mode of this transaction
     * @return the mode
     */
    QString getMode() const;

    /**
     * Set the payee of operation
     * @param iPayee the payee
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setPayee(const SKGPayeeObject &iPayee);

    /**
     * Get the payee of this transaction
     * @param oPayee the payee
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getPayee(SKGPayeeObject &oPayee) const;

    /**
     * Set the comment of operation
     * @param iComment the comment
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setComment(const QString &iComment);

    /**
     * Get the comment of this transaction
     * @return the comment
     */
    QString getComment() const;

    /**
     * Get the status of this transaction
     * @return the status
     */
    SKGOperationObject::OperationStatus getStatus() const;

    /**
     * Set the status of operation
     * @param iStatus the status
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setStatus(SKGOperationObject::OperationStatus iStatus);

    /**
     * Set date of this transaction
     * @param iDate the date
     * @param iRefreshSubOperations to refresh the sub transactions
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError setDate(QDate iDate, bool iRefreshSubOperations = true);

    /**
     * Get date of this transaction
     * @return the date
     */
    QDate getDate() const;

    /**
     * Set the unit
     * @param iUnit the unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setUnit(const SKGUnitObject &iUnit);

    /**
     * Get the unit
     * @param oUnit the unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getUnit(SKGUnitObject &oUnit) const;

    /**
     * To know if a transaction is grouped
     * @return true or false
     */
    bool isInGroup() const;

    /**
     * To know if the current transaction is a transfer of the other one
     * @param oOperation the other operation
     * @return true or false
     */
    bool isTransfer(SKGOperationObject &oOperation) const;

    /**
     * Set the group operation
     * @param iOperation the transaction (itself to remove from group)
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setGroupOperation(const SKGOperationObject &iOperation);

    /**
     * Get the group operation
     * @param oOperation the operation
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getGroupOperation(SKGOperationObject &oOperation) const;

    /**
     * Get all transactions in the same group
     * @param oGroupedOperations all transaction in the same group
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getGroupedOperations(SKGListSKGObjectBase &oGroupedOperations) const;

    /**
     * To bookmark or not an operation
     * @param iBookmark the bookmark: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError bookmark(bool iBookmark);

    /**
     * To know if the transaction is bookmarked
     * @return an object managing the error
     *   @see SKGError
     */
    bool isBookmarked() const;

    /**
     * To set the imported attribute of an operation
     * @param iImported the imported status: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setImported(bool iImported);

    /**
     * To know if the transaction has been imported or not
     * @return an object managing the error
     *   @see SKGError
     */
    bool isImported() const;

    /**
     * Set the import identifier of operation, t_imported is set to 'T'
     * @param iImportID the import identifier (it is used to check if the transaction is already imported)
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setImportID(const QString &iImportID);

    /**
     * Get the import identifier of operation
     * @return the comment
     */
    QString getImportID() const;

    /**
     * To set the template attribute of an operation
     * @param iTemplate the template status: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setTemplate(bool iTemplate);

    /**
     * To know if the transaction is a template or not
     * @return an object managing the error
     *   @see SKGError
     */
    bool isTemplate() const;

    /**
     * Add a new suboperation to this transaction
     * @param oSubOperation the created suboperation
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError addSubOperation(SKGSubOperationObject &oSubOperation);

    /**
     * Get the number of sub transactions
     * @return number of sub transactions
     */
    int getNbSubOperations() const;

    /**
     * Get all subtransactions of this transaction
     * @param oSubOperations all subtransactions of this transaction
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getSubOperations(SKGListSKGObjectBase &oSubOperations) const;

    /**
     * Get the current amount
     * @return the current amount
     */
    double getCurrentAmount() const;

    /**
     * Get the account balance for this transaction
     * @return the account balance
     */
    double getBalance() const;

    /**
     * Get amount of the transaction at a date
     * @param iDate date
     * @return amount of the transaction
     */
    // cppcheck-suppress passedByValue
    double getAmount(QDate iDate) const;

    /**
     * Create a recurrent transaction based on this one
     * @param oRecurrentOperation the created recurrent operation
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError addRecurrentOperation(SKGRecurrentOperationObject &oRecurrentOperation) const;

    /**
     * Get the recurrent transactions based on this one
     * @param oRecurrentOperation the recurrent transactions
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getRecurrentOperations(SKGListSKGObjectBase &oRecurrentOperation) const;

    /**
     * Get the recurrent transaction ID this transaction is linked to
     * @return an object managing the error
     *   @see SKGError
     */
    int getRecurrentOperation() const;

    /**
     * Link this transaction to a particular recurrent one
     * @param recurrentId ID of the recurrent operation
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setRecurrentOperation(int recurrentId);

    /**
     * Merge current transaction with another one
     * @param iDeletedOne after merge this transaction will be deleted
     * @param iMode the alignment mode
     * @param iSendMessage send warning message
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError mergeAttribute(const SKGOperationObject &iDeletedOne,
                            SKGOperationObject::AmountAlignmentMode iMode = SKGOperationObject::DEFAULT,
                            bool iSendMessage = true);

    /**
     * Merge current transaction with another one
     * @param iDeletedOne after merge this transaction will be deleted
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError mergeSuboperations(const SKGOperationObject &iDeletedOne);
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGOperationObject, Q_MOVABLE_TYPE);
#endif
