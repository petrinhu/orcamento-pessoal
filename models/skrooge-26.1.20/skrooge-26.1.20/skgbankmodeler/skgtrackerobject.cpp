/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGTrackerObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtrackerobject.h"

#include <klocalizedstring.h>

#include "skgdocumentbank.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"

SKGTrackerObject::SKGTrackerObject()
    : SKGTrackerObject(nullptr)
{
}

SKGTrackerObject::SKGTrackerObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("v_refund"), iID)
{
}

SKGTrackerObject::~SKGTrackerObject() = default;

SKGTrackerObject::SKGTrackerObject(const SKGTrackerObject &iObject) = default;

SKGTrackerObject::SKGTrackerObject(const SKGObjectBase &iObject)

{
    if (iObject.getRealTable() == QStringLiteral("refund")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_refund"), iObject.getID());
    }
}

SKGTrackerObject &SKGTrackerObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGTrackerObject &SKGTrackerObject::operator=(const SKGTrackerObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGTrackerObject::createTracker(SKGDocumentBank *iDocument, const QString &iName, SKGTrackerObject &oTracker, bool iSendPopupMessageOnCreation)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Check if refund is already existing
    if (iName.isEmpty()) {
        oTracker = SKGTrackerObject(nullptr, 0);
    } else if (iDocument != nullptr) {
        iDocument->getObject(QStringLiteral("v_refund"), "t_name='" % SKGServices::stringToSqlString(iName) % u'\'', oTracker);
        if (oTracker.getID() == 0) {
            // No, we have to create it
            oTracker = SKGTrackerObject(iDocument);
            err = oTracker.setName(iName);
            IFOKDO(err, oTracker.save())

            if (!err && iSendPopupMessageOnCreation) {
                err = iDocument->sendMessage(i18nc("Information message", "Tracker '%1' has been created", iName), SKGDocument::Positive);
            }
        }
    }

    return err;
}

SKGError SKGTrackerObject::getSubOperations(SKGListSKGObjectBase &oSubOperations) const
{
    SKGError err = getDocument()->getObjects(QStringLiteral("v_suboperation"), "r_refund_id=" % SKGServices::intToString(getID()), oSubOperations);
    return err;
}

SKGError SKGTrackerObject::setClosed(bool iClosed)
{
    return setAttribute(QStringLiteral("t_close"), iClosed ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGTrackerObject::isClosed() const
{
    return (getAttribute(QStringLiteral("t_close")) == QStringLiteral("Y"));
}

SKGError SKGTrackerObject::setComment(const QString &iComment)
{
    return setAttribute(QStringLiteral("t_comment"), iComment);
}

QString SKGTrackerObject::getComment() const
{
    return getAttribute(QStringLiteral("t_comment"));
}

double SKGTrackerObject::getCurrentAmount() const
{
    return SKGServices::stringToDouble(getAttributeFromView(QStringLiteral("v_refund_amount"), QStringLiteral("f_CURRENTAMOUNT")));
}

SKGError SKGTrackerObject::merge(const SKGTrackerObject &iTracker)
{
    SKGError err;

    SKGObjectBase::SKGListSKGObjectBase ops;
    IFOKDO(err, iTracker.getSubOperations(ops))
    int nb = ops.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGSubOperationObject op(ops.at(i));
        err = op.setTracker(*this);
        IFOKDO(err, op.save(true, false))
    }

    IFOKDO(err, iTracker.remove(false))
    return err;
}
