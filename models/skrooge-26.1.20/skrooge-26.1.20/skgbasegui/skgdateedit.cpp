/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A date editor with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdateedit.h"

#include <kdatevalidator.h>
#include <qlineedit.h>

#include <klocalizedstring.h>

SKGDateEdit::SKGDateEdit(QWidget *iParent, const char *name)
    : KPIM::KDateEdit(iParent)
    , m_mode(CURRENT)
{
    setObjectName(name);
    setMode(CURRENT);
    setToolTip(i18n("Date of the transaction\nup or down to add or remove one day\nCTRL + up or CTRL + down to add or remove one month"));
}

SKGDateEdit::~SKGDateEdit() = default;

SKGDateEdit::Mode SKGDateEdit::mode() const
{
    return m_mode;
}

void SKGDateEdit::setMode(Mode iMode)
{
    if (iMode != m_mode) {
        m_mode = iMode;

        auto *val = qobject_cast<KPIM::KDateValidator *>(const_cast<QValidator *>(validator()));
        val->setFixupBehavior(m_mode == CURRENT    ? KPIM::KDateValidator::FixupCurrent
                                  : m_mode == NEXT ? KPIM::KDateValidator::FixupForward
                                                   : KPIM::KDateValidator::FixupBackward);

        Q_EMIT modeChanged();
    }
}
