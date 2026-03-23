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

#ifndef KDEPIM_KDATEVALIDATOR_H
#define KDEPIM_KDATEVALIDATOR_H

#include <qvalidator.h>

#include "skgbasegui_export.h"

namespace KPIM
{

class KDateValidatorPrivate;

class SKGBASEGUI_EXPORT KDateValidator : public QValidator
{
    Q_OBJECT

public:
    enum FixupBehavior {
        NoFixup = 0,
        FixupCurrent,
        FixupForward,
        FixupBackward
    };
    Q_ENUM(FixupBehavior)

    explicit KDateValidator(QObject *iParent = nullptr);
    ~KDateValidator() override;

    void setKeywords(const QStringList &iKeywords);
    QStringList keywords() const;

    void setFixupBehavior(FixupBehavior behavior);
    FixupBehavior fixupBehavior() const;

    /*reimp*/
    State validate(QString &str, int &pos) const override;
    /*reimp*/
    void fixup(QString &input) const override;

private:
    KDateValidatorPrivate *const d;
};

} // namespace KPIM

#endif
