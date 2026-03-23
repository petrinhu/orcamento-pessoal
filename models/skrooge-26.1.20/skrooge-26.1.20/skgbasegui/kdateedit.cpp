/*
  This file is part of libkdepim.

  Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2002 David Jarvie <software@astrojar.org.uk>
  Copyright (c) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
  Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either

  This library is distributed in the hope that it will be useful,
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

// krazy:excludeall=qclasses as we want to subclass from QComboBox, not KComboBox

#include "kdateedit.h"

#include <klocalizedstring.h>

#include <qabstractitemview.h>
#include <qapplication.h>
#include <qcompleter.h>
#include <qevent.h>
#include <qlineedit.h>
#include <qscreen.h>
#include <qvalidator.h>

#include "kdatevalidator.h"

using namespace KPIM;

KDateEdit::KDateEdit(QWidget *iParent)
    : QComboBox(iParent)
    , mReadOnly(false)
{
    // need at least one entry for popup to work
    setMaxCount(1);
    setEditable(true);

    // Check if we can use the QLocale::ShortFormat
    mAlternativeDateFormatToUse = QLocale().dateFormat(QLocale::ShortFormat);
    if (!mAlternativeDateFormatToUse.contains(QStringLiteral("yyyy"))) {
        mAlternativeDateFormatToUse = mAlternativeDateFormatToUse.replace(QStringLiteral("yy"), QStringLiteral("yyyy"));
    }

    mDate = QDate::currentDate();
    QString today = QLocale().toString(mDate, mAlternativeDateFormatToUse);

    addItem(today);
    setCurrentIndex(0);

    connect(lineEdit(), &QLineEdit::returnPressed, this, &KDateEdit::lineEnterPressed);
    connect(this, &KDateEdit::editTextChanged, this, &KDateEdit::slotTextChanged);

    mPopup = new KDatePickerPopup(KDatePickerPopup::DatePicker | KDatePickerPopup::Words, QDate::currentDate(), this);
    mPopup->hide();
    mPopup->installEventFilter(this);

    connect(mPopup, &KDatePickerPopup::dateChanged, this, &KDateEdit::dateSelected);

    // handle keyword entry
    setupKeywords();
    lineEdit()->installEventFilter(this);

    auto newValidator = new KDateValidator(this);
    newValidator->setKeywords(mKeywordMap.keys());
    setValidator(newValidator);

    mTextChanged = false;
}

KDateEdit::~KDateEdit() = default;

void KDateEdit::setDate(QDate iDate)
{
    assignDate(iDate);
    updateView();
}

QDate KDateEdit::date() const
{
    return mDate;
}

void KDateEdit::setReadOnly(bool readOnly)
{
    mReadOnly = readOnly;
    lineEdit()->setReadOnly(readOnly);
}

bool KDateEdit::isReadOnly() const
{
    return mReadOnly;
}

void KDateEdit::showPopup()
{
    if (mReadOnly) {
        return;
    }

    QRect desk = QGuiApplication::primaryScreen()->geometry();

    QPoint popupPoint = mapToGlobal(QPoint(0, 0));

    int dateFrameHeight = mPopup->sizeHint().height();
    if (popupPoint.y() + height() + dateFrameHeight > desk.bottom()) {
        popupPoint.setY(popupPoint.y() - dateFrameHeight);
    } else {
        popupPoint.setY(popupPoint.y() + height());
    }

    int dateFrameWidth = mPopup->sizeHint().width();
    if (popupPoint.x() + dateFrameWidth > desk.right()) {
        popupPoint.setX(desk.right() - dateFrameWidth);
    }

    if (popupPoint.x() < desk.left()) {
        popupPoint.setX(desk.left());
    }

    if (popupPoint.y() < desk.top()) {
        popupPoint.setY(desk.top());
    }

    if (mDate.isValid()) {
        mPopup->setDate(mDate);
    } else {
        mPopup->setDate(QDate::currentDate());
    }

    mPopup->popup(popupPoint);

    // The combo box is now shown pressed. Make it show not pressed again
    // by causing its (invisible) list box to Q_EMIT a 'selected' signal.
    // First, ensure that the list box contains the date currently displayed.
    QDate date2 = parseDate();
    assignDate(date2);
    updateView();

    // Now, simulate an Enter to unpress it
    QAbstractItemView *lb = view();
    if (lb != nullptr) {
        lb->setCurrentIndex(lb->model()->index(0, 0));
        QKeyEvent *keyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
        QApplication::postEvent(lb, keyEvent);
    }
}

void KDateEdit::dateSelected(QDate iDate)
{
    if (assignDate(iDate)) {
        updateView();
        Q_EMIT dateChanged(iDate);
        Q_EMIT dateEntered(iDate);

        if (iDate.isValid()) {
            mPopup->hide();
        }
    }
}

void KDateEdit::lineEnterPressed()
{
    bool replaced = false;

    QDate date2 = parseDate(&replaced);

    if (assignDate(date2)) {
        if (replaced) {
            updateView();
        }

        Q_EMIT dateChanged(date2);
        Q_EMIT dateEntered(date2);
    }
}

QDate KDateEdit::parseDate(bool *replaced) const
{
    QString text = currentText();
    QDate result;

    if (replaced != nullptr) {
        (*replaced) = false;
    }

    if (text.isEmpty()) {
        result = QDate();
    } else if (mKeywordMap.contains(text.toLower())) {
        QDate today = QDate::currentDate();
        int i = mKeywordMap[text.toLower()];
        if (i == 30) {
            today = today.addMonths(1);
        } else if (i >= 100) {
            /* A day name has been entered. Convert to offset from today.
             * This uses some math tricks to figure out the offset in days
             * to the next date the given day of the week occurs. There
             * are two cases, that the new day is >= the current day, which means
             * the new day has not occurred yet or that the new day < the current day,
             * which means the new day is already passed (so we need to find the
             * day in the next week).
             */
            i -= 100;
            int currentDay = today.dayOfWeek();
            if (i >= currentDay) {
                i -= currentDay;
            } else {
                i += 7 - currentDay;
            }
        }

        result = today.addDays(i);
        if (replaced != nullptr) {
            (*replaced) = true;
        }
    } else {
        result = QLocale().toDate(text, mAlternativeDateFormatToUse);
    }

    return result;
}

