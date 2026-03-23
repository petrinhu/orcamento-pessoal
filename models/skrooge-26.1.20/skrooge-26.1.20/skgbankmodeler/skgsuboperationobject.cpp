/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGSubOperationObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgsuboperationobject.h"

#include <klocalizedstring.h>

#include "skgcategoryobject.h"
#include "skgdocument.h"
#include "skgoperationobject.h"
#include "skgservices.h"
#include "skgtrackerobject.h"

SKGSubOperationObject::SKGSubOperationObject()
    : SKGSubOperationObject(nullptr)
{
}

SKGSubOperationObject::SKGSubOperationObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_suboperation"), iID)
{
}

SKGSubOperationObject::~SKGSubOperationObject() = default;

SKGSubOperationObject::SKGSubOperationObject(const SKGSubOperationObject &iObject) = default;

SKGSubOperationObject::SKGSubOperationObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("suboperation")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_suboperation"), iObject.getID());
    }
}

SKGSubOperationObject &SKGSubOperationObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGSubOperationObject &SKGSubOperationObject::operator=(const SKGSubOperationObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGSubOperationObject::setDate(QDate iDate)
{
    return setAttribute(QStringLiteral("d_date"), iDate.isValid() ? SKGServices::dateToSqlString(iDate) : QString());
}

QDate SKGSubOperationObject::getDate() const
{
    return SKGServices::stringToDate(getAttribute(QStringLiteral("d_date")));
}

SKGError SKGSubOperationObject::setOrder(int iOrder)
{
    return setAttribute(QStringLiteral("i_order"), SKGServices::intToString(iOrder));
}

int SKGSubOperationObject::getOrder() const
{
    return SKGServices::stringToInt(getAttribute(QStringLiteral("i_order")));
}

SKGError SKGSubOperationObject::setComment(const QString &iComment)
{
    return setAttribute(QStringLiteral("t_comment"), iComment);
}

QString SKGSubOperationObject::getComment() const
{
    return getAttribute(QStringLiteral("t_comment"));
}

QString SKGSubOperationObject::getFormula() const
{
    return getAttribute(QStringLiteral("t_formula"));
}

SKGError SKGSubOperationObject::setFormula(const QString &iFormula)
{
    return setAttribute(QStringLiteral("t_formula"), iFormula);
}

SKGError SKGSubOperationObject::getParentOperation(SKGOperationObject &oOperation) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_operation"), "id=" % getAttribute(QStringLiteral("rd_operation_id")), oOperation);
    return err;
}

SKGError SKGSubOperationObject::setParentOperation(const SKGOperationObject &iOperation)
{
    SKGError err;
    if (!getDate().isValid()) {
        err = setDate(iOperation.getDate());
    }
    IFOKDO(err, setAttribute(QStringLiteral("rd_operation_id"), SKGServices::intToString(iOperation.getID())))
    return err;
}

SKGError SKGSubOperationObject::getCategory(SKGCategoryObject &oCategory) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_category"), "id=" % getAttribute(QStringLiteral("r_category_id")), oCategory);
    return err;
}

SKGError SKGSubOperationObject::setCategory(const SKGCategoryObject &iCategory)
{
    return setAttribute(QStringLiteral("r_category_id"), SKGServices::intToString(iCategory.getID()));
}

SKGError SKGSubOperationObject::setQuantity(double iValue)
{
    return setAttribute(QStringLiteral("f_value"), SKGServices::doubleToString(iValue));
}

double SKGSubOperationObject::getQuantity() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_value")));
}

SKGError SKGSubOperationObject::setTracker(const SKGTrackerObject &iTracker, bool iForce)
{
    SKGError err;
    SKGTrackerObject previous;
    getTracker(previous);
    if (iTracker != previous) {
        if (!iForce && previous.isClosed()) {
            err = SKGError(ERR_FAIL, i18nc("Error message", "Impossible to remove a transaction from a closed tracker"));
        } else if (!iForce && iTracker.isClosed()) {
            err = SKGError(ERR_FAIL, i18nc("Error message", "Impossible to add a transaction in a closed tracker"));
        } else {
            err = setAttribute(QStringLiteral("r_refund_id"), SKGServices::intToString(iTracker.getID()));
        }
    }
    return err;
}

SKGError SKGSubOperationObject::getTracker(SKGTrackerObject &oTracker) const
{
    QString idS = getAttribute(QStringLiteral("r_refund_id"));
    if (idS.isEmpty()) {
        idS = '0';
    }
    SKGError err;
    if ((getDocument() != nullptr) && idS != QStringLiteral("0")) {
        err = getDocument()->getObject(QStringLiteral("v_refund"), "id=" % idS, oTracker);
    }
    return err;
}
