/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * The grantlee's filter to get attribute of an object.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgobjectfilter.h"
#include "skgobjectbase.h"
#include "skgtraces.h"

#include "ktexttemplate/util.h"

QVariant SKGObjectFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    SKGObjectBase obj = input.value<SKGObjectBase>();
    return obj.getAttribute(KTextTemplate::getSafeString(argument));
}

bool SKGObjectFilter::isSafe() const
{
    return true;
}
