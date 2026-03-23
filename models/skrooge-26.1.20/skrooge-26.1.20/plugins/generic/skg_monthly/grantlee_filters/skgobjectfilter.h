/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOBJECTFILTER_H
#define SKGOBJECTFILTER_H
/** @file
 * The grantlee's filter to get attribute of an object.
 *
 * @author Stephane MANKOWSKI
 */
#include <KTextTemplate/Filter>
#include <qobject.h>

/**
 * The grantlee's filter to get attribute of an object
 */
class SKGObjectFilter : public KTextTemplate::Filter
{
public:
    /**
     * Do the filtering
     * @param input the input
     * @param argument the argument
     * @param autoescape the autoescape mode
     * @return the filtered value
     */
    QVariant doFilter(const QVariant &input, const QVariant &argument = QVariant(), bool autoescape = false) const override;

    /**
     * To know if the filter is safe
     * @return true or false
     */
    bool isSafe() const override;
};

#endif // SKGOBJECTFILTER_H
