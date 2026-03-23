/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for KMY import / export.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportpluginkmy.h"

#include <kcompressiondevice.h>
#include <kpluginfactory.h>

#include <qdom.h>
#include <qmath.h>

#include "skgbankincludes.h"
#include "skgimportexportmanager.h"
#include "skgobjectbase.h"
#include "skgpayeeobject.h"
#include "skgservices.h"
#include "skgtraces.h"
#include <utility>

/**
 * Transactions treated.
 */
QSet<QString> SKGImportPluginKmy::m_opTreated;

/**
 * Map id / unit.
 */
QMap<QString, SKGUnitObject> SKGImportPluginKmy::m_mapIdUnit;

/**
 * Map id / account.
 */
QMap<QString, SKGAccountObject> SKGImportPluginKmy::m_mapIdAccount;

/**
 * Map id / category.
 */
QMap<QString, SKGCategoryObject> SKGImportPluginKmy::m_mapIdCategory;

/**
 * Map id / payee.
 */
QMap<QString, SKGPayeeObject> SKGImportPluginKmy::m_mapIdPayee;

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGImportPluginKmy, "metadata.json")

SKGImportPluginKmy::SKGImportPluginKmy(QObject *iImporter, const QVariantList &iArg)
    : SKGImportPlugin(iImporter)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iArg)
}

SKGImportPluginKmy::~SKGImportPluginKmy() = default;

bool SKGImportPluginKmy::isImportPossible()
{
    SKGTRACEINFUNC(10)
    return isExportPossible();
}

