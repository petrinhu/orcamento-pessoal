/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPERIODEDIT_H
#define SKGPERIODEDIT_H
/** @file
 * A period editor.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include "ui_skgperiodedit.h"
/**
 * This file is a period editor.
 */
class SKGBASEGUI_EXPORT SKGPeriodEdit : public QWidget
{
    Q_OBJECT

    /**
     * Where clause
     */
    Q_PROPERTY(QString whereClause READ getWhereClause NOTIFY changed)

    /**
     * Text
     */
    Q_PROPERTY(QString text READ text NOTIFY changed)

public:
    /**
     * This enumerate defines the period mode
     */
    enum PeriodMode {
        ALL, /**< All date*/
        CURRENT, /**< Current month, year, …*/
        PREVIOUS, /**< Previous month, year, …*/
        LAST, /**< Last month, year, …*/
        CUSTOM, /**< Custom dates*/
        TIMELINE /**< Timeline.*/
    };
    /**
     * This enumerate defines the period mode
     */
    Q_ENUM(PeriodMode)

    /**
     * This enumerate defines the period mode
     */
    enum PeriodInterval {
        DAY = 0, /**< Day*/
        WEEK = 1, /**< Week (7 days)*/
        MONTH = 2, /**< Month*/
        QUARTER = 4, /**< Quarter (3 months)*/
        SEMESTER = 5, /**< Semester (6 months)*/
        YEAR = 3 /**< Year*/
    };
    /**
     * This enumerate defines the period interval
     */
    Q_ENUM(PeriodInterval)

    /**
     * Default Constructor
     * @param iParent the parent
     * @param iModeEnabled if true then the widgets are enabled/disabled instead of shown/hidden
     */
    explicit SKGPeriodEdit(QWidget *iParent, bool iModeEnabled = false);

    /**
     * Default Destructor
     */
    ~SKGPeriodEdit() override;

    /**
     * Get the current state
     * MUST BE OVERWRITTEN
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    virtual QString getState();

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    virtual void setState(const QString &iState);

    /**
     * Get the text
     * @return the text
     */
    virtual QString text() const;

    /**
     * Get the mode
     * @return the mode
     */
    virtual SKGPeriodEdit::PeriodMode mode() const;

    /**
     * Get the where clauses
     * @param iForecast enable forecast where clause
     * @param oWhereClausForPreviousData the where clause for previous data. Can be nullptr.
     * @param oWhereClausForForecastData the where clause for forecast data. Can be nullptr.
     * @return the where clause
     */
    virtual QString getWhereClause(bool iForecast = true, QString *oWhereClausForPreviousData = nullptr, QString *oWhereClausForForecastData = nullptr) const;

    /**
     * Get begin and end dates
     * @param iPeriod the period
     * @param iInterval the interval
     * @param iValue the number of iInterval
     * @param oBeginDate the begin date of the period
     * @param oEndDate the end date of the period
     * @param iDate the input date
     */
    // cppcheck-suppress passedByValue
    static void getDates(PeriodMode iPeriod, PeriodInterval iInterval, int iValue, QDate &oBeginDate, QDate &oEndDate, QDate iDate = QDate::currentDate());

    /**
     * Get begin and end dates
     * @param oBeginDate the begin date of the period
     * @param oEndDate the end date of the period
     */
    // cppcheck-suppress passedByValue
    void getDates(QDate &oBeginDate, QDate &oEndDate);

private Q_SLOTS:
    void refresh();

Q_SIGNALS:
    /**
     * Emitted when the period change
     */
    void changed();

private:
    Ui::skgperiodedit_base ui{};

    bool m_modeEnable;
    int m_count;
};

#endif // SKGPERIODEDIT_H
