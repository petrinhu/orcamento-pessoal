/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDOCUMENTBANK_H
#define SKGDOCUMENTBANK_H
/** @file
 * This file defines classes SKGDocumentBank.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbankmodeler_export.h"
#include "skgdefinebank.h"
#include "skgdocument.h"

class SKGUnitValueObject;

/**
 * This class manages skg bank documents
 */
class SKGBANKMODELER_EXPORT SKGDocumentBank : public SKGDocument
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.skrooge.SKGDocumentBank")

public:
    /**
     * Constructor
     */
    explicit SKGDocumentBank();

    /**
     * Destructor
     */
    ~SKGDocumentBank() override;

    /**
     * dump the document in the std output.
     * It is useful for debug.
     * @param iMode to select what you want to dump.
     * @code
     * document->dump (DUMPUNIT|DUMPPARAMETERS);
     * @endcode
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError dump(int iMode = DUMPBANKOBJECT) const override;

    /**
     * Create or modify an account
     * @param iName account name
     * @param iNumber account number
     * @param iBankName name of the bank
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError addOrModifyAccount(const QString &iName, const QString &iNumber, const QString &iBankName) const;

    /**
     * Create or modify the value of an unit
     * @param iUnitName unit name
     * @param iDate date
     * @param iValue unit value for the date @p iDate
     * @param oValue this output unit value
     * @return an object managing the error.
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    virtual SKGError addOrModifyUnitValue(const QString &iUnitName, QDate iDate, double iValue, SKGUnitValueObject *oValue = nullptr) const;

    /**
     * Get Primary unit. WARNING: This value can be not uptodated in a transaction.
     * @return Primary unit.
     */
    virtual SKGServices::SKGUnitInfo getPrimaryUnit() const;

    /**
     * Get Secondary unit. WARNING: This value can be not uptodated in a transaction.
     * @return Secondary unit.
     */
    virtual SKGServices::SKGUnitInfo getSecondaryUnit() const;

    /**
     * Get the preferred category for a payee. WARNING: This value can be not uptodated in a transaction.
     * @param iPayee the payee
     * @param iComputeAllPayees compute all categories for all payees and put them in cache. This is better when you know that you will need all.
     * @return The preferred category.
     */
    virtual QString getCategoryForPayee(const QString &iPayee, bool iComputeAllPayees = true) const;

    /**
     * Return the number version of views, indexes and triggers
     * @return
     */
    virtual QString getViewsIndexesAndTriggersVersion() const;

    /**
     * Get display schemas
     * @param iRealTable the real table name
     * @return list of schemas
     */
    SKGDocument::SKGModelTemplateList getDisplaySchemas(const QString &iRealTable) const override;

    /**
     * Get the display string for any modeler object (table, attribute)
     * @param iString the name of the object (example: v_operation, v_unit.t_name)
     * @return the display string
     */
    QString getDisplay(const QString &iString) const override;

    /**
     * Get the real attribute
     * @param iString the name of the attribute (something like t_BANK)
     * @return the real attribute (something like bank.rd_bank_id.t_name)
     */
    QString getRealAttribute(const QString &iString) const override;
    /**
     * Get the icon for attribute
     * @param iString the name of the attribute
     * @return the icon name
     */
    QString getIconName(const QString &iString) const override;

    /**
     * Get the attribute type
     * @param iAttributeName the name of an attribute
     * @return the type
     */
    SKGServices::AttributeType getAttributeType(const QString &iAttributeName) const override;

    /**
     * Get the file extension for this kind of document (must be overwritten)
     * @return file extension (like skg)
     */
    QString getFileExtension() const override;

    /**
     * Get the header of the file (useful for magic mime type).
     * @return document header
     */
    QString getDocumentHeader() const override;

    /**
     * Get budget report
     * @param iMonth the month
     * @return the report
     */
    virtual QVariantList getBudget(const QString &iMonth) const;

    /**
     * Get main categories report
     * @param iPeriod the period
     * @param iNb number of categories
     * @return the report
     */
    virtual QVariantList getMainCategories(const QString &iPeriod, int iNb = 5);

    /**
     * Get 5 main variation of categories report
     * @param iPeriod the period
     * @param iPreviousPeriod the previous period
     * @param iOnlyIssues only "Expenses increased" and "Incomes decreased"
     * @param oCategoryList to get the category for each variation
     * @return the list of variation string
     */
    virtual QStringList
    get5MainCategoriesVariationList(const QString &iPeriod, const QString &iPreviousPeriod, bool iOnlyIssues, QStringList *oCategoryList = nullptr);

    /**
     * Get the report
     * Do not forget to delete the pointer
     * @return the report
     */
    SKGReport *getReport() const override;

    /**
     * Refresh all views and indexes in the database
     * @param iForce force the refresh
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError refreshViewsIndexesAndTriggers(bool iForce = false) const override;

    /**
     * Get formatted money in primary unit
     * @param iValue value
     * @param iForcedNbOfDigit the number of digit (-1 means nb of digit of the unit)
     * @return formatted value in red or black
     */
    Q_INVOKABLE QString formatPrimaryMoney(double iValue, int iForcedNbOfDigit = -1) const override;

    /**
     * Get formatted money in primary unit
     * @param iValue value
     * @param iForcedNbOfDigit the number of digit (-1 means nb of digit of the unit)
     * @return formatted value in red or black
     */
    Q_INVOKABLE QString formatSecondaryMoney(double iValue, int iForcedNbOfDigit = -1) const override;

public Q_SLOTS:
    /**
     * Close the current transaction.
     * A transaction is needed to modify the SKGDocument.
     * This transaction is also used to manage the undo/redo.
     * @see beginTransaction
     * @param succeedded : true to indicate that current transaction has been successfully executed
     *                   : false to indicate that current transaction has failed
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError endTransaction(bool succeedded) override;

    /**
     * Enable/disable balances computation
     */
    virtual void setComputeBalances(bool iEnabled);

    /**
     * Refresh the case.
     * @param iTable the modified table triggering the cache refresh.
     */
    void refreshCache(const QString &iTable) const override;

protected:
    /**
     * Migrate the current SKGDocument to the latest version of the data model.
     * WARNING: This method must be used in a transaction.
     * @see beginTransaction
     * @param oMigrationDone to know if a migration has been done or not.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError migrate(bool &oMigrationDone) override;

    /**
     * Compute balance of each transaction.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError computeBalances() const;

    /**
     * Compute the budget suboperation links.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError computeBudgetSuboperationLinks() const;

private:
    Q_DISABLE_COPY(SKGDocumentBank)

    QString m_5mainVariations_inputs;
    QStringList m_5mainVariations_cache;
    QStringList m_5mainVariationsCat_cache;
    bool m_computeBalances{true};
    QStringList getMigationSteps();
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGDocumentBank, Q_COMPLEX_TYPE);
#endif