SKGError SKGImportPluginKmy::importSecurities(QDomElement &docElem)
{
    SKGError err;
    QDomElement securities = docElem.firstChildElement(QStringLiteral("SECURITIES"));
    if (!err && !securities.isNull()) {
        SKGTRACEINRC(10, "SKGImportPluginKmy::importFile-SECURITIES", err)
        QDomNodeList securityList = securities.elementsByTagName(QStringLiteral("SECURITY"));
        int nb = securityList.count();
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            QDomElement security = securityList.at(i).toElement();
            QString unitName = security.attribute(QStringLiteral("name"));

            // We try a creation
            SKGUnitObject unitObj(m_importer->getDocument());
            SKGUnitObject::createCurrencyUnit(m_importer->getDocument(), unitName, unitObj);

            if (!err && (unitObj.getID() == 0)) {
                // Creation of unit
                err = unitObj.setName(unitName);
                QString symbol = security.attribute(QStringLiteral("symbol"));
                if (symbol.isEmpty()) {
                    symbol = std::move(unitName);
                }
                IFOKDO(err, unitObj.setSymbol(symbol))
                IFOKDO(err, unitObj.setCountry(security.attribute(QStringLiteral("trading-market"))))
                IFOKDO(err, unitObj.setType(SKGUnitObject::SHARE))
                IFOK(err)
                {
                    // Set pairs
                    QDomNodeList pairList = security.elementsByTagName(QStringLiteral("PAIR"));
                    int nb2 = pairList.count();
                    for (int j = 0; !err && j < nb2; ++j) {
                        QDomElement pair = pairList.at(j).toElement();
                        if (pair.attribute(QStringLiteral("key")).toLower() == QStringLiteral("kmm-security-id")) {
                            err = unitObj.setInternetCode(pair.attribute(QStringLiteral("value")));
                        }
                    }
                }
                IFOKDO(err, unitObj.save())
            }

            m_mapIdUnit[security.attribute(QStringLiteral("id"))] = unitObj;

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginKmy::importPrices(QDomElement &docElem)
{
    SKGError err;
    QDomElement prices = docElem.firstChildElement(QStringLiteral("PRICES"));
    if (!err && !prices.isNull()) {
        SKGTRACEINRC(10, "SKGImportPluginKmy::importFile-PRICES", err)
        QDomNodeList pricepairList = prices.elementsByTagName(QStringLiteral("PRICEPAIR"));
        int nb = pricepairList.count();
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import units"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            QDomElement pricepair = pricepairList.at(i).toElement();

            SKGUnitObject unitObj = m_mapIdUnit.value(pricepair.attribute(QStringLiteral("from")));
            if (unitObj.getID() != 0) {
                // Unit is existing
                QDomNodeList pricesList = pricepair.elementsByTagName(QStringLiteral("PRICE"));
                int nb2 = pricesList.count();
                for (int j = 0; !err && j < nb2; ++j) {
                    QDomElement price = pricesList.at(j).toElement();

                    SKGUnitValueObject unitValObj;
                    err = unitObj.addUnitValue(unitValObj);
                    IFOKDO(err, unitValObj.setDate(QDate::fromString(price.attribute(QStringLiteral("date")), QStringLiteral("yyyy-MM-dd"))))
                    IFOKDO(err, unitValObj.setQuantity(toKmyValue(price.attribute(QStringLiteral("price")))))
                    IFOKDO(err, unitValObj.save(true, false))
                }
            }

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginKmy::importInstitutions(QMap<QString, SKGBankObject> &mapIdBank, QDomElement &docElem)
{
    SKGError err;
    QDomElement institutions = docElem.firstChildElement(QStringLiteral("INSTITUTIONS"));
    if (!err && !institutions.isNull()) {
        SKGTRACEINRC(10, "SKGImportPluginKmy::importFile-INSTITUTIONS", err)
        QDomNodeList institutionList = institutions.elementsByTagName(QStringLiteral("INSTITUTION"));
        int nb = institutionList.count();
        for (int i = 0; !err && i < nb; ++i) {
            // Get bank object
            QDomElement bank = institutionList.at(i).toElement();
            SKGBankObject bankObject(m_importer->getDocument());
            err = bankObject.setName(bank.attribute(QStringLiteral("name")));
            IFOKDO(err, bankObject.setNumber(bank.attribute(QStringLiteral("sortcode"))))
            IFOKDO(err, bankObject.save())
            mapIdBank[bank.attribute(QStringLiteral("id"))] = bankObject;
        }
    }
    return err;
}

SKGError SKGImportPluginKmy::importPayees(QMap<QString, SKGPayeeObject> &mapIdPayee, QDomElement &docElem)
{
    SKGError err;
    QDomElement payees = docElem.firstChildElement(QStringLiteral("PAYEES"));
    if (!err && !payees.isNull()) {
        SKGTRACEINRC(10, "SKGImportPluginKmy::importFile-PAYEES", err)
        QDomNodeList payeeList = payees.elementsByTagName(QStringLiteral("PAYEE"));
        int nb = payeeList.count();
        for (int i = 0; !err && i < nb; ++i) {
            // Get account object
            QDomElement payee = payeeList.at(i).toElement();
            QDomElement address = payee.firstChildElement(QStringLiteral("ADDRESS"));
            SKGPayeeObject payeeObject;
            err = SKGPayeeObject::createPayee(m_importer->getDocument(), payee.attribute(QStringLiteral("name")), payeeObject);
            IFOK(err)
            {
                QString add = address.attribute(QStringLiteral("street")) % u' ' % address.attribute(QStringLiteral("postcode")) % u' '
                    % address.attribute(QStringLiteral("city")) % u' ' % address.attribute(QStringLiteral("state")) % u' '
                    % address.attribute(QStringLiteral("telephone"));
                add.replace(QStringLiteral("  "), QStringLiteral(" "));
                err = payeeObject.setAddress(add.trimmed());
                IFOKDO(err, payeeObject.save())
            }
            IFOK(err)
            {
                mapIdPayee[payee.attribute(QStringLiteral("id"))] = payeeObject;
            }
        }
    }
    return err;
}

SKGError SKGImportPluginKmy::importTransactions(QDomElement &docElem, SKGAccountObject &kmymoneyTemporaryAccount, QMap<QString, SKGPayeeObject> &mapIdPayee)
{
    SKGError err;
    SKGTRACEINRC(10, "SKGImportPluginKmy::importFile-TRANSACTION", err)
    QDomNodeList transactionList = docElem.elementsByTagName(QStringLiteral("TRANSACTION"));
    int nb = transactionList.count();
    err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import transactions"), nb);
    QVector<QDomNode> suboperationsList;
    for (int i = 0; !err && i < nb; ++i) {
        // Get transaction object
        QDomElement operation = transactionList.at(i).toElement();

        SKGOperationObject operationObj;
        err = kmymoneyTemporaryAccount.addOperation(operationObj, true);
        IFOKDO(err, operationObj.setDate(QDate::fromString(operation.attribute(QStringLiteral("postdate")), QStringLiteral("yyyy-MM-dd"))))
        IFOKDO(err, operationObj.setComment(operation.attribute(QStringLiteral("memo"))))
        IFOKDO(err, operationObj.setImported(true))
        IFOKDO(err, operationObj.setImportID("KMY-" % operation.attribute(QStringLiteral("id"))))
        IFOK(err)
        {
            QString unitName = operation.attribute(QStringLiteral("commodity"));
            if (unitName.isEmpty()) {
                unitName = QStringLiteral("??");
            }
            SKGUnitObject unit(m_importer->getDocument());
            err = unit.setName(unitName);
            IFOKDO(err, unit.setSymbol(unitName))
            IFOK(err)
            {
                if (unit.exist()) {
                    err = unit.load();
                } else {
                    err = unit.save();
                }

                IFOKDO(err, operationObj.setUnit(unit))
            }
        }
        IFOKDO(err, operationObj.save(false, false))

        // Is it a schedule ?
        IFOK(err)
        {
            QDomElement recu = operation.parentNode().toElement();
            if (recu.tagName() == QStringLiteral("SCHEDULED_TX")) {
                // Yes ==> creation of the schedule
                IFOKDO(err, operationObj.load())
                IFOKDO(err, operationObj.setTemplate(true))
                IFOKDO(err, operationObj.save(true, false))

                // Yes ==> creation of the schedule
                SKGRecurrentOperationObject recuObj;
                IFOKDO(err, operationObj.addRecurrentOperation(recuObj))
                IFOKDO(err, recuObj.setDate(operationObj.getDate()))
                IFOKDO(err, recuObj.autoWriteEnabled(recu.attribute(QStringLiteral("autoEnter")) == QStringLiteral("1")))
                IFOKDO(err, recuObj.setAutoWriteDays(0))

                int occu = SKGServices::stringToInt(recu.attribute(QStringLiteral("occurenceMultiplier")));
                QString occuType = recu.attribute(QStringLiteral("occurence")); // krazy:exclude=spelling
                SKGRecurrentOperationObject::PeriodUnit period;
                if (occuType == QStringLiteral("32") || occuType == QStringLiteral("1024") || occuType == QStringLiteral("256")
                    || occuType == QStringLiteral("8192")) {
                    period = SKGRecurrentOperationObject::MONTH;
                } else if (occuType == QStringLiteral("1")) {
                    period = SKGRecurrentOperationObject::DAY;
                    IFOKDO(err, recuObj.timeLimit(true))
                    IFOKDO(err, recuObj.setTimeLimit(1))
                } else if (occuType == QStringLiteral("2")) {
                    period = SKGRecurrentOperationObject::DAY;
                } else if (occuType == QStringLiteral("4")) {
                    period = SKGRecurrentOperationObject::WEEK;
                } else if (occuType == QStringLiteral("18")) {
                    period = SKGRecurrentOperationObject::WEEK;
                    occu *= 2;
                } else {
                    period = SKGRecurrentOperationObject::YEAR;
                }

                IFOKDO(err, recuObj.setPeriodIncrement(occu))
                IFOKDO(err, recuObj.setPeriodUnit(period))

                IFOK(err)
                {
                    QString endDate = recu.attribute(QStringLiteral("endDate"));
                    if (!endDate.isEmpty()) {
                        IFOKDO(err, recuObj.timeLimit(true))
                        IFOKDO(err, recuObj.setTimeLimit(QDate::fromString(recu.attribute(QStringLiteral("endDate")), QStringLiteral("yyyy-MM-dd"))))
                    }
                }

                if (occuType == QStringLiteral("1") && !recu.attribute(QStringLiteral("lastPayment")).isEmpty()) {
                    // Single schedule already done
                    IFOKDO(err, recuObj.timeLimit(true))
                    IFOKDO(err, recuObj.setTimeLimit(0))
                }
                IFOKDO(err, recuObj.save(true, false))
            }
        }

        // Get splits
        bool parentSet = false;
        double quantity = 0;
        QDomElement splits = operation.firstChildElement(QStringLiteral("SPLITS"));

        int nbSuboperations = 0;
        suboperationsList.resize(0);
        {
            QDomNodeList suboperationsListTmp = splits.elementsByTagName(QStringLiteral("SPLIT"));
            nbSuboperations = suboperationsListTmp.count();

            for (int j = 0; !err && j < nbSuboperations; ++j) {
                QDomElement suboperation = suboperationsListTmp.at(j).toElement();
                if (m_mapIdCategory.contains(suboperation.attribute(QStringLiteral("account")))) {
                    suboperationsList.push_front(suboperation);
                } else {
                    suboperationsList.push_back(suboperation);
                }
            }
        }

        SKGSubOperationObject suboperationObj;
        for (int j = 0; !err && j < nbSuboperations; ++j) {
            QDomElement suboperation = suboperationsList.at(j).toElement();

            // Set transaction attributes
            IFOKDO(err, operationObj.setPayee(mapIdPayee[suboperation.attribute(QStringLiteral("payee"))]))
            if (!err && operationObj.getComment().isEmpty()) {
                err = operationObj.setComment(suboperation.attribute(QStringLiteral("memo")));
            }
            IFOKDO(err, operationObj.setNumber(suboperation.attribute(QStringLiteral("number"))))

            // Set state
            if (operationObj.getStatus() == SKGOperationObject::NONE) {
                QString val = suboperation.attribute(QStringLiteral("reconcileflag"));
                IFOKDO(err,
                       operationObj.setStatus(val == QStringLiteral("1")       ? SKGOperationObject::MARKED
                                                  : val == QStringLiteral("2") ? SKGOperationObject::CHECKED
                                                                               : SKGOperationObject::NONE))
            }
            IFOKDO(err, operationObj.save())

            if (m_mapIdCategory.contains(suboperation.attribute(QStringLiteral("account")))) {
                // It is a split on category
                SKGCategoryObject cat = m_mapIdCategory.value(suboperation.attribute(QStringLiteral("account")));

                double q = -toKmyValue(suboperation.attribute(QStringLiteral("shares")));
                if (!err && ((suboperationObj.getID() == 0) || suboperationObj.getQuantity() != q)) {
                    err = operationObj.addSubOperation(suboperationObj);
                }
                IFOKDO(err, suboperationObj.setQuantity(q))
                IFOKDO(err, suboperationObj.setCategory(cat))
                IFOKDO(err, suboperationObj.setComment(suboperation.attribute(QStringLiteral("memo"))))
                IFOKDO(err, suboperationObj.save(true, false))
            } else {
                QString accountSubOp = suboperation.attribute(QStringLiteral("account"));
                if (!accountSubOp.isEmpty()) {
                    if (nbSuboperations == 1) {
                        SKGUnitObject unit = m_mapIdUnit.value(accountSubOp);
                        if (unit.getID() != 0) {
                            IFOKDO(err, operationObj.setUnit(unit))
                        }
                    }

                    if (!m_mapIdAccount.contains(accountSubOp)
                        || (nbSuboperations == 2 && m_mapIdAccount.value(accountSubOp) == kmymoneyTemporaryAccount
                            && suboperationsList.at(0).toElement().attribute(QStringLiteral("action")).isEmpty()
                            && suboperationsList.at(1).toElement().attribute(QStringLiteral("action")).isEmpty())) {
                        // Set as initial balance
                        IFOKDO(err, operationObj.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                        IFOKDO(err, operationObj.setStatus(SKGOperationObject::CHECKED))
                        IFOKDO(err, operationObj.save(true, false))

                        if (!err && (suboperationObj.getID() == 0)) {
                            err = operationObj.addSubOperation(suboperationObj);
                        }

                        IFOKDO(err, suboperationObj.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
                        IFOKDO(err, suboperationObj.save(true, false))
                    } else {
                        // It is a transfer of account
                        SKGAccountObject act = m_mapIdAccount.value(accountSubOp);

                        if (parentSet) {
                            // If the parent is already set, it means that is a transfer
                            SKGOperationObject operationObj2;
                            IFOKDO(err, act.addOperation(operationObj2, true))
                            IFOKDO(err, operationObj2.setTemplate(operationObj.isTemplate()))
                            IFOKDO(err, operationObj2.setDate(operationObj.getDate()))
                            IFOKDO(err, operationObj2.setNumber(operationObj.getNumber()))
                            IFOKDO(err, operationObj2.setComment(operationObj.getComment()))
                            SKGPayeeObject payeeObject;
                            operationObj.getPayee(payeeObject);
                            IFOKDO(err, operationObj2.setPayee(payeeObject))
                            IFOK(err)
                            {
                                QString val = suboperation.attribute(QStringLiteral("reconcileflag"));
                                err = operationObj2.setStatus(val == QStringLiteral("1")       ? SKGOperationObject::MARKED
                                                                  : val == QStringLiteral("2") ? SKGOperationObject::CHECKED
                                                                                               : SKGOperationObject::NONE);
                            }
                            IFOKDO(err, operationObj2.setImported(true))
                            IFOKDO(err, operationObj2.setImportID(operationObj.getImportID()))

                            SKGUnitObject unit = m_mapIdUnit.value(accountSubOp);
                            if (unit.getID() != 0) {
                                IFOKDO(err, operationObj2.setUnit(unit))
                            } else {
                                IFOKDO(err, operationObj.getUnit(unit))
                                IFOKDO(err, operationObj2.setUnit(unit))
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
                                quantity = toKmyValue(suboperation.attribute(QStringLiteral("shares")));
                                err = suboperationObj2.setQuantity(quantity);
                            }
                            IFOKDO(err, suboperationObj2.setComment(suboperation.attribute(QStringLiteral("memo"))))
                            IFOKDO(err, suboperationObj2.save(true, false))
                        } else {
                            // We set the parent
                            if (Q_LIKELY(!err) && (act.getID() == 0)) {
                                err =
                                    SKGError(ERR_FAIL,
                                             i18nc("Error message", "Account with identifier %1 not found", suboperation.attribute(QStringLiteral("account"))));
                            }
                            IFOKDO(err, operationObj.setParentAccount(act, true))
                            IFOKDO(err, operationObj.save())

                            // Compute quantity
                            quantity = toKmyValue(suboperation.attribute(QStringLiteral("shares")));

                            // Create sub transaction on operationObj
                            quantity -= SKGServices::stringToDouble(operationObj.getAttribute(QStringLiteral("f_QUANTITY")));
                            if (quantity != 0 || nbSuboperations == 1) {
                                IFOKDO(err, operationObj.addSubOperation(suboperationObj))
                                IFOKDO(err, suboperationObj.setQuantity(quantity))
                                IFOKDO(err, suboperationObj.setComment(suboperation.attribute(QStringLiteral("memo"))))
                                IFOKDO(err, suboperationObj.save(true, false))
                            }

                            parentSet = true;
                        }
                    }
                }
            }
        }

        if (!err && i % 500 == 0) {
            err = m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE"));
        }

        IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
    }

    SKGENDTRANSACTION(m_importer->getDocument(), err)

    return err;
}

SKGError SKGImportPluginKmy::importBudget(QDomElement &docElem)
{
    SKGError err;
    QDomElement budgets = docElem.firstChildElement(QStringLiteral("BUDGETS"));
    if (!err && !budgets.isNull()) {
        SKGTRACEINRC(10, "SKGImportPluginKmy::importFile-BUDGETS", err)
        // Build cache of categories
        QMap<int, bool> catExpense;
        {
            SKGStringListList list;
            err = m_importer->getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT id, t_TYPEEXPENSE FROM v_category_display"), list);
            int nb = list.count();
            for (int i = 1; i < nb; ++i) {
                catExpense[SKGServices::stringToInt(list.at(i).at(0))] = (list.at(i).at(0) == QStringLiteral("-"));
            }
        }

        QDomNodeList budgetList = budgets.elementsByTagName(QStringLiteral("BUDGET"));
        int nb = budgetList.count();
        IFOKDO(err, m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import budgets"), nb))
        for (int i = 0; !err && i < nb; ++i) {
            QDomElement budget = budgetList.at(i).toElement();
            QDomNodeList accountList = budget.elementsByTagName(QStringLiteral("ACCOUNT"));
            int nb2 = accountList.count();
            for (int j = 0; !err && j < nb2; ++j) {
                QDomElement account = accountList.at(j).toElement();
                SKGCategoryObject cat = m_mapIdCategory.value(account.attribute(QStringLiteral("id")));
                QString budgetlevel = account.attribute(QStringLiteral("budgetlevel"));

                QDomNodeList periodList = account.elementsByTagName(QStringLiteral("PERIOD"));
                int nb3 = periodList.count();
                for (int k = 0; !err && k < nb3; ++k) {
                    QDomElement period = periodList.at(k).toElement();

                    double q = toKmyValue(period.attribute(QStringLiteral("amount")));

                    // Are we able to find to sign with the category ?
                    if (catExpense[cat.getID()]) {
                        q = -q;
                    }

                    QStringList dates = SKGServices::splitCSVLine(period.attribute(QStringLiteral("start")), '-');
                    if (dates.count() == 3) {
                        // We try a creation
                        for (int m = 1; !err && m <= (budgetlevel == QStringLiteral("monthly") ? 12 : 1); ++m) {
                            SKGBudgetObject budget2(m_importer->getDocument());
                            err = budget2.setCategory(cat);
                            IFOKDO(err, budget2.setBudgetedAmount(q))
                            IFOKDO(err, budget2.setYear(SKGServices::stringToDouble(dates.at(0))))
                            IFOKDO(err,
                                   budget2.setMonth(budgetlevel == QStringLiteral("monthbymonth") ? SKGServices::stringToDouble(dates.at(1))
                                                        : budgetlevel == QStringLiteral("yearly") ? 0
                                                                                                  : m));
                            IFOKDO(err, budget2.save(true, false))
                        }
                    }
                }
            }

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginKmy::importAccounts(SKGBankObject &bank,
                                            SKGAccountObject &kmymoneyTemporaryAccount,
                                            QMap<QString, SKGBankObject> &mapIdBank,
                                            QDomElement &docElem)
{
    SKGError err;
    IFOKDO(err, m_importer->getDocument()->addOrModifyAccount(QStringLiteral("KMYMONEY-TEMPORARY-ACCOUNT"), QString(), QStringLiteral("KMYMONEY")))
    IFOKDO(err, bank.setName(QStringLiteral("KMYMONEY")))
    IFOKDO(err, bank.load())
    IFOKDO(err, kmymoneyTemporaryAccount.setName(QStringLiteral("KMYMONEY-TEMPORARY-ACCOUNT")))
    IFOKDO(err, kmymoneyTemporaryAccount.load())

    QDomElement accounts = docElem.firstChildElement(QStringLiteral("ACCOUNTS"));
    if (!err && !accounts.isNull()) {
        SKGTRACEINRC(10, "SKGImportPluginKmy::importFile-ACCOUNTS", err)
        QDomNodeList accountList = accounts.elementsByTagName(QStringLiteral("ACCOUNT"));
        int nb = accountList.count();
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import accounts"), nb);
        QMap<QString, QString> type15Account;
        QMap<QString, QString> type15Unit;
        for (int i = 0; !err && i < nb; ++i) {
            // Get account object
            QDomElement account = accountList.at(i).toElement();
            QString type = account.attribute(QStringLiteral("type"));
            if (type == QStringLiteral("15")) {
                // Actions: the real account is the parent
                type15Account[account.attribute(QStringLiteral("id"))] = account.attribute(QStringLiteral("parentaccount"));
                type15Unit[account.attribute(QStringLiteral("id"))] = account.attribute(QStringLiteral("currency"));
            } else if (type != QStringLiteral("12") && type != QStringLiteral("13") && type != QStringLiteral("16")) {
                // Get the bank
                QString bankId = account.attribute(QStringLiteral("institution"));
                SKGBankObject bankObj = (bankId.isEmpty() ? bank : mapIdBank[account.attribute(QStringLiteral("institution"))]);

                // Creation of the account
                SKGAccountObject accountObj;
                IFOKDO(err, bankObj.addAccount(accountObj))
                IFOKDO(err, accountObj.setName(account.attribute(QStringLiteral("name"))))
                IFOKDO(err, accountObj.setNumber(account.attribute(QStringLiteral("number"))))
                IFOKDO(err, accountObj.setComment(account.attribute(QStringLiteral("description"))))
                IFOK(err)
                {
                    SKGAccountObject::AccountType typeA =
                        (type == QStringLiteral("4")
                             ? SKGAccountObject::CREDITCARD
                             : (type == QStringLiteral("7")
                                    ? SKGAccountObject::INVESTMENT
                                    : (type == QStringLiteral("9")
                                           ? SKGAccountObject::ASSETS
                                           : (type == QStringLiteral("3")
                                                  ? SKGAccountObject::WALLET
                                                  : (type == QStringLiteral("10") ? SKGAccountObject::LOAN : SKGAccountObject::CURRENT)))));
                    err = accountObj.setType(typeA);
                }
                IFOK(err)
                {
                    // Set pairs
                    QDomNodeList pairList = account.elementsByTagName(QStringLiteral("PAIR"));
                    int nb2 = pairList.count();
                    for (int j = 0; !err && j < nb2; ++j) {
                        QDomElement pair = pairList.at(j).toElement();
                        if (pair.attribute(QStringLiteral("key")).toLower() == QStringLiteral("mm-closed")
                            && pair.attribute(QStringLiteral("value")).toLower() == QStringLiteral("yes")) {
                            err = accountObj.setClosed(true);
                        } else if (pair.attribute(QStringLiteral("key")).toLower() == QStringLiteral("preferredaccount")
                                   && pair.attribute(QStringLiteral("value")).toLower() == QStringLiteral("yes")) {
                            err = accountObj.bookmark(true);
                        } else if (pair.attribute(QStringLiteral("key")).toLower() == QStringLiteral("minbalanceabsolute")) {
                            err = accountObj.setMinLimitAmount(toKmyValue(pair.attribute(QStringLiteral("value"))));
                            IFOKDO(err, accountObj.minLimitAmountEnabled(true))
                        } else if (pair.attribute(QStringLiteral("key")).toLower() == QStringLiteral("maxcreditabsolute")) {
                            err = accountObj.setMaxLimitAmount(-toKmyValue(pair.attribute(QStringLiteral("value"))));
                            IFOKDO(err, accountObj.maxLimitAmountEnabled(true))
                        }
                    }
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

                m_mapIdAccount[account.attribute(QStringLiteral("id"))] = accountObj;
            } else if (type == QStringLiteral("16")) {
                m_mapIdAccount[account.attribute(QStringLiteral("id"))] = kmymoneyTemporaryAccount;
            } else {
                // Create category
                SKGCategoryObject cat = m_mapIdCategory.value(account.attribute(QStringLiteral("id")));
                if (cat.getID() != 0) {
                    // Already existing ==> we must set the right name
                    err = cat.setName(account.attribute(QStringLiteral("name")));
                    IFOKDO(err, cat.save())
                } else {
                    // We must create it
                    cat = SKGCategoryObject(m_importer->getDocument());
                    err = cat.setName(account.attribute(QStringLiteral("name")));
                    IFOKDO(err, cat.save())
                }
                if (err) {
                    // The category already exists
                    SKGCategoryObject catp;
                    err = cat.getParentCategory(catp);
                    QString fullName = catp.getFullName() % OBJECTSEPARATOR % cat.getName();
                    IFOKDO(err, cat.remove(false))
                    IFOKDO(err, SKGCategoryObject::createPathCategory(m_importer->getDocument(), fullName, cat))
                }
                m_mapIdCategory[account.attribute(QStringLiteral("id"))] = cat;

                // Create sub categories
                QDomNodeList subaccountList = account.elementsByTagName(QStringLiteral("SUBACCOUNT"));
                int nb2 = subaccountList.count();
                for (int j = 0; !err && j < nb2; ++j) {
                    QDomElement subaccount = subaccountList.at(j).toElement();

                    // Is child already existing ?
                    SKGCategoryObject cat2 = m_mapIdCategory.value(subaccount.attribute(QStringLiteral("id")));
                    if (cat2.getID() != 0) {
                        // Yes ==> reparent
                        err = cat2.setParentCategory(cat);
                        IFOKDO(err, cat2.save(true, false))
                    } else {
                        // No ==> create
                        IFOKDO(err, cat.addCategory(cat2))
                        IFOKDO(err, cat2.setName(subaccount.attribute(QStringLiteral("id"))))
                        IFOKDO(err, cat2.save())
                        m_mapIdCategory[subaccount.attribute(QStringLiteral("id"))] = cat2;
                    }
                }
            }

            QStringList list = type15Account.keys();
            for (const auto &k : std::as_const(list)) {
                m_mapIdAccount[k] = m_mapIdAccount.value(type15Account[k]);
                m_mapIdUnit[account.attribute(QStringLiteral("id"))] = m_mapIdUnit.value(account.attribute(QStringLiteral("currency")));
            }

            list = type15Unit.keys();
            for (const auto &k : std::as_const(list)) {
                m_mapIdUnit[k] = m_mapIdUnit[type15Unit[k]];
            }

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginKmy::importFile()
{
    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }

    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Initialisation
    m_mapIdUnit.clear();
    m_mapIdAccount.clear();
    m_mapIdCategory.clear();
    m_mapIdPayee.clear();

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
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Import step", "Import %1 file", "KMY"), 8);

            // Step 1-Get units
            IFOKDO(err, importSecurities(docElem))
            IFOKDO(err, m_importer->getDocument()->stepForward(1))

            // Step 2-Get units prices
            IFOKDO(err, importPrices(docElem))
            IFOKDO(err, m_importer->getDocument()->stepForward(2))

            // Step 3-Create banks
            QMap<QString, SKGBankObject> mapIdBank;
            IFOKDO(err, importInstitutions(mapIdBank, docElem))
            IFOKDO(err, m_importer->getDocument()->stepForward(3))

            // Step 4-Create account and categories
            // Create bank and temporary account for kmymoney import
            SKGAccountObject kmymoneyTemporaryAccount(m_importer->getDocument());
            SKGBankObject bank(m_importer->getDocument());
            IFOKDO(err, importAccounts(bank, kmymoneyTemporaryAccount, mapIdBank, docElem))
            IFOKDO(err, m_importer->getDocument()->stepForward(4))

            // Step 5-Read payees
            QMap<QString, SKGPayeeObject> mapIdPayee;
            IFOKDO(err, importPayees(mapIdPayee, docElem))
            IFOKDO(err, m_importer->getDocument()->stepForward(5))

            // Step 6-Create transactions
            IFOKDO(err, importTransactions(docElem, kmymoneyTemporaryAccount, mapIdPayee))
            IFOKDO(err, m_importer->getDocument()->stepForward(6))

            // Step 7-Get budgets
            IFOKDO(err, importBudget(docElem))
            IFOKDO(err, m_importer->getDocument()->stepForward(7))

            // Step 8-Remove useless account and temporary account
            {IFOKDO(err, kmymoneyTemporaryAccount.remove(false, true)) IFOKDO(
                err,
                m_importer->getDocument()->executeSqliteOrder(
                    "DELETE FROM account WHERE rd_bank_id=" % SKGServices::intToString(bank.getID())
                    % " AND (SELECT COUNT(1) FROM operation WHERE operation.rd_account_id=account.id)=0"))} IFOKDO(err,
                                                                                                                   m_importer->getDocument()->stepForward(8))

                SKGENDTRANSACTION(m_importer->getDocument(), err)

                    IFOKDO(err, m_importer->getDocument()->executeSqliteOrder(QStringLiteral("ANALYZE")))
        }
    }

    // Clean
    m_mapIdUnit.clear();
    m_mapIdAccount.clear();
    m_mapIdCategory.clear();
    m_mapIdPayee.clear();

    return err;
}

bool SKGImportPluginKmy::isExportPossible()
{
    SKGTRACEINFUNC(10)
    return (m_importer->getDocument() == nullptr ? true : m_importer->getFileNameExtension() == QStringLiteral("KMY"));
}

SKGError SKGImportPluginKmy::exportHeader(QDomDocument &doc, QDomElement &root)
{
    SKGError err;
    QDomElement fileindo = doc.createElement(QStringLiteral("FILEINFO"));
    root.appendChild(fileindo);

    {
        // <CREATION_DATE>
        QDomElement creation_date = doc.createElement(QStringLiteral("CREATION_DATE"));
        fileindo.appendChild(creation_date);
        creation_date.setAttribute(QStringLiteral("date"), SKGServices::dateToSqlString(QDateTime::currentDateTime()));

        // <LAST_MODIFIED_DATE>
        QDomElement last_modified_date = doc.createElement(QStringLiteral("LAST_MODIFIED_DATE"));
        fileindo.appendChild(last_modified_date);
        last_modified_date.setAttribute(QStringLiteral("date"), SKGServices::dateToSqlString(QDateTime::currentDateTime()));

        // <VERSION>
        QDomElement version = doc.createElement(QStringLiteral("VERSION"));
        fileindo.appendChild(version);
        version.setAttribute(QStringLiteral("id"), QStringLiteral("1"));

        // <FIXVERSION>
        QDomElement fixversion = doc.createElement(QStringLiteral("FIXVERSION"));
        fileindo.appendChild(fixversion);
        fixversion.setAttribute(QStringLiteral("id"), QStringLiteral("2"));
    }

    // <USER>
    QDomElement user = doc.createElement(QStringLiteral("USER"));
    root.appendChild(user);
    user.setAttribute(QStringLiteral("email"), QString());
    user.setAttribute(QStringLiteral("name"), QString());
    {
        // ADDRESS
        QDomElement address = doc.createElement(QStringLiteral("ADDRESS"));
        user.appendChild(address);
        address.setAttribute(QStringLiteral("street"), QString());
        address.setAttribute(QStringLiteral("zipcode"), QString());
        address.setAttribute(QStringLiteral("county"), QString());
        address.setAttribute(QStringLiteral("city"), QString());
        address.setAttribute(QStringLiteral("telephone"), QString());
    }
    return err;
}

SKGError SKGImportPluginKmy::exportSecurities(QDomDocument &doc, QDomElement &root, const QString &stdUnit)
{
    SKGError err;
    QDomElement securities = doc.createElement(QStringLiteral("SECURITIES"));
    root.appendChild(securities);

    QDomElement currencies = doc.createElement(QStringLiteral("CURRENCIES"));
    root.appendChild(currencies);

    SKGObjectBase::SKGListSKGObjectBase objects;
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_unit"), QStringLiteral("t_type!='I'"), objects))
    int nb = objects.count();
    securities.setAttribute(QStringLiteral("count"), SKGServices::intToString(nb));

    QStringList importedCurrency;
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export units"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGUnitObject obj(objects.at(i));
            if (obj.getType() == SKGUnitObject::SHARE || obj.getType() == SKGUnitObject::OBJECT) {
                QDomElement security = doc.createElement(QStringLiteral("SECURITY"));
                securities.appendChild(security);

                SKGUnitObject parentUnit;
                obj.getUnit(parentUnit);
                QString unitP = SKGUnitObject::getInternationalCode(parentUnit.getName());
                if (unitP.isEmpty()) {
                    unitP = stdUnit;
                }

                security.setAttribute(QStringLiteral("id"), obj.getName());
                security.setAttribute(QStringLiteral("trading-currency"), unitP);
                security.setAttribute(QStringLiteral("saf"), QStringLiteral("100000"));
                security.setAttribute(QStringLiteral("symbol"), obj.getSymbol());
                security.setAttribute(QStringLiteral("trading-market"), obj.getCountry());
                security.setAttribute(QStringLiteral("type"), QStringLiteral("4"));
                security.setAttribute(QStringLiteral("name"), obj.getName());

                QString internetCode = obj.getInternetCode();
                if (!internetCode.isEmpty()) {
                    QDomElement keyvaluepairs2 = doc.createElement(QStringLiteral("KEYVALUEPAIRS"));
                    security.appendChild(keyvaluepairs2);

                    QDomElement pair1 = doc.createElement(QStringLiteral("PAIR"));
                    keyvaluepairs2.appendChild(pair1);
                    pair1.setAttribute(QStringLiteral("key"), QStringLiteral("kmm-online-source"));
                    pair1.setAttribute(QStringLiteral("value"), QStringLiteral("Yahoo"));

                    QDomElement pair2 = doc.createElement(QStringLiteral("PAIR"));
                    keyvaluepairs2.appendChild(pair2);
                    pair2.setAttribute(QStringLiteral("key"), QStringLiteral("kmm-security-id"));
                    pair2.setAttribute(QStringLiteral("value"), internetCode);
                }
            } else {
                QDomElement currency = doc.createElement(QStringLiteral("CURRENCY"));
                currencies.appendChild(currency);

                QString unit = SKGUnitObject::getInternationalCode(obj.getName());
                if (unit.isEmpty()) {
                    unit = obj.getName();
                }

                currency.setAttribute(QStringLiteral("saf"), QStringLiteral("100"));
                currency.setAttribute(QStringLiteral("symbol"), obj.getSymbol());
                currency.setAttribute(QStringLiteral("type"), QStringLiteral("3"));
                currency.setAttribute(QStringLiteral("id"), unit);
                currency.setAttribute(QStringLiteral("name"), obj.getName());
                currency.setAttribute(QStringLiteral("ppu"), QStringLiteral("100"));
                currency.setAttribute(QStringLiteral("scf"), QStringLiteral("100"));

                importedCurrency.push_back(obj.getSymbol());
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }

    // <CURRENCIES>
    QStringList units = SKGUnitObject::getListofKnownCurrencies(false);
    nb = units.count();
    int nbreal = 0;
    for (int i = 0; i < nb; ++i) {
        SKGServices::SKGUnitInfo info = SKGUnitObject::getUnitInfo(units.at(i));
        if (info.Name != info.Symbol && !importedCurrency.contains(info.Symbol)) {
            QDomElement currency = doc.createElement(QStringLiteral("CURRENCY"));
            currencies.appendChild(currency);
            currency.setAttribute(QStringLiteral("saf"), QStringLiteral("100"));
            currency.setAttribute(QStringLiteral("symbol"), info.Symbol);
            currency.setAttribute(QStringLiteral("type"), QStringLiteral("3"));
            currency.setAttribute(QStringLiteral("id"), SKGUnitObject::getInternationalCode(info.Name));
            currency.setAttribute(QStringLiteral("name"), info.Name);
            currency.setAttribute(QStringLiteral("ppu"), QStringLiteral("100"));
            currency.setAttribute(QStringLiteral("scf"), QStringLiteral("100"));

            ++nbreal;
        }
    }
    currencies.setAttribute(QStringLiteral("count"), SKGServices::intToString(nbreal));

    // <PRICES>
    QDomElement prices = doc.createElement(QStringLiteral("PRICES"));
    root.appendChild(prices);
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_unit"), QStringLiteral("t_type='S'"), objects))
    nb = objects.count();
    prices.setAttribute(QStringLiteral("count"), SKGServices::intToString(nb));
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export units"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGUnitObject obj(objects.at(i));
            QDomElement pricepair = doc.createElement(QStringLiteral("PRICEPAIR"));
            prices.appendChild(pricepair);

            QString unitP = SKGUnitObject::getInternationalCode(obj.getName());
            if (unitP.isEmpty()) {
                unitP = stdUnit;
            }

            pricepair.setAttribute(QStringLiteral("from"), obj.getName());
            pricepair.setAttribute(QStringLiteral("to"), unitP);

            SKGObjectBase::SKGListSKGObjectBase unitValues;
            err = obj.getUnitValues(unitValues);
            int nb2 = unitValues.count();
            for (int j = 0; !err && j < nb2; ++j) {
                QDomElement price = doc.createElement(QStringLiteral("PRICE"));
                pricepair.appendChild(price);

                SKGUnitValueObject unitval(unitValues.at(j));
                price.setAttribute(QStringLiteral("price"), SKGImportPluginKmy::kmyValue(unitval.getQuantity()));
                price.setAttribute(QStringLiteral("source"), QStringLiteral("Utilisateur"));
                price.setAttribute(QStringLiteral("date"), SKGServices::dateToSqlString(unitval.getDate()));
            }

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }

    // <REPORTS>
    QDomElement reports = doc.createElement(QStringLiteral("REPORTS"));
    root.appendChild(reports);
    reports.setAttribute(QStringLiteral("count"), QStringLiteral("0"));

    return err;
}

SKGError SKGImportPluginKmy::exportBudgets(QDomDocument &doc, QDomElement &root)
{
    SKGError err;
    QDomElement budgets = doc.createElement(QStringLiteral("BUDGETS"));
    root.appendChild(budgets);

    SKGObjectBase::SKGListSKGObjectBase objects;
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_budget"), QStringLiteral("1=1 ORDER BY i_year, i_month"), objects))
    int nb = objects.count();
    int nbBudgets = 0;
    int currentYear = 0;
    QDomElement budget;

    QMap<QString, QDomElement> mapCatAccount;
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export budgets"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGBudgetObject obj(objects.at(i));
            SKGCategoryObject cat;
            obj.getCategory(cat);
            QString catId = getKmyUniqueIdentifier(cat);
            int year = obj.getYear();
            QString yearString = SKGServices::intToString(year);
            int month = obj.getMonth();
            QString monthString = SKGServices::intToString(month);
            if (monthString.isEmpty()) {
                monthString = u'0' % monthString;
            }
            if (currentYear != year) {
                budget = doc.createElement(QStringLiteral("BUDGET"));
                budgets.appendChild(budget);
                budget.setAttribute(QStringLiteral("version"), QStringLiteral("2"));
                budget.setAttribute(QStringLiteral("id"), yearString);
                budget.setAttribute(QStringLiteral("start"), yearString % "-01-01");
                budget.setAttribute(QStringLiteral("name"), yearString);

                currentYear = year;
                mapCatAccount.clear();
                ++nbBudgets;
            }

            QDomElement account = mapCatAccount[catId];
            if (account.isNull() && !catId.isEmpty()) {
                account = doc.createElement(QStringLiteral("ACCOUNT"));
                budget.appendChild(account);
                account.setAttribute(QStringLiteral("budgetsubaccounts"), QStringLiteral("0"));
                account.setAttribute(QStringLiteral("id"), catId);
                mapCatAccount[catId] = account;
            }
            if (!account.isNull()) {
                account.setAttribute(QStringLiteral("budgetlevel"), obj.getMonth() == 0 ? QStringLiteral("yearly") : QStringLiteral("monthbymonth"));

                QDomElement period = doc.createElement(QStringLiteral("PERIOD"));
                account.appendChild(period);
                period.setAttribute(QStringLiteral("amount"), SKGImportPluginKmy::kmyValue(qAbs(obj.getBudgetedAmount())));
                period.setAttribute(QStringLiteral("start"), yearString % u'-' % (obj.getMonth() == 0 ? QStringLiteral("01") : monthString) % "-01");
            }

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    budgets.setAttribute(QStringLiteral("count"), SKGServices::intToString(nbBudgets));
    return err;
}

SKGError SKGImportPluginKmy::exportSchedules(QDomDocument &doc, QDomElement &root)
{
    SKGError err;
    QDomElement schedules = doc.createElement(QStringLiteral("SCHEDULES"));
    root.appendChild(schedules);

    SKGObjectBase::SKGListSKGObjectBase objects;
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_recurrentoperation"), QString(), objects))
    int nb = objects.count();
    schedules.setAttribute(QStringLiteral("count"), SKGServices::intToString(nb));
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export scheduled transactions"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGRecurrentOperationObject obj(objects.at(i));
            SKGOperationObject op;
            err = obj.getParentOperation(op);
            IFOK(err)
            {
                QDomElement scheduled_tx = doc.createElement(QStringLiteral("SCHEDULED_TX"));
                schedules.appendChild(scheduled_tx);

                scheduled_tx.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(obj));
                scheduled_tx.setAttribute(QStringLiteral("name"), getKmyUniqueIdentifier(obj));
                scheduled_tx.setAttribute(QStringLiteral("startDate"), obj.getAttribute(QStringLiteral("d_date")));
                scheduled_tx.setAttribute(QStringLiteral("lastPayment"), obj.getAttribute(QStringLiteral("d_date")));
                bool autoEnter = obj.isAutoWriteEnabled();
                scheduled_tx.setAttribute(QStringLiteral("autoEnter"), autoEnter ? QStringLiteral("1") : QStringLiteral("0"));

                QString occuType;
                int occu = obj.getPeriodIncrement();
                SKGRecurrentOperationObject::PeriodUnit punit = obj.getPeriodUnit();
                if (punit == SKGRecurrentOperationObject::MONTH) {
                    occuType = QStringLiteral("32");
                } else if (punit == SKGRecurrentOperationObject::WEEK) {
                    occuType = '4';
                } else if (punit == SKGRecurrentOperationObject::DAY) {
                    occuType = '2';
                } else {
                    occuType = QStringLiteral("16384");
                }

                scheduled_tx.setAttribute(QStringLiteral("occurenceMultiplier"), SKGServices::intToString(occu));
                scheduled_tx.setAttribute(QStringLiteral("occurence"), occuType); // krazy:exclude=spelling
                scheduled_tx.setAttribute(QStringLiteral("weekendOption"), QStringLiteral("0"));
                scheduled_tx.setAttribute(QStringLiteral("paymentType"), QStringLiteral("1"));
                QChar type = '1';
                SKGOperationObject op2;
                if (op.isTransfer(op2)) {
                    type = '4';
                } else if (op.getCurrentAmount() > 0) {
                    type = '2';
                }
                scheduled_tx.setAttribute(QStringLiteral("type"), type);
                scheduled_tx.setAttribute(QStringLiteral("fixed"), QStringLiteral("1"));

                QString endDate;
                if (obj.hasTimeLimit()) {
                    QDate firstDate = obj.getDate();

                    // We must compute the date
                    int p = occu * (obj.getTimeLimit() - 1);
                    if (punit == SKGRecurrentOperationObject::DAY) {
                        firstDate = firstDate.addDays(p);
                    } else if (punit == SKGRecurrentOperationObject::MONTH) {
                        firstDate = firstDate.addMonths(p);
                    } else if (punit == SKGRecurrentOperationObject::YEAR) {
                        firstDate = firstDate.addYears(p);
                    }

                    endDate = firstDate.toString(QStringLiteral("yyyy-MM-dd"));
                }
                scheduled_tx.setAttribute(QStringLiteral("endDate"), endDate);

                err = exportOperation(op, doc, scheduled_tx);
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginKmy::exportTransactions(QDomDocument &doc, QDomElement &root, const QString &stdUnit)
{
    SKGError err;
    QDomElement transactions = doc.createElement(QStringLiteral("TRANSACTIONS"));
    root.appendChild(transactions);

    SKGObjectBase::SKGListSKGObjectBase objects;
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_operation"), QStringLiteral("t_template='N' ORDER BY d_date DESC"), objects))
    int nb = objects.count();
    transactions.setAttribute(QStringLiteral("count"), SKGServices::intToString(nb));
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export transactions"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGOperationObject op(objects.at(i));
            err = exportOperation(op, doc, transactions);
            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }

    // <KEYVALUEPAIRS>
    QDomElement keyvaluepairs = doc.createElement(QStringLiteral("KEYVALUEPAIRS"));
    root.appendChild(keyvaluepairs);
    {
        QDomElement pair = doc.createElement(QStringLiteral("PAIR"));
        keyvaluepairs.appendChild(pair);
        pair.setAttribute(QStringLiteral("key"), QStringLiteral("kmm-baseCurrency"));
        pair.setAttribute(QStringLiteral("value"), stdUnit);
    }
    return err;
}

SKGError SKGImportPluginKmy::exportPayees(QDomDocument &doc, QDomElement &root)
{
    SKGError err;
    QDomElement payees = doc.createElement(QStringLiteral("PAYEES"));
    root.appendChild(payees);

    SKGObjectBase::SKGListSKGObjectBase listPayees;
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_payee"), QString(), listPayees))
    int nb = listPayees.count();
    payees.setAttribute(QStringLiteral("count"), SKGServices::intToString(nb));
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export payees"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGPayeeObject payeeObject(listPayees.at(i));
            QDomElement payee = doc.createElement(QStringLiteral("PAYEE"));
            payees.appendChild(payee);

            payee.setAttribute(QStringLiteral("matchingenabled"), QStringLiteral("0"));
            payee.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(payeeObject));
            payee.setAttribute(QStringLiteral("name"), payeeObject.getName());
            payee.setAttribute(QStringLiteral("email"), QString());
            payee.setAttribute(QStringLiteral("reference"), QString());

            QDomElement address = doc.createElement(QStringLiteral("ADDRESS"));
            payee.appendChild(address);

            address.setAttribute(QStringLiteral("street"), payeeObject.getAddress());
            address.setAttribute(QStringLiteral("postcode"), QString());
            address.setAttribute(QStringLiteral("zip"), QString());
            address.setAttribute(QStringLiteral("city"), QString());
            address.setAttribute(QStringLiteral("telephone"), QString());
            address.setAttribute(QStringLiteral("state"), QString());

            m_mapIdPayee[SKGServices::intToString(i + 1)] = payeeObject;

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginKmy::exportInstitutions(QDomDocument &doc, QDomElement &root)
{
    SKGError err;
    QDomElement institutions = doc.createElement(QStringLiteral("INSTITUTIONS"));
    root.appendChild(institutions);
    SKGObjectBase::SKGListSKGObjectBase objects;
    IFOKDO(err,
           m_importer->getDocument()->getObjects(QStringLiteral("v_bank"),
                                                 QStringLiteral("EXISTS(SELECT 1 FROM account WHERE account.rd_bank_id=v_bank.id)"),
                                                 objects))
    int nb = objects.count();
    institutions.setAttribute(QStringLiteral("count"), SKGServices::intToString(nb));
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export banks"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGBankObject obj(objects.at(i));
            QDomElement institution = doc.createElement(QStringLiteral("INSTITUTION"));
            institutions.appendChild(institution);

            institution.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(obj));
            institution.setAttribute(QStringLiteral("name"), obj.getName());
            institution.setAttribute(QStringLiteral("sortcode"), obj.getNumber());
            institution.setAttribute(QStringLiteral("manager"), QString());

            QDomElement address = doc.createElement(QStringLiteral("ADDRESS"));
            institution.appendChild(address);

            address.setAttribute(QStringLiteral("street"), QString());
            address.setAttribute(QStringLiteral("zip"), QString());
            address.setAttribute(QStringLiteral("city"), QString());
            address.setAttribute(QStringLiteral("telephone"), QString());

            QDomElement accountids = doc.createElement(QStringLiteral("ACCOUNTIDS"));
            institution.appendChild(accountids);

            SKGObjectBase::SKGListSKGObjectBase accounts;
            err = obj.getAccounts(accounts);
            int nb2 = accounts.count();
            for (int j = 0; !err && j < nb2; ++j) {
                QDomElement accountid = doc.createElement(QStringLiteral("ACCOUNTID"));
                accountids.appendChild(accountid);

                accountid.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(accounts.at(j)));
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }

    return err;
}

SKGError SKGImportPluginKmy::exportCategories(QDomDocument &doc,
                                              QDomElement &accounts,
                                              const QString &stdUnit,
                                              QDomElement &accountIncome,
                                              QDomElement &accountExpense,
                                              int nbAccount)
{
    // The v_category_display are retrieved to improve performances of getCurrentAmount
    SKGError err;
    SKGObjectBase::SKGListSKGObjectBase objects;
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_category_display"), QString(), objects))
    accounts.setAttribute(QStringLiteral("count"), SKGServices::intToString(5 + nbAccount + objects.count()));
    int nb = objects.count();
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export categories"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGCategoryObject obj(objects.at(i));
            QDomElement account = doc.createElement(QStringLiteral("ACCOUNT"));
            accounts.appendChild(account);

            account.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(obj));
            account.setAttribute(QStringLiteral("name"), obj.getName());
            account.setAttribute(QStringLiteral("number"), QString());
            account.setAttribute(QStringLiteral("type"), obj.getCurrentAmount() < 0 ? QStringLiteral("13") : QStringLiteral("12"));

            account.setAttribute(QStringLiteral("institution"), QString());

            SKGCategoryObject parentCat;
            obj.getParentCategory(parentCat);

            QString parentId = (parentCat.getID() != 0 ? getKmyUniqueIdentifier(parentCat)
                                                       : (obj.getCurrentAmount() < 0 ? QStringLiteral("AStd::Expense") : QStringLiteral("AStd::Income")));
            if (parentId == QStringLiteral("AStd::Expense")) {
                QDomElement subaccount = doc.createElement(QStringLiteral("SUBACCOUNT"));
                accountExpense.appendChild(subaccount);
                subaccount.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(obj));
            } else if (parentId == QStringLiteral("AStd::Income")) {
                QDomElement subaccount = doc.createElement(QStringLiteral("SUBACCOUNT"));
                accountIncome.appendChild(subaccount);
                subaccount.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(obj));
            }

            account.setAttribute(QStringLiteral("parentaccount"), parentId);
            account.setAttribute(QStringLiteral("lastmodified"), QString());
            account.setAttribute(QStringLiteral("lastreconciled"), QString());
            account.setAttribute(QStringLiteral("opened"), QString());
            account.setAttribute(QStringLiteral("currency"), stdUnit);
            account.setAttribute(QStringLiteral("description"), QString());

            QDomElement subaccounts = doc.createElement(QStringLiteral("SUBACCOUNTS"));
            account.appendChild(subaccounts);

            SKGObjectBase::SKGListSKGObjectBase categories;
            IFOKDO(err, obj.getCategories(categories))
            int nb2 = categories.count();
            for (int j = 0; !err && j < nb2; ++j) {
                QDomElement subaccount = doc.createElement(QStringLiteral("SUBACCOUNT"));
                subaccounts.appendChild(subaccount);

                subaccount.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(categories.at(j)));
            }
            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }
    return err;
}

SKGError SKGImportPluginKmy::exportAccounts(QDomDocument &doc,
                                            QDomElement &root,
                                            const QString &stdUnit,
                                            QDomElement &accounts,
                                            QDomElement &accountIncome,
                                            QDomElement &accountExpense,
                                            int &nbAccounts)
{
    SKGError err;
    accounts = doc.createElement(QStringLiteral("ACCOUNTS"));
    root.appendChild(accounts);

    QDomElement accountAsset;
    {
        QDomElement account = doc.createElement(QStringLiteral("ACCOUNT"));
        accounts.appendChild(account);

        account.setAttribute(QStringLiteral("id"), QStringLiteral("AStd::Equity"));
        account.setAttribute(QStringLiteral("name"), QStringLiteral("Equity"));
        account.setAttribute(QStringLiteral("number"), QString());
        account.setAttribute(QStringLiteral("type"), QStringLiteral("16"));
        account.setAttribute(QStringLiteral("institution"), QString());
        account.setAttribute(QStringLiteral("parentaccount"), QString());
        account.setAttribute(QStringLiteral("lastmodified"), QString());
        account.setAttribute(QStringLiteral("lastreconciled"), QString());
        account.setAttribute(QStringLiteral("opened"), QString());
        account.setAttribute(QStringLiteral("currency"), stdUnit);
        account.setAttribute(QStringLiteral("description"), QString());

        QDomElement subaccounts = doc.createElement(QStringLiteral("SUBACCOUNTS"));
        account.appendChild(subaccounts);
    }

    {
        QDomElement account = doc.createElement(QStringLiteral("ACCOUNT"));
        accounts.appendChild(account);

        account.setAttribute(QStringLiteral("id"), QStringLiteral("AStd::Asset"));
        account.setAttribute(QStringLiteral("name"), QStringLiteral("Asset"));
        account.setAttribute(QStringLiteral("number"), QString());
        account.setAttribute(QStringLiteral("type"), QStringLiteral("9"));
        account.setAttribute(QStringLiteral("institution"), QString());
        account.setAttribute(QStringLiteral("parentaccount"), QString());
        account.setAttribute(QStringLiteral("lastmodified"), QString());
        account.setAttribute(QStringLiteral("lastreconciled"), QString());
        account.setAttribute(QStringLiteral("opened"), QString());
        account.setAttribute(QStringLiteral("currency"), stdUnit);
        account.setAttribute(QStringLiteral("description"), QString());

        QDomElement subaccounts = doc.createElement(QStringLiteral("SUBACCOUNTS"));
        account.appendChild(subaccounts);
        accountAsset = subaccounts;
    }

    {
        QDomElement account = doc.createElement(QStringLiteral("ACCOUNT"));
        accounts.appendChild(account);

        account.setAttribute(QStringLiteral("id"), QStringLiteral("AStd::Liability"));
        account.setAttribute(QStringLiteral("name"), QStringLiteral("Liability"));
        account.setAttribute(QStringLiteral("number"), QString());
        account.setAttribute(QStringLiteral("type"), QStringLiteral("10"));
        account.setAttribute(QStringLiteral("institution"), QString());
        account.setAttribute(QStringLiteral("parentaccount"), QString());
        account.setAttribute(QStringLiteral("lastmodified"), QString());
        account.setAttribute(QStringLiteral("lastreconciled"), QString());
        account.setAttribute(QStringLiteral("opened"), QString());
        account.setAttribute(QStringLiteral("currency"), stdUnit);
        account.setAttribute(QStringLiteral("description"), QString());

        QDomElement subaccounts = doc.createElement(QStringLiteral("SUBACCOUNTS"));
        account.appendChild(subaccounts);
    }

    {
        QDomElement account = doc.createElement(QStringLiteral("ACCOUNT"));
        accounts.appendChild(account);

        account.setAttribute(QStringLiteral("id"), QStringLiteral("AStd::Income"));
        account.setAttribute(QStringLiteral("name"), QStringLiteral("Income"));
        account.setAttribute(QStringLiteral("number"), QString());
        account.setAttribute(QStringLiteral("type"), QStringLiteral("12"));
        account.setAttribute(QStringLiteral("institution"), QString());
        account.setAttribute(QStringLiteral("parentaccount"), QString());
        account.setAttribute(QStringLiteral("lastmodified"), QString());
        account.setAttribute(QStringLiteral("lastreconciled"), QString());
        account.setAttribute(QStringLiteral("opened"), QString());
        account.setAttribute(QStringLiteral("currency"), stdUnit);
        account.setAttribute(QStringLiteral("description"), QString());

        QDomElement subaccounts = doc.createElement(QStringLiteral("SUBACCOUNTS"));
        account.appendChild(subaccounts);
        accountIncome = subaccounts;
    }

    {
        QDomElement account = doc.createElement(QStringLiteral("ACCOUNT"));
        accounts.appendChild(account);

        account.setAttribute(QStringLiteral("id"), QStringLiteral("AStd::Expense"));
        account.setAttribute(QStringLiteral("name"), QStringLiteral("Expense"));
        account.setAttribute(QStringLiteral("number"), QString());
        account.setAttribute(QStringLiteral("type"), QStringLiteral("13"));
        account.setAttribute(QStringLiteral("institution"), QString());
        account.setAttribute(QStringLiteral("parentaccount"), QString());
        account.setAttribute(QStringLiteral("lastmodified"), QString());
        account.setAttribute(QStringLiteral("lastreconciled"), QString());
        account.setAttribute(QStringLiteral("opened"), QString());
        account.setAttribute(QStringLiteral("currency"), stdUnit);
        account.setAttribute(QStringLiteral("description"), QString());

        QDomElement subaccounts = doc.createElement(QStringLiteral("SUBACCOUNTS"));
        account.appendChild(subaccounts);
        accountExpense = subaccounts;
    }

    SKGObjectBase::SKGListSKGObjectBase objects;
    IFOKDO(err, m_importer->getDocument()->getObjects(QStringLiteral("v_account"), QString(), objects))
    int nb = objects.count();
    IFOK(err)
    {
        err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export accounts"), nb);
        for (int i = 0; !err && i < nb; ++i) {
            SKGAccountObject obj(objects.at(i));
            QDomElement account = doc.createElement(QStringLiteral("ACCOUNT"));
            accounts.appendChild(account);

            account.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(obj));
            account.setAttribute(QStringLiteral("name"), obj.getName());
            account.setAttribute(QStringLiteral("number"), obj.getNumber());
            account.setAttribute(QStringLiteral("type"),
                                 obj.getType() == SKGAccountObject::CREDITCARD
                                     ? QStringLiteral("4")
                                     : (obj.getType() == SKGAccountObject::INVESTMENT
                                            ? QStringLiteral("7")
                                            : (obj.getType() == SKGAccountObject::ASSETS
                                                   ? QStringLiteral("9")
                                                   : (obj.getType() == SKGAccountObject::WALLET
                                                          ? QStringLiteral("3")
                                                          : (obj.getType() == SKGAccountObject::LOAN ? QStringLiteral("10") : QStringLiteral("1"))))));

            SKGBankObject bank;
            err = obj.getBank(bank);
            account.setAttribute(QStringLiteral("institution"), getKmyUniqueIdentifier(bank));

            account.setAttribute(QStringLiteral("parentaccount"), QStringLiteral("AStd::Asset"));
            account.setAttribute(QStringLiteral("lastmodified"), QString());
            account.setAttribute(QStringLiteral("lastreconciled"), QString());
            account.setAttribute(QStringLiteral("opened"), QString());
            SKGUnitObject unit;
            obj.getUnit(unit);
            QString unitS = SKGUnitObject::getInternationalCode(unit.getName());
            if (unitS.isEmpty()) {
                unitS = QStringLiteral("EUR");
            }
            account.setAttribute(QStringLiteral("currency"), unitS);
            account.setAttribute(QStringLiteral("description"), QString());

            // Bookmarked account
            QDomElement keyvaluepairs = doc.createElement(QStringLiteral("KEYVALUEPAIRS"));
            account.appendChild(keyvaluepairs);
            if (obj.isBookmarked()) {
                QDomElement pair = doc.createElement(QStringLiteral("PAIR"));
                keyvaluepairs.appendChild(pair);
                pair.setAttribute(QStringLiteral("key"), QStringLiteral("PreferredAccount"));
                pair.setAttribute(QStringLiteral("value"), QStringLiteral("Yes"));
            }
            // Closed account
            if (obj.isClosed()) {
                QDomElement pair = doc.createElement(QStringLiteral("PAIR"));
                keyvaluepairs.appendChild(pair);
                pair.setAttribute(QStringLiteral("key"), QStringLiteral("mm-closed"));
                pair.setAttribute(QStringLiteral("value"), QStringLiteral("yes"));
            }

            // Maximum and minimum limits
            if (obj.isMaxLimitAmountEnabled()) {
                QDomElement pair = doc.createElement(QStringLiteral("PAIR"));
                keyvaluepairs.appendChild(pair);
                pair.setAttribute(QStringLiteral("key"), QStringLiteral("maxCreditAbsolute"));
                pair.setAttribute(QStringLiteral("value"), kmyValue(-obj.getMaxLimitAmount()));
            }
            if (obj.isMinLimitAmountEnabled()) {
                QDomElement pair = doc.createElement(QStringLiteral("PAIR"));
                keyvaluepairs.appendChild(pair);
                pair.setAttribute(QStringLiteral("key"), QStringLiteral("minBalanceAbsolute"));
                pair.setAttribute(QStringLiteral("value"), kmyValue(obj.getMinLimitAmount()));
            }

            // Add it in asset
            QDomElement subaccount = doc.createElement(QStringLiteral("SUBACCOUNT"));
            accountAsset.appendChild(subaccount);
            subaccount.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(obj));

            IFOKDO(err, m_importer->getDocument()->stepForward(i + 1))
        }

        SKGENDTRANSACTION(m_importer->getDocument(), err)
    }

    nbAccounts = nb;
    return err;
}

SKGError SKGImportPluginKmy::exportFile()
{
    // Initialisation
    m_opTreated.clear();

    if (m_importer->getDocument() == nullptr) {
        return SKGError(ERR_ABORT, i18nc("Error message", "Invalid parameters"));
    }
    SKGError err;
    SKGTRACEINFUNCRC(2, err)

    // Open file
    KCompressionDevice file(m_importer->getLocalFileName(false), KCompressionDevice::GZip);
    if (!file.open(QIODevice::WriteOnly)) {
        err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("Error message", "Save file '%1' failed", m_importer->getFileName().toDisplayString()));
    } else {
        QDomDocument doc(QStringLiteral("KMYMONEY-FILE"));
        QDomComment comment = doc.createComment(QStringLiteral("Generated by libskgbankmodeler"));
        doc.appendChild(comment);

        QDomElement root = doc.createElement(QStringLiteral("KMYMONEY-FILE"));
        doc.appendChild(root);
        {
            err = m_importer->getDocument()->beginTransaction("#INTERNAL#" % i18nc("Export step", "Export %1 file", "KMY"), 8);
            IFOK(err)
            {
                // Step 1-<FILEINFO>
                IFOKDO(err, exportHeader(doc, root))
                IFOKDO(err, m_importer->getDocument()->stepForward(1))

                // // Step 2-<INSTITUTIONS>
                IFOKDO(err, exportInstitutions(doc, root))
                IFOKDO(err, m_importer->getDocument()->stepForward(2))

                // Step 3-<PAYEES>
                IFOKDO(err, exportPayees(doc, root))
                IFOKDO(err, m_importer->getDocument()->stepForward(3))

                // Step 4-<ACCOUNTS>
                // Std accounts
                QString stdUnit = SKGUnitObject::getInternationalCode(m_importer->getDocument()->getPrimaryUnit().Name);
                if (stdUnit.isEmpty()) {
                    stdUnit = QStringLiteral("EUR");
                }

                QDomElement accountIncome;
                QDomElement accountExpense;
                QDomElement accounts;

                int nbAccounts = 0;
                IFOKDO(err, exportAccounts(doc, root, stdUnit, accounts, accountIncome, accountExpense, nbAccounts))
                IFOKDO(err, m_importer->getDocument()->stepForward(4))

                // Step 5
                IFOKDO(err, exportCategories(doc, accounts, stdUnit, accountIncome, accountExpense, nbAccounts))
                IFOKDO(err, m_importer->getDocument()->stepForward(5))

                // Step 6-<TRANSACTIONS>
                IFOKDO(err, exportTransactions(doc, root, stdUnit))

                // Step 6-<SCHEDULES>
                IFOKDO(err, exportSchedules(doc, root))
                IFOKDO(err, m_importer->getDocument()->stepForward(6))

                // Step 7-<BUDGETS>
                IFOKDO(err, exportBudgets(doc, root))
                IFOKDO(err, m_importer->getDocument()->stepForward(7))

                // Step 8-<SECURITIES> and <CURRENCIES>
                IFOKDO(err, exportSecurities(doc, root, stdUnit))

                // Save file
                IFOK(err)
                {
                    file.write(doc.toString().toUtf8());
                }
                IFOKDO(err, m_importer->getDocument()->stepForward(8))
            }

            SKGENDTRANSACTION(m_importer->getDocument(), err)
        }

        file.close();
    }

    // Clean
    m_opTreated.clear();

    return err;
}

