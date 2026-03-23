/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A period editor.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgperiodedit.h"
#include "skgmainpanel.h"

#include <qdom.h>
#include <qstringliteral.h>

#include "skgservices.h"
#include "skgtraces.h"

SKGPeriodEdit::SKGPeriodEdit(QWidget *iParent, bool iModeEnabled)
    : QWidget(iParent)
    , m_modeEnable(iModeEnabled)
    , m_count(0)
{
    ui.setupUi(this);
    this->setFocusProxy(ui.kPeriod);

    ui.kPeriod->addItem(i18nc("Period mode", "All Dates"), static_cast<int>(SKGPeriodEdit::ALL));
    ui.kPeriod->addItem(i18nc("Period mode", "Current…"), static_cast<int>(SKGPeriodEdit::CURRENT));
    ui.kPeriod->addItem(i18nc("Period mode", "Previous…"), static_cast<int>(SKGPeriodEdit::PREVIOUS));
    ui.kPeriod->addItem(i18nc("Period mode", "Last…"), static_cast<int>(SKGPeriodEdit::LAST));
    ui.kPeriod->addItem(i18nc("Period mode", "Custom…"), static_cast<int>(SKGPeriodEdit::CUSTOM));
    ui.kPeriod->addItem(i18nc("Period mode", "Timeline…"), static_cast<int>(SKGPeriodEdit::TIMELINE));

    ui.kInterval->addItem(i18nc("Period interval", "day(s)"), 0);
    ui.kInterval->addItem(i18nc("Period interval", "week(s)"), 1);
    ui.kInterval->addItem(i18nc("Period interval", "month(s)"), 2);
    ui.kInterval->addItem(i18nc("Period interval", "quarter(s)"), 4);
    ui.kInterval->addItem(i18nc("Period interval", "semester(s)"), 5);
    ui.kInterval->addItem(i18nc("Period interval", "year(s)"), 3);

    ui.kPeriod->setCurrentIndex(1);
    ui.kInterval->setCurrentIndex(2);

    connect(ui.kPeriod, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged), this, &SKGPeriodEdit::refresh);
    connect(ui.kInterval, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged), this, &SKGPeriodEdit::refresh);
    connect(ui.kDateBegin, &SKGDateEdit::dateEntered, this, &SKGPeriodEdit::refresh);
    connect(ui.kDateEnd, &SKGDateEdit::dateEntered, this, &SKGPeriodEdit::refresh);
    connect(ui.kNbIntervals, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SKGPeriodEdit::refresh);
    connect(ui.kTimeline, &QSlider::valueChanged, this, &SKGPeriodEdit::refresh);
    connect(ui.kFuture, &QCheckBox::checkStateChanged, this, &SKGPeriodEdit::refresh);
}

SKGPeriodEdit::~SKGPeriodEdit() = default;

QString SKGPeriodEdit::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("period"), SKGServices::intToString(mode()));
    if (mode() == CUSTOM) {
        root.setAttribute(QStringLiteral("date_begin"), SKGServices::intToString(ui.kDateBegin->date().toJulianDay()));
        root.setAttribute(QStringLiteral("date_end"), SKGServices::intToString(ui.kDateEnd->date().toJulianDay()));
    }
    root.setAttribute(QStringLiteral("interval"), SKGServices::intToString(ui.kInterval->itemData(ui.kInterval->currentIndex()).toInt()));
    root.setAttribute(QStringLiteral("nb_intervals"), SKGServices::intToString(ui.kNbIntervals->value()));
    root.setAttribute(QStringLiteral("timeline"), SKGServices::intToString(ui.kTimeline->value()));
    root.setAttribute(QStringLiteral("future"), ui.kFuture->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));

    return doc.toString();
}

void SKGPeriodEdit::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString period = root.attribute(QStringLiteral("period"));
    QString interval = root.attribute(QStringLiteral("interval"));
    QString nb_interval = root.attribute(QStringLiteral("nb_intervals"));
    QString timeline = root.attribute(QStringLiteral("timeline"));
    QString date_begin = root.attribute(QStringLiteral("date_begin"));
    QString date_end = root.attribute(QStringLiteral("date_end"));
    QString future = root.attribute(QStringLiteral("future"));

    // Default values
    if (period.isEmpty()) {
        period = '1';
    }
    if (interval.isEmpty()) {
        interval = '2';
    }
    if (nb_interval.isEmpty()) {
        nb_interval = '1';
    }
    if (timeline.isEmpty()) {
        timeline = '1';
    }
    ui.kPeriod->setCurrentIndex(ui.kPeriod->findData(SKGServices::stringToInt(period)));
    ui.kInterval->setCurrentIndex(ui.kInterval->findData(SKGServices::stringToInt(interval)));
    ui.kTimeline->setValue(SKGServices::stringToInt(timeline));
    ui.kNbIntervals->setValue(SKGServices::stringToInt(nb_interval));
    ui.kFuture->setChecked(future == QStringLiteral("Y"));
    if (!date_begin.isEmpty()) {
        ui.kDateBegin->setDate(QDate::fromJulianDay(SKGServices::stringToInt(date_begin)));
    }
    if (!date_end.isEmpty()) {
        ui.kDateEnd->setDate(QDate::fromJulianDay(SKGServices::stringToInt(date_end)));
    }

    refresh();
}

