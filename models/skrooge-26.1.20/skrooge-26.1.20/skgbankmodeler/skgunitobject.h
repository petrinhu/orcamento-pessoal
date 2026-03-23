/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNITOBJECT_H
#define SKGUNITOBJECT_H
/** @file
 * This file defines classes SKGUnitObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qurl.h>

#include "skgbankmodeler_export.h"
#include "skgerror.h"
#include "skgnamedobject.h"
#include "skgservices.h"

class SKGUnitValueObject;
class SKGDocumentBank;

/**
 * This class is a unit
 */
class SKGBANKMODELER_EXPORT SKGUnitObject final : public SKGNamedObject
{
public:
    /**
     * This enumerate defines type of units
     */
    enum UnitType {
        PRIMARY, /**< to define the main currency (only one)*/
        SECONDARY, /**< to define the secondary currency (only one)*/
        CURRENCY, /**< for currency: dollar, euro, yen, … */
        SHARE, /**< for stock: Google, EDF, … */
        INDEX, /**< for index: CAC 40, … */
        OBJECT /**< for objects: cars, houses, … */
    };
    /**
     * This enumerate defines type of units
     */
    Q_ENUM(UnitType)

    /**
     * This enumerate defines the mode of download
     */
    enum UnitDownloadMode {
        LAST, /**< last value only*/
        LAST_MONTHLY, /**< one value par month since last download*/
        LAST_WEEKLY, /**< one value par week since last download*/
        LAST_DAILY, /**< one value par day since last download*/
        ALL_MONTHLY, /**< one value par month*/
        ALL_WEEKLY, /**< one value par week*/
        ALL_DAILY /**< one value par day*/
    };
    /**
     * This enumerate defines the mode of download
     */
    Q_ENUM(UnitDownloadMode)

    /**
     * Default constructor
     */
    explicit SKGUnitObject();

    /**
     * Constructor
     * @param iDocument the document containing the object
     * @param iID the identifier in @p iTable of the object
     */
    explicit SKGUnitObject(SKGDocument *iDocument, int iID = 0);