SKGError SKGImportPluginKmy::exportOperation(const SKGOperationObject &iOperation, QDomDocument &iDoc, QDomElement &iTransaction)
{
    SKGError err;
    SKGTRACEINFUNCRC(2, err)
    if (!m_opTreated.contains(getKmyUniqueIdentifier(iOperation))) {
        QDomElement transaction = iDoc.createElement(QStringLiteral("TRANSACTION"));
        iTransaction.appendChild(transaction);

        SKGUnitObject unit;
        iOperation.getUnit(unit);

        QString date = iOperation.getAttribute(QStringLiteral("d_date"));
        transaction.setAttribute(QStringLiteral("id"), getKmyUniqueIdentifier(iOperation));
        transaction.setAttribute(QStringLiteral("entrydate"), date);
        transaction.setAttribute(QStringLiteral("postdate"), date);
        transaction.setAttribute(QStringLiteral("memo"), iOperation.getComment());
        transaction.setAttribute(QStringLiteral("commodity"), SKGUnitObject::getInternationalCode(unit.getName()));

        QString reconcileflag = (iOperation.getStatus() == SKGOperationObject::MARKED
                                     ? QStringLiteral("1")
                                     : (iOperation.getStatus() == SKGOperationObject::CHECKED ? QStringLiteral("2") : QStringLiteral("0")));

        SKGAccountObject act;
        IFOKDO(err, iOperation.getParentAccount(act))

        QDomElement splits = iDoc.createElement(QStringLiteral("SPLITS"));
        transaction.appendChild(splits);

        QDomElement split = iDoc.createElement(QStringLiteral("SPLIT"));
        splits.appendChild(split);

        SKGPayeeObject payeeObject;
        iOperation.getPayee(payeeObject);
        QString payeeId = (payeeObject.getID() != 0 ? getKmyUniqueIdentifier(payeeObject) : QString());

        int indexSubOp = 1;

        // Split for account
        split.setAttribute(QStringLiteral("payee"), payeeId);
        split.setAttribute(QStringLiteral("reconciledate"), QString());
        split.setAttribute(QStringLiteral("id"), "S" % SKGServices::intToString(indexSubOp++).rightJustified(4, '0'));
        double val2 = SKGServices::stringToDouble(iOperation.getAttribute(QStringLiteral("f_QUANTITY")));
        split.setAttribute(QStringLiteral("shares"), SKGImportPluginKmy::kmyValue(val2));
        split.setAttribute(QStringLiteral("action"), QString());
        split.setAttribute(QStringLiteral("bankid"), QString());
        split.setAttribute(QStringLiteral("number"), iOperation.getNumber());
        split.setAttribute(QStringLiteral("reconcileflag"), reconcileflag);
        split.setAttribute(QStringLiteral("memo"), iOperation.getComment());
        QString originalAmount = iOperation.getProperty(QStringLiteral("SKG_OP_ORIGINAL_AMOUNT"));
        if (!originalAmount.isEmpty() && val2 != 0) {
            val2 = qAbs(SKGServices::stringToDouble(originalAmount)) * (val2 / qAbs(val2));
        }
        split.setAttribute(QStringLiteral("value"), SKGImportPluginKmy::kmyValue(val2));
        split.setAttribute(QStringLiteral("account"), getKmyUniqueIdentifier(act));

        SKGOperationObject obj2;
        if (!err && iOperation.isTransfer(obj2)) {
            // It is a Transfer
            QString reconcileflag2 = (obj2.getStatus() == SKGOperationObject::MARKED
                                          ? QStringLiteral("1")
                                          : (obj2.getStatus() == SKGOperationObject::CHECKED ? QStringLiteral("2") : QStringLiteral("0")));

            SKGAccountObject act2;
            IFOKDO(err, obj2.getParentAccount(act2))

            QDomElement split2 = iDoc.createElement(QStringLiteral("SPLIT"));
            splits.appendChild(split2);

            // Split for account
            val2 = -val2;
            split2.setAttribute(QStringLiteral("payee"), payeeId);
            split2.setAttribute(QStringLiteral("reconciledate"), QString());
            split2.setAttribute(QStringLiteral("id"), "S" % SKGServices::intToString(indexSubOp++).rightJustified(4, '0'));
            split2.setAttribute(QStringLiteral("shares"),
                                SKGImportPluginKmy::kmyValue(SKGServices::stringToDouble(obj2.getAttribute(QStringLiteral("f_QUANTITY")))));
            split2.setAttribute(QStringLiteral("action"), QString());
            split2.setAttribute(QStringLiteral("bankid"), QString());
            split2.setAttribute(QStringLiteral("number"), obj2.getNumber());
            split2.setAttribute(QStringLiteral("reconcileflag"), reconcileflag2);
            split2.setAttribute(QStringLiteral("memo"), obj2.getComment());
            split2.setAttribute(QStringLiteral("value"), SKGImportPluginKmy::kmyValue(val2));
            split2.setAttribute(QStringLiteral("account"), getKmyUniqueIdentifier(act2));

            m_opTreated.insert(getKmyUniqueIdentifier(obj2));
        } else {
            SKGObjectBase::SKGListSKGObjectBase subops;
            IFOKDO(err, iOperation.getSubOperations(subops))
            int nb2 = subops.count();
            for (int j = 0; !err && j < nb2; ++j) {
                QDomElement split2 = iDoc.createElement(QStringLiteral("SPLIT"));
                splits.appendChild(split2);

                SKGSubOperationObject subop(subops.at(j));
                SKGCategoryObject cat;
                subop.getCategory(cat);
                split2.setAttribute(QStringLiteral("payee"), payeeId);
                split2.setAttribute(QStringLiteral("reconciledate"), QString());
                split2.setAttribute(QStringLiteral("id"), "S" % SKGServices::intToString(indexSubOp++).rightJustified(4, '0'));
                QString shape3 = SKGImportPluginKmy::kmyValue(-subop.getQuantity());
                split2.setAttribute(QStringLiteral("shares"), shape3);
                split2.setAttribute(QStringLiteral("action"), QString());
                split2.setAttribute(QStringLiteral("bankid"), QString());
                split2.setAttribute(QStringLiteral("number"), iOperation.getNumber());
                split2.setAttribute(QStringLiteral("reconcileflag"), reconcileflag);
                split2.setAttribute(QStringLiteral("memo"), subop.getComment());
                split2.setAttribute(QStringLiteral("value"), shape3);
                split2.setAttribute(QStringLiteral("account"),
                                    date == QStringLiteral("0000-00-00") ? QStringLiteral("AStd::Equity")
                                                                         : (cat.getID() != 0 ? getKmyUniqueIdentifier(cat) : QString()));
            }
        }

        m_opTreated.insert(getKmyUniqueIdentifier(iOperation));
    }
    return err;
}

