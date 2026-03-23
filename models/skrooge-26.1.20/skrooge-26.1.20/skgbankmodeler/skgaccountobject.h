/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGACCOUNTOBJECT_H
#define SKGACCOUNTOBJECT_H
/** @file
 * This file defines classes SKGAccountObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgnamedobject.h"
class SKGBankObject;
class SKGOperationObject;
class SKGUnitObject;
class SKGInterestObject;

/**
 * This class manages account object
 */
class SKGBANKMODELER_EXPORT SKGAccountObject : public SKGNamedObject
{
public:
    /**
     * Describe a interest item
     */
    struct SKGInterestItem {
        SKGObjectBase object; /**< The object (an operation, an interest or nothing for initial balance) */
        QDate date; /**< The date */
        QDate valueDate; /**< The valueDate */
        int base{}; /**< Base */
        double amount{}; /**< Amount */
        double coef{}; /**< Coef */
        double rate{}; /**< Rate */
        double annualInterest{}; /**< Annual Interest */
        double accruedInterest{}; /**< Accrued Interest */
    };

    using SKGInterestItemList = QVector<SKGAccountObject::SKGInterestItem>;

    /**
     * This enumerate defines type of account
     */
    enum AccountType {
        CURRENT, /**< to define a bank account*/
        CREDITCARD, /**< to define a credit card account*/
        INVESTMENT, /**< to define an account for investment */
        ASSETS, /**< to define a assets account */
        OTHER, /**< other kind of account */
        WALLET, /**< to define a wallet account */
        LOAN, /**< to define a loan account */
        SAVING, /**< to define a saving account */
        PENSION /**< to define a pension account */
    };
    /**
     * This enumerate defines type of account
     */
    Q_ENUM(AccountType)

