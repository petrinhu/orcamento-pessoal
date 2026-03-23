/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGAdvice.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgadvice.h"
#include "skgdefine.h"
#include <utility>

SKGAdvice::SKGAdvice()
    : QObject()
{
}

SKGAdvice::SKGAdvice(const SKGAdvice &iAdvice)
    : QObject()
    , m_uuid(iAdvice.m_uuid)
    , m_priority(iAdvice.m_priority)
    , m_shortMessage(iAdvice.m_shortMessage)
    , m_longMessage(iAdvice.m_longMessage)
    , m_autoCorrections(iAdvice.m_autoCorrections)

{
}

SKGAdvice::~SKGAdvice() = default;

SKGAdvice &SKGAdvice::operator=(const SKGAdvice &iAdvice)
{
    if (&iAdvice != this) {
        m_priority = iAdvice.m_priority;
        m_shortMessage = iAdvice.m_shortMessage;
        m_longMessage = iAdvice.m_longMessage;
        m_autoCorrections = iAdvice.m_autoCorrections;
        m_uuid = iAdvice.m_uuid;
        Q_EMIT modified();
    }
    return *this;
}

void SKGAdvice::setUUID(const QString &iUUID)
{
    if (m_uuid != iUUID) {
        m_uuid = iUUID;
        Q_EMIT modified();
    }
}

QString SKGAdvice::getUUID() const
{
    return m_uuid;
}

void SKGAdvice::setPriority(int iPriority)
{
    if (m_priority != iPriority) {
        m_priority = iPriority;
        Q_EMIT modified();
    }
}

int SKGAdvice::getPriority() const
{
    return m_priority;
}

void SKGAdvice::setShortMessage(const QString &iMessage)
{
    if (m_shortMessage != iMessage) {
        m_shortMessage = iMessage;
        Q_EMIT modified();
    }
}

QString SKGAdvice::getShortMessage() const
{
    return m_shortMessage;
}

void SKGAdvice::setLongMessage(const QString &iMessage)
{
    if (m_longMessage != iMessage) {
        m_longMessage = iMessage;
        Q_EMIT modified();
    }
}

QString SKGAdvice::getLongMessage() const
{
    return m_longMessage;
}

void SKGAdvice::setAutoCorrections(const QStringList &iCorrections)
{
    SKGAdvice::SKGAdviceActionList tmp;
    tmp.reserve(iCorrections.count());
    for (const auto &c : std::as_const(iCorrections)) {
        SKGAdviceAction a;
        a.Title = c;
        a.IsRecommended = false;
        tmp.push_back(a);
    }

    setAutoCorrections(tmp);
}

void SKGAdvice::setAutoCorrections(const SKGAdvice::SKGAdviceActionList &iCorrections)
{
    m_autoCorrections = iCorrections;
    Q_EMIT modified();
}

SKGAdvice::SKGAdviceActionList SKGAdvice::getAutoCorrections() const
{
    return m_autoCorrections;
}