    /**
     * Destructor
     */
    virtual ~SKGUnitObject();

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    SKGUnitObject(const SKGUnitObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGUnitObject(const SKGNamedObject &iObject);

    /**
     * Copy constructor
     * @param iObject the object to copy
     */
    explicit SKGUnitObject(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGUnitObject &operator=(const SKGObjectBase &iObject);

    /**
     * Operator affectation
     * @param iObject the object to copy
     */
    SKGUnitObject &operator=(const SKGUnitObject &iObject);

    /**
     * Get unit information
     * @return the unit information
     *   @see SKGUnitInfo
     */
    SKGServices::SKGUnitInfo getUnitInfo();

    /**
     * Get unit information
     * @param iUnitName the currency unit name (nls), or the international code, or the symbol (@see getListofKnownCurrencies)
     * @return the unit information
     *   @see SKGUnitInfo
     */
    static SKGServices::SKGUnitInfo getUnitInfo(const QString &iUnitName);

    /**
     * Create an existing currency unit
     * @param iDocument the document containing the object
     * @param iUnitName the currency unit name (nls), or the international code, or the symbol (@see getListofKnownCurrencies)
     * @param oUnit the created unit object
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError createCurrencyUnit(SKGDocumentBank *iDocument, const QString &iUnitName, SKGUnitObject &oUnit);

    /**
     * Return the list of known currencies.
     * @param iIncludingObsolete including obsolete currencies
     * @return the list of known currencies
     */
    static QStringList getListofKnownCurrencies(bool iIncludingObsolete = false);

    /**
     * Return the International code of currencies.
     * @param iUnitName the currency unit name (nls)
     * @return the International code
     */
    static QString getInternationalCode(const QString &iUnitName);

    /**
     * Convert a value from one unit to another one
     * @param iValue initial value
     * @param iUnitFrom source unit
     * @param iUnitTo target unit
     * @param iDate the date to use to compute the units values
     * @return Value in target unit
     */
    // cppcheck-suppress passedByValue
    static double convert(double iValue, const SKGUnitObject &iUnitFrom, const SKGUnitObject &iUnitTo, QDate iDate = QDate::currentDate());

    /**
     * Set the symbol of this unit
     * @param iSymbol the symbol
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setSymbol(const QString &iSymbol);

    /**
     * Get the symbol of this unit
     * @return the symbol
     */
    QString getSymbol() const;

    /**
     * Set the download source of this unit
     * @param iSource the download source ("" means native source)
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setDownloadSource(const QString &iSource);

    /**
     * Get the download source of this unit
     * @return the download source ("" means native source)
     */
    QString getDownloadSource() const;

    /**
     * Set the number of decimal of this unit
     * @param iNb number of decimal
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setNumberDecimal(int iNb);

    /**
     * Get the number of decimal of this unit
     * @return the number of decimal
     */
    int getNumberDecimal() const;

    /**
     * Set the country of this unit
     * @param iCountry the country
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setCountry(const QString &iCountry);

    /**
     * Get the country of this unit
     * @return the country
     */
    QString getCountry() const;

    /**
     * Set the Internet code of this unit
     * @param iCode the Internet code
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setInternetCode(const QString &iCode);

    /**
     * Get the Internet code of this unit
     * @return the Internet code
     */
    QString getInternetCode() const;

    /**
     * Set the type of this unit
     * @param iType the type
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setType(SKGUnitObject::UnitType iType);

    /**
     * Get the type of this unit
     * @return the type
     */
    SKGUnitObject::UnitType getType() const;

    /**
     * Add a unit value
     * @param oUnitValue the created unit value
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError addUnitValue(SKGUnitValueObject &oUnitValue);

    /**
     * Get unit values
     * @param oUnitValueList the list of unit values of this unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getUnitValues(SKGListSKGObjectBase &oUnitValueList) const;

    /**
     * Get last unit value
     * @param oUnitValue the last unit value
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getLastUnitValue(SKGUnitValueObject &oUnitValue) const;

    /**
     * Get unit value at a date
     * @param iDate the date
     * @param oUnitValue the last unit value
     * @return an object managing the error.
     *   @see SKGError
     */
    // cppcheck-suppress passedByValue
    SKGError getUnitValue(QDate iDate, SKGUnitValueObject &oUnitValue) const;

    /**
     * Download values
     * @param iMode download mode (used only for native source)
     * @param iNbMaxValues nb max of values to download (used only for native source)
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError downloadUnitValue(UnitDownloadMode iMode = LAST, int iNbMaxValues = 50);

    /**
     * Open the URL where the values are downloaded
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError openURL() const;

    /**
     * Get the URL where the values are downloaded
     * @param oUrl the url
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getUrl(QUrl &oUrl) const;

    /**
     * Get the list of sources for the download
     * @return the list of sources
     */
    static QStringList downloadSources();

    /**
     * Get the source comment/help
     * @param iSource the source name (@see  downloadSources)
     * @return the comment/help
     */
    static QString getCommentFromSource(const QString &iSource);

    /**
     * Create a new source
     * @param iNewSource the name of the new source
     * @param iOpenSource to open the created source with the appropriate editor
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError addSource(const QString &iNewSource, bool iOpenSource = true);

    /**
     * To know if a source can be modified or deleted
     * @param iSource the name of the source to modified or deleted
     * @return true or false
     */
    static bool isWritable(const QString &iSource);

    /**
     * Delete a new source
     * @param iSource the name of the source to delete
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError deleteSource(const QString &iSource);

    /**
     * Get amount of the unit at a date
     * @param iDate date
     * @return amount of the unit
     */
    // cppcheck-suppress passedByValue
    double getAmount(QDate iDate = QDate::currentDate()) const;

    /**
     * Get daily change in percentage at a date.
     * @param iDate date
     * @return daily change
     */
    // cppcheck-suppress passedByValue
    double getDailyChange(QDate iDate = QDate::currentDate()) const;

    /**
     * Set the unit
     * @param iUnit the unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError setUnit(const SKGUnitObject &iUnit);

    /**
     * Remove the unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError removeUnit();

    /**
     * Get the unit
     * @param oUnit the unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getUnit(SKGUnitObject &oUnit) const;

    /**
     * Split unit.
     * All quantity in transactions will be multiply by iRatio.
     * All unit values will be divise by iRatio.
     * @param iRatio the split ratio
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError split(double iRatio) const;

    /**
     * Get all transactions of this unit
     * @param oOperations all transactions of this unit
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError getOperations(SKGListSKGObjectBase &oOperations) const;

    /**
     * Merge iUnit in current unit
     * @param iUnit the unit. All transactions will be transferred into this unit. The unit will be removed
     * @return an object managing the error
     *   @see SKGError
     */
    SKGError merge(const SKGUnitObject &iUnit);

    /**
     * Remove all unit values not relevant.
     * The curve is preserved but useless values are removed.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError simplify();

protected:
    /**
     * Get where clause needed to identify objects.
     * For this class, the whereclause is based on name
     * @return the where clause
     */
    QString getWhereclauseId() const override;

private:
    static QList<SKGServices::SKGUnitInfo> currencies;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGUnitObject, Q_MOVABLE_TYPE);
#endif
