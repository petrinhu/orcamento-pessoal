/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for XHB import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginxhb.h"

#include <kcompressiondevice.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtrackerobject.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginXhb, "metadata.json")

SKGImportPluginXhb::SKGImportPluginXhb(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginXhb::~SKGImportPluginXhb() = default;

bool SKGImportPluginXhb::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("XHB"));
}

SKGError SKGImportPluginXhb::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Initialisation
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

        // Get root
        QDomElement docElem = doc.documentElement();

        if (!contentOK) {
            err.setReturnCode(ERR_ABORT).setMessage(i18nc("Error message", "%1-%2: '%3'", errorLine, errorCol, errorMsg));
        }
        if (!contentOK) {
            err.addError(ERR_INVALIDARG, i18nc("Error message", "Invalid XML content in file '%1'", m_importer->getFileName().toDisplayString()));
        } else {
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "XHB"), 4);

            QMap<QString, SKGAccountObject> mapIdAccount;
            QMap<QString, SKGCategoryObject> mapIdCategory;
            QMap<QString, SKGPayeeObject> mapIdPayee;

            SKGUnitObject unit;
            IFOKDO(err, m_importer->getDefaultUnit(unit))

            // Step 1-Create accounts
            IFOK(err)
            {
                QDomNodeList accountList = docElem.elementsByTagName(QStringLiteral("account"));
                int nb = accountList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import accounts"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get account object
                    QDomElement account = accountList.at(i).toElement();

                    // Create the bank
                    SKGBankObject bank(m_importer->getDocument());
                    QString bname = getAttribute(account, QStringLiteral("bankname"));
                    if (bname.isEmpty()) {
                        bname = QStringLiteral("HOMEBANK");
                    }
                    IFOKDO(err, bank.setName(bname))
                    IFOKDO(err, bank.save())

                    // Creation of the account
                    SKGAccountObject accountObj;
                    IFOKDO(err, bank.addAccount(accountObj))
                    IFOKDO(err, accountObj.setName(getAttribute(account, QStringLiteral("name"))))
                    IFOKDO(err, accountObj.setNumber(getAttribute(account, QStringLiteral("number"))))
                    IFOKDO(err, accountObj.setMinLimitAmount(SKGServices::stringToDouble(getAttribute(account, QStringLiteral("minimum")))))
                    IFOKDO(err, accountObj.minLimitAmountEnabled(accountObj.getMinLimitAmount() != 0.0))
                    QString type = getAttribute(account, QStringLiteral("type"));
                    IFOKDO(err,
                           accountObj.setType(type == QStringLiteral("2")
                                                  ? SKGAccountObject::WALLET
                                                  : (type == QStringLiteral("3")
                                                         ? SKGAccountObject::ASSETS
                                                         : (type == QStringLiteral("4")
                                                                ? SKGAccountObject::CREDITCARD
                                                                : (type == QStringLiteral("5") ? SKGAccountObject::LOAN : SKGAccountObject::CURRENT)))))
                    IFOK(err)
                    {
                        QString flags = getAttribute(account, QStringLiteral("flags"));
                        err = accountObj.setClosed(flags == QStringLiteral("2") || flags == QStringLiteral("3"));
                    }
                    IFOKDO(err, accountObj.save())

                    // Change parent bank in case of ASSETS
                    if (accountObj.getType() == SKGAccountObject::WALLET) {
                        // Get blank bank
                        SKGBankObject blankBank(m_importer->getDocument());
                        IFOKDO(err, blankBank.setName(QString()))
                        if (blankBank.exist()) {
                            err = blankBank.load();
                        } else {
                            err = blankBank.save();
                        }
                        IFOKDO(err, accountObj.setBank(blankBank))
                        IFOKDO(err, accountObj.save())
                    }

                    // Set initial balance
                    IFOKDO(err, accountObj.setInitialBalance(SKGServices::stringToDouble(getAttribute(account, QStringLiteral("initial"))), unit))

                    mapIdAccount[getAttribute(account, QStringLiteral("key"))] = accountObj;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Step 2-Get payees
            IFOK(err)
            {
                QDomNodeList partyList = docElem.elementsByTagName(QStringLiteral("pay"));
                int nb = partyList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import payees"), nb);
                for (int i = 0; !err && i < nb; ++i) {
                    // Get payee object
                    QDomElement party = partyList.at(i).toElement();
                    SKGPayeeObject payeeObject;
                    err = SKGPayeeObject::createPayee(m_importer->getDocument(), getAttribute(party, QStringLiteral("name")), payeeObject);
                    mapIdPayee[getAttribute(party, QStringLiteral("key"))] = payeeObject;

                    IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Step 3-Create categories
            IFOK(err)
            {
                QDomNodeList categoryList = docElem.elementsByTagName(QStringLiteral("cat"));
                int nb = categoryList.count();
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import categories"), 2 * nb);
                for (int j = 0; !err && j < 2; ++j) {
                    for (int i = 0; !err && i < nb; ++i) {
                        // Get account object
                        QDomElement category = categoryList.at(i).toElement();

                        // Creation of the category
                        bool catCreated = false;
                        SKGCategoryObject catObj(m_importer->getDocument());
                        QString parent2 = getAttribute(category, QStringLiteral("parent"));
                        if (parent2.isEmpty()) {
                            parent2 = '0';
                        }
                        if (parent2 == QStringLiteral("0") && j == 0) {
                            IFOKDO(err, catObj.setName(getAttribute(category, QStringLiteral("name"))))
                            IFOKDO(err, catObj.save())

                            mapIdCategory[getAttribute(category, QStringLiteral("key"))] = catObj;
                            catCreated = true;

                        } else if (parent2 != QStringLiteral("0") && j == 1) {
                            SKGCategoryObject catParentObj = mapIdCategory[parent2];
                            IFOKDO(err, catObj.setName(getAttribute(category, QStringLiteral("name"))))
                            IFOKDO(err, catObj.setParentCategory(catParentObj))
                            IFOKDO(err, catObj.save())

                            mapIdCategory[getAttribute(category, QStringLiteral("key"))] = catObj;
                            catCreated = true;
                        }

                        // Creation of the attached budget
                        if (!err && catCreated) {
                            QString b0 = getAttribute(category, QStringLiteral("b0"));
                            QString b1 = getAttribute(category, QStringLiteral("b1"));
                            if (!b0.isEmpty() || !b1.isEmpty()) {
                                for (int m = 1; !err && m <= 12; ++m) {
                                    SKGBudgetObject budget(m_importer->getDocument());

                                    int year = QDate::currentDate().year();
                                    IFOKDO(err, budget.setCategory(catObj))
                                    IFOKDO(err, budget.setYear(year))
                                    IFOKDO(err, budget.setMonth(m))
                                    IFOKDO(err,
                                           budget.setBudgetedAmount(SKGServices::stringToDouble(
                                               getAttribute(category, QLatin1Char('b') % SKGServices::intToString(b0.isEmpty() ? m : 0)))))
                                    IFOKDO(err, budget.save())
                                }
                            }
                        }

                        IFOKDO(err, m_importer->getDocument()->stepForward(nb * j + i + 1))
                    }
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(3))

            // Step 4-Create transaction
            IFOK(err)
            {
                err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), 2);
                for (int j = 1; !err && j <= 2; ++j) {
                    QDomNodeList transactionList = docElem.elementsByTagName(j == 1 ? QStringLiteral("ope") : QStringLiteral("fav"));
                    int nb = transactionList.count();
                    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb);
                    for (int i = 0; !err && i < nb; ++i) {
                        // Get account object
                        QDomElement transaction = transactionList.at(i).toElement();

                        // Get attributes
                        QString idAccount = getAttribute(transaction, QStringLiteral("account"));
                        // QString dst_account = getAttribute(transaction,  QStringLiteral("dst_account"));
                        if (idAccount != QStringLiteral("0")) {
                            SKGAccountObject account = mapIdAccount[idAccount];
                            QDate date = QDate(1, 1, 1).addDays(
                                1 + SKGServices::stringToInt(getAttribute(transaction, j == 1 ? QStringLiteral("date") : QStringLiteral("nextdate"))));
                            QString mode = getAttribute(transaction, QStringLiteral("paymode"));
                            if (mode == QStringLiteral("1")) {
                                mode = i18nc("Noun: type of payment", "Credit card");
                            } else if (mode == QStringLiteral("2")) {
                                mode = i18nc("Noun: type of payment", "Check");
                            } else if (mode == QStringLiteral("3")) {
                                mode = i18nc("Noun: type of payment", "Cash");
                            } else if (mode == QStringLiteral("4") || mode == QStringLiteral("5")) {
                                mode = i18nc("Noun: type of payment", "Transfer");
                            } else {
                                mode = i18nc("Noun: type of payment", "Other");
                            }
                            QString comment =
                                QString(getAttribute(transaction, QStringLiteral("wording")) % u' ' % getAttribute(transaction, QStringLiteral("info")))
                                    .trimmed();
                            int flags = SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("flags")));
                            double amount = SKGServices::stringToDouble(getAttribute(transaction, QStringLiteral("amount")));
                            SKGOperationObject::OperationStatus status = (flags == 1 || flags == 3 ? SKGOperationObject::CHECKED : SKGOperationObject::NONE);
                            SKGPayeeObject payee = mapIdPayee[getAttribute(transaction, QStringLiteral("payee"))];
                            SKGCategoryObject category = mapIdCategory[getAttribute(transaction, QStringLiteral("category"))];

                            // Creation of the transaction
                            SKGOperationObject opObj;
                            IFOKDO(err, account.addOperation(opObj, true))
                            IFOKDO(err, opObj.setDate(date))
                            IFOKDO(err, opObj.setUnit(unit))
                            IFOKDO(err, opObj.setPayee(payee))
                            IFOKDO(err, opObj.setMode(mode))

                            IFOKDO(err, opObj.setComment(comment))
                            IFOKDO(err, opObj.setTemplate(j == 2))
                            IFOKDO(err, opObj.setImported(true))
                            IFOKDO(err, opObj.setImportID("HXB-" % SKGServices::intToString(i)))
                            IFOKDO(err, opObj.setStatus(status))
                            IFOKDO(err, opObj.save())

                            SKGSubOperationObject subObj;
                            IFOKDO(err, opObj.addSubOperation(subObj))
                            IFOKDO(err, subObj.setCategory(category))
                            IFOKDO(err, subObj.setComment(comment))
                            IFOKDO(err, subObj.setQuantity(amount))
                            IFOKDO(err, subObj.save())

                            QString tags = getAttribute(transaction, QStringLiteral("tags"));
                            if (!tags.isEmpty()) {
                                SKGTrackerObject tracker;
                                IFOKDO(err, SKGTrackerObject::createTracker(m_importer->getDocument(), tags, tracker))
                                IFOKDO(err, subObj.setTracker(tracker))
                                IFOKDO(err, subObj.save())
                            }

                            // Transfer
                            /*if(dst_account != idAccount && dst_account != "0") {
                                SKGAccountObject account = mapIdAccount[dst_account];
                                SKGOperationObject op2Obj;
                                if(!err) err = account.addOperation(op2Obj, true);
                                if(!err) err = op2Obj.setDate(date);
                                if(!err) err = op2Obj.setUnit(unit);
                                if(!err) err = op2Obj.setPayee(payee);
                                if(!err) err = op2Obj.setMode(mode);

                                if(!err) err = op2Obj.setComment(comment);
                                if(!err) err = op2Obj.setTemplate(j == 2);
                                if(!err) err = op2Obj.setImported(true);
                                if(!err) err = op2Obj.setImportID("HXB-" % SKGServices::intToString(i) % "_TR");
                                if(!err) err = op2Obj.setStatus(status);
                                if(!err) err = op2Obj.save();

                                SKGSubOperationObject subObj;
                                if(!err) err = op2Obj.addSubOperation(subObj);
                                if(!err) err = subObj.setCategory(category);
                                if(!err) err = subObj.setComment(comment);
                                if(!err) err = subObj.setQuantity(-amount);
                                if(!err) err = subObj.save();

                                if(!err) err = op2Obj.setGroupOperation(opObj);
                                if(!err) err = op2Obj.save();
                            }*/

                            // Scheduled transactions
                            if (j == 2) {
                                // Creation of scheduled transaction
                                SKGRecurrentOperationObject recu;
                                IFOKDO(err, opObj.addRecurrentOperation(recu))
                                int limit = SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("limit")));
                                IFOKDO(err, recu.timeLimit(flags & (1 << 7)))
                                IFOKDO(err, recu.setTimeLimit(limit))
                                int punit = SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("unit")));
                                int pstep = SKGServices::stringToInt(getAttribute(transaction, QStringLiteral("every")));
                                SKGRecurrentOperationObject::PeriodUnit p = SKGRecurrentOperationObject::MONTH;
                                if (punit == 0) {
                                    p = SKGRecurrentOperationObject::DAY;
                                } else if (punit == 1) {
                                    p = SKGRecurrentOperationObject::WEEK;
                                } else if (punit == 3) {
                                    p = SKGRecurrentOperationObject::YEAR;
                                }
                                IFOKDO(err, recu.setPeriodIncrement(pstep))
                                IFOKDO(err, recu.setPeriodUnit(p))
                                IFOKDO(err, recu.warnEnabled(flags & (1 << 5)))
                                IFOKDO(err, recu.autoWriteEnabled(flags & (1 << 2)))
                                IFOKDO(err, recu.save())
                                // #define OF_VALID        (1<<0)
                                // #define OF_INCOME       (1<<1)
                                // #define OF_AUTO         (1<<2)
                                // #define OF_ADDED        (1<<3)
                                // #define OF_CHANGED      (1<<4)
                                // #define OF_REMIND       (1<<5)
                                // #define OF_CHEQ2        (1<<6)
                                // #define OF_LIMIT        (1<<7)
                            }
                        }

                        if (!err && i % 500 == 0) {
                            err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
                        }
                        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
                    }

                    SKGENDTRANSACTION(m_importer->getDocument(), err)
                    IFOKDO(err, m_importer->getDocument()->stepForward(j))
                }

                SKGENDTRANSACTION(m_importer->getDocument(), err)
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(4))

            SKGENDTRANSACTION(m_importer->getDocument(), err)

            IFOKDO(err, m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE")))
        }
    }

    return err;
}

QString SKGImportPluginXhb::getAttribute(const QDomElement &iElement, const QString &iAttribute)
{
    QString val = iElement.attribute(iAttribute);
    if (val == QStringLiteral("(null)")) {
        val = QString();
    }
    return val;
}

QString SKGImportPluginXhb::getMimeTypeFilter() const
{
    return "*.xhb|" % i18nc("A file format", "Homebank document");
}

#include <skgimportpluginxhb.moc>
