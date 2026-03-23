/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGOperationObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgoperationobject.h"

#include <klocalizedstring.h>
#include <utility>

#include "skgaccountobject.h"
#include "skgdocument.h"
#include "skgpayeeobject.h"
#include "skgrecurrentoperationobject.h"
#include "skgservices.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"
#include "skgunitobject.h"

SKGOperationObject::SKGOperationObject()
    : SKGOperationObject(nullptr)
{
}

SKGOperationObject::SKGOperationObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_operation"), iID)
{
}

SKGOperationObject::~SKGOperationObject() = default;

SKGOperationObject::SKGOperationObject(const SKGOperationObject &iObject) = default;

SKGOperationObject::SKGOperationObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("operation")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_operation"), iObject.getID());
    }
}

SKGOperationObject &SKGOperationObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGOperationObject &SKGOperationObject::operator=(const SKGOperationObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGOperationObject::duplicate(SKGOperationObject &oOperation, QDate iDate, bool iTemplateMode) const
{
    SKGError err;
    SKGTRACEINFUNCRC(20, err)
    QDate previousDate = getDate();

    // Create the duplicated operation
    oOperation = SKGOperationObject(getDocument(), this->getID()); // To be sure the object is on v_operation
    IFOKDO(err, oOperation.load())
    IFOKDO(err, oOperation.resetID())
    IFOKDO(err, oOperation.setDate(iDate))
    IFOKDO(err, oOperation.setStatus(SKGOperationObject::NONE))
    IFOKDO(err, oOperation.setImported(false))
    IFOKDO(err, oOperation.setTemplate(iTemplateMode))
    IFOKDO(err, oOperation.setImportID(QString()))
    IFOKDO(err, oOperation.bookmark(false))
    IFOKDO(err, oOperation.setNumber(QString()))
    IFOKDO(err, oOperation.setGroupOperation(oOperation))
    IFOKDO(err, oOperation.setAttribute(QStringLiteral("d_createdate"), SKGServices::dateToSqlString(QDateTime::currentDateTime())))
    IFOKDO(err, oOperation.save(false, false))

    // Duplicate subop
    IFOK(err)
    {
        SKGListSKGObjectBase subops;
        err = getSubOperations(subops);
        int nbsupops = subops.count();
        for (int i = 0; !err && i < nbsupops; ++i) {
            SKGSubOperationObject subop(subops.at(i));
            err = subop.resetID();
            IFOKDO(err, subop.setParentOperation(oOperation))
            IFOKDO(err, subop.setDate(subop.getDate().addDays(previousDate.daysTo(iDate))))
            IFOKDO(err, subop.save(false))
        }
    }

    // Duplicate grouped transaction to support recurrent transfers
    IFOK(err)
    {
        SKGListSKGObjectBase goupops;
        err = getGroupedOperations(goupops);
        int nbgoupops = goupops.count();
        for (int i = 0; !err && i < nbgoupops; ++i) {
            SKGOperationObject groupop(goupops.at(i));
            if (groupop != *this) {
                // Create the duplicated operation
                SKGOperationObject newgroupop = groupop;
                err = newgroupop.resetID();
                IFOKDO(err, newgroupop.setDate(iDate))
                IFOKDO(err, newgroupop.setStatus(SKGOperationObject::NONE))
                IFOKDO(err, newgroupop.setImported(false))
                IFOKDO(err, newgroupop.setTemplate(iTemplateMode))
                IFOKDO(err, newgroupop.setImportID(QString()))
                IFOKDO(err, newgroupop.bookmark(false))
                IFOKDO(err, newgroupop.setNumber(QString()))
                IFOKDO(err, newgroupop.setGroupOperation(newgroupop))
                IFOKDO(err, newgroupop.setGroupOperation(oOperation))
                IFOKDO(err, newgroupop.save(false))

                // Duplicate subop
                IFOK(err)
                {
                    SKGListSKGObjectBase subops;
                    err = groupop.getSubOperations(subops);
                    int nbsupops = subops.count();
                    for (int j = 0; !err && j < nbsupops; ++j) {
                        SKGSubOperationObject subop(subops.at(j));
                        err = subop.resetID();
                        IFOKDO(err, subop.setParentOperation(newgroupop))
                        IFOKDO(err, subop.setDate(subop.getDate().addDays(previousDate.daysTo(iDate))))
                        IFOKDO(err, subop.save(false))
                    }
                }
            }
        }
    }

    IFOKDO(err, oOperation.load())
    return err;
}

SKGError SKGOperationObject::getParentAccount(SKGAccountObject &oAccount) const
{
    SKGObjectBase objTmp;
    SKGError err = getDocument()->getObject(QStringLiteral("v_account"), "id=" % getAttribute(QStringLiteral("rd_account_id")), objTmp);
    oAccount = objTmp;
    return err;
}

SKGError SKGOperationObject::setParentAccount(const SKGAccountObject &iAccount, bool iForce)
{
    SKGError err;
    QString currentAccount = getAttribute(QStringLiteral("rd_account_id"));
    QString newAccount = SKGServices::intToString(iAccount.getID());
    if (newAccount == QStringLiteral("0")) {
        err = SKGError(ERR_FAIL,
                       i18nc("Error message",
                             "%1 failed because linked object is not yet saved in the database.",
                             QStringLiteral("SKGOperationObject::setParentAccount")));
    } else {
        if (newAccount != currentAccount) {
            if (iAccount.isClosed() && !iForce) {
                err = SKGError(ERR_FAIL, i18nc("Error message", "Impossible to add a transaction in a closed account"));
            } else {
                err = setAttribute(QStringLiteral("rd_account_id"), newAccount);
            }
        }
    }
    return err;
}

SKGError SKGOperationObject::setMode(const QString &iMode)
{
    return setAttribute(QStringLiteral("t_mode"), iMode);
}

QString SKGOperationObject::getMode() const
{
    return getAttribute(QStringLiteral("t_mode"));
}

SKGError SKGOperationObject::setPayee(const SKGPayeeObject &iPayee)
{
    return setAttribute(QStringLiteral("r_payee_id"), SKGServices::intToString(iPayee.getID()));
}

SKGError SKGOperationObject::getPayee(SKGPayeeObject &oPayee) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_payee"),
                                            "id=" % SKGServices::intToString(SKGServices::stringToInt(getAttribute(QStringLiteral("r_payee_id")))),
                                            oPayee);
    return err;
}

