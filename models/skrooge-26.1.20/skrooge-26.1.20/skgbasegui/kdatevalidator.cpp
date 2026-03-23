/*
  This file is part of libkdepim.

  Copyright (c) 2002 Cornelius Schumacher <schumacher@kde.org>
  Copyright (c) 2002 David Jarvie <software@astrojar.org.uk>
  Copyright (c) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>
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

// krazy:excludeall=qclasses as we want to subclass from QComboBox, not KComboBox

#include "kdatevalidator.h"
#include <skgservices.h>

#include <qdatetime.h>
#include <qstringlist.h>

namespace KPIM
{

class KDateValidatorPrivate
{
public:
    KDateValidatorPrivate() = default;

    QStringList keywords;
    KDateValidator::FixupBehavior behavior{KDateValidator::FixupCurrent};
    QString mAlternativeDateFormatToUse;
};

} // namespace KPIM

using namespace KPIM;

KDateValidator::KDateValidator(QObject *iParent)
    : QValidator(iParent)
    , d(new KDateValidatorPrivate)
{
    // Check if we can use the QLocale::ShortFormat
    d->mAlternativeDateFormatToUse = QLocale().dateFormat(QLocale::ShortFormat);
    if (!d->mAlternativeDateFormatToUse.contains(QStringLiteral("yyyy"))) {
        d->mAlternativeDateFormatToUse = d->mAlternativeDateFormatToUse.replace(QStringLiteral("yy"), QStringLiteral("yyyy"));
    }
}

KDateValidator::~KDateValidator()
{
    delete d;
}

QValidator::State KDateValidator::validate(QString &str, int & /*pos*/) const
{
    int length = str.length();

    // empty string is intermediate so one can clear the edit line and start from scratch
    if (length <= 0) {
        return Intermediate;
    }

    if (d->keywords.contains(str.toLower())) {
        return Acceptable;
    }

    QDate date = QLocale().toDate(str, d->mAlternativeDateFormatToUse);
    if (date.isValid()) {
        return Acceptable;
    }
    return Intermediate;
}

void KDateValidator::fixup(QString &input) const
{
    if (d->behavior == NoFixup) {
        return;
    }

    QDate result = SKGServices::partialStringToDate(input, d->behavior == FixupBackward);
    if (result.isValid()) {
        input = QLocale().toString(result, d->mAlternativeDateFormatToUse);
    }
}

void KDateValidator::setKeywords(const QStringList &iKeywords)
{
    d->keywords = iKeywords;
}

QStringList KDateValidator::keywords() const
{
    return d->keywords;
}

void KDateValidator::setFixupBehavior(FixupBehavior behavior)
{
    d->behavior = behavior;
}

KDateValidator::FixupBehavior KDateValidator::fixupBehavior() const
{
    return d->behavior;
}
