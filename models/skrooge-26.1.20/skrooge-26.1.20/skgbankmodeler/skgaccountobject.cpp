/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGAccountObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgaccountobject.h"

#include <klocalizedstring.h>

#include "skgbankobject.h"
#include "skgdocumentbank.h"
#include "skginterestobject.h"
#include "skgoperationobject.h"
#include "skgpayeeobject.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"
#include "skgunitobject.h"

int factorial(int n)
{
    return (n == 1 || n == 0) ? 1 : factorial(n - 1) * n;
}

SKGAccountObject::SKGAccountObject()
    : SKGAccountObject(nullptr, 0)
{
}

SKGAccountObject::SKGAccountObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("v_account"), iID)
{
}

SKGAccountObject::~SKGAccountObject() = default;

SKGAccountObject::SKGAccountObject(const SKGAccountObject &iObject) = default;

SKGAccountObject::SKGAccountObject(const SKGNamedObject &iObject)
    : SKGNamedObject(iObject.getDocument(), QStringLiteral("v_account"), iObject.getID())
{
    if (iObject.getRealTable() == QStringLiteral("account")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_account"), iObject.getID());
    }
}

SKGAccountObject::SKGAccountObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("account")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_account"), iObject.getID());
    }
}

SKGAccountObject &SKGAccountObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGAccountObject &SKGAccountObject::operator=(const SKGAccountObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGAccountObject::setInitialBalance(double iBalance, const SKGUnitObject &iUnit)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (getDocument() != nullptr) {
        // Delete previous initial balance for this account
        err = getDocument()->executeSqliteOrder("DELETE FROM operation  WHERE d_date='0000-00-00' AND rd_account_id=" % SKGServices::intToString(getID()));

        // Creation of new initial balance
        IFOK(err)
        {
            SKGOperationObject initialBalanceOp;
            err = addOperation(initialBalanceOp, true);
            IFOKDO(err, initialBalanceOp.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
            IFOKDO(err, initialBalanceOp.setUnit(iUnit))
            IFOKDO(err, initialBalanceOp.setStatus(SKGOperationObject::CHECKED))
            IFOKDO(err, initialBalanceOp.save())

            SKGSubOperationObject initialBalanceSubOp;
            IFOKDO(err, initialBalanceOp.addSubOperation(initialBalanceSubOp))
            IFOKDO(err, initialBalanceSubOp.setAttribute(QStringLiteral("d_date"), QStringLiteral("0000-00-00")))
            IFOKDO(err, initialBalanceSubOp.setQuantity(iBalance))
            IFOKDO(err, initialBalanceSubOp.save())
        }
    }
    return err;
}

SKGError SKGAccountObject::getInitialBalance(double &oBalance, SKGUnitObject &oUnit)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Initialisation
    oBalance = 0;
    oUnit = SKGUnitObject();
    QString unitName = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit().Symbol;

    // Get initial balance
    SKGStringListList listTmp;
    err = getDocument()->executeSelectSqliteOrder("SELECT f_QUANTITY, t_UNIT FROM  v_operation_tmp1  WHERE d_date='0000-00-00' AND rd_account_id="
                                                      % SKGServices::intToString(getID()),
                                                  listTmp);
    if (!err && listTmp.count() > 1) {
        oBalance = SKGServices::stringToDouble(listTmp.at(1).at(0));
        unitName = listTmp.at(1).at(1);

        oUnit = SKGUnitObject(getDocument());
        err = oUnit.setSymbol(unitName);
        IFOKDO(err, oUnit.load())
    }
    return err;
}

SKGError SKGAccountObject::setBank(const SKGBankObject &iBank)
{
    return setAttribute(QStringLiteral("rd_bank_id"), SKGServices::intToString(iBank.getID()));
}

SKGError SKGAccountObject::getBank(SKGBankObject &oBank) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_bank"), "id=" % getAttribute(QStringLiteral("rd_bank_id")), oBank);
    return err;
}

SKGError SKGAccountObject::setLinkedAccount(const SKGAccountObject &iAccount)
{
    return setAttribute(QStringLiteral("r_account_id"), SKGServices::intToString(iAccount.getID()));
}

SKGError SKGAccountObject::getLinkedAccount(SKGAccountObject &oAccount) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_account"), "id=" % getAttribute(QStringLiteral("r_account_id")), oAccount);
    return err;
}

SKGError SKGAccountObject::getLinkedByAccounts(SKGListSKGObjectBase &oAccounts) const
{
    SKGError err;
    if (getDocument() != nullptr) {
        err = getDocument()->getObjects(QStringLiteral("v_account"), "r_account_id=" % SKGServices::intToString(getID()), oAccounts);
    }
    return err;
}

SKGError SKGAccountObject::setNumber(const QString &iNumber)
{
    return setAttribute(QStringLiteral("t_number"), iNumber);
}

QString SKGAccountObject::getNumber() const
{
    return getAttribute(QStringLiteral("t_number"));
}

SKGError SKGAccountObject::setComment(const QString &iComment)
{
    return setAttribute(QStringLiteral("t_comment"), iComment);
}

QString SKGAccountObject::getComment() const
{
    return getAttribute(QStringLiteral("t_comment"));
}

