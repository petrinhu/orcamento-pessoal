/*
  This file is part of libkdepim.

  Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2002 David Jarvie <software@astrojar.org.uk>
  Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

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

#ifndef KDEPIM_KDATEEDIT_H
#define KDEPIM_KDATEEDIT_H

#include "kdatepickerpopup.h"

#include <qcombobox.h>
#include <qdatetime.h>
#include <qevent.h>
#include <qmap.h>

#include "skgbasegui_export.h"

class QEvent;

namespace KPIM
{

/**
  A date editing widget that consists of an editable combo box.
  The combo box contains the date in text form, and clicking the combo
  box arrow will display a 'popup' style date picker.

  This widget also supports advanced features like allowing the user
  to type in the day name to get the date. The following keywords
  are supported (in the native language): tomorrow, yesterday, today,
  monday, tuesday, wednesday, thursday, friday, saturday, sunday.

  @author Cornelius Schumacher <schumacher@kde.org>
  @author Mike Pilone <mpilone@slac.com>
  @author David Jarvie <software@astrojar.org.uk>
  @author Tobias Koenig <tokoe@kde.org>
*/
class SKGBASEGUI_EXPORT KDateEdit : public QComboBox // krazy:exclude=qclasses
{
    Q_OBJECT
    Q_PROPERTY(QDate date READ date WRITE setDate NOTIFY dateChanged)

public:
    explicit KDateEdit(QWidget *iParent);
    ~KDateEdit() override;

    /**
      @return The date entered. This date could be invalid,
              you have to check validity yourself.
     */
    QDate date() const;

    /**
      Sets whether the widget is read-only for the user. If read-only, the
      date pop-up is inactive, and the displayed date cannot be edited.

      @param readOnly True to set the widget read-only, false to set it read-write.
     */
    void setReadOnly(bool readOnly);

    /**
      @return True if the widget is read-only, false if read-write.
     */
    bool isReadOnly() const;

    void showPopup() override;

Q_SIGNALS:
    /**
      This signal is emitted whenever the user has entered a new date.
      When the user changes the date by editing the line edit field,
      the signal is not emitted until focus leaves the line edit field.
      The passed date can be invalid.
     */
    // cppcheck-suppress passedByValue
    void dateEntered(QDate date);

    /**
      This signal is emitted whenever the user modifies the date.
      The passed date can be invalid.
     */
    // cppcheck-suppress passedByValue
    void dateChanged(QDate date);

public Q_SLOTS:
    /**
      Sets the date.

      @param iDate The new date to display. This date must be valid or
                  it will not be set
     */
    // cppcheck-suppress passedByValue
    void setDate(QDate iDate);

protected Q_SLOTS:
    void lineEnterPressed();
    void slotTextChanged(const QString & /*unused*/);
    void dateSelected(QDate /*iDate*/);

protected:
    bool eventFilter(QObject * /*watched*/ /*iObject*/, QEvent * /*iEvent*/ /*event*/) override;
    void focusOutEvent(QFocusEvent * /*e*/) override;
    void keyPressEvent(QKeyEvent * /*e*/) override;

    /**
      Sets the date, without altering the display.
      This method is used internally to set the widget's date value.
      As a virtual method, it allows derived classes to perform additional
      validation on the date value before it is set. Derived classes should
      return true if QDate::isValid(@p date) returns false.

      @param iDate The new date to set.
      @return True if the date was set, false if it was considered invalid and
              remains unchanged.
     */
    // cppcheck-suppress passedByValue
    virtual bool assignDate(QDate iDate);

    /**
      Fills the keyword map. Reimplement it if you want additional keywords.
     */
    void setupKeywords();

private:
    QDate parseDate(bool *replaced = nullptr) const;
    void updateView();

    KDatePickerPopup *mPopup;

    QDate mDate;
    bool mReadOnly;
    bool mTextChanged;
    QString mAlternativeDateFormatToUse;

    QMap<QString, int> mKeywordMap;
};

} // namespace KPIM

#endif
