/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A simple period selector.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgsimpleperiodedit.h"
#include <skgservices.h>

SKGSimplePeriodEdit::SKGSimplePeriodEdit(QWidget *iParent)
    : SKGComboBox(iParent)
    , m_Mode(PREVIOUS_MONTHS)
{
}

SKGSimplePeriodEdit::~SKGSimplePeriodEdit() = default;

QDate SKGSimplePeriodEdit::firstDate() const
{
    return m_FirstDate;
}

void SKGSimplePeriodEdit::setFirstDate(QDate iDate)
{
    if (m_FirstDate != iDate) {
        m_FirstDate = iDate;

        refreshList();
        Q_EMIT changed();
    }
}
SKGSimplePeriodEdit::Modes SKGSimplePeriodEdit::mode() const
{
    return m_Mode;
}
void SKGSimplePeriodEdit::setMode(SKGSimplePeriodEdit::Modes iMode)
{
    if (m_Mode != iMode) {
        m_Mode = iMode;

        refreshList();
        Q_EMIT changed();
    }
}

QString SKGSimplePeriodEdit::period() const
{
    QString period = currentData().toString();
    if (period.isEmpty()) {
        // Specific month
        period = text();
    }
    return period;
}

void SKGSimplePeriodEdit::refreshList()
{
    QDate today = QDate::currentDate();
    QDate c = SKGServices::periodToDate(SKGServices::dateToPeriod(m_FirstDate, QStringLiteral("M")));
    if (!c.isValid()) {
        c = today;
    }
    c = c.addDays(1 - c.day());

    QString smonth = SKGServices::dateToPeriod(today, QStringLiteral("M"));
    QString squarter = SKGServices::dateToPeriod(today, QStringLiteral("Q"));
    QString ssemester = SKGServices::dateToPeriod(today, QStringLiteral("S"));
    QString syear = SKGServices::dateToPeriod(today, QStringLiteral("Y"));

    // Build the list
    QStringList list;
    do {
        QString cmonth = SKGServices::dateToPeriod(c, QStringLiteral("M"));
        QString cquarter = SKGServices::dateToPeriod(c, QStringLiteral("Q"));
        QString csemester = SKGServices::dateToPeriod(c, QStringLiteral("S"));
        QString cyear = SKGServices::dateToPeriod(c, QStringLiteral("Y"));
        if ((cmonth != smonth && (((m_Mode & PREVIOUS_MONTHS)) != 0u)) || (cmonth == smonth && (((m_Mode & CURRENT_MONTH)) != 0u))) {
            list.insert(0, cmonth);
        }
        if (!list.contains(cquarter)
            && ((cquarter != squarter && (((m_Mode & PREVIOUS_PERIODS)) != 0u)) || (cquarter == squarter && (((m_Mode & CURRENT_PERIOD)) != 0u)))) {
            list.insert(0, cquarter);
        }
        if (!list.contains(csemester)
            && ((csemester != ssemester && (((m_Mode & PREVIOUS_PERIODS)) != 0u)) || (csemester == ssemester && (((m_Mode & CURRENT_PERIOD)) != 0u)))) {
            list.insert(0, csemester);
        }
        if (!list.contains(cyear)
            && ((cyear != syear && ((((m_Mode & PREVIOUS_PERIODS)) != 0u) || (((m_Mode & PREVIOUS_YEARS)) != 0u)))
                || (cyear == syear && ((((m_Mode & CURRENT_PERIOD)) != 0u) || (((m_Mode & CURRENT_YEAR)) != 0u))))) {
            list.insert(0, cyear);
        }
        if (cmonth == smonth || c >= today) {
            break;
        }
        c = c.addMonths(1);
    } while (true);

    // Set the list and the current item
    QString current = text();
    bool previous = blockSignals(true);
    clear();

    // Add dynamic items
    if (list.contains(smonth) && (((m_Mode & ALL)) != 0u)) {
        addItem(i18nc("A period including all dates", "All dates"), "ALL");
    }

    if (list.contains(smonth)) {
        addItem(i18nc("The current month", "Current month"), smonth);
    }

    if (list.contains(squarter)) {
        addItem(i18nc("The current quarter", "Current quarter"), squarter);
    }

    if (list.contains(ssemester)) {
        addItem(i18nc("The current semester", "Current semester"), ssemester);
    }

    if (list.contains(syear)) {
        addItem(i18nc("The current year", "Current year"), syear);
    }

    QString period = SKGServices::getNeighboringPeriod(smonth);
    if (list.contains(period)) {
        addItem(i18nc("The month before the current month", "Last month"), period);
    }

    period = SKGServices::getNeighboringPeriod(squarter);
    if (list.contains(period)) {
        addItem(i18nc("The quarter before the current quarter", "Last quarter"), period);
    }

    period = SKGServices::getNeighboringPeriod(ssemester);
    if (list.contains(period)) {
        addItem(i18nc("The semester before the current semester", "Last semester"), period);
    }

    period = SKGServices::getNeighboringPeriod(syear);
    if (list.contains(period)) {
        addItem(i18nc("The year before the current year", "Last year"), period);
    }
    addItems(list);

    if (!current.isEmpty()) {
        setText(current);
    } else {
        setCurrentIndex(0);
    }
    blockSignals(previous);
}
