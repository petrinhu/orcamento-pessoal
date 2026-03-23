/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINOFX_H
#define SKGIMPORTPLUGINOFX_H
/** @file
 * This file is Skrooge plugin for OFX import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"
#include <libofx/libofx.h>

#include <qdatetime.h>

/**
 * This file is Skrooge plugin for OFX import / export.
 */
class SKGImportPluginOfx : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginOfx(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginOfx() override;

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
     * Return the mime type filter
     * @return the mime type filter. Example: "*.csv|CSV file"
     */
    QString getMimeTypeFilter() const override;

private:
    Q_DISABLE_COPY(SKGImportPluginOfx)

    static int ofxAccountCallback(struct OfxAccountData data, void *pv);
    static int ofxTransactionCallback(struct OfxTransactionData data, void *pv);
    static int ofxStatementCallback(struct OfxStatementData data, void *pv);
    static QStringList m_ofxInitialBalanceName;
    static QList<double> m_ofxInitialBalanceAmount;
    static QList<QDate> m_ofxInitialBalanceDate;
    static SKGError getAccount(OfxAccountData *iAccountData, SKGDocumentBank *iDoc, SKGAccountObject &oAccount);
    static QString getAccountName(OfxAccountData *iAccountData);

    static SKGError m_ofxError;
    static QMap<QString, SKGAccountObject> m_accounts;
};

#endif // SKGIMPORTPLUGINOFX_H
