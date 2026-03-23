/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for GNC import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugingnc.h"

#include <kcompressiondevice.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <qdom.h>

#include <cmath>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgpayeeobject.h"
#include "skgservices.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginGnc, "metadata.json")

SKGImportPluginGnc::SKGImportPluginGnc(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginGnc::~SKGImportPluginGnc() = default;

bool SKGImportPluginGnc::isImportPossible()
{
    SKGTRACEINFUNC(10)
    if (m_importer->getDocument() == nullptr) {
        return true;
    }
    QString extension = m_importer->getFileNameExtension();
    return (extension == QStringLiteral("UNCOMPRESSED") || extension == QStringLiteral("GNUCASH") || extension == QStringLiteral("GNC"));
}

SKGError SKGImportPluginGnc::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Open file
    KCompressionDevice file(m_importer->getLocalFileName(), KCompressionDevice::GZip);
    if (!file.open(QIODevice::ReadOnly)) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Open file '%1' failed", m_importer->getFileName().toDisplayString()));
    } else {
        QDomDocument doc;

        // Set the file without uncompression
        QString errorMsg;
        int errorLine = 0;
        int errorCol = 0;
        bool contentOK = false;
        const auto &content = QString::fromUtf8(file.readAll());
        const auto &result = doc.setContent(content);

        contentOK = !!result;
        errorLine = result.errorLine;
        errorCol = result.errorColumn;
        errorMsg = result.errorMessage;
        file.close();

        if (!contentOK) {
            err.setReturnCode(ERR_ABORT)
                .setMessage(i18nc("Error message", "%1-%2: '%3'", errorLine, errorCol, errorMsg))
                .addError(ERR_INVALIDARG, i18nc("Error message", "Invalid XML content in file '%1'", m_importer->getFileName().toDisplayString()));
        } else {
            // Get root
            QDomElement docElem = doc.documentElement();

            // // Get book
            QDomElement book = docElem.firstChildElement(QStringLiteral("gnc:book"));
            if (book.isNull()) {
                book = docElem;
            }
            if (book.isNull()) {
                err = SKGError(ERR_INVALIDARG, i18nc("Error message", "Invalid XML content in file '%1'", m_importer->getFileName().toDisplayString()));
            } else {
                QMap<QString, SKGUnitObject> mapUnitIdUnit;
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "GNC"), 6);
                IFOK(err)
                {
                    // Step 1-Get units
                    SKGUnitObject defaultUnit;
                    {
                        QDomNodeList unitList = book.elementsByTagName(QStringLiteral("gnc:commodity"));
                        int nbUnit = unitList.count();
                        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nbUnit);
                        for (int i = 0; !err && i < nbUnit; ++i) {
                            QDomElement unit = unitList.at(i).toElement();
                            QDomElement unitName = unit.firstChildElement(QStringLiteral("cmdty:id"));
                            QDomElement unitSpace = unit.firstChildElement(QStringLiteral("cmdty:space"));
                            if (!unitName.isNull() && !unitSpace.isNull()) {
                                SKGUnitObject unitObj(m_importer->getDocument());
                                if (unitSpace.text() == QStringLiteral("ISO4217")) {
                                    // We try a creation
                                    SKGUnitObject::createCurrencyUnit(m_importer->getDocument(), unitName.text(), unitObj);
                                }

                                if (!err && !unitObj.exist()) {
                                    // Creation of unit
                                    err = unitObj.setName(unitName.text());
                                    IFOKDO(err, unitObj.setSymbol(unitName.text()))
                                    IFOKDO(err, unitObj.setType(SKGUnitObject::SHARE))
                                    IFOKDO(err, unitObj.save())
                                }

                                if (!err && !defaultUnit.exist()) {
                                    defaultUnit = unitObj;
                                }

                                mapUnitIdUnit[unitName.text()] = unitObj;
                            }

                            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                        }

                        SKGENDTRANSACTION(m_importer->getDocument(), err)
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(1))

                    // Step 2-Get accounts and categories
                    SKGAccountObject gnucashTemporaryAccount(m_importer->getDocument());
                    QMap<QString, QString> mapIdName;
                    QMap<QString, QChar> mapIdType;
                    QMap<QString, SKGUnitObject> mapIdUnit;
                    QMap<QString, SKGAccountObject> mapIdAccount;
                    QMap<QString, SKGCategoryObject> mapIdCategory;

                    // Create bank and temporary account for gnucash import
                    SKGBankObject bank(m_importer->getDocument());
                    IFOKDO(err,
                           m_importer->getDocument()->addOrModifyAccount(QStringLiteral("GNUCASH-TEMPORARY-ACCOUNT"), QString(), QStringLiteral("GNUCASH")))
                    IFOKDO(err, bank.setName(QStringLiteral("GNUCASH")))
                    IFOKDO(err, bank.load())
                    IFOKDO(err, gnucashTemporaryAccount.setName(QStringLiteral("GNUCASH-TEMPORARY-ACCOUNT")))
                    IFOKDO(err, gnucashTemporaryAccount.load())

                    {
                        // Create accounts
                        QDomNodeList accountList = book.elementsByTagName(QStringLiteral("gnc:account"));
                        int nbAccount = accountList.count();
                        IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import banks and accounts"), nbAccount))
                        for (int i = 0; !err && i < nbAccount; ++i) {
                            QDomElement account = accountList.at(i).toElement();
                            QDomElement parentNode = account.parentNode().toElement();
                            if (parentNode.tagName() != QStringLiteral("gnc:template-transactions")) {
                                QDomElement accountId = account.firstChildElement(QStringLiteral("act:id"));
                                QDomElement accountName = account.firstChildElement(QStringLiteral("act:name"));
                                QDomElement accountType = account.firstChildElement(QStringLiteral("act:type"));
                                QDomElement accountCode = account.firstChildElement(QStringLiteral("act:code"));
                                QDomElement accountDescription = account.firstChildElement(QStringLiteral("act:description"));

                                QString realAccountName = accountName.text();

                                if (!accountId.isNull() && !accountType.isNull() && !realAccountName.isEmpty()) {
                                    QString type = accountType.text();
                                    if (type == QStringLiteral("INCOME") || type == QStringLiteral("EXPENSE")) {
                                        // It is a category
                                        QString fullName = realAccountName;

                                        QDomElement accountParent = account.firstChildElement(QStringLiteral("act:parent"));
                                        if (!err && !accountParent.isNull() && mapIdType[accountParent.text()] == 'C') {
                                            fullName = mapIdName[accountParent.text()] % OBJECTSEPARATOR % fullName;
                                        }

                                        SKGCategoryObject catObj;
                                        err = SKGCategoryObject::createPathCategory(m_importer->getDocument(), fullName, catObj);

                                        mapIdCategory[accountId.text()] = catObj;
                                        mapIdType[accountId.text()] = 'C'; // Memorize an account
                                    } else if (type == QStringLiteral("BANK") || type == QStringLiteral("CREDIT") || type == QStringLiteral("STOCK")
                                               || type == QStringLiteral("ASSET") || type == QStringLiteral("LIABILITY") || type == QStringLiteral("RECEIVABLE")
                                               || type == QStringLiteral("PAYABLE") || type == QStringLiteral("TRADING") || type == QStringLiteral("CREDITCARD")
                                               || type == QStringLiteral("CASH") || type == QStringLiteral("MUTUAL")) {
                                        // It is a real account
                                        SKGAccountObject act(m_importer->getDocument());
                                        err = bank.addAccount(act);
                                        IFOKDO(err, act.setName(realAccountName))
                                        int index = 1;
                                        while (!err && act.exist()) {
                                            index++;
                                            realAccountName = accountName.text() % " (" % SKGServices::intToString(index) % u')';
                                            err = act.setName(realAccountName);
                                        }
                                        if (!err && !accountCode.isNull()) {
                                            err = act.setNumber(accountCode.text());
                                        }
                                        if (!err && !accountDescription.isNull()) {
                                            err = act.setComment(accountDescription.text());
                                        }
                                        IFOKDO(err,
                                               act.setType(type == QStringLiteral("ASSET") ? SKGAccountObject::ASSETS
                                                               : type == QStringLiteral("STOCK") || type == QStringLiteral("MUTUAL")
                                                                   || type == QStringLiteral("RECEIVABLE") || type == QStringLiteral("PAYABLE")
                                                                   || type == QStringLiteral("TRADING")
                                                               ? SKGAccountObject::INVESTMENT
                                                               : type == QStringLiteral("CASH")      ? SKGAccountObject::WALLET
                                                               : type == QStringLiteral("LIABILITY") ? SKGAccountObject::LOAN
                                                               : type == QStringLiteral("CREDIT")    ? SKGAccountObject::CREDITCARD
                                                                                                     : SKGAccountObject::CURRENT))
                                        IFOKDO(err, act.save())

                                        // Change parent bank in case of ASSETS
                                        if (act.getType() == SKGAccountObject::WALLET) {
                                            // Get blank bank
                                            SKGBankObject blankBank(m_importer->getDocument());
                                            IFOKDO(err, blankBank.setName(QString()))
                                            if (blankBank.exist()) {
                                                err = blankBank.load();
                                            } else {
                                                err = blankBank.save();
                                            }
                                            IFOKDO(err, act.setBank(blankBank))
                                            IFOKDO(err, act.save())
                                        }

                                        mapIdAccount[accountId.text()] = act;
                                        mapIdType[accountId.text()] = 'A'; // Memorize an account

                                        QDomNodeList accountUnits = account.elementsByTagName(QStringLiteral("cmdty:id"));
                                        if (!err && (accountUnits.count() != 0)) {
                                            mapIdUnit[accountId.text()] = mapUnitIdUnit[accountUnits.at(0).toElement().text()];
                                        }
                                    } else if (type == QStringLiteral("EQUITY")) {
                                        mapIdType[accountId.text()] = 'E'; // Memorize an account
                                    } else {
                                        mapIdType[accountId.text()] = ' '; // Memorize an account
                                    }

                                    mapIdName[accountId.text()] = std::move(realAccountName);
                                }
                            }

                            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                        }

                        SKGENDTRANSACTION(m_importer->getDocument(), err)
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(2))

                    // Step 3- Get transactions
                    QMap<QString, SKGOperationObject> mapIdScheduledOperation;
                    {
                        QDomNodeList operationsList = book.elementsByTagName(QStringLiteral("gnc:transaction"));
                        int nbOperations = operationsList.count();
                        IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nbOperations))
                        QVector<SubOpInfo> suboperationsList;
                        for (int i = 0; !err && i < nbOperations; ++i) {
                            QDomElement operation = operationsList.at(i).toElement();

                            QDomElement operationId = operation.firstChildElement(QStringLiteral("trn:id"));
                            QDomElement operationDate = operation.firstChildElement(QStringLiteral("trn:date-posted"));
                            QDomElement operationNumber = operation.firstChildElement(QStringLiteral("trn:num"));
                            QDomElement operationPayee = operation.firstChildElement(QStringLiteral("trn:description"));
                            QDomElement operationSlots = operation.firstChildElement(QStringLiteral("trn:slots"));

                            SKGOperationObject operationObj;
                            err = gnucashTemporaryAccount.addOperation(operationObj, true);
                            if (!err && !operationDate.isNull()) {
                                // Format 2007-01-13 00:00:00 +0100
                                err = operationObj.setDate(QDate::fromString(operationDate.text().left(10), QStringLiteral("yyyy-MM-dd")));
                            }
                            if (!err && !operationNumber.isNull()) {
                                err = operationObj.setNumber(operationNumber.text());
                            }
                            if (!err && !operationPayee.isNull()) {
                                SKGPayeeObject payeeObject;
                                err = SKGPayeeObject::createPayee(m_importer->getDocument(), operationPayee.text(), payeeObject);
                                IFOKDO(err, operationObj.setPayee(payeeObject))
                            }
                            IFOKDO(err, operationObj.setImported(true))
                            IFOKDO(err, operationObj.setImportID("GNC-" % operationId.text()))
                            IFOKDO(err, operationObj.setUnit(defaultUnit))

                            IFOK(err)
                            {
                                QDomElement parentNode = operation.parentNode().toElement();
                                err = operationObj.setTemplate(parentNode.tagName() == QStringLiteral("gnc:template-transactions"));
                            }
                            if (!operationSlots.isNull()) {
                                QDomNodeList slotList = operationSlots.elementsByTagName(QStringLiteral("slot"));
                                int nbSlots = slotList.count();
                                for (int k = 0; !err && k < nbSlots; ++k) {
                                    QDomElement slot = slotList.at(k).toElement();

                                    QDomElement key = slot.firstChildElement(QStringLiteral("slot:key"));
                                    QDomElement value = slot.firstChildElement(QStringLiteral("slot:value"));
                                    if (!key.isNull() && !value.isNull()) {
                                        if (key.text() == QStringLiteral("notes")) {
                                            err = operationObj.setComment(value.text());
                                        }
                                    }
                                }
                            }
                            IFOKDO(err, operationObj.save())

                            // Get splits
                            bool parentSet = false;
                            QDomElement splits = operation.firstChildElement(QStringLiteral("trn:splits"));

                            int nbSuboperations = 0;
                            suboperationsList.resize(0);
                            {
                                QDomNodeList suboperationsListTmp = splits.elementsByTagName(QStringLiteral("trn:split"));
                                nbSuboperations = suboperationsListTmp.count();

                                int nbRealSuboperations = 0;
                                for (int j = 0; !err && j < nbSuboperations; ++j) {
                                    SubOpInfo info;
                                    info.subOp = suboperationsListTmp.at(j).toElement();
                                    info.account = info.subOp.firstChildElement(QStringLiteral("split:account"));
                                    info.value = 0;
                                    QStringList vals = SKGServices::splitCSVLine(info.subOp.firstChildElement(QStringLiteral("split:quantity")).text(), '/');
                                    if (vals.count() == 1) {
                                        info.value = SKGServices::stringToDouble(vals.at(0));
                                    } else if (vals.count() == 2) {
                                        info.value = SKGServices::stringToDouble(vals.at(0)) / SKGServices::stringToDouble(vals.at(1));
                                    }

                                    QDomElement suboperationSlots = info.subOp.firstChildElement(QStringLiteral("split:slots"));
                                    if (!suboperationSlots.isNull()) {
                                        QDomNodeList slotList = suboperationSlots.elementsByTagName(QStringLiteral("slot"));
                                        int nbSlots = slotList.count();
                                        for (int k = 0; !err && k < nbSlots; ++k) {
                                            QDomElement slot = slotList.at(k).toElement();

                                            QDomElement key = slot.firstChildElement(QStringLiteral("slot:key"));
                                            QDomElement value = slot.firstChildElement(QStringLiteral("slot:value"));
                                            if (!key.isNull() && !value.isNull()) {
                                                if (key.text() == QStringLiteral("sched-xaction")) {
                                                    // This is a scheduled transaction
                                                    QDomNodeList scheduledSlotList = value.elementsByTagName(QStringLiteral("slot"));
                                                    int nbscheduledSlotList = scheduledSlotList.count();
                                                    for (int k2 = 0; !err && k2 < nbscheduledSlotList; ++k2) {
                                                        QDomElement slot2 = scheduledSlotList.at(k2).toElement();

                                                        QDomElement key2 = slot2.firstChildElement(QStringLiteral("slot:key"));
                                                        QDomElement value2 = slot2.firstChildElement(QStringLiteral("slot:value"));
                                                        if (!key2.isNull() && !value2.isNull()) {
                                                            if (key2.text() == QStringLiteral("account")) {
                                                                mapIdScheduledOperation[info.account.text()] = operationObj;
                                                                info.account = value2;
                                                            } else if (key2.text() == QStringLiteral("debit-formula") && !value2.text().isEmpty()) {
                                                                QStringList vals2 = SKGServices::splitCSVLine(value2.text(), '/');
                                                                if (vals2.count() == 1) {
                                                                    info.value = SKGServices::stringToDouble(vals2.at(0));
                                                                } else if (vals2.count() == 2) {
                                                                    info.value =
                                                                        SKGServices::stringToDouble(vals2.at(0)) / SKGServices::stringToDouble(vals2.at(1));
                                                                }
                                                            } else if (key2.text() == QStringLiteral("credit-formula") && !value2.text().isEmpty()) {
                                                                QStringList vals2 = SKGServices::splitCSVLine(value2.text(), '/');
                                                                if (vals2.count() == 1) {
                                                                    info.value = -SKGServices::stringToDouble(vals2.at(0));
                                                                } else if (vals2.count() == 2) {
                                                                    info.value =
                                                                        -SKGServices::stringToDouble(vals2.at(0)) / SKGServices::stringToDouble(vals2.at(1));
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    if (!std::isnan(info.value)) {
                                        QChar accountType = mapIdType[info.account.text()];
                                        if (accountType == 'C') {
                                            suboperationsList.push_front(info);
                                        } else {
                                            suboperationsList.push_back(info);
                                        }

                                        ++nbRealSuboperations;
                                    }
                                }
                                nbSuboperations = nbRealSuboperations;
                            }

                            SKGSubOperationObject suboperationObj;
                            for (int j = 0; !err && j < nbSuboperations; ++j) {
                                SubOpInfo suboperationInfo = suboperationsList.at(j);
                                QDomElement suboperation = suboperationInfo.subOp.toElement();
                                QDomElement suboperationReconciled = suboperation.firstChildElement(QStringLiteral("split:reconciled-state"));
                                QDomElement suboperationMemo = suboperation.firstChildElement(QStringLiteral("split:memo"));
                                QDomElement suboperationAction = suboperation.firstChildElement(QStringLiteral("split:action"));

                                // Set transaction attribute
                                if (!suboperationReconciled.isNull() && operationObj.getStatus() == SKGOperationObject::NONE) {
                                    QString val = suboperationReconciled.text();
                                    IFOKDO(err,
                                           operationObj.setStatus(val == QStringLiteral("c")       ? SKGOperationObject::MARKED
                                                                      : val == QStringLiteral("y") ? SKGOperationObject::CHECKED
                                                                                                   : SKGOperationObject::NONE))
                                }
                                if (!err && !suboperationMemo.isNull() && operationObj.getComment().isEmpty()) {
                                    err = operationObj.setComment(suboperationMemo.text());
                                }
                                if (!err && !suboperationAction.isNull() && operationObj.getMode().isEmpty()) {
                                    err = operationObj.setMode(suboperationAction.text());
                                }
                                IFOKDO(err, operationObj.save())

                                QChar accountType = mapIdType[suboperationInfo.account.text()];
                                if (accountType == 'C') {
                                    // It is a split on category
                                    SKGCategoryObject cat = mapIdCategory[suboperationInfo.account.text()];

                                    double q = -suboperationInfo.value;
                                    if (!err && (!suboperationObj.exist() || suboperationObj.getQuantity() != q)) {
                                        err = operationObj.addSubOperation(suboperationObj);
                                    }
                                    IFOKDO(err, suboperationObj.setQuantity(q))
                                    IFOKDO(err, suboperationObj.setCategory(cat))
                                    if (!err && !suboperationMemo.isNull()) {
                                        err = suboperationObj.setComment(suboperationMemo.text());
                                    }
                                    IFOKDO(err, suboperationObj.save())
                                } else if (accountType == 'E') {
                                    // Set as initial balance
                                    IFOKDO(err, operationObj.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                                    IFOKDO(err, operationObj.setStatus(SKGOperationObject::CHECKED))
                                    IFOKDO(err, operationObj.save())

                                    IFOKDO(err, suboperationObj.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                                    if (suboperationObj.getDocument() != nullptr) {
                                        IFOKDO(err, suboperationObj.save())
                                    }
                                } else if (accountType == 'A') {
                                    // It is a transfer of account
                                    SKGAccountObject act(m_importer->getDocument());
                                    IFOKDO(err, act.setName(mapIdName[suboperationInfo.account.text()]))
                                    IFOKDO(err, act.load())

                                    // Set Unit
                                    SKGUnitObject unitName = mapIdUnit[suboperationInfo.account.text()];
                                    double quantity = 0;
                                    if (parentSet) {
                                        // If the parent is already set, it means that is a transfer
                                        SKGOperationObject operationObj2;
                                        IFOKDO(err, act.addOperation(operationObj2, true))
                                        IFOKDO(err, operationObj2.setDate(operationObj.getDate()))
                                        IFOKDO(err, operationObj2.setNumber(operationObj.getNumber()))
                                        IFOKDO(err, operationObj2.setMode(operationObj.getMode()))
                                        if (!err && !suboperationMemo.isNull()) {
                                            err = operationObj2.setComment(suboperationMemo.text());
                                        }
                                        SKGPayeeObject payeeObject;
                                        operationObj.getPayee(payeeObject);
                                        IFOKDO(err, operationObj2.setPayee(payeeObject))
                                        IFOK(err)
                                        {
                                            QString val = suboperationReconciled.text();
                                            err = operationObj2.setStatus(val == QStringLiteral("c")       ? SKGOperationObject::MARKED
                                                                              : val == QStringLiteral("y") ? SKGOperationObject::CHECKED
                                                                                                           : SKGOperationObject::NONE);
                                        }
                                        IFOKDO(err, operationObj2.setImported(true))
                                        IFOKDO(err, operationObj2.setImportID(operationObj.getImportID()))
                                        IFOKDO(err, operationObj2.setTemplate(operationObj.isTemplate()))
                                        if (!err && unitName.exist()) {
                                            err = operationObj2.setUnit(unitName);
                                        }
                                        IFOKDO(err, operationObj2.save())
                                        IFOKDO(err, operationObj2.setGroupOperation(operationObj))
                                        IFOKDO(err, operationObj2.save())

                                        // Create sub transaction on operationObj2
                                        SKGSubOperationObject suboperationObj2;
                                        IFOKDO(err, operationObj2.addSubOperation(suboperationObj2))
                                        IFOK(err)
                                        {
                                            // We must take the quality of the split having an action
                                            quantity = suboperationInfo.value;
                                            err = suboperationObj2.setQuantity(suboperationInfo.value);
                                        }
                                        if (!err && !suboperationMemo.isNull()) {
                                            err = suboperationObj2.setComment(suboperationMemo.text());
                                        }
                                        IFOKDO(err, suboperationObj2.save())
                                    } else {
                                        // We set the parent
                                        IFOKDO(err, operationObj.setParentAccount(act))
                                        if (!err && unitName.exist()) {
                                            err = operationObj.setUnit(unitName);
                                        }
                                        IFOKDO(err, operationObj.save())

                                        // Compute quantity
                                        quantity = suboperationInfo.value;

                                        // Create sub transaction on operationObj
                                        quantity -= SKGServices::stringToDouble(operationObj.getAttribute(QStringLiteral("f_QUANTITY")));
                                        if (fabs(quantity) > 10e-9) {
                                            IFOKDO(err, operationObj.addSubOperation(suboperationObj))
                                            IFOKDO(err, suboperationObj.setQuantity(quantity))
                                            if (!err && !suboperationMemo.isNull()) {
                                                err = suboperationObj.setComment(suboperationMemo.text());
                                            }
                                            IFOKDO(err, suboperationObj.save())
                                        }

                                        parentSet = true;
                                    }
                                }
                            }

                            if (!err && i % 500 == 0) {
                                err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                            }

                            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                        }

                        SKGENDTRANSACTION(m_importer->getDocument(), err)
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(3))

                    // Step 4-Get scheduled
                    {
                        // Create scheduled
                        QDomNodeList scheduleList = book.elementsByTagName(QStringLiteral("gnc:schedxaction"));
                        int nbSchedule = scheduleList.count();
                        IFOKDO(err,
                               m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import scheduled transactions"), nbSchedule))
                        for (int i = 0; !err && i < nbSchedule; ++i) {
                            QDomElement schedule = scheduleList.at(i).toElement();
                            QDomElement scheduleAutoCreate = schedule.firstChildElement(QStringLiteral("sx:autoCreate"));
                            QDomElement scheduleAutoCreateNotify = schedule.firstChildElement(QStringLiteral("sx:autoCreateNotify"));
                            QDomElement scheduleAdvanceCreateDays = schedule.firstChildElement(QStringLiteral("sx:advanceCreateDays"));
                            QDomElement scheduleAdvanceRemindDays = schedule.firstChildElement(QStringLiteral("sx:advanceRemindDays"));
                            QDomElement scheduleOccur = schedule.firstChildElement(QStringLiteral("sx:num-occur"));
                            QDomElement scheduleEnable = schedule.firstChildElement(QStringLiteral("sx:enabled"));
                            QDomElement scheduleLast = schedule.firstChildElement(QStringLiteral("sx:last"));
                            if (scheduleLast.isNull()) {
                                scheduleLast = schedule.firstChildElement(QStringLiteral("sx:start"));
                            }

                            QDomElement scheduleMult;
                            QDomElement schedulePeriod;
                            QDomElement scheduleDate;
                            QDomNodeList scheduleScheduleList = schedule.elementsByTagName(QStringLiteral("gnc:recurrence"));
                            if (scheduleScheduleList.count() != 0) {
                                QDomElement scheduleSchedule = scheduleScheduleList.at(0).toElement();
                                scheduleMult = scheduleSchedule.firstChildElement(QStringLiteral("recurrence:mult"));
                                schedulePeriod = scheduleSchedule.firstChildElement(QStringLiteral("recurrence:period_type"));

                                scheduleDate = scheduleLast.firstChildElement(QStringLiteral("gdate"));
                            }

                            QDomElement scheduleTempl = schedule.firstChildElement(QStringLiteral("sx:templ-acct"));
                            if (!scheduleTempl.isNull()) {
                                SKGOperationObject operation = mapIdScheduledOperation[scheduleTempl.text()];
                                SKGRecurrentOperationObject recuOperation;
                                err = operation.addRecurrentOperation(recuOperation);
                                if (!err && !scheduleOccur.isNull()) {
                                    err = recuOperation.timeLimit(true);
                                    IFOKDO(err, recuOperation.setTimeLimit(SKGServices::stringToInt(scheduleOccur.text())))
                                }
                                if (!err && !scheduleAutoCreate.isNull()) {
                                    err = recuOperation.autoWriteEnabled(scheduleAutoCreate.text() == QStringLiteral("y"));
                                }
                                if (!err && !scheduleAdvanceCreateDays.isNull()) {
                                    err = recuOperation.setAutoWriteDays(SKGServices::stringToInt(scheduleAdvanceCreateDays.text()));
                                }
                                if (!err && !scheduleAutoCreateNotify.isNull()) {
                                    err = recuOperation.warnEnabled(scheduleAutoCreateNotify.text() == QStringLiteral("y"));
                                }
                                if (!err && !scheduleAdvanceRemindDays.isNull()) {
                                    err = recuOperation.setWarnDays(SKGServices::stringToInt(scheduleAdvanceRemindDays.text()));
                                }

                                if (!err && !scheduleMult.isNull() && !schedulePeriod.isNull()) {
                                    int p = SKGServices::stringToInt(scheduleMult.text());
                                    SKGRecurrentOperationObject::PeriodUnit punit = SKGRecurrentOperationObject::DAY;
                                    if (schedulePeriod.text() == QStringLiteral("month")) {
                                        punit = SKGRecurrentOperationObject::MONTH;
                                    } else if (schedulePeriod.text() == QStringLiteral("week")) {
                                        punit = SKGRecurrentOperationObject::WEEK;
                                    }

                                    err = recuOperation.setPeriodIncrement(p);
                                    IFOKDO(err, recuOperation.setPeriodUnit(punit))
                                }

                                if (!err && !scheduleDate.isNull()) {
                                    err = recuOperation.setDate(QDate::fromString(scheduleDate.text().left(10), QStringLiteral("yyyy-MM-dd")));
                                }
                                IFOKDO(err, recuOperation.setDate(recuOperation.getNextDate()))
                                if (!err && !scheduleEnable.isNull() && scheduleEnable.text() == QStringLiteral("n")) {
                                    err = recuOperation.autoWriteEnabled(false);
                                    IFOKDO(err, recuOperation.setWarnDays(false))
                                }

                                IFOKDO(err, recuOperation.save())
                            }

                            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                        }

                        SKGENDTRANSACTION(m_importer->getDocument(), err)
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(4))

                    // Step 5-Get unit values
                    {
                        // Create unit values
                        QDomElement pricedb = book.firstChildElement(QStringLiteral("gnc:pricedb"));
                        if (!pricedb.isNull()) {
                            QDomNodeList priceList = pricedb.elementsByTagName(QStringLiteral("price"));
                            int nbPrice = priceList.count();
                            IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nbPrice))
                            for (int i = 0; !err && i < nbPrice; ++i) {
                                QDomElement price = priceList.at(i).toElement();

                                QDomNodeList priceIdList = price.elementsByTagName(QStringLiteral("cmdty:id"));
                                QDomNodeList priceDateList = price.elementsByTagName(QStringLiteral("ts:date"));
                                QDomElement priceValue = price.firstChildElement(QStringLiteral("price:value"));

                                if (!priceIdList.isEmpty() && !priceDateList.isEmpty() && !priceValue.isNull()) {
                                    QString unitName = priceIdList.at(0).toElement().text();
                                    QDate unitDate = QDate::fromString(priceDateList.at(0).toElement().text().left(10), QStringLiteral("yyyy-MM-dd"));

                                    double val = 0;
                                    QStringList vals = SKGServices::splitCSVLine(priceValue.text(), '/');
                                    if (vals.count() == 1) {
                                        val = SKGServices::stringToDouble(vals.at(0));
                                    } else if (vals.count() == 2) {
                                        val = SKGServices::stringToDouble(vals.at(0)) / SKGServices::stringToDouble(vals.at(1));
                                    }

                                    err = m_importer->getDocument()->addOrModifyUnitValue(unitName, unitDate, val);
                                }

                                IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                            }
                            SKGENDTRANSACTION(m_importer->getDocument(), err)
                        }
                    }
                    IFOKDO(err, m_importer->getDocument()->stepForward(5))

                    // Step 6-Remove temporary account
                    {
                        IFOKDO(err, gnucashTemporaryAccount.remove(false, true))

                        IFOKDO(err,
                               m_importer->getDocument()->executeSqliteOrder(
                                   "DELETE FROM account WHERE rd_bank_id=" % SKGServices::intToString(bank.getID())
                                   % " AND t_name='GNUCASH-TEMPORARY-ACCOUNT' AND (SELECT COUNT(1) FROM operation WHERE operation.rd_account_id=account.id)=0"))
                        IFOKDO(err, m_importer->getDocument()->stepForward(6))
                    }
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE")))
        }
    }

    return err;
}

QString SKGImportPluginGnc::getMimeTypeFilter() const
{
    return "*.uncompressed *.gnucash *.gnc|" % i18nc("A file format", "GnuCash document");
}

#include <skgimportplugingnc.moc>