    /**
     * Default constructor
     */
    explicit SKGAccountObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGAccountObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGAccountObject(const SKGAccountObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGAccountObject(const SKGNamedObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGAccountObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGAccountObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGAccountObject &operator=(const SKGAccountObject &iObject);

    /**
     * Destructor
     */
    virtual ~SKGAccountObject();

    /**
     * Get the parent bank
     * @param oBank the parent bank
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getBank(SKGBankObject &oBank) const;

    /**
     * Set the parent bank
     * @param iBank the parent bank
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setBank(const SKGBankObject &iBank);

    /**
     * Get the linked account
     * @param oAccount the linked account
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getLinkedAccount(SKGAccountObject &oAccount) const;

    /**
     * Get the list of accounts linked to this one
     * @param oAccounts the list of accounts linked
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getLinkedByAccounts(SKGListSKGObjectBase &oAccounts) const;

    /**
     * Set the linked account
     * @param iAccount the linked account
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setLinkedAccount(const SKGAccountObject &iAccount);

    /**
     * Set the initial balance of this account
     * @param iBalance the balance
     * @param iUnit the unit
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setInitialBalance(double iBalance, const SKGUnitObject &iUnit);

    /**
     * Get the initial balance of this account
     * @param oBalance the balance
     * @param oUnit the unit
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getInitialBalance(double &oBalance, SKGUnitObject &oUnit);

    /**
     * Set the number of the account
     * @param iNumber the number
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setNumber(const QString &iNumber);

    /**
     * Get the number of the account
     * @return the number
     */
    virtual QString getNumber() const;

    /**
     * Set the agency number of the account
     * @param iNumber the agency number
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setAgencyNumber(const QString &iNumber);

    /**
     * Get the agency number of the account
     * @return the number
     */
    virtual QString getAgencyNumber() const;

    /**
     * Set the agency address of the account
     * @param iAddress the agency address
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setAgencyAddress(const QString &iAddress);

    /**
     * Get the agency address of the account
     * @return the number
     */
    virtual QString getAgencyAddress() const;

    /**
     * Set the comment of account
     * @param iComment the comment
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setComment(const QString &iComment);

    /**
     * Get the comment of this account
     * @return the comment
     */
    virtual QString getComment() const;

    /**
     * Add a new transaction to this account
     * @param oOperation the created operation
     * @param iForce force the creation even if the account is closed
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError addOperation(SKGOperationObject &oOperation, bool iForce = false);

    /**
     * Get number of transactions
     * @return the number of transactions
     */
    virtual int getNbOperation() const;

    /**
     * Get all transactions of this account
     * @param oOperations all transactions of this account
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getOperations(SKGListSKGObjectBase &oOperations) const;

    /**
     * Get the current amount
     * @return the current amount
     */
    virtual double getCurrentAmount() const;

    /**
     * Get amount of the account at a date
     * @param iDate date
     * @param iOnlyCurrencies only the transactions based on currencies are taken into account, not shares
     * @return amount of the account
     */
    // cppcheck-suppress passedByValue
    virtual double getAmount(QDate iDate, bool iOnlyCurrencies = false) const;

    /**
     * Set the type of this account
     * @param iType the type
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setType(SKGAccountObject::AccountType iType);

    /**
     * Get the type of this account
     * @return the type
     */
    virtual SKGAccountObject::AccountType getType() const;

    /**
     * To set the closed attribute of an account
     * @param iClosed the closed attribute: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setClosed(bool iClosed);

    /**
     * To know if the account has been closed or not
     * @return an object managing the error
     *   @see SKGError
     */
    virtual bool isClosed() const;

    /**
     * To bookmark or not on an account
     * @param iBookmark the bookmark: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError bookmark(bool iBookmark);

    /**
     * To know if the account is bookmarked
     * @return an object managing the error
     *   @see SKGError
     */
    virtual bool isBookmarked() const;

    /**
     * To enable the max amount limit or not on an account
     * @param iEnabled the state: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError maxLimitAmountEnabled(bool iEnabled);

    /**
     * To know if the account has a max amount limit
     * @return an object managing the error
     *   @see SKGError
     */
    virtual bool isMaxLimitAmountEnabled() const;

    /**
     * To set the max amount limit
     * @param iAmount the amount (in account's unit)
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setMaxLimitAmount(double iAmount);

    /**
     * To get the max amount limit (in account's unit)
     * @return an object managing the error
     *   @see SKGError
     */
    virtual double getMaxLimitAmount() const;

    /**
     * To enable the min amount limit or not on an account
     * @param iEnabled the state: true or false
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError minLimitAmountEnabled(bool iEnabled);

    /**
     * To know if the account has a min amount limit
     * @return an object managing the error
     *   @see SKGError
     */
    virtual bool isMinLimitAmountEnabled() const;

    /**
     * To set the min amount limit
     * @param iAmount the amount (in account's unit)
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setMinLimitAmount(double iAmount);

    /**
     * To get the min amount limit (in account's unit)
     * @return an object managing the error
     *   @see SKGError
     */
    virtual double getMinLimitAmount() const;

    /**
     * Set reconciliation date of this account
     * @param iDate the date
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    virtual SKGError setReconciliationDate(QDate iDate);

    /**
     * Get reconciliation date of this account
     * @return the date
     */
    virtual QDate getReconciliationDate() const;

    /**
     * Set reconciliation balance of this account
     * @param iAmount the balance
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setReconciliationBalance(double iAmount);

    /**
     * Get reconciliation balance of this account
     * @return the balance
     */
    virtual double getReconciliationBalance() const;

    /**
     * Get the unit
     * @param oUnit the unit
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getUnit(SKGUnitObject &oUnit) const;

    /**
     * Add an interest
     * @param oInterest the created interest
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError addInterest(SKGInterestObject &oInterest);

    /**
     * Get interests
     * @param oInterestList the list of interest in this account
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getInterests(SKGListSKGObjectBase &oInterestList) const;

    /**
     * Get interest object at a given date
     * @param iDate the date
     * @param oInterest the interest object
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    virtual SKGError getInterest(QDate iDate, SKGInterestObject &oInterest) const;

    /**
     * Get and compute interest items
     * @param oInterestList interest items
     * @param oInterests sum of annual interests
     * @param iYear the year of computation (default=current year)
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getInterestItems(SKGAccountObject::SKGInterestItemList &oInterestList, double &oInterests, int iYear = 0) const;

    /**
     * Compute all items
     * @param ioInterestList interest items
     * @param oInterests sum of annual interests
     * @param iYear the year of computation (default=current year)
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError computeInterestItems(SKGAccountObject::SKGInterestItemList &ioInterestList, double &oInterests, int iYear = 0) const;

    /**
     * Try to mark all imported transactions to obtain the expected balance
     * @param iBalance the expected balance in the account unit
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError autoReconcile(double iBalance);

    /**
     * Merge iAccount in current account
     * @param iAccount the account. All transactions will be transferred into this account. The account will be removed
     * @param iMergeInitalBalance to merge the initial balances too
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError merge(const SKGAccountObject &iAccount, bool iMergeInitalBalance);

    /**
     * Create a Transfer transaction to balance a credit card account (based on marked transactions)
     * @param iTargetAccount the target account  (should be the bank account)
     * @param iDate the date of the balance operation
     * @return an object managing the error
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    virtual SKGError transferDeferredOperations(const SKGAccountObject &iTargetAccount, QDate iDate = QDate::currentDate());

    /**
     * Get all list of transactions that can be checked to reconcile with target balance
     * @param iTargetBalance the target balance (in the unit of the account)
     * @param iSearchAllPossibleReconciliation to search all possible reconciliation
     * @return the list of possible solutions
     */
    virtual QVector<QVector<SKGOperationObject>> getPossibleReconciliations(double iTargetBalance, bool iSearchAllPossibleReconciliation = true) const;

private:
    /**
     * Get all list of transactions that can be checked to reconcile with target balance
     * @param iOperations the transactions to take into account
     * @param iBalance the current balance (in the unit of the account)
     * @param iTargetBalance the target balance (in the unit of the account)
     * @param iUnit the unit of the account (for better performances)
     * @param iSearchAllPossibleReconciliation to search all possible reconciliation
     * @return the list of possible solutions
     */
    QVector<QVector<SKGOperationObject>> getPossibleReconciliations(const SKGObjectBase::SKGListSKGObjectBase &iOperations,
                                                                    double iBalance,
                                                                    double iTargetBalance,
                                                                    const SKGServices::SKGUnitInfo &iUnit,
                                                                    bool iSearchAllPossibleReconciliation) const;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGAccountObject, Q_MOVABLE_TYPE);
#endif