QString SKGPeriodEdit::text() const
{
    QString during = ui.kPeriod->text().remove(QStringLiteral("…"));

    SKGPeriodEdit::PeriodMode m = mode();
    if (m == CUSTOM || m == TIMELINE) {
        during = i18nc("A period", "From %1 to %2", SKGMainPanel::dateToString(ui.kDateBegin->date()), SKGMainPanel::dateToString(ui.kDateEnd->date()));
    } else if (m != ALL) {
        if (m == PREVIOUS) {
            switch (ui.kInterval->itemData(ui.kInterval->currentIndex()).toInt()) {
            case 0:
                during = i18ncp("A period", "Previous day", "%1 previous days", ui.kNbIntervals->value());
                break;
            case 1:
                during = i18ncp("A period", "Previous week", "%1 previous weeks", ui.kNbIntervals->value());
                break;
            case 2:
                during = i18ncp("A period", "Previous month", "%1 previous months", ui.kNbIntervals->value());
                break;
            case 3:
                during = i18ncp("A period", "Previous year", "%1 previous years", ui.kNbIntervals->value());
                break;
            case 4:
                during = i18ncp("A period", "Previous quarter", "%1 previous quarters", ui.kNbIntervals->value());
                break;
            case 5:
            default:
                during = i18ncp("A period", "Previous semester", "%1 previous semesters", ui.kNbIntervals->value());
                break;
            }
        } else if (m == LAST) {
            switch (ui.kInterval->itemData(ui.kInterval->currentIndex()).toInt()) {
            case 0:
                during = i18ncp("A period", "Last day", "%1 last days", ui.kNbIntervals->value());
                break;
            case 1:
                during = i18ncp("A period", "Last week", "%1 last weeks", ui.kNbIntervals->value());
                break;
            case 2:
                during = i18ncp("A period", "Last month", "%1 last months", ui.kNbIntervals->value());
                break;
            case 3:
                during = i18ncp("A period", "Last year", "%1 last years", ui.kNbIntervals->value());
                break;
            case 4:
                during = i18ncp("A period", "Last quarter", "%1 last quarters", ui.kNbIntervals->value());
                break;
            case 5:
            default:
                during = i18ncp("A period", "Last semester", "%1 last semesters", ui.kNbIntervals->value());
                break;
            }
        } else if (m == CURRENT) {
            switch (ui.kInterval->itemData(ui.kInterval->currentIndex()).toInt()) {
            case 0:
                during = i18nc("A period", "Current day");
                break;
            case 1:
                during = i18nc("A period", "Current week");
                break;
            case 2:
                during = i18nc("A period", "Current month");
                break;
            case 3:
                during = i18nc("A period", "Current year");
                break;
            case 4:
                during = i18nc("A period", "Current quarter");
                break;
            case 5:
            default:
                during = i18nc("A period", "Current semester");
                break;
            }
        }
    }
    return during;
}

SKGPeriodEdit::PeriodMode SKGPeriodEdit::mode() const
{
    return static_cast<PeriodMode>(ui.kPeriod->itemData(ui.kPeriod->currentIndex()).toInt());
}

