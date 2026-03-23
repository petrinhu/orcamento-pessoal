/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTEXPORTMANAGER_H
#define SKGIMPORTEXPORTMANAGER_H
/** @file
 * This file defines classes SKGImportExportManager.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qdom.h>
#include <qmap.h>
#include <qobject.h>
#include <qstringlist.h>
#include <qurl.h>

#include "skgaccountobject.h"
#include "skgbankmodeler_export.h"
#include "skgerror.h"

class SKGDocumentBank;
class SKGAccountObject;
class SKGUnitObject;
class QDate;
class SKGImportPlugin;

/**
 *Manage import and export
 */
class SKGBANKMODELER_EXPORT SKGImportExportManager : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     * @param iDocument the document
     * @param iFileName the file name
     */
    explicit SKGImportExportManager(SKGDocumentBank *iDocument = nullptr, const QUrl &iFileName = QUrl(QString()));

    /**
     * Destructor
     */
    virtual ~SKGImportExportManager() override;

    /**
     * Set the codec used for imports.
     * @param iCodec the codec name.
     */
    inline void setCodec(const QString &iCodec)
    {
        m_codec = iCodec;
    }

    /**
     * Get the codec used for imports.
     * @return code.
     */
    inline QString getCodec() const
    {
        return m_codec;
    }

    /**
     * Set the default account for import in case of account is not detected in imported file.
     * @param iAccount the account where to import. nullptr if you want to create a specific account for that.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setDefaultAccount(SKGAccountObject *iAccount);

    /**
     * Set the default unit for import in case of unit is not detected in imported file.
     * @param iUnit the unit where to import. nullptr if you want to create a specific unit for that.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError setDefaultUnit(SKGUnitObject *iUnit);

    /**
     * Enable/disable the automatically validation of imported transactions.
     * @param iValidation the mode.
     */
    void setAutomaticValidation(bool iValidation);

    /**
     * get the automatic validation state
     * @return The automatic validation state.
     */
    bool automaticValidation() const;

    /**
     * Enable/disable the automatically rules application on imported transactions.
     * @param iApply the mode.
     */
    void setAutomaticApplyRules(bool iApply);

    /**
     * get the automatic apply rules state
     * @return The automatic apply rules state.
     */
    bool automaticApplyRules() const;

    /**
     * To authorize the import of transactions after the last imported transaction
     * @param iSinceLast the mode.
     */
    void setSinceLastImportDate(bool iSinceLast);

    /**
     * get the since last import state
     * @return The since last import state.
     */
    bool sinceLastImportDate() const;

    /**
     * Get parameters for Export
     * @return the parameters
     */
    QMap<QString, QString> getExportParameters();

    /**
     * Set parameters for Export
     * @param iParameters the parameters
     */
    void setExportParameters(const QMap<QString, QString> &iParameters);

    /**
     * Get parameters for Import
     * @return the parameters
     */
    QMap<QString, QString> getImportParameters();

    /**
     * Set parameters for Import
     * @param iParameters the parameters
     */
    void setImportParameters(const QMap<QString, QString> &iParameters);

    /**
     * Get the default value of a parameter
     * @param iParameter the parameter
     */
    QString getParameterDefaultValue(const QString &iParameter);

    /**
     * Get the mime type filter for import
     * @param iIncludingAll to include the "All supported format"
     * @return the mime type filter
     */
    QString getImportMimeTypeFilter(bool iIncludingAll = true);

    /**
     * Import the file in the document
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError importFile();

    /**
     * Get the mime type filter for export
     * @param iIncludingAll to include the "All supported format"
     * @return the mime type filter
     */
    QString getExportMimeTypeFilter(bool iIncludingAll = true);

    /**
     * Export the file in the document
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError exportFile();

    /**
     * Anonymize the document.
     * This function must not be launched into a transaction
     * @param iKey the key for anonymisation. If "" then the anonymisation is not reversible, else the anonymisation can be undone with the same key.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError anonymize(const QString &iKey);

    /**
     * Find and group transactions
     * @param oNbOperationsMerged returns the number of transactions merged.
     * @param iAdditionnalCondition a condition on transactions to check (eg. A.t_imported='T' AND B.t_imported='T'")
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError findAndGroupTransfers(int &oNbOperationsMerged, const QString &iAdditionnalCondition);

    /**
     * Find and group transactions
     * @param oNbOperationsMerged returns the number of transactions merged.
     * @param iOnCurrentlyImport to apply the grouping only on currently imported transactions.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError findAndGroupTransfers(int &oNbOperationsMerged, bool iOnCurrentlyImport = false);

    /**
     * Clean transactions after an import coming from bank's web sites
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError cleanBankImport();

    /**
     * Get the document
     * @return the document.
     */
    inline SKGDocumentBank *getDocument()
    {
        return m_document;
    }

    /**
     * Get the file name extension
     * @return the file name.
     */
    QString getFileNameExtension() const;

    /**
     * Get the file name
     * @return the file name.
     */
    QUrl getFileName() const;

    /**
     * Get the local file name
     * @param iDownload create the local file by downloading the file.
     * @return the local file name.
     */
    QString getLocalFileName(bool iDownload = true);

    /**
     * Return the default account for import
     * @param oAccount the default account for import.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getDefaultAccount(SKGAccountObject &oAccount);

    /**
     * Return the preferred unit for a date for import
     * @param oUnit the default unit for import.
     * @param iDate the date.
     * @brief
     * If @see setDefaultUnit is used then getDefaultUnit will return this unit.
     * else return the unit compatible with entry date and with a value nearest than 1
     * else a new unit is created and returned
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError getDefaultUnit(SKGUnitObject &oUnit, const QDate *iDate = nullptr);

    /**
     * Finalize an import by changing state and applying rules
     *   @see SKGError
     */
    SKGError finalizeImportation();

    /**
     * Add an account to check during finalization
     * @param iAccount the account.
     * @param iBalance the balance.
     */
    void addAccountToCheck(const SKGAccountObject &iAccount, double iBalance);

    /**
     * get accounts to check
     * @return the accounts and balances to check.
     */
    QList<QPair<SKGAccountObject, double>> getAccountsToCheck();

    /**
     * Import a XML document
     * @param docElem The document to import.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError importItems(QDomElement &docElem);

private:
    Q_DISABLE_COPY(SKGImportExportManager)

    SKGImportPlugin *getImportPlugin();
    SKGImportPlugin *getExportPlugin();

    void loadPlugins();
    QMap<QString, SKGObjectBase> m_mapIdObject;
    SKGDocumentBank *m_document;
    QUrl m_fileName;
    QString m_localFileName;
    SKGAccountObject *m_defaultAccount;
    SKGUnitObject *m_defaultUnit;
    QString m_codec;
    bool m_automaticValidationOfImportedOperation{};
    bool m_automaticApplyRulesOfImportedOperation{};
    bool m_since_last_import;
    SKGImportPlugin *m_importPlugin;
    SKGImportPlugin *m_exportPlugin;
    QList<QPair<SKGAccountObject, double>> m_AccountToCheck;
    QList<SKGImportPlugin *> m_plugins;
};

#endif