SKGError SKGOperationObject::setComment(const QString &iComment)
{
    return setAttribute(QStringLiteral("t_comment"), iComment);
}

QString SKGOperationObject::getComment() const
{
    return getAttribute(QStringLiteral("t_comment"));
}

SKGError SKGOperationObject::setNumber(const QString &iNumber)
{
    return setAttribute(QStringLiteral("t_number"), iNumber);
}

QString SKGOperationObject::getNumber() const
{
    return getAttribute(QStringLiteral("t_number"));
}

SKGOperationObject::OperationStatus SKGOperationObject::getStatus() const
{
    QString t_status = getAttribute(QStringLiteral("t_status"));
    if (t_status == QStringLiteral("Y")) {
        return SKGOperationObject::CHECKED;
    }
    if (t_status == QStringLiteral("P")) {
        return SKGOperationObject::MARKED;
    }
    return SKGOperationObject::NONE;
}

SKGError SKGOperationObject::setStatus(SKGOperationObject::OperationStatus iStatus)
{
    return setAttribute(
        QStringLiteral("t_status"),
        (iStatus == SKGOperationObject::CHECKED ? QStringLiteral("Y") : (iStatus == SKGOperationObject::MARKED ? QStringLiteral("P") : QStringLiteral("N"))));
}

SKGError SKGOperationObject::setDate(QDate iDate, bool iRefreshSubOperations)
{
    SKGError err;
    // Compute delta of the change of date
    QDate previousDate = getDate();
    if (iRefreshSubOperations) {
        // Apply the delta on sub transactions
        SKGObjectBase::SKGListSKGObjectBase listSubOperations;
        getSubOperations(listSubOperations); // Error is not manage to avoid error in case of first creation
        int nbSubOperations = listSubOperations.count();
        for (int i = 0; !err && i < nbSubOperations; ++i) {
            SKGSubOperationObject sop(listSubOperations.at(i));
            QDate previousSubDate = sop.getDate();
            if (previousSubDate.isValid()) {
                if (previousDate.isValid()) {
                    int delta = previousDate.daysTo(iDate);
                    err = sop.setDate(previousSubDate.addDays(delta));
                    IFOKDO(err, sop.save(true, false))
                }
            } else {
                err = sop.setDate(iDate);
                IFOKDO(err, sop.save(true, false))
            }
        }
    }
    IFOKDO(err, setAttribute(QStringLiteral("d_date"), SKGServices::dateToSqlString(iDate)))
    return err;
}