void KDateEdit::focusOutEvent(QFocusEvent *e)
{
    if (mTextChanged) {
        lineEnterPressed();
        mTextChanged = false;
    }
    QComboBox::focusOutEvent(e);
}

void KDateEdit::keyPressEvent(QKeyEvent *e)
{
    QDate date2;

    if (!mReadOnly) {
        switch (e->key()) {
        case Qt::Key_Up:
            date2 = parseDate();
            if (!date2.isValid()) {
                break;
            }
            if ((e->modifiers() & Qt::ControlModifier) != 0u) { // Warning OSX: The KeypadModifier value will also be set when an arrow key is pressed as the
                                                                // arrow keys are considered part of the keypad.
                date2 = date2.addMonths(1);
            } else {
                date2 = date2.addDays(1);
            }
            break;
        case Qt::Key_Down:
            date2 = parseDate();
            if (!date2.isValid()) {
                break;
            }
            if ((e->modifiers() & Qt::ControlModifier) != 0u) { // Warning OSX: The KeypadModifier value will also be set when an arrow key is pressed as the
                                                                // arrow keys are considered part of the keypad.
                date2 = date2.addMonths(-1);
            } else {
                date2 = date2.addDays(-1);
            }
            break;
        case Qt::Key_PageUp:
            date2 = parseDate();
            if (!date2.isValid()) {
                break;
            }
            date2 = date2.addMonths(1);
            break;
        case Qt::Key_PageDown:
            date2 = parseDate();
            if (!date2.isValid()) {
                break;
            }
            date2 = date2.addMonths(-1);
            break;
        case Qt::Key_Equal:
            date2 = QDate::currentDate();
            break;
        default: {
        }
        }

        if (date2.isValid() && assignDate(date2)) {
            e->accept();
            updateView();
            Q_EMIT dateChanged(date2);
            Q_EMIT dateEntered(date2);
            return;
        }
    }

    QComboBox::keyPressEvent(e);
}

bool KDateEdit::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if (iObject == lineEdit()) {
        // We only process the focus out event if the text has changed
        // since we got focus
        if ((iEvent->type() == QEvent::FocusOut) && mTextChanged) {
            lineEnterPressed();
            mTextChanged = false;
        } else if (iEvent->type() == QEvent::KeyPress) {
            // Up and down arrow keys step the date
            auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
            if (keyEvent && (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter))) {
                lineEnterPressed();
                return true;
            }
        }
    }

    return QComboBox::eventFilter(iObject, iEvent);
}

void KDateEdit::slotTextChanged(const QString & /*unused*/)
{
    QDate date2 = parseDate();

    if (assignDate(date2)) {
        Q_EMIT dateChanged(date2);
    }

    mTextChanged = true;
}

void KDateEdit::setupKeywords()
{
    // Create the keyword list. This will be used to match against when the user
    // enters information.
    mKeywordMap.insert(i18nc("the day after today", "tomorrow"), 1);
    mKeywordMap.insert(i18nc("this day", "today"), 0);
    mKeywordMap.insert(i18nc("the day before today", "yesterday"), -1);
    mKeywordMap.insert(i18nc("the week after this week", "next week"), 7);
    mKeywordMap.insert(i18nc("the month after this month", "next month"), 30);

    QString dayName;
    for (int i = 1; i <= 7; ++i) {
        dayName = QLocale().dayName(i).toLower();
        mKeywordMap.insert(dayName, i + 100);
    }

    auto comp = new QCompleter(mKeywordMap.keys(), this);
    comp->setCaseSensitivity(Qt::CaseInsensitive);
    comp->setCompletionMode(QCompleter::InlineCompletion);
    setCompleter(comp);
}

bool KDateEdit::assignDate(QDate iDate)
{
    mDate = iDate;
    mTextChanged = false;
    return true;
}

void KDateEdit::updateView()
{
    QString dateString;
    if (mDate.isValid()) {
        dateString = QLocale().toString(mDate, mAlternativeDateFormatToUse);
    }

    // We do not want to generate a signal here,
    // since we explicitly setting the date
    bool blocked = signalsBlocked();
    blockSignals(true);
    removeItem(0);
    insertItem(0, dateString);
    blockSignals(blocked);
}
