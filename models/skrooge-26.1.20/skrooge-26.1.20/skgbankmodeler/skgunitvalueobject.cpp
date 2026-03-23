/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGUnitValueObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgunitvalueobject.h"

#include <klocalizedstring.h>

#include "skgdocument.h"
#include "skgunitobject.h"

SKGUnitValueObject::SKGUnitValueObject()
    : SKGUnitValueObject(nullptr)
{
}

SKGUnitValueObject::SKGUnitValueObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_unitvalue"), iID)
{
}

SKGUnitValueObject::SKGUnitValueObject(const SKGUnitValueObject &iObject) = default;

SKGUnitValueObject::SKGUnitValueObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("unitvalue")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_unitvalue"), iObject.getID());
    }
}

SKGUnitValueObject &SKGUnitValueObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGUnitValueObject &SKGUnitValueObject::operator=(const SKGUnitValueObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGUnitValueObject::~SKGUnitValueObject() = default;

SKGError SKGUnitValueObject::setQuantity(double iValue)
{
    return setAttribute(QStringLiteral("f_quantity"), SKGServices::doubleToString(iValue));
}

double SKGUnitValueObject::getQuantity() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_quantity")));
}

SKGError SKGUnitValueObject::setDate(QDate iDate)
{
    return setAttribute(QStringLiteral("d_date"), SKGServices::dateToSqlString(iDate));
}

QDate SKGUnitValueObject::getDate() const
{
    return SKGServices::stringToDate(getAttribute(QStringLiteral("d_date")));
}

QString SKGUnitValueObject::getWhereclauseId() const
{
    // Could we use the id
    QString output = SKGObjectBase::getWhereclauseId();
    if (output.isEmpty()) {
        // No, so we use the date and parent
        if (!(getAttribute(QStringLiteral("d_date")).isEmpty()) && !(getAttribute(QStringLiteral("rd_unit_id")).isEmpty())) {
            output = "d_date='" % getAttribute(QStringLiteral("d_date")) % "' AND rd_unit_id=" % getAttribute(QStringLiteral("rd_unit_id"));
        }
    }
    return output;
}

SKGError SKGUnitValueObject::getUnit(SKGUnitObject &oUnit) const
{
    SKGError err;
    if (getDocument() != nullptr) {
        err = getDocument()->getObject(QStringLiteral("v_unit"), "id=" % getAttribute(QStringLiteral("rd_unit_id")), oUnit);
    }
    return err;
}
