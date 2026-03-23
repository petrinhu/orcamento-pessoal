/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for SKG import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginskg.h"

#include <qdir.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>
#include <quuid.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgpropertyobject.h"
#include "skgservices.h"
#include "skgtraces.h"

#define SQLDRIVERNAME QStringLiteral("SKGSQLCIPHER")
/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginSkg, "metadata.json")

SKGImportPluginSkg::SKGImportPluginSkg(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)

    m_importParameters[QStringLiteral("password")] = QString();
}

SKGImportPluginSkg::~SKGImportPluginSkg() = default;

bool SKGImportPluginSkg::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true
                                                 : m_importer->getFileNameExtension() == QStringLiteral("SKG")
                    || m_importer->getFileNameExtension() == QStringLiteral("SQLITE") || m_importer->getFileNameExtension() == QStringLiteral("SQLCIPHER"));
}

SKGError SKGImportPluginSkg::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    QString lfile = m_importer->getLocalFileName();
    if (lfile.isEmpty()) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
    } else {
        SKGDocumentBank docOrigin;
        err = docOrigin.load(lfile, m_importParameters.value(QStringLiteral("password")));
        IFOK(err)
        {
            QMap<QString, SKGObjectBase> mapOriginNew;
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "SKG"), 10);

            // Step 1 - units
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listUnits;
                err = docOrigin.getObjects(QStringLiteral("v_unit"), QStringLiteral("1=1 order by rd_unit_id, t_type"), listUnits);
                IFOK(err)
                {
                    int nb = listUnits.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGUnitObject unitOrigin(listUnits.at(i));
                        SKGUnitObject unit(unitOrigin.cloneInto(m_importer->getDocument()));
                        if (unit.load().isFailed()) {
                            // This unit does not exist yet
                            IFOK(err)
                            {
                                SKGUnitObject::UnitType unitType = unitOrigin.getType();
                                if (unitType == SKGUnitObject::PRIMARY || unitType == SKGUnitObject::SECONDARY) {
                                    unitType = SKGUnitObject::CURRENCY;
                                }
                                err = unit.setType(unitType);
                            }
                            IFOK(err)
                            {
                                SKGUnitObject parentUnitOrigin;
                                unitOrigin.getUnit(parentUnitOrigin);

                                SKGUnitObject parentUnit(mapOriginNew[parentUnitOrigin.getUniqueID()]);
                                if (parentUnit != unit) {
                                    err = unit.setUnit(parentUnit);
                                }
                            }
                            IFOKDO(err, unit.save(false))
                        }
                        mapOriginNew[unitOrigin.getUniqueID()] = unit;

                        // Duplicate properties
                        IFOKDO(err, copyParameters(unitOrigin, unit))

                        // Unit values
                        SKGObjectBase::SKGListSKGObjectBase listUnitsValues;
                        IFOKDO(err, unitOrigin.getUnitValues(listUnitsValues))
                        int nb2 = listUnitsValues.count();
                        for (int j = 0; !err && j < nb2; ++j) {
                            SKGUnitValueObject unitValueOrigin(listUnitsValues.at(j));

                            SKGUnitValueObject unitval;
                            err = unit.addUnitValue(unitval);
                            IFOKDO(err, unitval.setDate(unitValueOrigin.getDate()))
                            IFOKDO(err, unitval.setQuantity(unitValueOrigin.getQuantity()))
                            IFOKDO(err, unitval.save(true))

                            // Duplicate properties
                            IFOKDO(err, copyParameters(unitValueOrigin, unitval))
                        }
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Step 2 - bank and accounts
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listBanks;
                err = docOrigin.getObjects(QStringLiteral("v_bank"), QString(), listBanks);
                IFOK(err)
                {
                    int nb = listBanks.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import banks and accounts"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGBankObject bankOrigin(listBanks.at(i));
                        SKGBankObject bank(bankOrigin.cloneInto(m_importer->getDocument()));
                        if (bank.load().isFailed()) {
                            // This bank does not exist yet
                            IFOKDO(err, bank.save(false))
                        }

                        // Duplicate properties
                        IFOKDO(err, copyParameters(bankOrigin, bank))

                        // Accounts
                        SKGObjectBase::SKGListSKGObjectBase listAccounts;
                        IFOKDO(err, bankOrigin.getAccounts(listAccounts))
                        int nb2 = listAccounts.count();
                        for (int j = 0; !err && j < nb2; ++j) {
                            SKGAccountObject accountOrigin(listAccounts.at(j));
                            SKGAccountObject account(accountOrigin.cloneInto(m_importer->getDocument()));
                            if (account.load().isFailed()) {
                                // This account does not exist yet
                                IFOKDO(err, account.setBank(bank))
                                // Initial balance will be set on transaction creation
                                IFOKDO(err, account.save(false))
                            }

                            // Duplicate properties
                            IFOKDO(err, copyParameters(accountOrigin, account))

                            mapOriginNew[accountOrigin.getUniqueID()] = account;
                        }
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Step 3 - categories
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listCategories;
                err = docOrigin.getObjects(QStringLiteral("v_category"), QString(), listCategories);
                IFOK(err)
                {
                    int nb = listCategories.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import categories"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGCategoryObject catOrigin(listCategories.at(i));

                        SKGCategoryObject cat;
                        err = SKGCategoryObject::createPathCategory(m_importer->getDocument(), catOrigin.getFullName(), cat);
                        // Duplicate properties
                        IFOKDO(err, copyParameters(catOrigin, cat))
                        mapOriginNew[catOrigin.getUniqueID()] = cat;
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(3))

            // Step 4 - trackers
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listRefund;
                err = docOrigin.getObjects(QStringLiteral("v_refund"), QString(), listRefund);
                IFOK(err)
                {
                    int nb = listRefund.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import trackers"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGTrackerObject tracOrigin(listRefund.at(i));
                        SKGTrackerObject trac(tracOrigin.cloneInto(m_importer->getDocument()));
                        err = trac.save();
                        // Duplicate properties
                        IFOKDO(err, copyParameters(tracOrigin, trac))
                        mapOriginNew[tracOrigin.getUniqueID()] = trac;
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(4))

            // Step 5 - rules
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listRules;
                err = docOrigin.getObjects(QStringLiteral("v_rule"), QString(), listRules);
                IFOK(err)
                {
                    int nb = listRules.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import rules"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGRuleObject rulOrigin(listRules.at(i));
                        SKGRuleObject rul(rulOrigin.cloneInto(m_importer->getDocument()));
                        err = rul.save(false); // Save only
                        // Duplicate properties
                        IFOKDO(err, copyParameters(rulOrigin, rul))
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(5))

            // Step 6 - payee
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listPayee;
                err = docOrigin.getObjects(QStringLiteral("v_payee"), QString(), listPayee);
                IFOK(err)
                {
                    int nb = listPayee.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import payees"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGPayeeObject paylOrigin(listPayee.at(i));

                        SKGPayeeObject pay;
                        err = SKGPayeeObject::createPayee(m_importer->getDocument(), paylOrigin.getName(), pay);
                        IFOKDO(err, pay.setAddress(paylOrigin.getAddress()))
                        IFOKDO(err, pay.save())
                        // Duplicate properties
                        IFOKDO(err, copyParameters(paylOrigin, pay))
                        mapOriginNew[paylOrigin.getUniqueID()] = pay;
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(6))

            // Step 7 - transactions and suboperation
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listOperations;
                err = docOrigin.getObjects(QStringLiteral("v_operation"), QString(), listOperations);
                IFOK(err)
                {
                    int nb = listOperations.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGOperationObject operationOrigin(listOperations.at(i));
                        SKGOperationObject operation(operationOrigin.cloneInto(m_importer->getDocument()));
                        IFOKDO(err, operation.setAttribute(QStringLiteral("r_recurrentoperation_id"), QString()))
                        IFOKDO(err, operation.setImported(true))
                        IFOK(err)
                        {
                            QString importID = operationOrigin.getImportID();
                            if (importID.isEmpty()) {
                                importID = "SKG-" % SKGServices::intToString(operationOrigin.getID());
                            }
                            err = operation.setImportID(importID);
                        }
                        IFOK(err)
                        {
                            SKGAccountObject actOrig;
                            err = operationOrigin.getParentAccount(actOrig);
                            IFOK(err)
                            {
                                SKGAccountObject act(mapOriginNew[actOrig.getUniqueID()]);
                                act.setClosed(false); // To be sure that the modification is possible. NOT SAVED
                                IFOKDO(err, operation.setParentAccount(act))
                            }
                        }
                        IFOK(err)
                        {
                            SKGPayeeObject payeeOrig;
                            operationOrigin.getPayee(payeeOrig); // Error not managed
                            IFOKDO(err, operation.setPayee(SKGPayeeObject(mapOriginNew[payeeOrig.getUniqueID()])))
                        }
                        IFOK(err)
                        {
                            SKGUnitObject unitOrig;
                            err = operationOrigin.getUnit(unitOrig);
                            IFOKDO(err, operation.setUnit(SKGUnitObject(mapOriginNew[unitOrig.getUniqueID()])))
                        }
                        IFOK(err)
                        {
                            SKGOperationObject groupOrig;
                            operationOrigin.getGroupOperation(groupOrig);
                            err = operation.setGroupOperation(SKGOperationObject(mapOriginNew[groupOrig.getUniqueID()]));
                        }
                        IFOKDO(err, operation.save(false))

                        mapOriginNew[operationOrigin.getUniqueID()] = operation;

                        // Duplicate properties
                        IFOKDO(err, copyParameters(operationOrigin, operation))

                        // Sub operation
                        SKGObjectBase::SKGListSKGObjectBase listSuboperations;
                        IFOKDO(err, operationOrigin.getSubOperations(listSuboperations))
                        int nb2 = listSuboperations.count();
                        for (int j = 0; !err && j < nb2; ++j) {
                            SKGSubOperationObject subopOrigin(listSuboperations.at(j));

                            SKGSubOperationObject subop(subopOrigin.cloneInto(m_importer->getDocument()));
                            err = subop.setParentOperation(operation);
                            IFOK(err)
                            {
                                SKGCategoryObject catOrig;
                                subopOrigin.getCategory(catOrig); // Error not managed
                                err = subop.setCategory(SKGCategoryObject(mapOriginNew[catOrig.getUniqueID()]));
                            }
                            IFOK(err)
                            {
                                SKGTrackerObject tracOrig;
                                subopOrigin.getTracker(tracOrig); // Error not managed
                                IFOKDO(err, subop.setTracker(SKGTrackerObject(mapOriginNew[tracOrig.getUniqueID()]), true))
                            }
                            IFOKDO(err, subop.save(false))

                            // Duplicate properties
                            IFOKDO(err, copyParameters(subopOrigin, subop))
                        }
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))

                        if (!err && i % 500 == 0) {
                            err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                        }
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(7))
            /*                                      SKGObjectBase opWithThisHash;
                                                if ( SKGObjectBase::getObject ( m_importer->getDocument(), "operation", "t_imported IN ('Y','P') AND
               t_import_id='"+QString ( hash.toHex() ) +'\'', opWithThisHash ).isSucceeded() )*/

            // Step 8 - recurrent
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase ListRecurrentOperations;
                err = docOrigin.getObjects(QStringLiteral("v_recurrentoperation"), QString(), ListRecurrentOperations);
                IFOK(err)
                {
                    int nb = ListRecurrentOperations.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import scheduled transactions"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGRecurrentOperationObject recuOrigin(ListRecurrentOperations.at(i));
                        SKGRecurrentOperationObject recu(recuOrigin.cloneInto(m_importer->getDocument()));

                        SKGOperationObject opOrig;
                        err = recuOrigin.getParentOperation(opOrig);
                        IFOKDO(err, recu.setParentOperation(SKGOperationObject(mapOriginNew[opOrig.getUniqueID()])))
                        IFOKDO(err, recu.save(false))

                        // Duplicate properties
                        IFOKDO(err, copyParameters(recuOrigin, recu))

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(8))

            // Step 9 - nodes
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listNodes;
                err = docOrigin.getObjects(QStringLiteral("v_node"), QString(), listNodes);
                IFOK(err)
                {
                    int nb = listNodes.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import bookmarks"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGNodeObject nodeOrigin(listNodes.at(i));

                        SKGNodeObject node;
                        err = SKGNodeObject::createPathNode(m_importer->getDocument(),
                                                            i18n("Imported bookmarks") % OBJECTSEPARATOR % nodeOrigin.getFullName(),
                                                            node);
                        IFOKDO(err, node.setData(nodeOrigin.getData()))
                        IFOKDO(err, node.setOrder(nodeOrigin.getOrder()))
                        IFOKDO(err, node.setAutoStart(nodeOrigin.isAutoStart()))
                        IFOKDO(err, node.save(true, false))
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(9))

            // Step 10 - interest
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase listInterests;
                err = docOrigin.getObjects(QStringLiteral("v_interest"), QString(), listInterests);
                IFOK(err)
                {
                    int nb = listInterests.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import interests"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGInterestObject interestOrigin(listInterests.at(i));
                        SKGInterestObject interest(interestOrigin.cloneInto(m_importer->getDocument()));
                        IFOK(err)
                        {
                            SKGAccountObject actOrig;
                            err = interestOrigin.getAccount(actOrig);
                            IFOKDO(err, interest.setAccount(SKGAccountObject(mapOriginNew[actOrig.getUniqueID()])))
                        }
                        IFOKDO(err, interest.save())

                        // Duplicate properties
                        IFOKDO(err, copyParameters(interestOrigin, interest))

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(10))

            SKGENDTRANSACTION(m_importer->getDocument(), err)

            IFOKDO(err, m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE")))
        }
    }
    return err;
}

