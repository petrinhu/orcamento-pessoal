/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGInterestObject.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skginterestobject.h"
#include "skgdocument.h"

SKGInterestObject::SKGInterestObject()
    : SKGInterestObject(nullptr)
{
}

SKGInterestObject::SKGInterestObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_interest"), iID)
{
}

SKGInterestObject::SKGInterestObject(const SKGInterestObject &iObject) = default;

SKGInterestObject::SKGInterestObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("interest")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_interest"), iObject.getID());
    }
}

SKGInterestObject &SKGInterestObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGInterestObject &SKGInterestObject::operator=(const SKGInterestObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGInterestObject::~SKGInterestObject() = default;

SKGError SKGInterestObject::setRate(double iValue)
{
    return setAttribute(QStringLiteral("f_rate"), SKGServices::doubleToString(iValue));
}

double SKGInterestObject::getRate() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_rate")));
}

SKGError SKGInterestObject::setDate(QDate iDate)
{
    return setAttribute(QStringLiteral("d_date"), SKGServices::dateToSqlString(iDate));
}

QDate SKGInterestObject::getDate() const
{
    return SKGServices::stringToDate(getAttribute(QStringLiteral("d_date")));
}

SKGError SKGInterestObject::setIncomeValueDateMode(SKGInterestObject::ValueDateMode iMode)
{
    return setAttribute(QStringLiteral("t_income_value_date_mode"),
                        (iMode == FIFTEEN ? QStringLiteral("F") : SKGServices::intToString(static_cast<int>(iMode) - 1)));
}

SKGInterestObject::ValueDateMode SKGInterestObject::getIncomeValueDateMode() const
{
    QString mode = getAttribute(QStringLiteral("t_income_value_date_mode"));
    return (mode == QStringLiteral("F") ? FIFTEEN : static_cast<SKGInterestObject::ValueDateMode>(SKGServices::stringToInt(mode) + 1));
}

SKGError SKGInterestObject::setExpenditueValueDateMode(SKGInterestObject::ValueDateMode iMode)
{
    return setAttribute(QStringLiteral("t_expenditure_value_date_mode"),
                        (iMode == FIFTEEN ? QStringLiteral("F") : SKGServices::intToString(static_cast<int>(iMode) - 1)));
}

SKGInterestObject::ValueDateMode SKGInterestObject::getExpenditueValueDateMode() const
{
    QString mode = getAttribute(QStringLiteral("t_expenditure_value_date_mode"));
    return (mode == QStringLiteral("F") ? FIFTEEN : static_cast<SKGInterestObject::ValueDateMode>(SKGServices::stringToInt(mode) + 1));
}

SKGError SKGInterestObject::setInterestComputationMode(SKGInterestObject::InterestMode iMode)
{
    return setAttribute(QStringLiteral("t_base"),
                        (iMode == FIFTEEN24 ? QStringLiteral("24") : (iMode == DAYS360 ? QStringLiteral("360") : QStringLiteral("365"))));
}

SKGInterestObject::InterestMode SKGInterestObject::getInterestComputationMode() const
{
    QString mode = getAttribute(QStringLiteral("t_base"));
    return (mode == QStringLiteral("24") ? FIFTEEN24 : (mode == QStringLiteral("360") ? DAYS360 : DAYS365));
}

QString SKGInterestObject::getWhereclauseId() const
{
    // Could we use the id
    QString output = SKGObjectBase::getWhereclauseId();
    if (output.isEmpty()) {
        // No, so we use the date and parent
        if (!(getAttribute(QStringLiteral("d_date")).isEmpty()) && !(getAttribute(QStringLiteral("rd_account_id")).isEmpty())) {
            output = "d_date='" % getAttribute(QStringLiteral("d_date")) % "' AND rd_account_id=" % getAttribute(QStringLiteral("rd_account_id"));
        }
    }
    return output;
}

SKGError SKGInterestObject::setAccount(const SKGAccountObject &iAccount)
{
    return setAttribute(QStringLiteral("rd_account_id"), SKGServices::intToString(iAccount.getID()));
}

SKGError SKGInterestObject::getAccount(SKGAccountObject &oAccount) const
{
    SKGError err = getDocument()->getObject(QStringLiteral("v_account"), "id=" % getAttribute(QStringLiteral("rd_account_id")), oAccount);
    return err;
}