QDate SKGOperationObject::getDate() const
{
    return SKGServices::stringToDate(getAttribute(QStringLiteral("d_date")));
}

SKGError SKGOperationObject::getUnit(SKGUnitObject &oUnit) const
{
    SKGError err = (getDocument() == nullptr ? SKGError(ERR_POINTER, i18nc("Error message", "Transaction impossible because the document is missing"))
                                             : getDocument()->getObject(QStringLiteral("v_unit"), "id=" % getAttribute(QStringLiteral("rc_unit_id")), oUnit));
    // SKGError err = getDocument()->getObject(QStringLiteral("v_unit"), "id=" % getAttribute(QStringLiteral("rc_unit_id")), oUnit);
    return err;
}

SKGError SKGOperationObject::setUnit(const SKGUnitObject &iUnit)
{
    return setAttribute(QStringLiteral("rc_unit_id"), SKGServices::intToString(iUnit.getID()));
}

bool SKGOperationObject::isInGroup() const
{
    return (getAttribute(QStringLiteral("i_group_id")) != QStringLiteral("0"));
}

bool SKGOperationObject::isTransfer(SKGOperationObject &oOperation) const
{
    SKGTRACEINFUNC(10)
    SKGObjectBase::SKGListSKGObjectBase ops;
    getGroupedOperations(ops);
    if (ops.count() == 2) {
        oOperation = (*this == SKGOperationObject(ops.at(0)) ? ops.at(1) : ops.at(0));
    }
    return (getAttribute(QStringLiteral("t_TRANSFER")) == QStringLiteral("Y"));
}

SKGError SKGOperationObject::getGroupedOperations(SKGListSKGObjectBase &oGroupedOperations) const
{
    SKGError err;
    QString gpId1 = getAttribute(QStringLiteral("i_group_id"));
    if (gpId1 == QStringLiteral("0") || gpId1.isEmpty()) {
        oGroupedOperations.clear();
    } else {
        err = getDocument()->getObjects(QStringLiteral("v_operation"), "i_group_id=" % gpId1, oGroupedOperations);
    }
    return err;
}

SKGError SKGOperationObject::getGroupOperation(SKGOperationObject &oOperation) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_operation"), "id=" % getAttribute(QStringLiteral("i_group_id")), oOperation);
    return err;
}

SKGError SKGOperationObject::setGroupOperation(const SKGOperationObject &iOperation)
{
    SKGError err;
    SKGTRACEINFUNCRC(20, err)

    // Is it a remove group ?
    if (iOperation == *this) {
        // Yes
        err = setAttribute(QStringLiteral("i_group_id"), QStringLiteral("0"));
    } else {
        // Get previous groups
        QString group1 = getAttribute(QStringLiteral("i_group_id"));
        QString group2 = iOperation.getAttribute(QStringLiteral("i_group_id"));

        // Create a new group
        SKGStringListList result;
        err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT max(i_group_id) from operation"), result);
        IFOK(err)
        {
            // Compute new group id
            QString newIdGroup('1');
            if (result.count() == 2) {
                newIdGroup = SKGServices::intToString(SKGServices::stringToInt(result.at(1).at(0)) + 1);
            }

            // Set group id
            SKGOperationObject op1 = SKGOperationObject(iOperation.getDocument(), iOperation.getID());
            err = op1.setAttribute(QStringLiteral("i_group_id"), newIdGroup);
            IFOKDO(err, op1.save(true, false))

            IFOKDO(err, setAttribute(QStringLiteral("i_group_id"), newIdGroup))

            // Update all objects of group2
            if (!err && !group1.isEmpty() && group1 != QStringLiteral("0")) {
                err = getDocument()->executeSqliteOrder("UPDATE operation SET i_group_id=" % newIdGroup % " WHERE i_group_id=" % group1);
            }

            // Update all objects of group2
            if (!err && !group2.isEmpty() && group2 != QStringLiteral("0")) {
                err = getDocument()->executeSqliteOrder("UPDATE operation SET i_group_id=" % newIdGroup % " WHERE i_group_id=" % group2);
            }
        }
    }

    return err;
}

