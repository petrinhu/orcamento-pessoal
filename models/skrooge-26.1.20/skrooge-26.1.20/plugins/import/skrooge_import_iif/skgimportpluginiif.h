/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINIIF_H
#define SKGIMPORTPLUGINIIF_H
/** @file
 * This file is Skrooge plugin for IIF import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgaccountobject.h"
#include "skgcategoryobject.h"
#include "skgimportplugin.h"
#include "skgpayeeobject.h"

/**
 * This file is Skrooge plugin for IIF import / export.
 */
class SKGImportPluginIif : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg arguments
     */
    explicit SKGImportPluginIif(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginIif() override;

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
    Q_DISABLE_COPY(SKGImportPluginIif)

    QString getVal(const QStringList &iVals, const QString &iAttribute) const;

    QMap<QString, QStringList> m_headers;
    QMap<QString, SKGAccountObject> m_accounts;
    QMap<QString, SKGCategoryObject> m_categories;
    QMap<QString, SKGPayeeObject> m_payees;
};

#endif // SKGIMPORTPLUGINIIF_H