QString SKGImportPluginKmy::kmyValue(double iValue)
{
    QString output;
    for (int i = 0; output.isEmpty() && i < 11; ++i) {
        QString d = SKGServices::doubleToString(pow(10, i) * iValue);
        if (d.indexOf(u'.') == -1) {
            output = d % u'/' % SKGServices::intToString(qPow(10, i));
        }
    }
    return output;
}

double SKGImportPluginKmy::toKmyValue(const QString &iString)
{
    double output = 0;
    QStringList vals = SKGServices::splitCSVLine(iString, '/');
    if (vals.count() == 1) {
        output = SKGServices::stringToDouble(vals.at(0));
    } else if (vals.count() == 2) {
        const auto div = SKGServices::stringToDouble(vals.at(1));
        if (div != 0)
            output = SKGServices::stringToDouble(vals.at(0)) / div;
    }
    return output;
}

QString SKGImportPluginKmy::getKmyUniqueIdentifier(const SKGObjectBase &iObject)
{
    QString id;
    if (iObject.getID() != 0) {
        QString table = iObject.getRealTable();
        if (table == QStringLiteral("operation") || table == QStringLiteral("suboperation")) {
            // T000000000000003623
            id = u'T' % SKGServices::intToString(iObject.getID()).rightJustified(18, '0');
        } else if (table == QStringLiteral("payee")) {
            // P000030
            id = u'P' % SKGServices::intToString(iObject.getID()).rightJustified(6, '0');
        } else {
            id = iObject.getUniqueID();
        }
    }
    return id;
}

QString SKGImportPluginKmy::getMimeTypeFilter() const
{
    return "*.kmy|" % i18nc("A file format", "KMyMoney document");
}

#include <skgimportpluginkmy.moc>
