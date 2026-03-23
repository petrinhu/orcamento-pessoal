/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGIMPORTPLUGINMNY_H
#define SKGIMPORTPLUGINMNY_H
/** @file
 * This file is Skrooge plugin for Microsoft Money import.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qset.h>

#include "skgimportplugin.h"

class SKGUnitObject;
class SKGAccountObject;
class SKGCategoryObject;
class SKGTrackerObject;
class SKGPayeeObject;

/**
 * This file is Skrooge plugin for KMY import / export.
 */
class SKGImportPluginMny : public SKGImportPlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGImportPlugin)

public:
    /**
     * Default constructor
     * @param iImporter the parent importer
     * @param iArg the arguments
     */
    explicit SKGImportPluginMny(QObject *iImporter, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGImportPluginMny() override;

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
    Q_DISABLE_COPY(SKGImportPluginMny)

    bool removeDir(const QString &dirName);

    SKGError readJsonFile(const QString &iFileName, QVariant &oVariant);

    static QMap<QString, SKGUnitObject> m_mapIdSecurity;
    static QMap<QString, SKGAccountObject> m_mapIdAccount;
    static QMap<QString, SKGCategoryObject> m_mapIdCategory;
    static QMap<QString, SKGTrackerObject> m_mapIdTracker;
    static QMap<QString, SKGPayeeObject> m_mapIdPayee;
};

#endif // SKGIMPORTPLUGINMNY_H
