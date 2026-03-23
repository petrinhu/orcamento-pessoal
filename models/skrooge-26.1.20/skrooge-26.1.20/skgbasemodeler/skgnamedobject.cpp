/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGNamedObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgnamedobject.h"
#include "skgdocument.h"

SKGNamedObject::SKGNamedObject()
    : SKGNamedObject(nullptr)
{
}

SKGNamedObject::SKGNamedObject(SKGDocument *iDocument, const QString &iTable, int iID)
    : SKGObjectBase(iDocument, iTable, iID)
{
}

SKGNamedObject::SKGNamedObject(const SKGNamedObject &iObject) = default;

SKGNamedObject::SKGNamedObject(const SKGObjectBase &iObject)
    : SKGObjectBase(iObject)
{
}

SKGNamedObject &SKGNamedObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGNamedObject &SKGNamedObject::operator=(const SKGNamedObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGNamedObject::~SKGNamedObject() = default;

SKGError SKGNamedObject::setName(const QString &iName)
{
    return setAttribute(QStringLiteral("t_name"), iName);
}

QString SKGNamedObject::getName() const
{
    return getAttribute(QStringLiteral("t_name"));
}

QString SKGNamedObject::getWhereclauseId() const
{
    // Could we use the id
    QString output = SKGObjectBase::getWhereclauseId();
    if (output.isEmpty()) {
        // No, so we use the name
        QString name = SKGServices::stringToSqlString(getName());
        if (!name.isEmpty() || getID() == 0) {
            output = QStringLiteral("t_name='") % name % u'\'';
        }
    }
    return output;
}

SKGError SKGNamedObject::getObjectByName(SKGDocument *iDocument, const QString &iTable, const QString &iName, SKGObjectBase &oObject)
{
    return iDocument != nullptr ? iDocument->getObject(iTable, "t_name='" % SKGServices::stringToSqlString(iName) % u'\'', oObject) : SKGError();
}
