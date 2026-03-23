/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGPayeeObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgpayeeobject.h"

#include <klocalizedstring.h>

#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skgoperationobject.h"
#include "skgtraces.h"

SKGPayeeObject::SKGPayeeObject()
    : SKGPayeeObject(nullptr)
{
}

SKGPayeeObject::SKGPayeeObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("v_payee"), iID)
{
}

SKGPayeeObject::~SKGPayeeObject() = default;

SKGPayeeObject::SKGPayeeObject(const SKGPayeeObject &iObject) = default;

SKGPayeeObject::SKGPayeeObject(const SKGObjectBase &iObject)

{
    if (iObject.getRealTable() == QStringLiteral("payee")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_payee"), iObject.getID());
    }
}

SKGPayeeObject &SKGPayeeObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGPayeeObject &SKGPayeeObject::operator=(const SKGPayeeObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGPayeeObject::createPayee(SKGDocumentBank *iDocument, const QString &iName, SKGPayeeObject &oPayee, bool iSendPopupMessageOnCreation)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Check if refund is already existing
    if (iName.isEmpty()) {
        oPayee = SKGPayeeObject(nullptr, 0);
    } else if (iDocument != nullptr) {
        iDocument->getObject(QStringLiteral("v_payee"), "t_name='" % SKGServices::stringToSqlString(iName) % u'\'', oPayee);
        if (oPayee.getID() == 0) {
            // No, we have to create it
            oPayee = SKGPayeeObject(iDocument);
            err = oPayee.setName(iName);
            IFOKDO(err, oPayee.save())

            if (!err && iSendPopupMessageOnCreation) {
                err = iDocument->sendMessage(i18nc("Information message", "Payee '%1' has been created", iName), SKGDocument::Positive);
            }
        }
    }

    return err;
}

SKGError SKGPayeeObject::getOperations(SKGListSKGObjectBase &oOperations) const
{
    SKGError err = getDocument()->getObjects(QStringLiteral("v_operation"), "r_payee_id=" % SKGServices::intToString(getID()), oOperations);
    return err;
}

SKGError SKGPayeeObject::setAddress(const QString &iAddress)
{
    return setAttribute(QStringLiteral("t_address"), iAddress);
}

QString SKGPayeeObject::getAddress() const
{
    return getAttribute(QStringLiteral("t_address"));
}

SKGError SKGPayeeObject::setClosed(bool iClosed)
{
    return setAttribute(QStringLiteral("t_close"), iClosed ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGPayeeObject::isClosed() const
{
    return (getAttribute(QStringLiteral("t_close")) == QStringLiteral("Y"));
}

SKGError SKGPayeeObject::bookmark(bool iBookmark)
{
    return setAttribute(QStringLiteral("t_bookmarked"), iBookmark ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGPayeeObject::isBookmarked() const
{
    return (getAttribute(QStringLiteral("t_bookmarked")) == QStringLiteral("Y"));
}

SKGError SKGPayeeObject::getCategory(SKGCategoryObject &oCategory) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_category"), "id=" % getAttribute(QStringLiteral("r_category_id")), oCategory);
    return err;
}

SKGError SKGPayeeObject::setCategory(const SKGCategoryObject &iCategory)
{
    return setAttribute(QStringLiteral("r_category_id"), SKGServices::intToString(iCategory.getID()));
}

SKGError SKGPayeeObject::merge(const SKGPayeeObject &iPayee)
{
    SKGError err;

    SKGObjectBase::SKGListSKGObjectBase ops;
    IFOKDO(err, iPayee.getOperations(ops))
    int nb = ops.count();
    for (int i = 0; !err && i < nb; ++i) {
        SKGOperationObject op(ops.at(i));
        err = op.setPayee(*this);
        IFOKDO(err, op.save(true, false))
    }

    IFOKDO(err, iPayee.remove(false))
    return err;
}
