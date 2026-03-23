/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGRecurrentOperationObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgrecurrentoperationobject.h"

#include <klocalizedstring.h>

#include "skgdocumentbank.h"
#include "skgoperationobject.h"
#include "skgservices.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"

SKGRecurrentOperationObject::SKGRecurrentOperationObject()
    : SKGRecurrentOperationObject(nullptr)
{
}

SKGRecurrentOperationObject::SKGRecurrentOperationObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_recurrentoperation"), iID)
{
}

SKGRecurrentOperationObject::~SKGRecurrentOperationObject() = default;

SKGRecurrentOperationObject::SKGRecurrentOperationObject(const SKGRecurrentOperationObject &iObject) = default;

SKGRecurrentOperationObject::SKGRecurrentOperationObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("recurrentoperation")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_recurrentoperation"), iObject.getID());
    }
}

SKGRecurrentOperationObject &SKGRecurrentOperationObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGRecurrentOperationObject &SKGRecurrentOperationObject::operator=(const SKGRecurrentOperationObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGRecurrentOperationObject::getParentOperation(SKGOperationObject &oOperation) const
{
    SKGObjectBase objTmp;
    SKGError err = getDocument()->getObject(QStringLiteral("v_operation"), "id=" % getAttribute(QStringLiteral("rd_operation_id")), objTmp);
    oOperation = objTmp;
    return err;
}

SKGError SKGRecurrentOperationObject::setParentOperation(const SKGOperationObject &iOperation)
{
    return setAttribute(QStringLiteral("rd_operation_id"), SKGServices::intToString(iOperation.getID()));
}

SKGError SKGRecurrentOperationObject::setTemplate(bool iTemplate)
{
    SKGError err;

    if (iTemplate == isTemplate()) {
        err.addError(ERR_UNEXPECTED, i18nc("Error message", "Trying to set the same template status \"%1\" to a recurrent operation", iTemplate));
        return err;
    }

    SKGOperationObject parentOp;
    err = getParentOperation(parentOp);
    if (iTemplate) {
        // Convert to template
        SKGOperationObject operationObjOrig = parentOp;
        IFOKDO(err, operationObjOrig.duplicate(parentOp, operationObjOrig.getDate(), true))

        IFOKDO(err, setParentOperation(parentOp))
        IFOKDO(err, save())

        IFOKDO(err, operationObjOrig.setAttribute(QStringLiteral("r_recurrentoperation_id"), SKGServices::intToString(getID())))
        IFOKDO(err, operationObjOrig.save())
    } else {
        // Convert to non-template
        SKGObjectBase::SKGListSKGObjectBase transactions;
        IFOKDO(err, getRecurredOperations(transactions))
        IFOK(err)
        {
            if (!transactions.isEmpty()) {
                SKGOperationObject lastObj(transactions.last());
                IFOKDO(err, setParentOperation(lastObj))
                IFOKDO(err, save())
                IFOKDO(err, lastObj.setAttribute(QStringLiteral("r_recurrentoperation_id"), QString()))
                IFOKDO(err, lastObj.save())

                SKGObjectBase::SKGListSKGObjectBase goupops;
                IFOKDO(err, parentOp.getGroupedOperations(goupops))
                IFOK(err)
                {
                    int nbgoupops = goupops.count();
                    for (int i = 0; !err && i < nbgoupops; ++i) {
                        SKGOperationObject groupop(goupops.at(i));
                        if (groupop != parentOp) {
                            IFOKDO(err, groupop.remove(true))
                        }
                    }
                }

                IFOKDO(err, parentOp.remove(true))
            } else {
                err.addError(ERR_FAIL, i18nc("Error message", "Need at least one transaction to convert a schedule to a non-template one"));
            }
        }
    }

    return err;
}

bool SKGRecurrentOperationObject::isTemplate() const
{
    SKGOperationObject op;
    SKGError err = getParentOperation(op);
    IFOK(err) return op.isTemplate();
    return false;
}

SKGError SKGRecurrentOperationObject::setPeriodIncrement(int iIncrement)
{
    return setAttribute(QStringLiteral("i_period_increment"), SKGServices::intToString(iIncrement));
}

int SKGRecurrentOperationObject::getPeriodIncrement() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_period_increment")));
}