SKGError SKGOperationObject::bookmark(bool iBookmark)
{
    return setAttribute(QStringLiteral("t_bookmarked"), iBookmark ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGOperationObject::isBookmarked() const
{
    return (getAttribute(QStringLiteral("t_bookmarked")) == QStringLiteral("Y"));
}

SKGError SKGOperationObject::setImported(bool iImported)
{
    return setAttribute(QStringLiteral("t_imported"), iImported ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGOperationObject::isImported() const
{
    return (getAttribute(QStringLiteral("t_imported")) != QStringLiteral("N"));
}

SKGError SKGOperationObject::setImportID(const QString &iImportID)
{
    SKGError err = setAttribute(QStringLiteral("t_import_id"), iImportID);
    if (!err && !iImportID.isEmpty()) {
        err = setAttribute(QStringLiteral("t_imported"), QStringLiteral("T"));
    }
    return err;
}

QString SKGOperationObject::getImportID() const
{
    return getAttribute(QStringLiteral("t_import_id"));
}

SKGError SKGOperationObject::setTemplate(bool iTemplate)
{
    return setAttribute(QStringLiteral("t_template"), iTemplate ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGOperationObject::isTemplate() const
{
    return (getAttribute(QStringLiteral("t_template")) != QStringLiteral("N"));
}

int SKGOperationObject::getNbSubOperations() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_NBSUBOPERATIONS")));
}

SKGError SKGOperationObject::addSubOperation(SKGSubOperationObject &oSubOperation)
{
    SKGError err;
    if (getID() == 0) {
        err = SKGError(
            ERR_FAIL,
            i18nc("Error message", "%1 failed because linked object is not yet saved in the database.", QStringLiteral("SKGOperationObject::addSubOperation")));
    } else {
        oSubOperation = SKGSubOperationObject(getDocument());
        err = oSubOperation.setParentOperation(*this);
        IFOKDO(err, oSubOperation.setDate(getDate()))
    }
    return err;
}

SKGError SKGOperationObject::getSubOperations(SKGListSKGObjectBase &oSubOperations) const
{
    SKGError err;
    if (getID() == 0) {
        err = SKGError(ERR_FAIL,
                       i18nc("Error message",
                             "%1 failed because linked object is not yet saved in the database.",
                             QStringLiteral("SKGOperationObject::getSubOperations")));
    } else {
        err = getDocument()->getObjects(QStringLiteral("v_suboperation"),
                                        "rd_operation_id=" % SKGServices::intToString(getID()) % " ORDER BY i_order, id",
                                        oSubOperations);
    }
    return err;
}

double SKGOperationObject::getCurrentAmount() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_CURRENTAMOUNT")));
}

double SKGOperationObject::getBalance() const
{
    double output = 0.0;
    SKGStringListList result;
    SKGError err = getDocument()->executeSelectSqliteOrder("SELECT TOTAL(f_CURRENTAMOUNT) FROM v_operation WHERE t_template='N' AND "
                   "rd_account_id=" % getAttribute(QStringLiteral("rd_account_id")) % " AND (d_date<'" % getAttribute(QStringLiteral("d_date")) % "' OR "
                   "(d_date='" % getAttribute(QStringLiteral("d_date")) % "' AND id<=" % SKGServices::intToString(getID()) % "))", result);
    IFOK(err)
    {
        output = SKGServices::stringToDouble(result.at(1).at(0));
    }

    return output;
}

double SKGOperationObject::getAmount(QDate iDate) const
{
    // Get quantity
    double quantity = SKGServices::stringToDouble(getAttribute(QStringLiteral("f_QUANTITY")));

    // Is the unit value already in cache ?
    double coef = 1;
    QString val = getDocument()->getCachedValue("unitvalue-" % getAttribute(QStringLiteral("rc_unit_id")));
    if (!val.isEmpty()) {
        // Yes
        coef = SKGServices::stringToDouble(val);
    } else {
        // No
        SKGUnitObject unit;
        if (getUnit(unit).isSucceeded()) {
            coef = unit.getAmount(iDate);
        }
    }

    return coef * quantity;
}

SKGError SKGOperationObject::addRecurrentOperation(SKGRecurrentOperationObject &oRecurrentOperation) const
{
    SKGError err;
    if (getID() == 0) {
        err = SKGError(ERR_FAIL,
                       i18nc("Error message",
                             "%1 failed because linked object is not yet saved in the database.",
                             QStringLiteral("SKGOperationObject::addRecurrentOperation")));
    } else {
        oRecurrentOperation = SKGRecurrentOperationObject(getDocument());
        err = oRecurrentOperation.setParentOperation(*this);
        IFOK(err) oRecurrentOperation.setDate(getDate());
    }
    return err;
}

SKGError SKGOperationObject::getRecurrentOperations(SKGListSKGObjectBase &oRecurrentOperation) const
{
    SKGError err;
    if (getID() == 0) {
        err = SKGError(ERR_FAIL,
                       i18nc("Error message",
                             "%1 failed because linked object is not yet saved in the database.",
                             QStringLiteral("SKGOperationObject::getRecurrentOperation")));
    } else {
        err = getDocument()->getObjects(QStringLiteral("v_recurrentoperation"), "rd_operation_id=" % SKGServices::intToString(getID()), oRecurrentOperation);
    }
    return err;
}

int SKGOperationObject::getRecurrentOperation() const
{
    const auto recurrentId = SKGServices::stringToInt(getAttribute(QStringLiteral("r_recurrentoperation_id")));
    if (recurrentId != 0) {
        return recurrentId;
    } else {
        SKGObjectBase::SKGListSKGObjectBase recops;
        SKGError err = getRecurrentOperations(recops);
        IFOK(err)
        {
            if (!recops.isEmpty()) {
                return recops.first().getID();
            }
        }
    }

    return 0;
}

SKGError SKGOperationObject::setRecurrentOperation(int recurrentId)
{
    SKGError err;
    const auto currentRecurrentId = getRecurrentOperation();
    if (currentRecurrentId != 0 && recurrentId != 0 && currentRecurrentId == recurrentId) {
        err = SKGError(ERR_FAIL, i18nc("Error message", "Trying to set the same recurrent transaction with id=%1.", recurrentId));
        return err;
    } else if (isTemplate()) {
        err = SKGError(ERR_FAIL, i18nc("Error message", "Cannot set a recurrent for a template operation."));
        return err;
    }

    SKGRecurrentOperationObject oldRop(getDocument(), currentRecurrentId);
    SKGRecurrentOperationObject newRop(getDocument(), recurrentId);
    if (oldRop.exist()) {
        // Unlink from the current schedule
        SKGOperationObject oldRopParent;
        err = oldRop.getParentOperation(oldRopParent);
        if (oldRopParent == *this) {
            SKGObjectBase::SKGListSKGObjectBase transactions;
            IFOKDO(err, oldRop.getRecurredOperations(transactions))
            IFOK(err)
            {
                if (!transactions.isEmpty()) {
                    SKGOperationObject lastObj(transactions.last());
                    IFOKDO(err, oldRop.setParentOperation(lastObj))
                    IFOKDO(err, oldRop.setDate(lastObj.getDate()))
                    IFOKDO(err, oldRop.setDate(oldRop.getNextDate()))
                    IFOKDO(err, oldRop.save())
                    IFOKDO(err, lastObj.setAttribute(QStringLiteral("r_recurrentoperation_id"), QStringLiteral("0")))
                    IFOKDO(err, lastObj.save())
                } else {
                    IFOKDO(err, oldRop.remove(true, true))
                }
            }
        }

        IFOKDO(err, setAttribute(QStringLiteral("r_recurrentoperation_id"), QStringLiteral("0")))
        IFOKDO(err, save())
    }

    if (newRop.exist()) {
        // Link to a new schedule
        const auto currentDate = getDate();
        QDate lastDate;
        SKGOperationObject newRopParent;
        err = newRop.getParentOperation(newRopParent);
        if (newRopParent.isTemplate()) {
            SKGObjectBase::SKGListSKGObjectBase transactions;
            IFOKDO(err, newRop.getRecurredOperations(transactions))
            IFOK(err)
            {
                if (!transactions.isEmpty()) {
                    SKGOperationObject lastObj(transactions.last());
                    lastDate = lastObj.getDate();
                }
            }
        } else {
            lastDate = newRopParent.getDate();
        }
        if (currentDate.isValid() && lastDate.isValid() && currentDate >= lastDate) {
            IFOK(err)
            {
                if (!newRopParent.isTemplate()) {
                    // Set old parent op as recurrent
                    IFOKDO(err, newRopParent.setAttribute(QStringLiteral("r_recurrentoperation_id"), SKGServices::intToString(recurrentId)))
                    IFOKDO(err, newRopParent.save())

                    // Set this transaction as reference
                    IFOKDO(err, newRop.setParentOperation(*this))
                } else {
                    // Set this op as recurrent
                    IFOKDO(err, setAttribute(QStringLiteral("r_recurrentoperation_id"), SKGServices::intToString(recurrentId)))
                    IFOKDO(err, save())
                }

                IFOKDO(err, newRop.setDate(currentDate))
            }
        } else {
            IFOKDO(err, setAttribute(QStringLiteral("r_recurrentoperation_id"), SKGServices::intToString(recurrentId)))
            IFOKDO(err, save())
        }
        // 492287 vvvv
        IFOKDO(err, newRop.setDate(newRop.getNextDate()))
        IFOKDO(err, newRop.save())
        // 492287 ^^^^
    }

    return err;
}

SKGError SKGOperationObject::mergeAttribute(const SKGOperationObject &iDeletedOne, SKGOperationObject::AmountAlignmentMode iMode, bool iSendMessage)
{
    // Merge operation
    SKGError err = setDate(iDeletedOne.getDate());
    IFOKDO(err, setImportID(iDeletedOne.getImportID()))
    IFOKDO(err, setAttribute(QStringLiteral("t_imported"), iDeletedOne.getAttribute(QStringLiteral("t_imported"))))
    if (!err && getComment().isEmpty()) {
        err = setComment(iDeletedOne.getComment());
    }
    SKGPayeeObject payee;
    getPayee(payee);
    if (!err && !payee.exist()) {
        iDeletedOne.getPayee(payee);
        err = setPayee(payee);
    }
    if (!err && getMode().isEmpty()) {
        err = setMode(iDeletedOne.getMode());
    }
    if (!err && !isBookmarked()) {
        err = bookmark(iDeletedOne.isBookmarked());
    }
    if (!err && getNumber().isEmpty()) {
        err = setNumber(iDeletedOne.getNumber());
    }
    IFOKDO(err, save())

    // Merge subtransactions
    double currentAmount = getCurrentAmount();
    double targettAmount = iDeletedOne.getCurrentAmount();
    if (qAbs(currentAmount - targettAmount) > 0.0001) {
        SKGObjectBase::SKGListSKGObjectBase subOps1;
        IFOKDO(err, getSubOperations(subOps1))

        SKGObjectBase::SKGListSKGObjectBase subOps2;
        IFOKDO(err, iDeletedOne.getSubOperations(subOps2))

        // Align amounts
        SKGOperationObject::AmountAlignmentMode mode = iMode;
        if (mode == DEFAULT) {
            if (subOps2.count() == 1 && subOps1.count() == 1) {
                mode = PROPORTIONAL;
            } else if (subOps2.count() >= 1 && subOps1.count() >= 1) {
                mode = ADDSUBOPERATION;
            }
        }

        if (mode == SKGOperationObject::ADDSUBOPERATION) {
            // Add sub transaction to align amount
            SKGSubOperationObject so1;
            IFOKDO(err, addSubOperation(so1))
            IFOKDO(err, so1.setQuantity(targettAmount - currentAmount))
            IFOKDO(err, so1.save())
        } else {
            // Keep ratio
            for (const auto &sopbase : std::as_const(subOps1)) {
                SKGSubOperationObject sop(sopbase);
                IFOKDO(err, sop.setQuantity(targettAmount * sop.getQuantity() / currentAmount))
                IFOKDO(err, sop.save())
            }
        }
        IFOKDO(err, load())
        if (iSendMessage) {
            IFOK(err)
            getDocument()->sendMessage(i18nc("An information message", "Amount has been changed to be aligned with the imported transaction"),
                                       SKGDocument::Positive);
        }
    }

    // transfers properties
    IFOKDO(err,
           getDocument()->executeSqliteOrder(QStringLiteral("DELETE FROM parameters WHERE t_uuid_parent='") % getUniqueID()
                                             % QStringLiteral("' AND t_name IN (SELECT t_name FROM parameters WHERE t_uuid_parent='")
                                             % iDeletedOne.getUniqueID() % QStringLiteral("')")))
    IFOKDO(err,
           getDocument()->executeSqliteOrder(QStringLiteral("UPDATE parameters SET t_uuid_parent='") % getUniqueID() % QStringLiteral("' WHERE t_uuid_parent='")
                                             % iDeletedOne.getUniqueID() % QStringLiteral("'")))

    // Delete useless operation
    IFOKDO(err, iDeletedOne.remove(false, true))
    return err;
}

SKGError SKGOperationObject::mergeSuboperations(const SKGOperationObject &iDeletedOne)
{
    SKGError err;
    SKGObjectBase::SKGListSKGObjectBase subops;
    err = iDeletedOne.getSubOperations(subops);
    int nb = subops.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGSubOperationObject subop(subops.at(i));
        err = subop.setParentOperation(*this);
        IFOKDO(err, subop.save())
    }
    IFOKDO(err, iDeletedOne.remove(false))
    return err;
}