SKGError SKGAccountObject::setAgencyNumber(const QString &iNumber)
{
    return setAttribute(QStringLiteral("t_agency_number"), iNumber);
}

QString SKGAccountObject::getAgencyNumber() const
{
    return getAttribute(QStringLiteral("t_agency_number"));
}

SKGError SKGAccountObject::setAgencyAddress(const QString &iAddress)
{
    return setAttribute(QStringLiteral("t_agency_address"), iAddress);
}

QString SKGAccountObject::getAgencyAddress() const
{
    return getAttribute(QStringLiteral("t_agency_address"));
}

SKGError SKGAccountObject::addOperation(SKGOperationObject &oOperation, bool iForce)
{
    SKGError err;
    if (getID() == 0) {
        err = SKGError(
            ERR_FAIL,
            i18nc("Error message", "%1 failed because linked object is not yet saved in the database.", QStringLiteral("SKGAccountObject::addOperation")));
    } else {
        oOperation = SKGOperationObject(getDocument());
        err = oOperation.setParentAccount(*this, iForce);
    }
    return err;
}

int SKGAccountObject::getNbOperation() const
{
    int nb = 0;
    if (getDocument() != nullptr) {
        getDocument()->getNbObjects(QStringLiteral("operation"), "rd_account_id=" % SKGServices::intToString(getID()), nb);
    }
    return nb;
}

SKGError SKGAccountObject::getOperations(SKGListSKGObjectBase &oOperations) const
{
    SKGError err;
    if (getDocument() != nullptr) {
        err = getDocument()->getObjects(QStringLiteral("v_operation"), "rd_account_id=" % SKGServices::intToString(getID()), oOperations);
    }
    return err;
}

double SKGAccountObject::getCurrentAmount() const
{
    return SKGServices::stringToDouble(getAttributeFromView(QStringLiteral("v_account_amount"), QStringLiteral("f_CURRENTAMOUNT")));
}

double SKGAccountObject::getAmount(QDate iDate, bool iOnlyCurrencies) const
{
    SKGTRACEINFUNC(10)
    double output = 0;
    if (getDocument() != nullptr) {
        // Search result in cache
        QString ids = SKGServices::intToString(getID());
        QString dates = SKGServices::dateToSqlString(iDate);
        QString key = "getamount-" % ids % u'-' % dates;
        QString val = getDocument()->getCachedValue(key);
        if (val.isEmpty()) {
            SKGStringListList listTmp;
            SKGError err = getDocument()->executeSelectSqliteOrder(
                "SELECT TOTAL(f_QUANTITY), rc_unit_id FROM v_operation_tmp1  WHERE "
                "d_date<='"
                    % dates % "' AND t_template='N' AND rd_account_id=" % ids % (iOnlyCurrencies ? " AND t_TYPEUNIT IN ('1', '2', 'C')" : "")
                    % " GROUP BY rc_unit_id",
                listTmp);
            int nb = listTmp.count();
            for (int i = 1; !err && i < nb; ++i) {
                QString quantity = listTmp.at(i).at(0);
                QString unitid = listTmp.at(i).at(1);

                double coef = 1;
                QString val2 = getDocument()->getCachedValue("unitvalue-" % unitid);
                if (!val2.isEmpty()) {
                    // Yes
                    coef = SKGServices::stringToDouble(val2);
                } else {
                    // No
                    SKGUnitObject unit(getDocument(), SKGServices::stringToInt(unitid));
                    if (unit.getType() != SKGUnitObject::PRIMARY) {
                        coef = unit.getAmount(iDate);
                    }
                }

                output += coef * SKGServices::stringToDouble(quantity);
            }
            getDocument()->addValueInCache(key, SKGServices::doubleToString(output));
        } else {
            output = SKGServices::stringToDouble(val);
        }
    }
    return output;
}

SKGError SKGAccountObject::setType(SKGAccountObject::AccountType iType)
{
    return setAttribute(
        QStringLiteral("t_type"),
        (iType == CURRENT
             ? QStringLiteral("C")
             : (iType == CREDITCARD
                    ? QStringLiteral("D")
                    : (iType == ASSETS
                           ? QStringLiteral("A")
                           : (iType == INVESTMENT ? QStringLiteral("I")
                                                  : (iType == WALLET ? QStringLiteral("W")
                                                                     : (iType == PENSION ? QStringLiteral("P")
                                                                                         : (iType == LOAN ? QStringLiteral("L")
                                                                                                          : (iType == SAVING ? QStringLiteral("S")
                                                                                                                             : QStringLiteral("O"))))))))));
}

SKGAccountObject::AccountType SKGAccountObject::getType() const
{
    QString typeString = getAttribute(QStringLiteral("t_type"));
    return (typeString == QStringLiteral("C")
                ? CURRENT
                : (typeString == QStringLiteral("D")
                       ? CREDITCARD
                       : (typeString == QStringLiteral("A")
                              ? ASSETS
                              : (typeString == QStringLiteral("I")
                                     ? INVESTMENT
                                     : (typeString == QStringLiteral("W")
                                            ? WALLET
                                            : (typeString == QStringLiteral("P")
                                                   ? PENSION
                                                   : (typeString == QStringLiteral("L") ? LOAN : (typeString == QStringLiteral("S") ? SAVING : OTHER))))))));
}

