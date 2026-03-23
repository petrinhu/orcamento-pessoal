/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINKMY_H
#define SKGIMPORTPLUGINKMY_H
/** @file
 * This file is Skrooge plugin for KMY import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qset.h>

#include "skgbankobject.h"
#include "skgimportplugin.h"

class SKGUnitObject;
class SKGAccountObject;
class SKGCategoryObject;
class SKGOperationObject;
class SKGPayeeObject;
class SKGObjectBase;
class QDomDocument;
class QDomElement;

/**
 * This file is Skrooge plugin for KMY import / export.
 */
class SKGImportPluginKmy : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginKmy(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginKmy() override;

    /**
     * To know if import is possible with this plugin
     */
    bool isImportPossible() override;

    /**
     * Import a file
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError importFile() override;

    /**
     * To know if export is possible with this plugin
     * @return true or false
     */
    bool isExportPossible() override;

    /**
     * Export a file
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError exportFile() override;

    /**
     * Return the mime type filter
     * @return the mime type filter. Example: "*.csv|CSV file"
     */
    QString getMimeTypeFilter() const override;

private:
    Q_DISABLE_COPY(SKGImportPluginKmy)

    SKGError importSecurities(QDomElement &docElem);
    SKGError importPrices(QDomElement &docElem);
    SKGError importInstitutions(QMap<QString, SKGBankObject> &mapIdBank, QDomElement &docElem);
    SKGError importPayees(QMap<QString, SKGPayeeObject> &mapIdPayee, QDomElement &docElem);
    SKGError importTransactions(QDomElement &docElem, SKGAccountObject &kmymoneyTemporaryAccount, QMap<QString, SKGPayeeObject> &mapIdPayee);
    SKGError importBudget(QDomElement &docElem);
    SKGError importAccounts(SKGBankObject &bank, SKGAccountObject &kmymoneyTemporaryAccount, QMap<QString, SKGBankObject> &mapIdBank, QDomElement &docElem);
    SKGError exportHeader(QDomDocument &doc, QDomElement &root);
    SKGError exportSecurities(QDomDocument &doc, QDomElement &root, const QString &stdUnit);
    SKGError exportBudgets(QDomDocument &doc, QDomElement &root);
    SKGError exportTransactions(QDomDocument &doc, QDomElement &root, const QString &stdUnit);
    SKGError exportPayees(QDomDocument &doc, QDomElement &root);
    SKGError exportSchedules(QDomDocument &doc, QDomElement &root);
    SKGError exportInstitutions(QDomDocument &doc, QDomElement &root);
    SKGError
    exportCategories(QDomDocument &doc, QDomElement &accounts, const QString &stdUnit, QDomElement &accountIncome, QDomElement &accountExpense, int nbAccount);
    SKGError exportAccounts(QDomDocument &doc,
                            QDomElement &root,
                            const QString &stdUnit,
                            QDomElement &accounts,
                            QDomElement &accountIncome,
                            QDomElement &accountExpense,
                            int &nbAccounts);

    static SKGError exportOperation(const SKGOperationObject &iOperation, QDomDocument &iDoc, QDomElement &iTransaction);
    static QString kmyValue(double iValue);
    static double toKmyValue(const QString &iString);

    static QString getKmyUniqueIdentifier(const SKGObjectBase &iObject);

    static QSet<QString> m_opTreated;
    static QMap<QString, SKGUnitObject> m_mapIdUnit;
    static QMap<QString, SKGAccountObject> m_mapIdAccount;
    static QMap<QString, SKGCategoryObject> m_mapIdCategory;
    static QMap<QString, SKGPayeeObject> m_mapIdPayee;
};

#endif // SKGIMPORTPLUGINKMY_H