void SKGPeriodEdit::getDates(SKGPeriodEdit::PeriodMode iPeriod,
                             SKGPeriodEdit::PeriodInterval iInterval,
                             int iValue,
                             QDate &oBeginDate,
                             QDate &oEndDate,
                             QDate iDate)
{
    QDate a = iDate;
    QDate b = a;
    int val = iValue;

    switch (iInterval) {
    case DAY:
        // Interval is in days
        break;
    case WEEK:
        // Interval is in weeks
        val *= 7;
        break;
    case MONTH:
    default:
        // Interval is in months
        break;
    case QUARTER:
        // Interval is in quarter
        val *= 3;
        break;
    case SEMESTER:
        // Interval is in semester
        val *= 6;
        break;
    case YEAR:
        // Interval is in years
        break;
    }

    switch (iPeriod) {
    case CURRENT:
        // Current Interval
        switch (iInterval) {
        case DAY:
            // Interval is in days
            break;
        case WEEK:
            // Interval is in weeks
            a = a.addDays(1 - a.dayOfWeek());
            b = a.addDays(7 - 1);
            break;
        case MONTH:
        default:
            // Interval is in months
            a = a.addDays(1 - a.day());
            b = a.addMonths(1).addDays(-1);
            break;
        case YEAR:
            // Interval is in years
            a = a.addDays(1 - a.day()).addMonths(1 - a.month());
            b = a.addYears(1).addDays(-1);
            break;
        case QUARTER:
            // Interval is in quarter
            a = a.addDays(1 - a.day()).addMonths(1 - ((a.month() - 1) % 4));
            b = a.addMonths(3).addDays(-1);
            break;
        case SEMESTER:
            // Interval is in semester
            a = a.addDays(1 - a.day()).addMonths(-((a.month() - 1) % 6));
            b = a.addMonths(6).addDays(-1);
            break;
        }
        break;
    case PREVIOUS:
        // Previous Interval
        switch (iInterval) {
        case DAY:
            // Interval is in days
            b = b.addDays(-1);
            a = b.addDays(-val + 1);
            break;
        case WEEK:
            // Interval is in weeks
            b = b.addDays(-a.dayOfWeek());
            a = b.addDays(-val + 1);
            break;
        case MONTH:
        default:
            // Interval is in months
            b = b.addDays(-b.day());
            a = b.addDays(1).addMonths(-val);
            break;
        case YEAR:
            // Interval is in years
            b = b.addMonths(1 - b.month()).addDays(-b.day());
            a = b.addDays(1).addYears(-val);
            break;
        case QUARTER:
            // Interval is in quarter
            b = b.addMonths(1 - ((b.month() - 1) % 4)).addDays(-b.day());
            a = b.addDays(1).addMonths(-val);
            break;
        case SEMESTER:
            // Interval is in semester
            b = b.addMonths(-((b.month() - 1) % 6)).addDays(-b.day());
            a = b.addDays(1).addMonths(-val);
            break;
        }
        break;
    case LAST:
        // Last Interval
        switch (iInterval) {
        case DAY:
            // Interval is in days
            a = a.addDays(-val);
            break;
        case WEEK:
            // Interval is in weeks
            a = a.addDays(-val);
            break;
        case MONTH:
        default:
            // Interval is in months
            a = a.addMonths(-val);
            break;
        case YEAR:
            // Interval is in years
            a = a.addYears(-val);
            break;
        case QUARTER:
            // Interval is in quarter
            a = a.addMonths(-val);
            break;
        case SEMESTER:
            // Interval is in semester
            a = a.addMonths(-val);
            break;
        }
        a = a.addDays(1);
        break;
    case TIMELINE:
        // Timeline
        switch (iInterval) {
        case DAY:
            // Interval is in days
            a = a.addDays(iValue - 12);
            b = a;
            break;
        case WEEK:
            // Interval is in weeks
            a = a.addDays(1 - a.dayOfWeek()).addDays((iValue - 12) * 7);
            b = a.addDays(7 - 1);
            break;
        case MONTH:
        default:
            // Interval is in months
            a = a.addDays(1 - a.day()).addMonths(iValue - 12);
            b = a.addMonths(1).addDays(-1);
            break;
        case YEAR:
            // Interval is in years
            a = a.addDays(1 - a.day()).addMonths(1 - a.month()).addYears(iValue - 12);
            b = a.addYears(1).addDays(-1);
            break;
        case QUARTER:
            // Interval is in quarter
            a = a.addDays(1 - a.day()).addMonths(1 - ((a.month() - 1) % 4)).addMonths(3 * (iValue - 12));
            b = a.addMonths(3).addDays(-1);
            break;
        case SEMESTER:
            // Interval is in semester
            a = a.addDays(1 - a.day()).addMonths(-((a.month() - 1) % 6)).addMonths(6 * (iValue - 12));
            b = a.addMonths(6).addDays(-1);
            break;
        }
        break;
    default:
        // Take all dates
        a = a.addYears(-1);
        break;
    }

    oBeginDate = a;
    oEndDate = b;
}

void SKGPeriodEdit::getDates(QDate &oBeginDate, QDate &oEndDate)
{
    SKGPeriodEdit::PeriodInterval interval = static_cast<SKGPeriodEdit::PeriodInterval>(ui.kInterval->itemData(ui.kInterval->currentIndex()).toInt());
    int val = ui.kNbIntervals->value();

    getDates(mode(), interval, mode() == TIMELINE ? ui.kTimeline->value() : val, oBeginDate, oEndDate);
}

