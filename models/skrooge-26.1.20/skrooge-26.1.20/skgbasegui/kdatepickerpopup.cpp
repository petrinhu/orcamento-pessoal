/*
  This file is part of libkdepim.

  Copyright (c) 2004 Bram Schoenmakers <bramschoenmakers@kde.nl>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#include "kdatepickerpopup.h"

#include <kdatepicker.h>
#include <klocalizedstring.h>

#include <qdatetime.h>
#include <qwidgetaction.h>

using namespace KPIM;

class KDatePickerAction : public QWidgetAction // clazy:exclude=missing-qobject-macro
{
public:
    KDatePickerAction(KDatePicker *widget, QObject *iParent)
        : QWidgetAction(iParent)
        , mDatePicker(widget)
        , mOriginalParent(widget->parentWidget())
    {
    }

protected:
    QWidget *createWidget(QWidget *iParent) override
    {
        mDatePicker->setParent(iParent);
        return mDatePicker;
    }

    void deleteWidget(QWidget *widget) override
    {
        if (widget != mDatePicker) {
            return;
        }

        mDatePicker->setParent(mOriginalParent);
    }

private:
    KDatePicker *mDatePicker;
    QWidget *mOriginalParent;
};

KDatePickerPopup::KDatePickerPopup(Items iItems, QDate iDate, QWidget *iParent)
    : QMenu(iParent)
{
    mItems = iItems;

    mDatePicker = new KDatePicker(this);
    mDatePicker->setCloseButton(false);

    connect(mDatePicker, &KDatePicker::dateEntered, this, &KDatePickerPopup::slotDateChanged);
    connect(mDatePicker, &KDatePicker::dateSelected, this, &KDatePickerPopup::slotDateChanged);

    mDatePicker->setDate(iDate);

    buildMenu();
}

void KDatePickerPopup::buildMenu()
{
    if (isVisible()) {
        return;
    }
    clear();

    if ((mItems & DatePicker) != 0u) {
        addAction(new KDatePickerAction(mDatePicker, this));

        if ((mItems & NoDate) != 0u || (mItems & Words) != 0u) {
            addSeparator();
        }
    }

    if ((mItems & Words) != 0u) {
        addAction(i18nc("@option yesterday", "&Yesterday"), this, &KDatePickerPopup::slotYesterday);
        addAction(i18nc("@option today", "&Today"), this, &KDatePickerPopup::slotToday);
        addAction(i18nc("@option tomorrow", "To&morrow"), this, &KDatePickerPopup::slotTomorrow);
        addAction(i18nc("@option next week", "Next &Week"), this, &KDatePickerPopup::slotNextWeek);
        addAction(i18nc("@option next month", "Next M&onth"), this, &KDatePickerPopup::slotNextMonth);

        if ((mItems & NoDate) != 0u) {
            addSeparator();
        }
    }

    if ((mItems & NoDate) != 0u) {
        addAction(i18nc("@option do not specify a date", "No Date"), this, &KDatePickerPopup::slotNoDate);
    }
}

KDatePicker *KDatePickerPopup::datePicker() const
{
    return mDatePicker;
}

void KDatePickerPopup::setDate(QDate date)
{
    mDatePicker->setDate(date);
}

#if 0
void KDatePickerPopup::setItems(int items)
{
    mItems = items;
    buildMenu();
}
#endif

void KDatePickerPopup::slotDateChanged(QDate date)
{
    Q_EMIT dateChanged(date);
    hide();
}

void KDatePickerPopup::slotYesterday()
{
    Q_EMIT dateChanged(QDate::currentDate().addDays(-1));
}

void KDatePickerPopup::slotToday()
{
    Q_EMIT dateChanged(QDate::currentDate());
}

void KDatePickerPopup::slotTomorrow()
{
    Q_EMIT dateChanged(QDate::currentDate().addDays(1));
}

void KDatePickerPopup::slotNoDate()
{
    Q_EMIT dateChanged(QDate());
}

void KDatePickerPopup::slotNextWeek()
{
    Q_EMIT dateChanged(QDate::currentDate().addDays(7));
}

void KDatePickerPopup::slotNextMonth()
{
    Q_EMIT dateChanged(QDate::currentDate().addMonths(1));
}
