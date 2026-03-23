/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGBankObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbankobject.h"

#include <klocalizedstring.h>

#include "skgaccountobject.h"
#include "skgdocumentbank.h"

SKGBankObject::SKGBankObject()
    : SKGBankObject(nullptr)
{
}

SKGBankObject::SKGBankObject(SKGDocument *iDocument, int iID)
    : SKGNamedObject(iDocument, QStringLiteral("v_bank"), iID)
{
}

SKGBankObject::~SKGBankObject() = default;

SKGBankObject::SKGBankObject(const SKGBankObject &iObject) = default;

SKGBankObject::SKGBankObject(const SKGNamedObject &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("bank")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_bank"), iObject.getID());
    }
}

SKGBankObject::SKGBankObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("bank")) {
        copyFrom(iObject);
    } else {
        *this = SKGNamedObject(iObject.getDocument(), QStringLiteral("v_bank"), iObject.getID());
    }
}

SKGBankObject &SKGBankObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGBankObject &SKGBankObject::operator=(const SKGBankObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGError SKGBankObject::addAccount(SKGAccountObject &oAccount)
{
    SKGError err;
    if (getID() == 0) {
        err =
            SKGError(ERR_FAIL,
                     i18nc("Error message", "%1 failed because linked object is not yet saved in the database.", QStringLiteral("SKGBankObject::addAccount")));
    } else {
        oAccount = SKGAccountObject(qobject_cast<SKGDocumentBank *>(getDocument()));
        err = oAccount.setAttribute(QStringLiteral("rd_bank_id"), SKGServices::intToString(getID()));
    }
    return err;
}

SKGError SKGBankObject::getAccounts(SKGListSKGObjectBase &oAccountList) const
{
    SKGError err = getDocument()->getObjects(QStringLiteral("v_account"), "rd_bank_id=" % SKGServices::intToString(getID()), oAccountList);
    return err;
}

SKGError SKGBankObject::setNumber(const QString &iNumber)
{
    return setAttribute(QStringLiteral("t_bank_number"), iNumber);
}

QString SKGBankObject::getNumber() const
{
    return getAttribute(QStringLiteral("t_bank_number"));
}

SKGError SKGBankObject::setIcon(const QString &iIcon)
{
    return setAttribute(QStringLiteral("t_icon"), iIcon);
}

QString SKGBankObject::getIcon() const
{
    return getAttribute(QStringLiteral("t_icon"));
}

double SKGBankObject::getCurrentAmount() const
{
    return SKGServices::stringToDouble(getAttributeFromView(QStringLiteral("v_bank_amount"), QStringLiteral("f_CURRENTAMOUNT")));
}
