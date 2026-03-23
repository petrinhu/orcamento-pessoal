/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGError.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgerror.h"

SKGError::SKGError()
    : m_message(QString())
    , m_property(QString())
{
}

SKGError::SKGError(int iRc, QString iMessage, QString iAction)
    : m_rc(iRc)
    , m_message(std::move(iMessage))
    , m_property(QString())
    , m_action(std::move(iAction))
    , m_previousError(nullptr)
{
}

SKGError::SKGError(const SKGError &iError)
{
    m_rc = iError.m_rc;
    m_message = iError.m_message;
    m_property = iError.m_property;
    m_action = iError.m_action;
    if (Q_UNLIKELY(iError.m_previousError != nullptr)) {
        m_previousError = new SKGError(*iError.m_previousError);
    } else {
        m_previousError = nullptr;
    }
}

SKGError::SKGError(SKGError &&iError) noexcept
{
    m_rc = iError.m_rc;
    m_message = iError.m_message;
    m_property = iError.m_property;
    m_action = iError.m_action;
    m_previousError = iError.m_previousError;
    iError.m_previousError = nullptr;
}

SKGError::~SKGError()
{
    delete m_previousError;
    m_previousError = nullptr;
}

SKGError &SKGError::operator=(const SKGError &iError)
{
    if (Q_LIKELY(&iError != this)) {
        delete m_previousError;
        m_previousError = nullptr;

        m_rc = iError.m_rc;
        m_message = iError.m_message;
        m_property = iError.m_property;
        m_action = iError.m_action;
        if (Q_UNLIKELY(iError.m_previousError != nullptr)) {
            m_previousError = new SKGError(*iError.m_previousError);
        }
    }
    return *this;
}

bool SKGError::operator!() const
{
    return isSucceeded();
}

SKGError::operator bool() const
{
    return isFailed();
}

bool SKGError::isFailed() const
{
    return (m_rc > 0);
}

bool SKGError::isSucceeded() const
{
    return (m_rc <= 0);
}

bool SKGError::isWarning() const
{
    return (m_rc < 0);
}

int SKGError::getReturnCode() const
{
    return m_rc;
}

SKGError &SKGError::setReturnCode(int iReturnCode)
{
    m_rc = iReturnCode;
    return *this;
}

SKGError &SKGError::setMessage(const QString &iMessage)
{
    m_message = iMessage;
    return *this;
}

QString SKGError::getMessage() const
{
    return m_message;
}

SKGError &SKGError::setProperty(const QString &iProperty)
{
    m_property = iProperty;
    return *this;
}

QString SKGError::getProperty() const
{
    return m_property;
}

QString SKGError::getFullMessage() const
{
    QString output('[');
    output += (m_rc == 0 ? QStringLiteral("SUC") : (m_rc > 0 ? QStringLiteral("ERR") : QStringLiteral("WAR")));
    output += '-';

    QString tmp;
    tmp.setNum(m_rc);
    output += tmp;
    output += ']';
    if (Q_LIKELY(!m_message.isEmpty())) {
        output += ": " % m_message;
    }
    return output;
}

QString SKGError::getFullMessageWithHistorical() const
{
    QString output = getFullMessage();
    if (Q_UNLIKELY(m_previousError)) {
        output += '\n' % m_previousError->getFullMessageWithHistorical();
    }
    return output;
}

int SKGError::getHistoricalSize() const
{
    int output = 0;
    if (Q_UNLIKELY(m_previousError)) {
        output += 1 + m_previousError->getHistoricalSize();
    }
    return output;
}

SKGError &SKGError::setAction(const QString &iAction)
{
    if (m_action != iAction) {
        m_action = iAction;
    }
    return *this;
}

QString SKGError::getAction() const
{
    return m_action;
}

SKGError &SKGError::addError(int iRc, const QString &iMessage, const QString &iAction)
{
    auto pe = new SKGError(*this);
    setReturnCode(iRc);
    setMessage(iMessage);
    setAction(iAction);
    delete m_previousError;
    m_previousError = pe;
    return *this;
}

SKGError &SKGError::addError(const SKGError &iError)
{
    return addError(iError.getReturnCode(), iError.getMessage());
}

SKGError *SKGError::getPreviousError() const
{
    return m_previousError;
}