QString SKGPeriodEdit::getWhereClause(bool iForecast, QString *oWhereClausForPreviousData, QString *oWhereClausForForecastData) const
{
    // Build where clause
    QString wc;

    SKGPeriodEdit::PeriodInterval interval = static_cast<SKGPeriodEdit::PeriodInterval>(ui.kInterval->itemData(ui.kInterval->currentIndex()).toInt());
    QString strfFormat;
    QString sqlInterval;
    QString sqlAttribute;
    QString startOf;
    int val = ui.kNbIntervals->value();
    int one = 1;
    QDate a;
    QDate b;

    getDates(mode(), interval, mode() == TIMELINE ? ui.kTimeline->value() : val, a, b);

    switch (interval) {
    case DAY:
        // Interval is in days
        sqlAttribute = QStringLiteral("d_date");
        strfFormat = QStringLiteral("'D'");
        sqlInterval = QStringLiteral("DAY");
        break;
    case WEEK:
        // Interval is in weeks
        sqlAttribute = QStringLiteral("d_DATEWEEK");
        strfFormat = QStringLiteral("'W'");
        sqlInterval = QStringLiteral("DAY");
        val *= 7;
        one *= 7;
        break;
    case MONTH:
    default:
        // Interval is in months
        sqlAttribute = QStringLiteral("d_DATEMONTH");
        strfFormat = QStringLiteral("'M'");
        sqlInterval = QStringLiteral("MONTH");
        break;
    case QUARTER:
        // Interval is in quarter
        sqlAttribute = QStringLiteral("d_DATEQUARTER");
        strfFormat = QStringLiteral("'Q'");
        sqlInterval = QStringLiteral("MONTH");
        val *= 3;
        one *= 3;
        break;
    case SEMESTER:
        // Interval is in quarter
        sqlAttribute = QStringLiteral("d_DATESEMESTER");
        strfFormat = QStringLiteral("'S'");
        sqlInterval = QStringLiteral("MONTH");
        val *= 6;
        one *= 6;
        break;
    case YEAR:
        // Interval is in years
        sqlAttribute = QStringLiteral("d_DATEYEAR");
        strfFormat = QStringLiteral("'Y'");
        sqlInterval = QStringLiteral("YEAR");
        break;
    }

    switch (mode()) {
    case CURRENT:
        if (ui.kFuture->isChecked()) {
            b = QDate(2099, 12, 31);
        }

        ui.kDateBegin->setDate(a);
        ui.kDateEnd->setDate(b);
        wc = sqlAttribute % " = (SELECT period(date('now', 'localtime'), " % strfFormat % "))";
        if (ui.kFuture->isChecked()) {
            wc += QStringLiteral(" OR d_date > (date('now', 'localtime'))");
        }
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData = sqlAttribute % " < (SELECT period(date('now', 'localtime'), " % strfFormat % "))";
        }
        if (oWhereClausForForecastData != nullptr) {
            *oWhereClausForForecastData = QStringLiteral("d_date > (date('now', 'localtime'))");
        }
        break;
    case PREVIOUS:
        ui.kDateBegin->setDate(a);
        ui.kDateEnd->setDate(b);
        startOf = QStringLiteral("start of ") % sqlInterval.toLower();
        wc = sqlAttribute % ">=(SELECT period(date('now', 'localtime','" % startOf % "', '-" % SKGServices::intToString(val) % u' ' % sqlInterval % "'),"
            % strfFormat % "))";
        if (iForecast) {
            wc += " AND period(d_date, " % strfFormat % ")<=(SELECT period(date('now', 'localtime','" % startOf % "', '-" % SKGServices::intToString(one) % u' '
                % sqlInterval % "')," % strfFormat % "))"; // For forecast based on scheduled transactions

            if (oWhereClausForPreviousData != nullptr) {
                *oWhereClausForPreviousData = sqlAttribute % " <= (SELECT period(date('now', 'localtime','" % startOf % "', '-" % SKGServices::intToString(val)
                    % u' ' % sqlInterval % "')," % strfFormat % "))";
            }
            if (oWhereClausForForecastData != nullptr) {
                *oWhereClausForForecastData = sqlAttribute % " > (SELECT period(date('now', 'localtime','" % startOf % "', '-" % SKGServices::intToString(one)
                    % u' ' % sqlInterval % "')," % strfFormat % "))";
            }
        }
        break;
    case LAST:
        if (ui.kFuture->isChecked()) {
            b = QDate(2099, 12, 31);
        }

        ui.kDateBegin->setDate(a);
        ui.kDateEnd->setDate(b);

        wc = "d_date >= (date('now', 'localtime','-" % SKGServices::intToString(val) % u' ' % sqlInterval % "', '+1 DAY'))";
        if (iForecast) {
            wc += QStringLiteral(" AND d_date<=(date('now', 'localtime'))"); // For forecast based on scheduled transactions
        }

        if (ui.kFuture->isChecked()) {
            wc += QStringLiteral(" OR d_date > (date('now', 'localtime'))");
        }
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData = "d_date <= (date('now', 'localtime','-" % SKGServices::intToString(val) % u' ' % sqlInterval % "'))";
        }
        if (oWhereClausForForecastData != nullptr) {
            *oWhereClausForForecastData = QStringLiteral("d_date > (date('now', 'localtime'))");
        }
        break;
    case CUSTOM:
        // Custom Date
        wc = "d_date>='" % SKGServices::dateToSqlString(ui.kDateBegin->date()) % u'\'';
        if (iForecast) {
            wc += "AND d_date<='" % SKGServices::dateToSqlString(ui.kDateEnd->date()) % u'\''; // For forecast based on scheduled transactions
        }
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData = "d_date<'" % SKGServices::dateToSqlString(ui.kDateBegin->date()) % u'\'';
        }
        if (oWhereClausForForecastData != nullptr) {
            *oWhereClausForForecastData = QStringLiteral("d_date > (date('now', 'localtime'))");
        }
        break;
    case TIMELINE:
        // Timeline
        ui.kDateBegin->setDate(a);
        ui.kDateEnd->setDate(b);
        wc = "d_date>='" % SKGServices::dateToSqlString(ui.kDateBegin->date()) % u'\'';
        if (iForecast) {
            wc += "AND d_date<='" % SKGServices::dateToSqlString(ui.kDateEnd->date()) % u'\''; // For forecast based on scheduled transactions
        }
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData = "d_date<'" % SKGServices::dateToSqlString(ui.kDateBegin->date()) % u'\'';
        }
        if (oWhereClausForForecastData != nullptr) {
            *oWhereClausForForecastData = "d_date>'" % SKGServices::dateToSqlString(ui.kDateEnd->date()) % u'\'';
        }
        break;
    default:
        // Take all dates
        a = a.addYears(-1);
        ui.kDateBegin->setDate(a);
        ui.kDateEnd->setDate(b);
        wc = QStringLiteral("1=1");
        if (oWhereClausForPreviousData != nullptr) {
            *oWhereClausForPreviousData = QStringLiteral("1=0");
        }
        if (oWhereClausForForecastData != nullptr) {
            *oWhereClausForForecastData = QStringLiteral("d_date > (date('now', 'localtime'))");
        }
        break;
    }

    wc = "((" % wc % ") OR d_date='0000') AND d_date!='0000-00-00'";
    if (oWhereClausForPreviousData != nullptr) {
        *oWhereClausForPreviousData = "((" % *oWhereClausForPreviousData % ") OR d_date='0000-00-00')";
    }

    return wc;
}