bool SKGImportPluginSkg::isExportPossible()
{
    SKGTRACEINFUNC(10)
    return isImportPossible();
}

SKGError SKGImportPluginSkg::exportFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    // Different modes:
    // In memory sqlite DB => skg : copySqliteDatabase + cryptFile
    // In memory sqlcipher DB => skg : sqlcipher_export + cryptFile
    // File sqlite DB => skg : cryptFile
    // File sqcipher DB => skg : cryptFile

    // In memory sqlite DB => sqlite : copySqliteDatabase + upload
    // In memory sqlcipher DB => sqlite : sqlcipher_export + upload
    // File sqlite DB => sqlite : upload
    // File sqcipher DB => sqlite : sqlcipher_export + upload

    // In memory sqlite DB => sqlcipher : sqlcipher_export + upload
    // In memory sqlcipher DB => sqlcipher : sqlcipher_export + upload
    // File sqlite DB => sqlcipher : sqlcipher_export + upload
    // File sqcipher DB => sqlcipher : sqlcipher_export + upload

    QString file = m_importer->getLocalFileName(false);
    QFile::remove(file);
    QString ext = QFileInfo(file).suffix().toUpper();
    QString pwd = m_importer->getDocument()->getPassword();

    QString tempFile = m_importer->getDocument()->getCurrentTemporaryFile();
    bool removeTempFile = false;
    QString DBMode = m_importer->getDocument()->getParameter(QStringLiteral("SKG_DATABASE_TYPE"));

    if (m_importer->getDocument()->getCurrentFileName().isEmpty() || DBMode != QStringLiteral("QSQLITE")
        || (ext == QStringLiteral("SQLCIPHER") && !pwd.isEmpty())) {
        // The database is only in memory
        tempFile = QDir::tempPath() % "/skg_" % QUuid::createUuid().toString() % ".skg";
        removeTempFile = true;

        auto fileDb = QSqlDatabase::addDatabase(ext == QStringLiteral("SQLITE") ? QStringLiteral("QSQLITE") : SQLDRIVERNAME, tempFile);
        fileDb.setDatabaseName(tempFile);
        if (!fileDb.open()) {
            // Set error message
            QSqlError sqlErr = fileDb.lastError();
            err = SKGError(SQLLITEERROR + sqlErr.nativeErrorCode().toInt(), sqlErr.text());
        } else {
            if (ext == QStringLiteral("SQLCIPHER") && !pwd.isEmpty()) {
                IFOKDO(err, SKGServices::executeSqliteOrder(fileDb, "PRAGMA KEY = '" % SKGServices::stringToSqlString(pwd) % "'"))
                IFOKDO(err,
                       m_importer->getDocument()->sendMessage(
                           i18nc("An information message", "The sqlcipher database has been protected with the same password than your document"),
                           SKGDocument::Information))
            }

            m_importer->getDocument()->getMainDatabase()->commit();
            if (DBMode == QStringLiteral("QSQLITE") && ext == QStringLiteral("QSQLITE")) {
                // The source database is a sqlite DB
                IFOKDO(err, SKGServices::copySqliteDatabase(fileDb, *(m_importer->getDocument()->getMainDatabase()), false))

            } else {
                // The source database is a sqcipher DB
                if (ext == QStringLiteral("SQLITE")) {
                    pwd = QString();
                }
                IFOKDO(err,
                       m_importer->getDocument()->executeSqliteOrders(
                           QStringList() << "ATTACH DATABASE '" % tempFile % "' AS sqlcipher KEY '" % SKGServices::stringToSqlString(pwd) % "'"
                                         << QStringLiteral("SELECT SQLCIPHER_EXPORT('sqlcipher')") << QStringLiteral("DETACH DATABASE sqlcipher")));
            }
            m_importer->getDocument()->getMainDatabase()->transaction();

            fileDb.close();
            QSqlDatabase::removeDatabase(tempFile);
        }
    }

    // Copy file to file
    IFOK(err)
    {
        if (ext != QStringLiteral("SKG")) {
            if (SKGServices::upload(QUrl::fromLocalFile(tempFile), QUrl::fromLocalFile(file))) {
                err.setReturnCode(ERR_FAIL).setMessage(i18nc("An error message", "Creation file '%1' failed", file));
            }
        } else {
            bool mode;
            err = SKGServices::cryptFile(tempFile, file, QString(), true, m_importer->getDocument()->getDocumentHeader(), mode);
            IFOK(err)
            {
                SKGDocumentBank doc;
                err = doc.load(file);
                IFOKDO(err, doc.removeAllTransactions())
                IFOKDO(err, doc.saveAs(file, true))
            }
        }
    }

    if (removeTempFile) {
        QFile(tempFile).remove();
    }

    return err;
}

SKGError SKGImportPluginSkg::copyParameters(const SKGObjectBase &iFrom, const SKGObjectBase &iTo)
{
    SKGError err;

    SKGObjectBase::SKGListSKGObjectBase params;
    err =
        iFrom.getDocument()->getObjects(QStringLiteral("parameters"), "t_uuid_parent='" % SKGServices::stringToSqlString(iFrom.getUniqueID()) % u'\'', params);
    IFOK(err)
    {
        int nb = params.count();
        SKGDocument *documentTarget = iTo.getDocument();
        for (int i = 0; !err && i < nb; ++i) {
            SKGPropertyObject orig(params.at(i));
            SKGPropertyObject param(orig.cloneInto(documentTarget));
            err = param.setAttribute(QStringLiteral("t_uuid_parent"), iTo.getUniqueID());
            IFOKDO(err, param.save(true, false))
        }
    }
    return err;
}

QString SKGImportPluginSkg::getMimeTypeFilter() const
{
    return "*.skg|" % i18nc("A file format", "Skrooge document") % '\n' % "*.sqlcipher|" % i18nc("A file format", "SQLCipher document") % '\n' % "*.sqlite|"
        % i18nc("A file format", "SQLite document");
}

#include <skgimportpluginskg.moc>
