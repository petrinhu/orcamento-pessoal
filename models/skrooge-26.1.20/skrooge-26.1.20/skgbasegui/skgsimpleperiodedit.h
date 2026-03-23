/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSIMPLEPERIODEDIT_H
#define SKGSIMPLEPERIODEDIT_H
/** @file
 * A simple period selector.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qdatetime.h>
#include <qflags.h>

#include "skgbasegui_export.h"
#include "skgcombobox.h"
/**
 * This file is a simple period selector.
 */
class SKGBASEGUI_EXPORT SKGSimplePeriodEdit : public SKGComboBox
{
    Q_OBJECT
    /**
     * First date
     */
    Q_PROPERTY(QDate firstDate READ firstDate WRITE setFirstDate NOTIFY changed USER true)

    /**
     * Previous mode
     */
    Q_PROPERTY(Modes mode READ mode WRITE setMode NOTIFY changed)

    /**
     * Period
     */
    Q_PROPERTY(QString period READ period NOTIFY changed)

public:
    /**
     * This enumerate for mode
     */
    enum Mode {
        NONE = 0u, /**< None */
        PREVIOUS_MONTHS = 1u, /**< Only previous months */
        PREVIOUS_YEARS = 32u, /**< Only previous years */
        PREVIOUS_PERIODS = 2u, /**< All previous periods including quarters, semesters and years*/
        CURRENT_MONTH = 4u, /**< Current month */
        CURRENT_YEAR = 64u, /**< Current years */
        CURRENT_PERIOD = 8u, /**< Current period including quarter, semester and year */
        ALL = 16u, /**< The "All dates" period */
        PREVIOUS_AND_CURRENT_MONTHS = 1u | 4u, /**< Previous and current months*/
        PREVIOUS_AND_CURRENT_YEARS = 32u | 64u, /**< Previous and current years*/
        PREVIOUS_AND_CURRENT_PERIODS = 1u | 2u | 4u | 8u, /**< Previous and current periods including quarters, semesters and years*/
        ALL_PERIODS = 1u | 2u | 4u | 8u | 16u /**< All periods*/
    };

    /**
     * This enumerate for additional options in menu
     */
    Q_ENUM(Mode)

    Q_DECLARE_FLAGS(Modes, Mode)

    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGSimplePeriodEdit(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGSimplePeriodEdit() override;

    /**
     * Get the period
     * @return the period
     */
    virtual QString period() const;

    /**
     * Get the first date to take into account
     * @return the first date to take into account
     */
    virtual QDate firstDate() const;

    /**
     * Set the first date to take into account
     * @param iDate the first date to take into account
     */
    // cppcheck-suppress passedByValue
    virtual void setFirstDate(QDate iDate);

    /**
     * To know if the mode of widget (default: PREVIOUS_MONTHS)
     * @return previous mode
     */
    virtual Modes mode() const;

    /**
     * Set the widget mode
     * @param iMode the mode
     */
    virtual void setMode(Modes iMode);

Q_SIGNALS:
    /**
     * Emitted when the changed
     */
    void changed();

private:
    QDate m_FirstDate;
    Modes m_Mode;

    void refreshList();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(SKGSimplePeriodEdit::Modes)

#endif // SKGSIMPLEPERIODEDIT_H
