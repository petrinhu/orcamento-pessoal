/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A progress bar with colors.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgprogressbar.h"

#include <kcolorscheme.h>
#include <qstringbuilder.h>

SKGProgressBar::SKGProgressBar(QWidget *iParent)
    : QProgressBar(iParent)
    , m_negative(-1)
    , m_neutral(-1)
    , m_positive(-1)
{
    // Define color style
    KColorScheme scheme(QPalette::Normal);
    QString negative = scheme.foreground(KColorScheme::NegativeText).color().name();
    QString neutral = scheme.foreground(KColorScheme::NeutralText).color().name();
    QString positive = scheme.foreground(KColorScheme::PositiveText).color().name();

    m_negativeStyleSheet =
        QStringLiteral(" QProgressBar { text-align: center; padding: 0.5px;} QProgressBar::chunk {text-align: center; border-radius:4px; background-color: ")
        % negative % ";}" % styleSheet();
    m_neutralStyleSheet =
        QStringLiteral(" QProgressBar { text-align: center; padding: 0.5px;} QProgressBar::chunk {text-align: center; border-radius:4px; background-color: ")
        % neutral % ";}" % styleSheet();
    m_positiveStyleSheet =
        QStringLiteral(" QProgressBar { text-align: center; padding: 0.5px;} QProgressBar::chunk {text-align: center; border-radius:4px; background-color: ")
        % positive % ";}" % styleSheet();
}

SKGProgressBar::~SKGProgressBar() = default;

void SKGProgressBar::setLimits(int negative, int neutral, int positive)
{
    m_negative = negative;
    m_neutral = neutral;
    m_positive = positive;

    setValue(value());
}

void SKGProgressBar::setValue(int iValue)
{
    QProgressBar::setValue(iValue);
    if (m_negative <= m_positive) {
        if (iValue <= m_negative) {
            setStyleSheet(m_negativeStyleSheet);
        } else if (iValue <= m_neutral) {
            setStyleSheet(m_neutralStyleSheet);
        } else if (iValue <= m_positive) {
            setStyleSheet(m_positiveStyleSheet);
        }
    } else {
        if (iValue <= m_positive) {
            setStyleSheet(m_positiveStyleSheet);
        } else if (iValue <= m_neutral) {
            setStyleSheet(m_neutralStyleSheet);
        } else if (iValue <= m_negative) {
            setStyleSheet(m_negativeStyleSheet);
        }
    }
}