SKGRecurrentOperationObject::PeriodUnit SKGRecurrentOperationObject::getPeriodUnit() const
{
    QString t_period_unit = getAttribute(QStringLiteral("t_period_unit"));
    if (t_period_unit == QStringLiteral("D")) {
        return DAY;
    }
    if (t_period_unit == QStringLiteral("W")) {
        return WEEK;
    }
    if (t_period_unit == QStringLiteral("M")) {
        return MONTH;
    }
    return YEAR;
}

SKGError SKGRecurrentOperationObject::setPeriodUnit(SKGRecurrentOperationObject::PeriodUnit iPeriod)
{
    return setAttribute(
        QStringLiteral("t_period_unit"),
        (iPeriod == DAY ? QStringLiteral("D") : (iPeriod == WEEK ? QStringLiteral("W") : (iPeriod == MONTH ? QStringLiteral("M") : QStringLiteral("Y")))));
}

SKGRecurrentOperationObject::Adapter SKGRecurrentOperationObject::getAdapter() const
{
    QString t_adapter = getAttribute(QStringLiteral("t_adapter"));
    if (t_adapter == QStringLiteral("P1")) {
        return PREVIOUS_MONDAY;
    }
    if (t_adapter == QStringLiteral("P2")) {
        return PREVIOUS_TUESDAY;
    }
    if (t_adapter == QStringLiteral("P3")) {
        return PREVIOUS_WEDNESDAY;
    }
    if (t_adapter == QStringLiteral("P4")) {
        return PREVIOUS_THURSDAY;
    }
    if (t_adapter == QStringLiteral("P5")) {
        return PREVIOUS_FRIDAY;
    }
    if (t_adapter == QStringLiteral("P6")) {
        return PREVIOUS_SATURDAY;
    }
    if (t_adapter == QStringLiteral("P7")) {
        return PREVIOUS_SUNDAY;
    }
    if (t_adapter == QStringLiteral("C1")) {
        return CLOSEST_MONDAY;
    }
    if (t_adapter == QStringLiteral("C2")) {
        return CLOSEST_TUESDAY;
    }
    if (t_adapter == QStringLiteral("C3")) {
        return CLOSEST_WEDNESDAY;
    }
    if (t_adapter == QStringLiteral("C4")) {
        return CLOSEST_THURSDAY;
    }
    if (t_adapter == QStringLiteral("C5")) {
        return CLOSEST_FRIDAY;
    }
    if (t_adapter == QStringLiteral("C6")) {
        return CLOSEST_SATURDAY;
    }
    if (t_adapter == QStringLiteral("C7")) {
        return CLOSEST_SUNDAY;
    }
    if (t_adapter == QStringLiteral("N1")) {
        return NEXT_MONDAY;
    }
    if (t_adapter == QStringLiteral("N2")) {
        return NEXT_TUESDAY;
    }
    if (t_adapter == QStringLiteral("N3")) {
        return NEXT_WEDNESDAY;
    }
    if (t_adapter == QStringLiteral("N4")) {
        return NEXT_THURSDAY;
    }
    if (t_adapter == QStringLiteral("N5")) {
        return NEXT_FRIDAY;
    }
    if (t_adapter == QStringLiteral("N6")) {
        return NEXT_SATURDAY;
    }
    if (t_adapter == QStringLiteral("N7")) {
        return NEXT_SUNDAY;
    }
    if (t_adapter == QStringLiteral("PO")) {
        return PREVIOUS_OPEN_DAY;
    }
    if (t_adapter == QStringLiteral("CO")) {
        return CLOSEST_OPEN_DAY;
    }
    if (t_adapter == QStringLiteral("NO")) {
        return NEXT_OPEN_DAY;
    }
    if (t_adapter == QStringLiteral("1MO")) {
        return FIRST_OPEN_DAY_OF_MONTH;
    }
    return NONE;
}

