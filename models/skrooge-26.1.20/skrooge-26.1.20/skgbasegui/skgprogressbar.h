/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPROGRESSBAR_H
#define SKGPROGRESSBAR_H
/** @file
 * A progress bar with colors
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbasegui_export.h"
#include <qprogressbar.h>

/**
 * A progress bar with colors
 */
class SKGBASEGUI_EXPORT SKGProgressBar : public QProgressBar
{
    Q_OBJECT

    /**
     * Property value
     */
    Q_PROPERTY(int value READ value WRITE setValue) // clazy:exclude=qproperty-without-notify

public:
    /**
     * Constructor
     * @param iParent the parent
     */
    explicit SKGProgressBar(QWidget *iParent);

    /**
     * Destructor
     */
    ~SKGProgressBar() override;

    /**
     * Set the limits
     * @param negative if the value @see setValue is less than @param negative the color will be negative
     * @param neutral if the value @see setValue is less than @param neutral the color will be neutral
     * @param positive if the value @see setValue is less than @param positive the color will be positive
     */
    virtual void setLimits(int negative, int neutral, int positive);

    /**
     * Set the value
     * @param iValue the value
     */
    virtual void setValue(int iValue);

private:
    int m_negative;
    int m_neutral;
    int m_positive;
    QString m_negativeStyleSheet;
    QString m_neutralStyleSheet;
    QString m_positiveStyleSheet;
};

#endif
