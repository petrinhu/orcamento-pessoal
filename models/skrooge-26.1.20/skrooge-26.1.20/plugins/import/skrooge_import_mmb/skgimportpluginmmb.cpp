/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for MMB import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginmmb.h"

#include <qfile.h>
#include <qsqldatabase.h>
#include <qsqlerror.h>

#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginMmb, "metadata.json")

SKGImportPluginMmb::SKGImportPluginMmb(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginMmb::~SKGImportPluginMmb() = default;

bool SKGImportPluginMmb::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("MMB"));
}

SKGError SKGImportPluginMmb::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    {
        QSqlDatabase originalDocument(QSqlDatabase::addDatabase(QStringLiteral("SKGSQLCIPHER"), QStringLiteral("originalDocument")));
        originalDocument.setDatabaseName(m_importer->getLocalFileName());
        if (!originalDocument.open()) {
            // Set error message
            QSqlError sqlErr = originalDocument.lastError();
            err = SKGError(SQLLITEERROR + sqlErr.nativeErrorCode().toInt(), sqlErr.text());
        }
        IFOK(err)
        {
            QMap<QString, SKGUnitObject> mapUnit;
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "MMB"), 9);

            // Step 1 - units
            IFOK(err)
            {
                SKGStringListList listUnits;
                err =
                    SKGServices::executeSelectSqliteOrder(originalDocument,
                                                          QStringLiteral("SELECT CURRENCYID, CURRENCYNAME, PFX_SYMBOL, SFX_SYMBOL, DECIMAL_POINT, "
                                                                         "GROUP_SEPARATOR, UNIT_NAME, CENT_NAME, SCALE, BASECONVRATE, CURRENCY_SYMBOL, "
                                                                         "(CASE WHEN EXISTS (SELECT 1 FROM INFOTABLE_V1 WHERE INFONAME='BASECURRENCYID' AND "
                                                                         "INFOVALUE=CURRENCYID) THEN 'Y' ELSE 'N' END) AS PRIMARYUNIT FROM CURRENCYFORMATS_V1"),
                                                          listUnits);
                IFOK(err)
                {
                    int nb = listUnits.count() - 1;
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listUnits.at(i + 1);

                        SKGUnitObject unit(m_importer->getDocument());
                        err = unit.setName(attributes.at(1));
                        IFOK(err)
                        {
                            QString symbol = attributes.at(2);
                            if (symbol.isEmpty()) {
                                symbol = attributes.at(3);
                            }
                            err = unit.setSymbol(symbol);
                        }
                        if (!err && attributes.at(11) == QStringLiteral("Y")) {
                            err = unit.setType(SKGUnitObject::PRIMARY);
                        }
                        IFOKDO(err, unit.save())

                        mapUnit[attributes.at(0)] = unit;

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Step 2 - bank and accounts
            QMap<QString, SKGAccountObject> mapAccount;
            IFOK(err)
            {
                SKGStringListList listAccounts;
                err = SKGServices::executeSelectSqliteOrder(
                    originalDocument,
                    QStringLiteral("SELECT ACCOUNTID, ACCOUNTNAME, ACCOUNTTYPE, ACCOUNTNUM, STATUS, NOTES, HELDAT, WEBSITE, CONTACTINFO, ACCESSINFO, "
                                   "INITIALBAL, FAVORITEACCT, CURRENCYID FROM ACCOUNTLIST_V1"),
                    listAccounts);
                IFOK(err)
                {
                    int nb = listAccounts.count() - 1;
                    IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import banks and accounts"), nb))
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listAccounts.at(i + 1);

                        // Create bank
                        SKGBankObject bank(m_importer->getDocument());
                        IFOK(err)
                        {
                            QString bankName = attributes.at(6);
                            if (bankName.isEmpty()) {
                                bankName = QStringLiteral("MMEX");
                            }
                            err = bank.setName(bankName);
                        }
                        IFOKDO(err, bank.save())

                        // Create account
                        SKGAccountObject account;
                        err = bank.addAccount(account);
                        IFOKDO(err, account.setName(attributes.at(1)))
                        IFOKDO(err, account.setType(attributes.at(2) == QStringLiteral("Checking") ? SKGAccountObject::CURRENT : SKGAccountObject::INVESTMENT))
                        IFOKDO(err, account.setNumber(attributes.at(3)))
                        IFOKDO(err, account.setComment(attributes.at(5)))
                        IFOKDO(err, account.setAgencyAddress(attributes.at(8)))
                        IFOKDO(err, account.bookmark(attributes.at(11) == QStringLiteral("TRUE")))
                        IFOKDO(err, account.setClosed(attributes.at(4) != QStringLiteral("Open")))
                        IFOKDO(err, account.save())
                        IFOKDO(err, account.setInitialBalance(SKGServices::stringToDouble(attributes.at(10)), mapUnit[attributes.at(12)]))
                        IFOKDO(err, account.save())

                        mapAccount[attributes.at(0)] = account;

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Step 3 - categories
            QMap<QString, SKGCategoryObject> mapCategory;
            IFOK(err)
            {
                SKGStringListList listCategories;
                err = SKGServices::executeSelectSqliteOrder(originalDocument, QStringLiteral("SELECT CATEGID, CATEGNAME FROM CATEGORY_V1"), listCategories);
                IFOK(err)
                {
                    int nb = listCategories.count() - 1;
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import categories"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listCategories.at(i + 1);

                        SKGCategoryObject cat(m_importer->getDocument());
                        err = cat.setName(attributes.at(1));
                        IFOKDO(err, cat.save())

                        mapCategory[attributes.at(0)] = cat;

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(3))

            // Step 4 - sub categories
            QMap<QString, SKGCategoryObject> mapSubCategory;
            IFOK(err)
            {
                SKGStringListList listCategories;
                err = SKGServices::executeSelectSqliteOrder(originalDocument,
                                                            QStringLiteral("SELECT SUBCATEGID, SUBCATEGNAME, CATEGID FROM SUBCATEGORY_V1"),
                                                            listCategories);
                IFOK(err)
                {
                    int nb = listCategories.count() - 1;
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import categories"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listCategories.at(i + 1);

                        SKGCategoryObject cat(m_importer->getDocument());
                        err = cat.setName(attributes.at(1));
                        IFOKDO(err, cat.setParentCategory(mapCategory[attributes.at(2)]))
                        IFOKDO(err, cat.save())

                        mapSubCategory[attributes.at(0)] = cat;

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(4))

            // Step 5 - payee
            QMap<QString, SKGPayeeObject> mapPayee;
            IFOK(err)
            {
                SKGStringListList listPayee;
                err = SKGServices::executeSelectSqliteOrder(originalDocument, QStringLiteral("SELECT PAYEEID, PAYEENAME FROM PAYEE_V1"), listPayee);
                IFOK(err)
                {
                    int nb = listPayee.count() - 1;
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import payees"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listPayee.at(i + 1);

                        SKGPayeeObject payee(m_importer->getDocument());
                        err = payee.setName(attributes.at(1));
                        IFOKDO(err, payee.save())

                        mapPayee[attributes.at(0)] = payee;

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(5))

            // Step 6 - transactions
            QMap<QString, SKGOperationObject> mapOperation;
            IFOK(err)
            {
                SKGStringListList listOperations;
                err = SKGServices::executeSelectSqliteOrder(
                    originalDocument,
                    QStringLiteral("SELECT * FROM ("
                                   "SELECT TRANSID, ACCOUNTID, TOACCOUNTID, PAYEEID, TRANSCODE, TRANSAMOUNT, STATUS, TRANSACTIONNUMBER, NOTES, CATEGID, "
                                   "SUBCATEGID, TRANSDATE, FOLLOWUPID, TOTRANSAMOUNT, 'N/A'  , 'N/A', 'N/A' FROM CHECKINGACCOUNT_V1 UNION "
                                   "SELECT BDID,    ACCOUNTID, TOACCOUNTID, PAYEEID, TRANSCODE, TRANSAMOUNT, STATUS, TRANSACTIONNUMBER, NOTES, CATEGID, "
                                   "SUBCATEGID, TRANSDATE, FOLLOWUPID, TOTRANSAMOUNT, REPEATS, NEXTOCCURRENCEDATE, NUMOCCURRENCES FROM BILLSDEPOSITS_V1)"),
                    listOperations);
                IFOK(err)
                {
                    int nb = listOperations.count() - 1;
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listOperations.at(i + 1);
                        if (attributes.at(6) != QStringLiteral("V")) { // Void
                            bool recurrent = (attributes.at(14) != QStringLiteral("N/A"));
                            const QString &idTarget = attributes.at(2);
                            SKGAccountObject acc = mapAccount[attributes.at(1)];
                            SKGUnitObject unit;
                            IFOKDO(err, acc.getUnit(unit))

                            SKGOperationObject op;
                            IFOKDO(err, acc.addOperation(op, true))
                            IFOK(err)
                            {
                                const QString &id = attributes.at(3);
                                if (id != QStringLiteral("-1")) {
                                    err = op.setPayee(mapPayee[id]);
                                }
                            }
                            IFOKDO(err, op.setNumber(attributes.at(7)))
                            IFOKDO(err, op.setComment(attributes.at(8)))
                            IFOKDO(err, op.setDate(SKGServices::stringToDate(attributes.at(11))))
                            IFOKDO(err, op.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T")))
                            IFOKDO(err, op.setImportID("MMEX-" % attributes.at(0)))
                            IFOKDO(err, op.setUnit(unit))
                            IFOKDO(err, op.bookmark(attributes.at(6) == QStringLiteral("F")))
                            IFOKDO(err, op.setTemplate(recurrent))
                            IFOKDO(err, op.setStatus(attributes.at(6) == QStringLiteral("R") ? SKGOperationObject::CHECKED : SKGOperationObject::NONE))
                            IFOKDO(err, op.save())

                            // Get splits
                            IFOK(err)
                            {
                                SKGStringListList listSubOperations;
                                err = SKGServices::executeSelectSqliteOrder(originalDocument,
                                                                            QStringLiteral("SELECT SPLITTRANSID, CATEGID, SUBCATEGID, SPLITTRANSAMOUNT FROM ")
                                                                                % (recurrent ? "BUDGET" : "") % "SPLITTRANSACTIONS_V1 WHERE TRANSID="
                                                                                % attributes.at(0),
                                                                            listSubOperations);
                                IFOK(err)
                                {
                                    int nb2 = listSubOperations.count() - 1;
                                    if (nb2 <= 0) {
                                        // No split
                                        SKGSubOperationObject subop;
                                        IFOKDO(err, op.addSubOperation(subop))
                                        IFOK(err)
                                        {
                                            QString id = attributes.at(10);
                                            if (id == QStringLiteral("-1")) {
                                                id = attributes.at(9);
                                                if (id != QStringLiteral("-1")) {
                                                    err = subop.setCategory(mapSubCategory[id]);
                                                }
                                            } else {
                                                err = subop.setCategory(mapCategory[id]);
                                            }
                                        }
                                        IFOK(err)
                                        {
                                            double q = SKGServices::stringToDouble(attributes.at(5));
                                            if (attributes.at(4) == QStringLiteral("Withdrawal") || idTarget != QStringLiteral("-1")) {
                                                q = -q;
                                            }
                                            err = subop.setQuantity(q);
                                        }
                                        IFOKDO(err, subop.save())
                                    } else {
                                        // Has splits
                                        for (int j = 0; !err && j < nb2; ++j) {
                                            const QStringList &attributesSubOp = listSubOperations.at(j + 1);

                                            SKGSubOperationObject subop;
                                            IFOKDO(err, op.addSubOperation(subop))
                                            IFOK(err)
                                            {
                                                QString id = attributesSubOp.at(2);
                                                if (id == QStringLiteral("-1")) {
                                                    id = attributesSubOp.at(1);
                                                    if (id != QStringLiteral("-1")) {
                                                        err = subop.setCategory(mapCategory[id]);
                                                    }
                                                } else {
                                                    err = subop.setCategory(mapSubCategory[id]);
                                                }
                                            }
                                            IFOK(err)
                                            {
                                                double q = SKGServices::stringToDouble(attributesSubOp.at(3));
                                                if (attributes.at(4) == QStringLiteral("Withdrawal") || idTarget != QStringLiteral("-1")) {
                                                    q = -q;
                                                }
                                                err = subop.setQuantity(q);
                                            }
                                            IFOKDO(err, subop.save())
                                        }
                                    }
                                }
                            }

                            // Case Transfer
                            if (!err && idTarget != QStringLiteral("-1")) {
                                SKGOperationObject op2;
                                err = op.duplicate(op2, op.getDate());
                                IFOKDO(err, op2.setStatus(op.getStatus()))
                                IFOKDO(err, op2.bookmark(op.isBookmarked()))
                                IFOKDO(err, op2.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T")))
                                IFOKDO(err, op2.setImportID("MMEX-" % attributes.at(0) % "_TR"))
                                IFOKDO(err, op2.save())

                                SKGObjectBase::SKGListSKGObjectBase subops;
                                IFOKDO(err, op.getSubOperations(subops))
                                if (!err && !subops.isEmpty()) {
                                    SKGSubOperationObject subop2(subops.at(0));
                                    err = subop2.setQuantity(-subop2.getQuantity());
                                    IFOKDO(err, subop2.save())
                                }
                                IFOKDO(err, op.setParentAccount(mapAccount[idTarget]))
                                IFOKDO(err, op.setGroupOperation(op2))
                                IFOKDO(err, op.save())
                            }

                            // Create recurrent transaction
                            if (recurrent) {
                                SKGRecurrentOperationObject recu;
                                IFOKDO(err, op.addRecurrentOperation(recu))
                                IFOKDO(err, recu.setDate(SKGServices::stringToDate(attributes.at(15))))
                                int nbTimes = SKGServices::stringToInt(attributes.at(16));
                                if (!err && nbTimes != -1) {
                                    err = recu.setTimeLimit(nbTimes);
                                    IFOKDO(err, recu.timeLimit(true))
                                }
                                int repeats = SKGServices::stringToInt(attributes.at(14));
                                IFOK(err)
                                {
                                    switch (repeats) {
                                    case 1:
                                        // Weekly
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::WEEK);
                                        IFOKDO(err, recu.setPeriodIncrement(1))
                                        break;
                                    case 2:
                                        // Bi-Weekly
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::WEEK);
                                        IFOKDO(err, recu.setPeriodIncrement(2))
                                        break;
                                    case 3:
                                        // Monthly
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::MONTH);
                                        IFOKDO(err, recu.setPeriodIncrement(1))
                                        break;
                                    case 4:
                                        // Bi-Monthly
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::MONTH);
                                        IFOKDO(err, recu.setPeriodIncrement(2))
                                        break;
                                    case 5:
                                        // Quarterly
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::MONTH);
                                        IFOKDO(err, recu.setPeriodIncrement(3))
                                        break;
                                    case 6:
                                        // Half yearly
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::MONTH);
                                        IFOKDO(err, recu.setPeriodIncrement(6))
                                        break;
                                    case 7:
                                        // Yearly
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::YEAR);
                                        IFOKDO(err, recu.setPeriodIncrement(1))
                                        break;
                                    case 8:
                                        // Four months
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::MONTH);
                                        IFOKDO(err, recu.setPeriodIncrement(4))
                                        break;
                                    case 9:
                                        // Four weeks
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::WEEK);
                                        IFOKDO(err, recu.setPeriodIncrement(4))
                                        break;
                                    case 10:
                                        // Daily
                                        err = recu.setPeriodUnit(SKGRecurrentOperationObject::DAY);
                                        IFOKDO(err, recu.setPeriodIncrement(1))
                                        break;

                                    default:
                                        break;
                                    }
                                }
                                IFOKDO(err, recu.save(true, false))
                            }
                            mapOperation[attributes.at(0)] = op;
                        }

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(6))

            // Step 7 - stock
            IFOK(err)
            {
                SKGStringListList listStock;
                err = SKGServices::executeSelectSqliteOrder(originalDocument,
                                                            QStringLiteral("SELECT STOCKID, HELDAT, PURCHASEDATE, STOCKNAME, SYMBOL, NUMSHARES, PURCHASEPRICE, "
                                                                           "NOTES, CURRENTPRICE, VALUE, COMMISSION FROM STOCK_V1"),
                                                            listStock);
                IFOK(err)
                {
                    int nb = listStock.count() - 1;
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import stocks"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listStock.at(i + 1);

                        // Create unit
                        SKGUnitObject unit(m_importer->getDocument());
                        err = unit.setName(attributes.at(3));
                        IFOKDO(err, unit.setSymbol(attributes.at(4)))
                        IFOKDO(err, unit.setType(SKGUnitObject::SHARE))
                        IFOKDO(err, unit.save())

                        SKGUnitValueObject unitValue;
                        IFOKDO(err, unit.addUnitValue(unitValue))
                        IFOKDO(err, unitValue.setDate(SKGServices::stringToDate(attributes.at(2))))
                        IFOKDO(err, unitValue.setQuantity(SKGServices::stringToDouble(attributes.at(6))))
                        IFOKDO(err, unitValue.save(true, false))

                        SKGUnitValueObject unitValue2;
                        IFOKDO(err, unit.addUnitValue(unitValue2))
                        IFOKDO(err, unitValue2.setDate(QDate::currentDate()))
                        IFOKDO(err, unitValue2.setQuantity(SKGServices::stringToDouble(attributes.at(8))))
                        IFOKDO(err, unitValue2.save(true, false))

                        // Create operation
                        SKGAccountObject acc = mapAccount[attributes.at(1)];
                        SKGUnitObject unitCurrency;
                        IFOKDO(err, acc.getUnit(unitCurrency))

                        SKGOperationObject op;
                        IFOKDO(err, acc.addOperation(op, true))
                        IFOKDO(err, op.setComment(attributes.at(7)))
                        IFOKDO(err, op.setDate(SKGServices::stringToDate(attributes.at(2))))
                        IFOKDO(err, op.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T")))
                        IFOKDO(err, op.setImportID("MMEX-STOCK-" % attributes.at(0)))
                        IFOKDO(err, op.setUnit(unit))
                        IFOKDO(err, op.save())

                        SKGSubOperationObject subop;
                        IFOKDO(err, op.addSubOperation(subop))
                        IFOKDO(err, subop.setQuantity(SKGServices::stringToDouble(attributes.at(5))))
                        IFOKDO(err, subop.save(true, false))

                        /*SKGOperationObject op2;
                        if(!err) err = acc.addOperation(op2, true);
                        if(!err) err = op2.setComment(attributes.at(7));
                        if(!err) err = op2.setDate(SKGServices::stringToDate(attributes.at(2)));
                        if(!err) err = op2.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T"));
                        if(!err) err = op2.setImportID("MMEX-STOCK-" % attributes.at(0)% "_TR");
                        if(!err) err = op2.setUnit(unitCurrency);
                        if(!err) err = op2.save();

                        SKGSubOperationObject subop2;
                        if(!err) err = op2.addSubOperation(subop2);
                        if(!err) err = subop2.setQuantity(-SKGServices::stringToDouble(attributes.at(5))*SKGServices::stringToDouble(attributes.at(6)));
                        if(!err) err = subop2.save();

                        if(!err) err = op.setGroupOperation(op2);
                        if(!err) err = op.save();*/

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(7))

            // Step 8 - asset
            IFOK(err)
            {
                SKGStringListList listAssets;
                err = SKGServices::executeSelectSqliteOrder(
                    originalDocument,
                    QStringLiteral("SELECT ASSETID, STARTDATE, ASSETNAME, VALUE, VALUECHANGE, NOTES, VALUECHANGERATE, ASSETTYPE FROM ASSETS_V1"),
                    listAssets);
                IFOK(err)
                {
                    int nb = listAssets.count() - 1;
                    if (nb != 0) {
                        // Create account
                        // Create bank
                        SKGBankObject bank(m_importer->getDocument());
                        IFOKDO(err, bank.setName(QStringLiteral("MMEX")))
                        IFOKDO(err, bank.save())

                        // Create account
                        SKGAccountObject account;
                        err = bank.addAccount(account);
                        IFOKDO(err, account.setName(i18nc("Noun, a type of account", "Assets")))
                        IFOKDO(err, account.setType(SKGAccountObject::ASSETS))
                        IFOKDO(err, account.save())

                        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import assets"), nb);
                        for (int i = 0; !err && i < nb; ++i) {
                            const QStringList &attributes = listAssets.at(i + 1);

                            // Create unit
                            SKGUnitObject unit(m_importer->getDocument());
                            err = unit.setName(attributes.at(2));
                            IFOKDO(err, unit.setSymbol(attributes.at(2)))
                            IFOKDO(err, unit.setType(SKGUnitObject::OBJECT))
                            IFOKDO(err,
                                   unit.setInternetCode(QStringLiteral("=")
                                                        % (attributes.at(4) == QStringLiteral("Depreciates") ? QStringLiteral("-") : QStringLiteral("+"))
                                                        % attributes.at(6)))
                            IFOKDO(err, unit.save())

                            SKGUnitValueObject unitValue;
                            IFOKDO(err, unit.addUnitValue(unitValue))
                            IFOKDO(err, unitValue.setDate(SKGServices::stringToDate(attributes.at(1))))
                            IFOKDO(err, unitValue.setQuantity(SKGServices::stringToDouble(attributes.at(3))))
                            IFOKDO(err, unitValue.save(true, false))

                            // Create operation
                            SKGOperationObject op;
                            IFOKDO(err, account.addOperation(op, true))
                            IFOKDO(err, op.setComment(attributes.at(5)))
                            IFOKDO(err, op.setDate(SKGServices::stringToDate(attributes.at(1))))
                            IFOKDO(err, op.setAttribute(QStringLiteral("t_imported"), QStringLiteral("T")))
                            IFOKDO(err, op.setImportID("MMEX-ASSET-" % attributes.at(0)))
                            IFOKDO(err, op.setUnit(unit))
                            IFOKDO(err, op.save())

                            SKGSubOperationObject subop;
                            IFOKDO(err, op.addSubOperation(subop))
                            IFOKDO(err, subop.setQuantity(1.0))
                            IFOKDO(err, subop.save(true, false))

                            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                        }

                        SKGENDTRANSACTION(m_importer->getDocument(), err)
                    }
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(8))

            // Step 9 - budgets
            IFOK(err)
            {
                SKGStringListList listBudgets;
                err = SKGServices::executeSelectSqliteOrder(
                    originalDocument,
                    QStringLiteral("SELECT BUDGETENTRYID, (SELECT Y.BUDGETYEARNAME FROM BUDGETYEAR_V1 Y WHERE Y.BUDGETYEARID=BUDGETTABLE_V1.BUDGETYEARID), "
                                   "CATEGID, SUBCATEGID, PERIOD, AMOUNT FROM BUDGETTABLE_V1"),
                    listBudgets);
                IFOK(err)
                {
                    int nb = listBudgets.count() - 1;
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import budgets"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        const QStringList &attributes = listBudgets.at(i + 1);

                        const QString &period = attributes.at(4);

                        int step = 0;
                        double coef = 0;
                        if (period == QStringLiteral("Weekly")) {
                            coef = 52.0 / 12.0;
                            step = 1;
                        } else if (period == QStringLiteral("Monthly")) {
                            coef = 1;
                            step = 1;
                        } else if (period == QStringLiteral("Bi-Weekly")) {
                            coef = 52.0 / 12.0 / 2.0;
                            step = 1;
                        } else if (period == QStringLiteral("Bi-Monthly")) {
                            coef = 1;
                            step = 2;
                        } else if (period == QStringLiteral("Quarterly")) {
                            coef = 1;
                            step = 4;
                        } else if (period == QStringLiteral("Half-Yearly")) {
                            coef = 1;
                            step = 6;
                        } else if (period == QStringLiteral("Yearly")) {
                            coef = 1;
                            step = 12;
                        }

                        if (step != 0) {
                            for (int j = 0; !err && j < 12; j = j + step) {
                                SKGBudgetObject bug(m_importer->getDocument());
                                err = bug.setYear(SKGServices::stringToInt(attributes.at(1)));
                                IFOKDO(err, bug.setMonth(step == 12 ? 0 : j + 1))
                                IFOK(err)
                                {
                                    QString id = attributes.at(3);
                                    if (id == QStringLiteral("-1")) {
                                        id = attributes.at(2);
                                        if (id != QStringLiteral("-1")) {
                                            err = bug.setCategory(mapCategory[id]);
                                        }
                                    } else {
                                        err = bug.setCategory(mapSubCategory[id]);
                                    }
                                }
                                IFOKDO(err, bug.setBudgetedAmount(coef * SKGServices::stringToDouble(attributes.at(5))))
                                IFOKDO(err, bug.save())
                            }
                        }

                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                }
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(9))

            SKGENDTRANSACTION(m_importer->getDocument(), err)

            IFOKDO(err, m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE")))
        }
        originalDocument.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("originalDocument"));
    return err;
}

QString SKGImportPluginMmb::getMimeTypeFilter() const
{
    return "*.mmb|" % i18nc("A file format", "Money Manager Ex document");
}

#include <skgimportpluginmmb.moc>