SKGError SKGRecurrentOperationObject::setAdapter(SKGRecurrentOperationObject::Adapter iAdapter)
{
    auto t_adapter = QStringLiteral("NONE");
    if (iAdapter == PREVIOUS_MONDAY)
        t_adapter = "P1";
    else if (iAdapter == PREVIOUS_TUESDAY)
        t_adapter = "P2";
    else if (iAdapter == PREVIOUS_WEDNESDAY)
        t_adapter = "P3";
    else if (iAdapter == PREVIOUS_THURSDAY)
        t_adapter = "P4";
    else if (iAdapter == PREVIOUS_FRIDAY)
        t_adapter = "P5";
    else if (iAdapter == PREVIOUS_SATURDAY)
        t_adapter = "P6";
    else if (iAdapter == PREVIOUS_SUNDAY)
        t_adapter = "P7";
    else if (iAdapter == CLOSEST_MONDAY)
        t_adapter = "C1";
    else if (iAdapter == CLOSEST_TUESDAY)
        t_adapter = "C2";
    else if (iAdapter == CLOSEST_WEDNESDAY)
        t_adapter = "C3";
    else if (iAdapter == CLOSEST_THURSDAY)
        t_adapter = "C4";
    else if (iAdapter == CLOSEST_FRIDAY)
        t_adapter = "C5";
    else if (iAdapter == CLOSEST_SATURDAY)
        t_adapter = "C6";
    else if (iAdapter == CLOSEST_SUNDAY)
        t_adapter = "C7";
    else if (iAdapter == NEXT_MONDAY)
        t_adapter = "N1";
    else if (iAdapter == NEXT_TUESDAY)
        t_adapter = "N2";
    else if (iAdapter == NEXT_WEDNESDAY)
        t_adapter = "N3";
    else if (iAdapter == NEXT_THURSDAY)
        t_adapter = "N4";
    else if (iAdapter == NEXT_FRIDAY)
        t_adapter = "N5";
    else if (iAdapter == NEXT_SATURDAY)
        t_adapter = "N6";
    else if (iAdapter == NEXT_SUNDAY)
        t_adapter = "N7";
    else if (iAdapter == PREVIOUS_OPEN_DAY)
        t_adapter = "PO";
    else if (iAdapter == CLOSEST_OPEN_DAY)
        t_adapter = "CO";
    else if (iAdapter == NEXT_OPEN_DAY)
        t_adapter = "NO";
    else if (iAdapter == FIRST_OPEN_DAY_OF_MONTH)
        t_adapter = "1MO";
    return setAttribute(QStringLiteral("t_adapter"), t_adapter);
}

SKGError SKGRecurrentOperationObject::setAutoWriteDays(int iDays)
{
    return setAttribute(QStringLiteral("i_auto_write_days"), SKGServices::intToString(iDays));
}

int SKGRecurrentOperationObject::getAutoWriteDays() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_auto_write_days")));
}

SKGError SKGRecurrentOperationObject::setWarnDays(int iDays)
{
    return setAttribute(QStringLiteral("i_warn_days"), SKGServices::intToString(iDays));
}

int SKGRecurrentOperationObject::getWarnDays() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_warn_days")));
}

bool SKGRecurrentOperationObject::hasTimeLimit() const
{
    return (getAttribute(QStringLiteral("t_times")) == QStringLiteral("Y"));
}

SKGError SKGRecurrentOperationObject::timeLimit(bool iTimeLimit)
{
    return setAttribute(QStringLiteral("t_times"), iTimeLimit ? QStringLiteral("Y") : QStringLiteral("N"));
}

SKGError SKGRecurrentOperationObject::setTimeLimit(QDate iLastDate)
{
    // Get parameters
    QDate firstDate = this->getDate();
    if (iLastDate < firstDate) {
        return setTimeLimit(0);
    }
    PeriodUnit period = this->getPeriodUnit();
    int occu = qMax(this->getPeriodIncrement(), 1);

    // Compute nb time
    int nbd = firstDate.daysTo(iLastDate);
    if (period == DAY) {
        nbd = nbd / occu;
    } else if (period == WEEK) {
        nbd = nbd / (7 * occu);
    } else if (period == MONTH) {
        nbd = (iLastDate.day() >= firstDate.day() ? 0 : -1) + (iLastDate.year() - firstDate.year()) * 12 + (iLastDate.month() - firstDate.month());
    } else if (period == YEAR) {
        nbd = nbd / (365 * occu);
    }

    if (nbd < -1) {
        nbd = -1;
    }
    return setTimeLimit(nbd + 1);
}