SKGError SKGAccountObject::setClosed(bool iClosed)
{
    return setAttribute(QStringLiteral("t_close"), iClosed ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGAccountObject::isClosed() const
{
    return (getAttribute(QStringLiteral("t_close")) == QStringLiteral("Y"));
}

SKGError SKGAccountObject::bookmark(bool iBookmark)
{
    return setAttribute(QStringLiteral("t_bookmarked"), iBookmark ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGAccountObject::isBookmarked() const
{
    return (getAttribute(QStringLiteral("t_bookmarked")) == QStringLiteral("Y"));
}

SKGError SKGAccountObject::maxLimitAmountEnabled(bool iEnabled)
{
    return setAttribute(QStringLiteral("t_maxamount_enabled"), iEnabled ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGAccountObject::isMaxLimitAmountEnabled() const
{
    return (getAttribute(QStringLiteral("t_maxamount_enabled")) == QStringLiteral("Y"));
}

SKGError SKGAccountObject::setMaxLimitAmount(double iAmount)
{
    SKGError err = setAttribute(QStringLiteral("f_maxamount"), SKGServices::doubleToString(iAmount));
    if (!err && getMinLimitAmount() > iAmount) {
        err = setMinLimitAmount(iAmount);
    }
    return err;
}

double SKGAccountObject::getMaxLimitAmount() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_maxamount")));
}

SKGError SKGAccountObject::minLimitAmountEnabled(bool iEnabled)
{
    return setAttribute(QStringLiteral("t_minamount_enabled"), iEnabled ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGAccountObject::isMinLimitAmountEnabled() const
{
    return (getAttribute(QStringLiteral("t_minamount_enabled")) == QStringLiteral("Y"));
}

SKGError SKGAccountObject::setMinLimitAmount(double iAmount)
{
    SKGError err = setAttribute(QStringLiteral("f_minamount"), SKGServices::doubleToString(iAmount));
    if (!err && getMaxLimitAmount() < iAmount) {
        err = setMaxLimitAmount(iAmount);
    }
    return err;
}

double SKGAccountObject::getMinLimitAmount() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_minamount")));
}

SKGError SKGAccountObject::setReconciliationDate(QDate iDate)
{
    return setAttribute(QStringLiteral("d_reconciliationdate"), SKGServices::dateToSqlString(iDate));
}

QDate SKGAccountObject::getReconciliationDate() const
{
    return SKGServices::stringToDate(getAttribute(QStringLiteral("d_reconciliationdate")));
}

SKGError SKGAccountObject::setReconciliationBalance(double iAmount)
{
    return setAttribute(QStringLiteral("f_reconciliationbalance"), SKGServices::doubleToString(iAmount));
}

double SKGAccountObject::getReconciliationBalance() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_reconciliationbalance")));
}

SKGError SKGAccountObject::getUnit(SKGUnitObject &oUnit) const
{
    // Get initial amount
    SKGStringListList listTmp;
    SKGError err = getDocument()->executeSelectSqliteOrder("SELECT t_UNIT FROM  v_suboperation_consolidated  WHERE d_date='0000-00-00' AND rd_account_id="
                                                               % SKGServices::intToString(getID()),
                                                           listTmp);
    IFOK(err)
    {
        // Is initial amount existing ?
        if (listTmp.count() > 1) {
            // Yes ==> then the amount is the amount of the initial value
            oUnit = SKGUnitObject(getDocument());
            err = oUnit.setSymbol(listTmp.at(1).at(0));
            IFOKDO(err, oUnit.load())
        } else {
            // No ==> we get the preferred unit
            SKGObjectBase::SKGListSKGObjectBase units;
            err = getDocument()->getObjects(QStringLiteral("v_unit"),
                                            "t_type IN ('1', '2', 'C') AND EXISTS(SELECT 1 FROM operation WHERE rc_unit_id=v_unit.id AND rd_account_id="
                                                % SKGServices::intToString(getID()) % ") ORDER BY t_type",
                                            units);
            int nb = units.count();
            if (nb != 0) {
                oUnit = units.at(0);
            }
        }
    }
    return err;
}

SKGError SKGAccountObject::addInterest(SKGInterestObject &oInterest)
{
    SKGError err;
    if (getID() == 0) {
        err = SKGError(
            ERR_FAIL,
            i18nc("Error message", "%1 failed because linked object is not yet saved in the database.", QStringLiteral("SKGAccountObject::addInterest")));
    } else {
        oInterest = SKGInterestObject(qobject_cast<SKGDocumentBank *>(getDocument()));
        err = oInterest.setAccount(*this);
    }
    return err;
}

SKGError SKGAccountObject::getInterests(SKGListSKGObjectBase &oInterestList) const
{
    SKGError err = getDocument()->getObjects(QStringLiteral("v_interest"), "rd_account_id=" % SKGServices::intToString(getID()), oInterestList);
    return err;
}

SKGError SKGAccountObject::getInterest(QDate iDate, SKGInterestObject &oInterest) const
{
    QString ids = SKGServices::intToString(getID());
    QString dates = SKGServices::dateToSqlString(iDate);
    SKGError err = SKGObjectBase::getDocument()->getObject(
        QStringLiteral("v_interest"),
        "rd_account_id=" % ids % " AND d_date<='" % dates % "' AND  ABS(strftime('%s','" % dates % "')-strftime('%s',d_date))=(SELECT MIN(ABS(strftime('%s','"
            % dates % "')-strftime('%s',u2.d_date))) FROM interest u2 WHERE u2.rd_account_id=" % ids % " AND u2.d_date<='" % dates % "')",
        oInterest);

    // If not found then get first
    IFKO(err)
    err = SKGObjectBase::getDocument()->getObject(QStringLiteral("v_interest"),
                                                  "rd_account_id=" % SKGServices::intToString(getID())
                                                      % " AND d_date=(SELECT MIN(d_date) FROM interest WHERE rd_account_id=" % SKGServices::intToString(getID())
                                                      % u')',
                                                  oInterest);
    return err;
}

SKGError SKGAccountObject::getInterestItems(SKGAccountObject::SKGInterestItemList &oInterestList, double &oInterests, int iYear) const
{
    oInterestList.clear();
    SKGError err;

    // Initial date
    int y = iYear;
    if (y == 0) {
        y = QDate::currentDate().year();
    }
    QDate initialDate = QDate(y, 1, 1);
    QDate lastDate = QDate(y, 12, 31);

    oInterests = 0;
    bool computationNeeded = false;

    // Add transactions
    SKGObjectBase::SKGListSKGObjectBase items;
    err = getDocument()->getObjects(QStringLiteral("v_operation"), "rd_account_id=" % SKGServices::intToString(getID()) %
                                    " AND t_template='N' AND t_TYPEUNIT IN ('1', '2', 'C')"
                                    " AND d_date>='" % SKGServices::dateToSqlString(initialDate) % "' "
                                    " AND d_date<='" % SKGServices::dateToSqlString(lastDate) % "' ORDER BY d_date", items);
    int nb = items.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGOperationObject ob(items.at(i));

        SKGInterestItem itemI;
        itemI.object = ob;
        itemI.date = ob.getDate();
        itemI.valueDate = itemI.date;
        itemI.rate = 0;
        itemI.base = 0;
        itemI.coef = 0;
        itemI.annualInterest = 0;
        itemI.accruedInterest = 0;
        itemI.amount = ob.getCurrentAmount();

        oInterestList.push_back(itemI);
    }

    // Add interest
    IFOK(err)
    {
        err = getDocument()->getObjects(QStringLiteral("v_interest"), "rd_account_id=" % SKGServices::intToString(getID()) %
                                        " AND d_date>='" % SKGServices::dateToSqlString(initialDate) % "' "
                                        " AND d_date<='" % SKGServices::dateToSqlString(lastDate) % "' ORDER BY d_date", items);

        int pos = 0;
        int nb2 = items.count();
        for (int i = 0; !err && i < nb2; ++i) {
            SKGInterestObject ob(items.at(i));

            SKGInterestItem itemI;
            itemI.object = ob;
            itemI.date = ob.getDate();
            itemI.valueDate = itemI.date;
            itemI.rate = ob.getRate();
            itemI.base = SKGServices::stringToInt(ob.getAttribute(QStringLiteral("t_base")));
            itemI.coef = 0;
            itemI.annualInterest = 0;
            itemI.accruedInterest = 0;
            itemI.amount = 0;

            int nb3 = oInterestList.count();
            for (int j = pos; !err && j < nb3; ++j) {
                if (itemI.date <= oInterestList.at(j).date) {
                    break;
                }
                ++pos;
            }

            oInterestList.insert(pos, itemI);
            computationNeeded = true;
        }
    }

    // Get first interest
    IFOK(err)
    {
        SKGInterestObject firstInterest;
        if (getInterest(initialDate, firstInterest).isSucceeded()) {
            if (firstInterest.getDate() < initialDate) {
                SKGInterestItem itemI;
                itemI.object = firstInterest;
                itemI.date = initialDate;
                itemI.valueDate = initialDate;
                itemI.rate = firstInterest.getRate();
                itemI.base = 0;
                itemI.coef = 0;
                itemI.annualInterest = 0;
                itemI.accruedInterest = 0;
                itemI.amount = 0;

                oInterestList.insert(0, itemI);
                computationNeeded = true;
            }
        }
    }

    // Launch computation
    IFOK(err)
    {
        if (computationNeeded) {
            err = computeInterestItems(oInterestList, oInterests, y);
        } else {
            // Drop temporary table
            IFOKDO(err, getDocument()->executeSqliteOrder(QStringLiteral("DROP TABLE IF EXISTS interest_result")))
            // Create fake table
            IFOKDO(err, getDocument()->executeSqliteOrder(QStringLiteral("CREATE TEMP TABLE interest_result(a)")))
        }
    }
    return err;
}

SKGError SKGAccountObject::computeInterestItems(SKGAccountObject::SKGInterestItemList &ioInterestList, double &oInterests, int iYear) const
{
    SKGError err;

    // Sum annual interest
    oInterests = 0;

    // Initial date
    int y = iYear;
    if (y == 0) {
        y = QDate::currentDate().year();
    }
    QDate initialDate = QDate(y, 1, 1);

    // Default interest item
    SKGInterestItem currentInterest;
    currentInterest.date = initialDate;
    currentInterest.valueDate = currentInterest.date;
    currentInterest.rate = 0;
    currentInterest.coef = 0;
    currentInterest.annualInterest = 0;
    currentInterest.accruedInterest = 0;

    int nb = ioInterestList.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGInterestItem tmp = ioInterestList.at(i);
        SKGObjectBase object = tmp.object;
        if (object.getRealTable() == QStringLiteral("operation")) {
            // Get transactions
            SKGOperationObject op(object);

            // Get current amount
            tmp.amount = op.getCurrentAmount();

            // Get value date computation mode
            SKGInterestObject::ValueDateMode valueMode = SKGInterestObject::FIFTEEN;
            SKGInterestObject::InterestMode baseMode = SKGInterestObject::FIFTEEN24;
            if (currentInterest.object.getRealTable() == QStringLiteral("interest")) {
                SKGInterestObject interestObj(currentInterest.object);
                valueMode = (tmp.amount >= 0 ? interestObj.getIncomeValueDateMode() : interestObj.getExpenditueValueDateMode());
                baseMode = interestObj.getInterestComputationMode();

                tmp.rate = interestObj.getRate();
            }

            // Compute value date
            if (object.getRealTable() == QStringLiteral("operation")) {
                if (valueMode == SKGInterestObject::FIFTEEN) {
                    if (tmp.amount >= 0) {
                        if (tmp.date.day() <= 15) {
                            tmp.valueDate = tmp.date.addDays(16 - tmp.date.day());
                        } else {
                            tmp.valueDate = tmp.date.addMonths(1).addDays(1 - tmp.date.day());
                        }
                    } else {
                        if (tmp.date.day() <= 15) {
                            tmp.valueDate = tmp.date.addDays(1 - tmp.date.day());
                        } else {
                            tmp.valueDate = tmp.date.addDays(16 - tmp.date.day());
                        }
                    }
                } else {
                    tmp.valueDate = tmp.date.addDays(tmp.amount >= 0 ? (static_cast<int>(valueMode)) - 1 : -(static_cast<int>(valueMode)) + 1);
                }
            }

            // Compute coef
            if (baseMode == SKGInterestObject::DAYS365) {
                QDate last(tmp.date.year(), 12, 31);
                tmp.coef = tmp.valueDate.daysTo(last) + 1;
                tmp.coef /= 365;
            } else if (baseMode == SKGInterestObject::DAYS360) {
                QDate last(tmp.date.year(), 12, 31);
                tmp.coef = 360 * (last.year() - tmp.valueDate.year()) + 30 * (last.month() - tmp.valueDate.month()) + (last.day() - tmp.valueDate.day());
                tmp.coef /= 360;
            } else {
                tmp.coef = 2 * (12 - tmp.valueDate.month()) + (tmp.valueDate.day() <= 15 ? 2 : 1);
                tmp.coef /= 24;
            }
            if (tmp.valueDate.year() != iYear) {
                tmp.coef = 0;
            }

            // Compute annual interest
            tmp.annualInterest = tmp.amount * tmp.coef * tmp.rate / 100;

        } else if (object.getRealTable() == QStringLiteral("interest")) {
            // Compute coef
            if (tmp.base == 365) {
                QDate last(tmp.date.year(), 12, 31);
                tmp.coef = tmp.valueDate.daysTo(last) + 1;
                tmp.coef /= 365;
            } else if (tmp.base == 360) {
                QDate last(tmp.date.year(), 12, 31);
                tmp.coef = 360 * (last.year() - tmp.valueDate.year()) + 30 * (last.month() - tmp.valueDate.month()) + (last.day() - tmp.valueDate.day());
                tmp.coef /= 360;
            } else {
                tmp.coef = 2 * (12 - tmp.valueDate.month()) + (tmp.valueDate.day() <= 15 ? 2 : 1);
                tmp.coef /= 24;
            }
            if (tmp.valueDate.year() != iYear) {
                tmp.coef = 0;
            }

            // Compute annual interest
            // BUG 329568: We must ignore transactions of the day
            tmp.amount = getAmount(tmp.valueDate.addDays(-1), true);
            tmp.annualInterest = tmp.amount * tmp.coef * (tmp.rate - currentInterest.rate) / 100;

            currentInterest = tmp;
        }

        // Compute sum
        oInterests += tmp.annualInterest;

        // Compute accrued interest
        tmp.accruedInterest = oInterests - getAmount(tmp.date, true) * tmp.coef * tmp.rate / 100;

        ioInterestList[i] = std::move(tmp);
    }

    // Create temporary table
    IFOK(err)
    {
        QStringList sqlOrders;
        sqlOrders << QStringLiteral("DROP TABLE IF EXISTS interest_result")
                  << QStringLiteral(
                         "CREATE TEMP TABLE interest_result("
                         "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                         "d_date DATE NOT NULL,"
                         "d_valuedate DATE NOT NULL,"
                         "t_comment TEXT NOT NULL DEFAULT '',"
                         "f_currentamount FLOAT NOT NULL DEFAULT 0,"
                         "f_coef FLOAT NOT NULL DEFAULT 0,"
                         "f_rate FLOAT NOT NULL DEFAULT 0,"
                         "f_annual_interest FLOAT NOT NULL DEFAULT 0,"
                         "f_accrued_interest FLOAT NOT NULL DEFAULT 0"
                         ")");
        err = getDocument()->executeSqliteOrders(sqlOrders);

        // Fill table
        int nb2 = ioInterestList.count();
        for (int i = 0; !err && i < nb2; ++i) {
            SKGInterestItem interest = ioInterestList.at(i);
            SKGObjectBase object = interest.object;
            QString sqlinsert =
                "INSERT INTO interest_result (d_date,d_valuedate,t_comment,f_currentamount,f_coef,f_rate,f_annual_interest,f_accrued_interest) "
                " VALUES ('"
                % SKGServices::dateToSqlString(interest.date) % "','" % SKGServices::dateToSqlString(interest.valueDate) % "','"
                % SKGServices::stringToSqlString(object.getRealTable() == QStringLiteral("operation")
                                                     ? i18nc("Noun", "Relative to transaction '%1'", SKGOperationObject(object).getDisplayName())
                                                     : i18nc("Noun", "Rate change"))
                % "'," % SKGServices::doubleToString(interest.amount) % u',' % SKGServices::doubleToString(interest.coef) % u','
                % SKGServices::doubleToString(interest.rate) % u',' % SKGServices::doubleToString(interest.annualInterest) % u','
                % SKGServices::doubleToString(interest.accruedInterest) % ")";

            err = getDocument()->executeSqliteOrder(sqlinsert);
        }
    }
    return err;
}

SKGError SKGAccountObject::transferDeferredOperations(const SKGAccountObject &iTargetAccount, QDate iDate)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    //
    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    if (doc != nullptr) {
        // Get marked transactions
        SKGObjectBase::SKGListSKGObjectBase transactions;
        IFOKDO(
            err,
            getDocument()->getObjects(QStringLiteral("v_operation"), "rd_account_id=" % SKGServices::intToString(getID()) % " AND t_status='P'", transactions))
        int nb = transactions.count();
        if (nb != 0) {
            SKGOperationObject mergedOperations;
            SKGOperationObject balancedOperations;
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(transactions.at(i));

                // Create the balance operation
                SKGOperationObject opdup;
                IFOKDO(err, op.duplicate(opdup, iDate))

                SKGListSKGObjectBase subops;
                IFOKDO(err, opdup.getSubOperations(subops))
                int nbsupops = subops.count();
                for (int j = 0; !err && j < nbsupops; ++j) {
                    SKGSubOperationObject subop(subops.at(j));
                    IFOKDO(err, subop.setDate(op.getDate()))
                    IFOKDO(err, subop.setQuantity(-subop.getQuantity()))
                    IFOKDO(err, subop.save())
                }

                if (i == 0) {
                    mergedOperations = opdup;
                } else {
                    IFOKDO(err, mergedOperations.mergeSuboperations(opdup))
                }

                // Create the duplicate in target account
                SKGOperationObject opduptarget;
                IFOKDO(err, op.duplicate(opduptarget))
                IFOKDO(err, opduptarget.setDate(op.getDate()))
                IFOKDO(err, opduptarget.setParentAccount(iTargetAccount))
                IFOKDO(err, opduptarget.setImported(op.isImported()))
                IFOKDO(err, opduptarget.setImportID(op.getImportID()))
                IFOKDO(err, opduptarget.setGroupOperation(mergedOperations))
                IFOKDO(err, opduptarget.setStatus(SKGOperationObject::MARKED))
                IFOKDO(err, opduptarget.save())
                IFOKDO(err, mergedOperations.load()) // To reload the modif done by the setGroupOperation

                // Check the operation
                IFOKDO(err, op.setStatus(SKGOperationObject::CHECKED))
                IFOKDO(err, op.save())
            }

            // Check the balance operation
            IFOKDO(err, mergedOperations.setPayee(SKGPayeeObject()))
            IFOKDO(err, mergedOperations.setStatus(SKGOperationObject::CHECKED))
            IFOKDO(err, mergedOperations.save())
        }
    }

    return err;
}

QVector<QVector<SKGOperationObject>> SKGAccountObject::getPossibleReconciliations(double iTargetBalance, bool iSearchAllPossibleReconciliation) const
{
    SKGTRACEINFUNC(5)
    QVector<QVector<SKGOperationObject>> output;
    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    if (doc != nullptr) {
        // Get unit
        SKGServices::SKGUnitInfo unit1 = doc->getPrimaryUnit();
        SKGUnitObject unitAccount;
        if (getUnit(unitAccount).isSucceeded()) {
            if (!unitAccount.getSymbol().isEmpty()) {
                unit1.Symbol = unitAccount.getSymbol();
                unit1.Value = SKGServices::stringToDouble(unitAccount.getAttribute(QStringLiteral("f_CURRENTAMOUNT")));
            }
        }
        SKGTRACEL(5) << "iTargetBalance=" << doc->formatMoney(iTargetBalance, unit1, false) << Qt::endl;

        // Get balance of checked transactions
        QString balanceString;
        getDocument()->executeSingleSelectSqliteOrder(
            "SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation s WHERE s.t_status='Y' AND s.t_template='N' AND s.rd_account_id="
                % SKGServices::intToString(getID()),
            balanceString);
        double balance = SKGServices::stringToDouble(balanceString);
        SKGTRACEL(5) << "balance=" << doc->formatMoney(balance, unit1, false) << Qt::endl;

        QString zero = doc->formatMoney(0, unit1, false);
        QString negativezero = doc->formatMoney(-EPSILON, unit1, false);
        QString sdiff = doc->formatMoney(balance - iTargetBalance * unit1.Value, unit1, false);
        if (sdiff == zero || sdiff == negativezero) {
            // This is an empty soluce
            output.push_back(QVector<SKGOperationObject>());
            SKGTRACEL(5) << "empty solution found !!!" << Qt::endl;
        } else {
            // Get all imported transaction
            SKGObjectBase::SKGListSKGObjectBase transactions;
            getDocument()->getObjects(QStringLiteral("v_operation"),
                                      "rd_account_id=" % SKGServices::intToString(getID())
                                          % " AND t_status!='Y' AND t_template='N' AND t_imported IN ('Y','P') ORDER BY d_date, id",
                                      transactions);
            int nb = transactions.count();

            if (!iSearchAllPossibleReconciliation) {
                // Check if all transactions are a solution
                double amount = 0.0;
                QVector<SKGOperationObject> list;
                list.reserve(nb);
                for (int i = 0; i < nb; ++i) {
                    SKGOperationObject op(transactions.at(i));
                    amount += op.getCurrentAmount();
                    list.push_back(op);
                }
                QString sdiff = doc->formatMoney(amount + balance - iTargetBalance * unit1.Value, unit1, false);
                if (sdiff == zero || sdiff == negativezero) {
                    SKGTRACEL(5) << "all transactions are a solution !!!" << Qt::endl;
                    output.push_back(list);
                    return output;
                }
            }

            // Search
            int nbmax = 500;
            SKGTRACEL(5) << "Nb transactions:" << nb << Qt::endl;
            if (nb > nbmax) {
                SKGTRACEL(5) << "Too many transactions (" << nb << ") ==> Reducing the size of the computation" << Qt::endl;
                for (int i = 0; i < nb - nbmax; ++i) {
                    SKGOperationObject op(transactions.at(0));
                    auto amount = op.getCurrentAmount();
                    balance += amount;
                    transactions.removeFirst();
                }
            }
            output = getPossibleReconciliations(transactions, balance, iTargetBalance, unit1, iSearchAllPossibleReconciliation);
        }
    }
    return output;
}

QVector<QVector<SKGOperationObject>> SKGAccountObject::getPossibleReconciliations(const SKGObjectBase::SKGListSKGObjectBase &iOperations,
                                                                                  double iBalance,
                                                                                  double iTargetBalance,
                                                                                  const SKGServices::SKGUnitInfo &iUnit,
                                                                                  bool iSearchAllPossibleReconciliation) const
{
    SKGTRACEINFUNC(5)
    QVector<QVector<SKGOperationObject>> output;
    output.reserve(5);
    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    if (doc != nullptr) {
        SKGTRACEL(5) << "iTargetBalance=" << doc->formatMoney(iTargetBalance, iUnit, false) << Qt::endl;

        // Comparison
        QString zero = doc->formatMoney(0, iUnit, false);
        QString negativezero = doc->formatMoney(-EPSILON, iUnit, false);

        // Check transactions list
        int nb = iOperations.count();
        if (nb > 0) {
            // Get all transactions of the next date
            QVector<SKGOperationObject> nextOperations;
            nextOperations.reserve(iOperations.count());
            QString date = iOperations.at(0).getAttribute(QStringLiteral("d_date"));
            for (int i = 0; i < nb; ++i) {
                SKGOperationObject op(iOperations.at(i));
                if (op.getAttribute(QStringLiteral("d_date")) == date) {
                    nextOperations.push_back(op);
                } else {
                    break;
                }
            }

            // Get all combination of transactions
            int nbNext = nextOperations.count();
            SKGTRACEL(5) << date << ":" << nbNext << " transactions found" << Qt::endl;
            std::vector<int> v(nbNext);
            for (int i = 0; i < nbNext; ++i) {
                v[i] = i;
            }

            double nextBalance = iBalance;
            int index = 0;
            if (nbNext > 7) {
                SKGTRACEL(5) << "Too many combination: " << factorial(nbNext) << " ==> limited to 5040" << Qt::endl;
                nbNext = 7;
            }
            nb = factorial(nbNext);
            bool stopTests = false;
            QVector<SKGOperationObject> combi;
            QVector<double> combiAmount;
            combi.reserve(nbNext);
            combiAmount.reserve(nbNext);
            do {
                // Build the next combination
                combi.resize(0);
                combiAmount.resize(0);
                double sumOperationPositives = 0.0;
                double sumOperationsNegatives = 0.0;
                for (int i = 0; i < nbNext; ++i) {
                    const SKGOperationObject &op = nextOperations.at(v[i]);
                    combi.push_back(op);
                    auto amount = op.getCurrentAmount();
                    combiAmount.push_back(amount);
                    if (Q_LIKELY(amount < 0)) {
                        sumOperationsNegatives += amount;
                    } else if (amount > 0) {
                        sumOperationPositives += amount;
                    }
                }

                // Test the combination
                double diff = iBalance - iTargetBalance * iUnit.Value;
                double previousDiff = diff;
                SKGTRACEL(5) << "Check combination " << (index + 1) << "/" << nb << ": Diff=" << doc->formatMoney(diff, iUnit, false) << Qt::endl;

                // Try to find an immediate soluce
                int nbop = combi.count();
                for (int j = 0; j < nbop; ++j) {
                    auto amount = combiAmount.at(j);
                    diff += amount;
                    if (Q_UNLIKELY(index == 0)) {
                        nextBalance += amount;
                    }
                    QString sdiff = doc->formatMoney(diff, iUnit, false);
                    SKGTRACEL(5) << (j + 1) << "/" << nbop << ": Amount=" << amount << " / New diff=" << sdiff << Qt::endl;
                    if (sdiff == zero || sdiff == negativezero) {
                        // This is a soluce
                        auto s = combi.mid(0, j + 1);
                        if (output.contains(s)) {
                            SKGTRACEL(5) << "found but already existing !!!" << Qt::endl;
                        } else {
                            output.push_back(s);
                            SKGTRACEL(5) << "found !!!" << Qt::endl;
                            if (j == nbop - 1 || iSearchAllPossibleReconciliation) {
                                // No need to test all combinations
                                SKGTRACEL(5) << "No need to test all combinations" << Qt::endl;
                                stopTests = true;
                            }
                        }
                    }
                }

                // Check if tests of all combinations can be cancelled
                if ((previousDiff > 0 && previousDiff + sumOperationsNegatives > 0) || (previousDiff < 0 && previousDiff + sumOperationPositives < 0)) {
                    SKGTRACEL(5) << "No need to test all combinations due to signs of transactions and diffs" << Qt::endl;
                    stopTests = true;
                }

                ++index;
            } while (index < nb && std::next_permutation(v.begin(), v.end()) && !stopTests);

            // Try to find next solutions
            auto reconciliations = getPossibleReconciliations(iOperations.mid(nbNext), nextBalance, iTargetBalance, iUnit, iSearchAllPossibleReconciliation);
            int nbReconciliations = reconciliations.count();
            output.reserve(nbReconciliations + 5);
            for (int i = 0; i < nbReconciliations; ++i) {
                QVector<SKGOperationObject> output2 = nextOperations;
                output2 = output2 << reconciliations.at(i);
                output.push_back(output2);
            }
        }
    }

    SKGTRACEL(5) << output.count() << " soluces found" << Qt::endl;
    if (!output.isEmpty()) {
        SKGTRACEL(5) << "Size of the first soluce: " << output.at(0).count() << Qt::endl;
    }
    return output;
}

SKGError SKGAccountObject::autoReconcile(double iBalance)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)

    // Soluces
    auto soluces = getPossibleReconciliations(iBalance);
    int nbSoluces = soluces.count();
    if (nbSoluces > 0) {
        if (nbSoluces > 1) {
            err = getDocument()->sendMessage(i18nc("An information message", "More than one solution is possible for this auto reconciliation."));
        }

        // Choose the longest solution
        QVector<SKGOperationObject> soluce;
        int length = 0;
        for (int i = 0; i < nbSoluces; ++i) {
            const auto &s = soluces.at(i);
            int l = s.count();
            if (l > length) {
                soluce = s;
                length = l;
            }
        }

        // Check all
        SKGTRACEL(5) << length << " transactions marked" << Qt::endl;
        for (int i = 0; i < length; ++i) {
            SKGOperationObject op(soluce.at(i));
            err = op.setStatus(SKGOperationObject::MARKED);
            IFOKDO(err, op.save(true, false))
        }
    } else {
        err = SKGError(
            ERR_FAIL,
            i18nc("Error message", "Can not find the imported transactions for obtaining the expected final balance"),
            QString("skg://skrooge_operation_plugin/?title_icon=quickopen&title="
                    % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions of account \"%1\" used for auto reconciliation", getDisplayName()))
                    % "&operationWhereClause="
                    % SKGServices::encodeForUrl("rd_account_id=" + SKGServices::intToString(getID())
                                                + " AND t_template='N' AND ((t_status='N' AND t_imported IN ('Y','P')) OR t_status='Y')")));
    }

    return err;
}

SKGError SKGAccountObject::merge(const SKGAccountObject &iAccount, bool iMergeInitalBalance)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Get initial balances
    double balance1 = 0.0;
    SKGUnitObject unit1;
    err = getInitialBalance(balance1, unit1);

    double balance2 = 0.0;
    SKGUnitObject unit2;
    if (iMergeInitalBalance) {
        IFOKDO(err, const_cast<SKGAccountObject *>(&iAccount)->getInitialBalance(balance2, unit2))
    }

    // Transfer transactions
    SKGObjectBase::SKGListSKGObjectBase ops;
    IFOKDO(err, iAccount.getOperations(ops))
    int nb = ops.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGOperationObject op(ops.at(i));
        err = op.setParentAccount(*this);
        IFOKDO(err, op.save(true, false))
    }

    // Set initial balance
    SKGUnitObject unit = unit1;
    if (!unit1.exist()) {
        unit = unit2;
    }
    if (unit.exist() && balance2 != 0.0) {
        double balance = balance1 + SKGUnitObject::convert(balance2, unit2, unit);
        IFOKDO(err, setInitialBalance(balance, unit))
    }
    // Remove account
    IFOKDO(err, iAccount.remove(false))
    return err;
}
