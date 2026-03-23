/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINCSV_H
#define SKGIMPORTPLUGINCSV_H
/** @file
 * This file is Skrooge plugin for CSV import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"

/**
 * This file is Skrooge plugin for CSV import / export.
 */
class SKGImportPluginCsv : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginCsv(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginCsv() override;

    /**
     * Set parameters for Import
     * @param iParameters the parameters
     */
    void setImportParameters(const QMap<QString, QString> &iParameters) override;

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

    /**
     * Import units
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError importCSVUnit();

    /**
     * Import rules
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError importCSVRule();

private:
    Q_DISABLE_COPY(SKGImportPluginCsv)

    /**
     * Set the CSV mapping.
     * A mapping is ordered list to described the mapping between the csv file and
     * the transaction object.
     * List of supported key word:
     * date
     * number
     * mode
     * payee
     * comment
     * status
     * bookmarked
     * account
     * category
     * amount
     * quantity
     * sign
     * unit
     * This list is a list of transaction attributes.
     * @param iCSVMapping the mapping. nullptr to build automatically the CSV mapping.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError setCSVMapping(const QStringList *iCSVMapping);

    /**
     * Set the index of the header in the CSV file.
     * @param iIndex the index. -1 to search automatically the index of the header.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError setCSVHeaderIndex(int iIndex = -1);

    /**
     * Get the index of the header in the CSV file.
     * @return the index
     */
    virtual int getCSVHeaderIndex();

    /**
     * Get the CSV separator
     * @param iLine the line to split to find the separator is not determined yet.
     * @return the separator
     */
    virtual QChar getCSVSeparator(const QString &iLine = QString());

    QStringList getCSVMappingFromLine(const QString &iLine);

    QStringList m_csvMapping;
    QChar m_csvSeparator;
    int m_csvHeaderIndex;
};

#endif // SKGIMPORTPLUGINCSV_H