SKGError SKGRecurrentOperationObject::setTimeLimit(int iTimeLimit)
{
    return setAttribute(QStringLiteral("i_nb_times"), SKGServices::intToString(iTimeLimit));
}

int SKGRecurrentOperationObject::getTimeLimit() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_nb_times")));
}

SKGError SKGRecurrentOperationObject::setDate(QDate iDate)
{
    return setAttribute(QStringLiteral("d_date"), SKGServices::dateToSqlString(iDate));
}

QDate SKGRecurrentOperationObject::getNextDate() const
{
    // Compute next date
    QDate nextDate = getDate();
    PeriodUnit punit = getPeriodUnit();
    int p = getPeriodIncrement();
    if (punit == DAY) {
        nextDate = nextDate.addDays(p);
    } else if (punit == WEEK) {
        nextDate = nextDate.addDays(7 * p);
    } else if (punit == MONTH) {
        nextDate = nextDate.addMonths(p);
    } else if (punit == YEAR) {
        nextDate = nextDate.addYears(p);
    }

    // Apply the adapter
    QDate nextDateAdapted = nextDate;
    const auto adapter = getAdapter();
    if (adapter >= PREVIOUS_MONDAY && adapter <= PREVIOUS_SUNDAY) {
        int targetWeekDay = (adapter - PREVIOUS_MONDAY + 1);
        int diff = (nextDateAdapted.dayOfWeek() - targetWeekDay + 7) % 7;
        nextDateAdapted = nextDateAdapted.addDays(-diff);
    } else if (adapter >= NEXT_MONDAY && adapter <= NEXT_SUNDAY) {
        int targetWeekDay = (adapter - NEXT_MONDAY + 1);
        int diff = (targetWeekDay - nextDateAdapted.dayOfWeek() + 7) % 7;
        nextDateAdapted = nextDateAdapted.addDays(diff);
    } else if (adapter >= CLOSEST_MONDAY && adapter <= CLOSEST_SUNDAY) {
        int targetWeekDay = (adapter - CLOSEST_MONDAY + 1);
        int forward = (targetWeekDay - nextDateAdapted.dayOfWeek() + 7) % 7;
        int backward = (nextDateAdapted.dayOfWeek() - targetWeekDay + 7) % 7;
        if (backward <= forward) {
            nextDateAdapted = nextDateAdapted.addDays(-backward);
        } else {
            nextDateAdapted = nextDateAdapted.addDays(forward);
        }
    } else if (adapter == PREVIOUS_OPEN_DAY) {
        if (nextDateAdapted.dayOfWeek() == Qt::Saturday) {
            nextDateAdapted = nextDateAdapted.addDays(-1);
        } else if (nextDateAdapted.dayOfWeek() == Qt::Sunday) {
            nextDateAdapted = nextDateAdapted.addDays(-2);
        }
    } else if (adapter == NEXT_OPEN_DAY) {
        if (nextDateAdapted.dayOfWeek() == Qt::Saturday) {
            nextDateAdapted = nextDateAdapted.addDays(2);
        } else if (nextDateAdapted.dayOfWeek() == Qt::Sunday) {
            nextDateAdapted = nextDateAdapted.addDays(1);
        }
    } else if (adapter == CLOSEST_OPEN_DAY) {
        if (nextDateAdapted.dayOfWeek() == Qt::Saturday) {
            nextDateAdapted = nextDateAdapted.addDays(-1);
        } else if (nextDateAdapted.dayOfWeek() == Qt::Sunday) {
            nextDateAdapted = nextDateAdapted.addDays(1);
        }
    } else if (adapter == FIRST_OPEN_DAY_OF_MONTH) {
        nextDateAdapted = QDate(nextDateAdapted.year(), nextDateAdapted.month(), 1);
        SKGTRACE << "First day of month is (before) " << nextDateAdapted.toString() << Qt::endl;
        if (nextDateAdapted.dayOfWeek() == Qt::Saturday) {
            nextDateAdapted = nextDateAdapted.addDays(2);
        } else if (nextDateAdapted.dayOfWeek() == Qt::Sunday) {
            nextDateAdapted = nextDateAdapted.addDays(1);
        }
        SKGTRACE << "First day of month is (after) " << nextDateAdapted.toString() << Qt::endl;
    }
    if (nextDateAdapted > getDate()) {
        nextDate = nextDateAdapted;
    }

    return nextDate;
}