void SKGPeriodEdit::refresh()
{
    int p = ui.kPeriod->currentIndex();

    // Check dates
    QDate d1 = ui.kDateBegin->date();
    QDate d2 = ui.kDateEnd->date();
    if (d1 > d2) {
        ui.kDateBegin->setDate(d2);
        ui.kDateEnd->setDate(d1);
    }

    ++m_count;
    if (m_count == 5) {
        m_modeEnable = false;
    }

    ui.kDateSelect->setEnabled(p != ALL);
    ui.kTimeline->setEnabled(p == TIMELINE);
    ui.kFuture->setEnabled(p == CURRENT || p == LAST);
    ui.kInterval->setEnabled(p == CURRENT || p == PREVIOUS || p == LAST || p == TIMELINE);
    ui.kNbIntervals->setEnabled(p == PREVIOUS || p == LAST);

    if (!m_modeEnable) {
        ui.kDateSelect->setVisible(p != ALL);
        ui.kTimeline->setVisible(p == TIMELINE);
        ui.kFuture->setVisible(p == CURRENT || p == LAST);
        ui.kInterval->setVisible(p == CURRENT || p == PREVIOUS || p == LAST || p == TIMELINE);
        ui.kNbIntervals->setVisible(p == PREVIOUS || p == LAST);
    }

    ui.kDateSelect->setEnabled(p == CUSTOM);

    // Needed to refresh dates
    getWhereClause();

    Q_EMIT changed();
}