QDate SKGRecurrentOperationObject::getDate() const
{
    return SKGServices::stringToDate(getAttribute(QStringLiteral("d_date")));
}

SKGError SKGRecurrentOperationObject::warnEnabled(bool iWarn)
{
    return setAttribute(QStringLiteral("t_warn"), iWarn ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGRecurrentOperationObject::isWarnEnabled() const
{
    return (getAttribute(QStringLiteral("t_warn")) == QStringLiteral("Y"));
}

SKGError SKGRecurrentOperationObject::autoWriteEnabled(bool iAutoWrite)
{
    return setAttribute(QStringLiteral("t_auto_write"), iAutoWrite ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGRecurrentOperationObject::isAutoWriteEnabled() const
{
    return (getAttribute(QStringLiteral("t_auto_write")) == QStringLiteral("Y"));
}

SKGError SKGRecurrentOperationObject::getRecurredOperations(SKGListSKGObjectBase &oOperations) const
{
    return getDocument()->getObjects(QStringLiteral("v_operation"),
                                     "r_recurrentoperation_id=" % SKGServices::intToString(getID()) % " ORDER BY d_date",
                                     oOperations);
}

SKGError SKGRecurrentOperationObject::process(int &oNbInserted, bool iForce, QDate iDate)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    oNbInserted = 0;

    if (!hasTimeLimit() || getTimeLimit() > 0) {
        if (isAutoWriteEnabled() || iForce) {
            QDate nextDate = getDate();
            if (nextDate.isValid() && iDate >= nextDate.addDays(-getAutoWriteDays())) {
                SKGOperationObject op;
                err = getParentOperation(op);
                IFOK(err)
                {
                    // Create the duplicated operation
                    SKGOperationObject newOp;
                    err = op.duplicate(newOp, nextDate);
                    IFOKDO(err, newOp.setRecurrentOperation(getID()))
                    IFOKDO(err, load())

                    if (!err && hasTimeLimit()) {
                        err = setTimeLimit(getTimeLimit() - 1);
                    }
                    IFOKDO(err, save())

                    // Process again in case of multi insert needed
                    int nbi = 0;
                    IFOKDO(err, process(nbi, iForce, iDate))
                    oNbInserted = oNbInserted + 1 + nbi;

                    // Send message
                    IFOKDO(err, newOp.load())
                    IFOK(err)
                    {
                        err = getDocument()->sendMessage(i18nc("An information message", "Transaction '%1' has been inserted", newOp.getDisplayName()),
                                                         SKGDocument::Positive);
                    }
                }
            }
        }

        if (isWarnEnabled() && !err) {
            QDate nextDate = getDate();
            if (QDate::currentDate() >= nextDate.addDays(-getWarnDays())) {
                SKGOperationObject op;
                err = getParentOperation(op);
                IFOK(err)
                {
                    int nbdays = QDate::currentDate().daysTo(nextDate);
                    if (nbdays > 0) {
                        err = getDocument()->sendMessage(
                            i18np("Transaction '%2' will be inserted in one day", "Transaction '%2' will be inserted in %1 days", nbdays, getDisplayName()));
                    }
                }
            }
        }
    }
    return err;
}

SKGError SKGRecurrentOperationObject::process(SKGDocumentBank *iDocument, int &oNbInserted, bool iForce, QDate iDate)
{
    SKGError err;
    oNbInserted = 0;

    // Get all transaction with auto_write
    SKGListSKGObjectBase recuOps;
    if (iDocument != nullptr) {
        err = iDocument->getObjects(QStringLiteral("v_recurrentoperation"), QString(), recuOps);
    }

    int nb = recuOps.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGRecurrentOperationObject recu(recuOps.at(i));
        int nbi = 0;
        err = recu.process(nbi, iForce, iDate);
        oNbInserted += nbi;
    }

    return err;
}
