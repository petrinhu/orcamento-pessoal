/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGServices.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgservices.h"
#include "skgdefine.h"

#include <kaboutdata.h>
#include <kiconloader.h>
#include <kio/filecopyjob.h>
#include <klocalizedstring.h>
#ifndef Q_OS_WIN
#include <sys/time.h>
#endif

#include <qapplication.h>
#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qjsengine.h>
#include <qlocale.h>
#include <qmath.h>
#include <qregularexpression.h>
#include <qsavefile.h>
#include <qsqldatabase.h>
#include <qsqldriver.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qsqlrecord.h>
#include <qstandardpaths.h>
#include <qtemporaryfile.h>
#include <qthread.h>
#include <qvariant.h>

#include "skgdocument.h"
#include "skgtraces.h"

#define SQLCIPHERHEARDER "SQLCipher format"

int SKGServices::SKGSqlTraces = (SKGServices::getEnvVariable(QStringLiteral("SKGTRACESQL")).isEmpty()
                                     ? -1
                                     : SKGServices::stringToInt(SKGServices::getEnvVariable(QStringLiteral("SKGTRACESQL"))));

SKGError SKGServices::m_lastCallbackError;

QString SKGServices::searchCriteriasToWhereClause(const SKGServices::SKGSearchCriteriaList &iSearchCriterias,
                                                  const QStringList &iAttributes,
                                                  const SKGDocument *iDocument,
                                                  bool iForDisplay)
{
    QString whereclause;
    int nbCriterias = iSearchCriterias.count();
    int nbAttributes = iAttributes.count();
    for (int i = 0; i < nbCriterias; ++i) {
        SKGSearchCriteria criteria = iSearchCriterias.at(i);
        QString subWhereClause;

        int nbWords = criteria.words.count();
        for (int w = 0; w < nbWords; ++w) {
            QString subWhereClause2;

            QString word = criteria.words[w].toLower();
            QString att;
            QString op(':');
            bool modeStartWith = true;

            // Check if the word follows the format attribute:value
            int pos = word.indexOf(QStringLiteral(":"));
            int pos2 = word.indexOf(QStringLiteral("<="));
            int pos3 = word.indexOf(QStringLiteral(">="));
            int pos4 = word.indexOf(QStringLiteral("="));
            int pos5 = word.indexOf(QStringLiteral("<"));
            int pos6 = word.indexOf(QStringLiteral(">"));
            int pos7 = word.indexOf(QStringLiteral("#"));
            int opLength = 1;
            if (pos2 != -1 && (pos2 < pos || pos == -1)) {
                pos = pos2;
                opLength = 2;
            }
            if (pos3 != -1 && (pos3 < pos || pos == -1)) {
                pos = pos3;
                opLength = 2;
            }
            if (pos4 != -1 && (pos4 < pos || pos == -1)) {
                pos = pos4;
            }
            if (pos5 != -1 && (pos5 < pos || pos == -1)) {
                pos = pos5;
            }
            if (pos6 != -1 && (pos6 < pos || pos == -1)) {
                pos = pos6;
            }
            if (pos7 != -1 && (pos7 < pos || pos == -1)) {
                pos = pos7;
            }

            if (pos != -1) {
                att = word.left(pos);
                if (att.endsWith(QStringLiteral("."))) {
                    modeStartWith = false;
                    att = att.left(att.size() - 1);
                }
                op = word.mid(pos, opLength);
                word = word.right(word.size() - pos - op.size());
            }

            word = SKGServices::stringToSqlString(word);

            for (int j = 0; j < nbAttributes; ++j) {
                QString attDatabase = iAttributes.at(j);
                QString attForComparison = (iDocument != nullptr ? iDocument->getDisplay(attDatabase) : attDatabase).toLower();
                if (att.isEmpty() || (modeStartWith && attForComparison.startsWith(att))
                    || (!modeStartWith && attForComparison.compare(att, Qt::CaseInsensitive) == 0)) {
                    if (iForDisplay) {
                        QString n = attForComparison + op + word;
                        if (subWhereClause2.isEmpty()) {
                            subWhereClause2 = std::move(n);
                        } else {
                            subWhereClause2 = i18nc("Logical condition", "%1 or %2", subWhereClause2, n);
                        }
                    } else {
                        if (!subWhereClause2.isEmpty()) {
                            subWhereClause2 = subWhereClause2 % QStringLiteral(" OR ");
                        }

                        if (attDatabase.startsWith(QStringLiteral("p_"))) {
                            // Case property
                            QString propName = attDatabase.right(attDatabase.length() - 2);
                            if (op == QStringLiteral(":")) {
                                subWhereClause2 = subWhereClause2 % QStringLiteral("i_PROPPNAME='") % SKGServices::stringToSqlString(propName)
                                    % QStringLiteral("' AND (lower(i_PROPVALUE) LIKE '%") % word % "%')";
                            } else if (op == QStringLiteral("#")) {
                                subWhereClause2 = subWhereClause2 % QStringLiteral("i_PROPPNAME='") % SKGServices::stringToSqlString(propName)
                                    % QStringLiteral("' AND REGEXP('") % word % QStringLiteral("',i_PROPVALUE)");
                            } else {
                                attDatabase = QStringLiteral("i_PROPPNAME='") % SKGServices::stringToSqlString(propName) % QStringLiteral("' AND i_PROPVALUE");
                                subWhereClause2 = subWhereClause2 % attDatabase % op % word;
                            }
                        } else {
                            // Case normal attribute
                            if (op == QStringLiteral(":")) {
                                subWhereClause2 = subWhereClause2 % QStringLiteral("lower(") % attDatabase % QStringLiteral(") LIKE '%") % word % "%'";
                            } else if (op == QStringLiteral("#")) {
                                subWhereClause2 =
                                    subWhereClause2 % QStringLiteral("REGEXP('") % word % QStringLiteral("',") % attDatabase % QStringLiteral(")");
                            } else {
                                if (attDatabase.startsWith(QStringLiteral("f_")) || attDatabase.startsWith(QStringLiteral("i_"))) {
                                    subWhereClause2 = subWhereClause2 % attDatabase % op % word;
                                } else {
                                    subWhereClause2 = subWhereClause2 % QStringLiteral("lower(") % attDatabase % QStringLiteral(")") % op % QStringLiteral("'")
                                        % word % QStringLiteral("'");
                                }
                            }
                        }
                    }
                }
            }
            if (iForDisplay) {
                if (!subWhereClause2.isEmpty()) {
                    if (subWhereClause.isEmpty()) {
                        subWhereClause = subWhereClause2;
                    } else {
                        subWhereClause = i18nc("Logical condition", "(%1) and (%2)", subWhereClause, subWhereClause2);
                    }
                }
            } else {
                if (!subWhereClause2.isEmpty()) {
                    if (!subWhereClause.isEmpty()) {
                        subWhereClause = subWhereClause % QStringLiteral(" AND ");
                    }
                    subWhereClause = subWhereClause % QStringLiteral("(") % subWhereClause2 % QStringLiteral(")");
                } else {
                    subWhereClause = QStringLiteral("1=0");
                }
            }
        }
        if (iForDisplay) {
            if (!subWhereClause.isEmpty()) {
                if (criteria.mode == '+') {
                    if (whereclause.isEmpty()) {
                        whereclause = std::move(subWhereClause);
                    } else {
                        whereclause = i18nc("Logical condition", "(%1) or (%2)", whereclause, subWhereClause);
                    }
                } else if (criteria.mode == '-') {
                    if (subWhereClause.isEmpty()) {
                        whereclause = i18nc("Logical condition", "not (%1)", subWhereClause);
                    } else {
                        whereclause = i18nc("Logical condition", "(%1) and not (%2)", whereclause, subWhereClause);
                    }
                }
            }
        } else {
            if (!subWhereClause.isEmpty()) {
                if (criteria.mode == '+') {
                    if (!whereclause.isEmpty()) {
                        whereclause = whereclause % QStringLiteral(" OR ");
                    }
                    whereclause = whereclause % QStringLiteral("(") % subWhereClause % QStringLiteral(")");
                } else if (criteria.mode == '-') {
                    if (!whereclause.isEmpty()) {
                        whereclause = whereclause % QStringLiteral(" AND NOT");
                    } else {
                        whereclause = QStringLiteral("NOT");
                    }
                    whereclause = whereclause % QStringLiteral("(") % subWhereClause % QStringLiteral(")");
                }
            }
        }
    }
    return whereclause;
}

SKGServices::SKGSearchCriteriaList SKGServices::stringToSearchCriterias(const QString &iString)
{
    SKGServices::SKGSearchCriteriaList output;

    QStringList words = SKGServices::splitCSVLine(iString, u' ', true);

    int nbwords = words.count();
    output.reserve(nbwords);

    SKGServices::SKGSearchCriteria criteria;
    criteria.mode = '+';
    bool atLeastOnePlus = false;
    for (int i = 0; i < nbwords; ++i) {
        QString word = words.at(i);
        bool isWordStartingByPlus = word.startsWith(QStringLiteral("+"));
        bool isWordStartingByLess = word.startsWith(QStringLiteral("-"));
        if (isWordStartingByPlus || isWordStartingByLess) {
            QChar nextChar;
            if (word.size() > 1) {
                nextChar = word[1];
            }
            if (nextChar < '0' || nextChar > '9') {
                word = word.right(word.length() - 1);
                if (Q_LIKELY(i != 0)) {
                    if (criteria.mode == '-') {
                        output.push_back(criteria);
                    } else {
                        output.push_front(criteria);
                        atLeastOnePlus = true;
                    }
                }
                criteria.words.clear();
                criteria.mode = (isWordStartingByPlus ? '+' : '-');
            }
        }
        criteria.words.push_back(word);
    }
    if (criteria.mode == '-') {
        output.push_back(criteria);
    } else {
        output.push_front(criteria);
        atLeastOnePlus = true;
    }

    if (!atLeastOnePlus) {
        // Add a '+' always true
        SKGServices::SKGSearchCriteria criteria2;
        criteria2.mode = '+';
        criteria2.words.push_back(QString());
        output.push_front(criteria2);
    }

    return output;
}

QString SKGServices::getEnvVariable(const QString &iAttribute)
{
    return QString::fromUtf8(qgetenv(iAttribute.toUtf8().constData()));
}

QString SKGServices::intToString(qlonglong iNumber)
{
    QString output;
    output.setNum(iNumber);
    return output;
}

qlonglong SKGServices::stringToInt(const QString &iNumber)
{
    if (Q_UNLIKELY(iNumber.isEmpty())) {
        return 0;
    }

    bool ok;
    qlonglong output = iNumber.toLongLong(&ok);
    if (Q_LIKELY(!ok)) {
        SKGTRACE << "WARNING: SKGServices::stringToInt(" << iNumber << ") failed" << Qt::endl;
    }

    return output;
}

QString SKGServices::stringToSqlString(const QString &iString)
{
    QString output;

    for (const auto &c : iString) {
        if (c.isPrint() || c == QChar('\n')) {
            output.append(QChar(c));
        }
    }

    output.replace('\'', QStringLiteral("''"));
    return output;
}

QString SKGServices::stringToHtml(const QString &iString)
{
    QString output = iString;
    output.replace('&', QStringLiteral("&amp;")); // Must be done first
    output.replace('<', QStringLiteral("&lt;"));
    output.replace('>', QStringLiteral("&gt;"));
    output.replace('"', QStringLiteral("&quot;"));

    return output;
}

QString SKGServices::htmlToString(const QString &iString)
{
    QString output = iString;
    output.replace(QStringLiteral("&lt;"), QStringLiteral("<"));
    output.replace(QStringLiteral("&gt;"), QStringLiteral(">"));
    output.replace(QStringLiteral("&quot;"), QStringLiteral("\""));
    output.replace(QStringLiteral("&amp;"), QStringLiteral("&"));

    return output;
}

QString SKGServices::stringsToCsv(const QStringList &iList, QChar iSeparator)
{
    QString output;
    int nb = iList.count();
    for (int i = 0; i < nb; ++i) {
        output.append(SKGServices::stringToCsv(iList.at(i)));
        if (Q_LIKELY(i < nb - 1)) {
            output.append(iSeparator);
        }
    }

    return output;
}

QString SKGServices::stringToCsv(const QString &iNumber)
{
    QString output = iNumber;
    output.replace('"', QStringLiteral("#SKGDOUBLECOTE#"));
    output.replace(QStringLiteral("#SKGDOUBLECOTE#"), QStringLiteral("\"\""));
    output = u'"' % output % u'"';
    return output;
}

double SKGServices::stringToDouble(const QString &iNumber, bool iUseLocale)
{
    if (Q_UNLIKELY(iNumber.isEmpty() || iNumber == QStringLiteral("nan"))) {
        return 0;
    }
    if (Q_UNLIKELY(iNumber == QStringLiteral("inf"))) {
        return 1e300;
    }
    if (Q_UNLIKELY(iNumber == QStringLiteral("-inf"))) {
        return -1e300;
    }
    QString number = iNumber;
    static QRegularExpression reg(QStringLiteral("[^0-9-+/eE,.]"));
    number.remove(reg);
    if (number.contains(QStringLiteral("/"))) {
        // Use script engine
        QJSEngine myEngine;
        const auto result = myEngine.evaluate(number);
        if (result.isNumber()) {
            return result.toNumber();
        }
    }

    bool ok;
    double output = QLocale().toDouble(number, &ok);
    if (Q_LIKELY(!iUseLocale) || Q_LIKELY(!ok)) {
        output = number.toDouble(&ok);
        if (Q_LIKELY(!ok)) {
            QString tmp = number;
            tmp.replace(u',', u'.');
            if (tmp.count(u'.') > 1) {
                tmp.remove(tmp.indexOf(u'.'), 1);
            }
            output = tmp.toDouble(&ok);
            if (Q_LIKELY(!ok)) {
                QString tmp2 = number;
                tmp2.replace(u'.', u',');
                if (tmp2.count(u',') > 1) {
                    tmp2.remove(tmp2.indexOf(u','), 1);
                }
                output = tmp2.toDouble(&ok);
                if (!ok) {
                    QString tmp3 = std::move(number);
                    tmp3.remove(u',');
                    output = tmp3.toDouble(&ok);
                }
            }
        }
    }
    if (Q_LIKELY(!ok)) {
        SKGTRACE << "WARNING: SKGServices::stringToDouble(" << iNumber << ") failed" << Qt::endl;
    }
    return output;
}

QString SKGServices::doubleToString(double iNumber)
{
    QString output;
    output.setNum(iNumber, 'g', 10);
    return output;
}

QString SKGServices::getNextString(const QString &iString)
{
    QString output = iString;
    bool ok;
    qlonglong val = output.toLongLong(&ok);
    if (Q_LIKELY(ok)) {
        // This is a int
        output = SKGServices::intToString(val + 1);
    } else {
        // This is a string
        output = QString();
    }
    return output;
}

QString SKGServices::dateToPeriod(QDate iDate, const QString &iPeriod)
{
    QString period;
    if (iPeriod == QStringLiteral("D")) {
        // Day
        period = iDate.toString(QStringLiteral("yyyy-MM-dd"));
    } else if (iPeriod == QStringLiteral("W")) {
        // Week
        int yearNumber;
        auto weekNumber = iDate.weekNumber(&yearNumber);
        period = SKGServices::intToString(yearNumber) % QStringLiteral("-W") % SKGServices::intToString(weekNumber).rightJustified(2, '0');
    } else if (iPeriod == QStringLiteral("M")) {
        // Month
        period = iDate.toString(QStringLiteral("yyyy-MM"));
    } else if (iPeriod == QStringLiteral("Q")) {
        // Quarter
        period = iDate.toString(QStringLiteral("yyyy-Q")) % (iDate.month() <= 3 ? '1' : (iDate.month() <= 6 ? '2' : (iDate.month() <= 9 ? '3' : '4')));
    } else if (iPeriod == QStringLiteral("S")) {
        // Semester
        period = iDate.toString(QStringLiteral("yyyy-S")) % (iDate.month() <= 6 ? '1' : '2');
    } else if (iPeriod == QStringLiteral("Y")) {
        // Year
        period = iDate.toString(QStringLiteral("yyyy"));
    }
    return period;
}

QString SKGServices::timeToString(const QDateTime &iDateTime)
{
    QDateTime d = iDateTime;
    if (Q_UNLIKELY(!d.isValid())) {
        d = QDateTime::currentDateTime();
    }
    return d.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QString SKGServices::dateToSqlString(QDate iDate)
{
    return dateToSqlString(iDate.startOfDay());
}

QString SKGServices::dateToSqlString(const QDateTime &iDateTime)
{
    QDateTime d = iDateTime;
    if (Q_UNLIKELY(!d.isValid())) {
        d = QDateTime::currentDateTime();
    }
    return d.toString(QStringLiteral("yyyy-MM-dd"));
}

int SKGServices::nbWorkingDays(QDate iFrom, QDate iTo)
{
    int nb = 0;
    QDate min = (iFrom < iTo ? iFrom : iTo);
    QDate max = (iFrom < iTo ? iTo : iFrom);

    while (min != max) {
        if (min.dayOfWeek() <= 5) {
            ++nb;
        }
        min = min.addDays(1);
    }
    if (nb == 0) {
        nb = 1;
    }
    return nb;
}

QDateTime SKGServices::stringToTime(const QString &iDateString)
{
    QDateTime output;
    const auto &tmp = iDateString.left(10).split(u'-');
    if (tmp.count() == 3)
        output = QDateTime(QDate(SKGServices::stringToInt(tmp.at(0)), SKGServices::stringToInt(tmp.at(1)), SKGServices::stringToInt(tmp.at(2))

                                     ),
                           QTime(0, 0, 0));

    return output;
}

QDate SKGServices::stringToDate(const QString &iDateString)
{
    QDate output;
    const auto &tmp = iDateString.left(10).split(u'-');
    if (tmp.count() == 3)
        output = QDate(SKGServices::stringToInt(tmp.at(0)), SKGServices::stringToInt(tmp.at(1)), SKGServices::stringToInt(tmp.at(2))

        );

    return output;
}

QDate SKGServices::partialStringToDate(const QString &iDateString, bool iFixupBackward)
{
    QDate result;
    QStringList items = iDateString.split('/');
    int size = items.count();
    bool ok = false;

    if (size == 1) {
        int dayCount = items.at(0).toInt(&ok);

        result = QDate(QDate::currentDate().year(), QDate::currentDate().month(), dayCount);

        if (iFixupBackward) {
            if (result > QDate::currentDate()) {
                result = result.addMonths(-1);
            }
        } else {
            if (result < QDate::currentDate()) {
                result = result.addMonths(1);
            }
        }
    } else if (size == 2) {
        int dayCount = items.at(0).toInt(&ok);
        int monthCount = items.at(1).toInt(&ok);

        result = QDate(QDate::currentDate().year(), monthCount, dayCount);

        if (iFixupBackward) {
            if (result > QDate::currentDate()) {
                result = result.addYears(-1);
            }
        } else {
            if (result < QDate::currentDate()) {
                result = result.addYears(1);
            }
        }
    } else if (size == 3) {
        int dayCount = items.at(0).toInt(&ok);
        int monthCount = items.at(1).toInt(&ok);
        int yearCount = items.at(2).toInt(&ok);
        int lengthYear = items.at(2).size();

        result = QDate(QDate::currentDate().year(), monthCount, dayCount);

        if (lengthYear < 4) {
            auto y = static_cast<int>(result.year() / qPow(10, lengthYear)) * qPow(10, lengthYear) + yearCount;
            if (y > result.year() && iFixupBackward) {
                y = y - qPow(10, lengthYear);
            } else if (y < result.year() && !iFixupBackward) {
                y = y + qPow(10, lengthYear);
            }
            result = result.addYears(y - result.year());
        } else {
            result = result.addYears(yearCount - result.year());
        }
    }

    if (!ok) {
        result = QDate();
    }
    return result;
}

QStringList SKGServices::splitCSVLine(const QString &iString, QChar iSeparator, bool iCoteDefineBlock)
{
    return splitCSVLine(iString, iSeparator, iCoteDefineBlock, nullptr);
}

QStringList SKGServices::splitCSVLine(const QString &iString, QChar iSeparator, bool iCoteDefineBlock, QChar *oRealSeparator)
{
    QStringList items;
    QString item;
    bool isInBlock = false;
    QChar realSeparator = iSeparator;

    QChar cote = ' '; // Not yet defined
    int nb = iString.length();
    items.reserve(nb);
    for (int pos = 0; pos < nb; ++pos) {
        QChar c = iString.at(pos);
        if (isInBlock) {
            if (c == cote) {
                if (pos < nb - 1 && iString.at(pos + 1) == cote) {
                    ++pos; // separator escaped
                } else {
                    items.push_back(item);
                    item.clear();
                    isInBlock = false;
                    // 320112 vvvv
                    // Reset the block character to authorize mix
                    cote = ' ';
                    // 320112 ^^^^

                    if (realSeparator != ' ')
                        while (pos < nb - 1 && iString.at(pos + 1) == ' ') {
                            ++pos;
                        }
                    ++pos;
                    if (pos < nb) {
                        realSeparator = iString.at(pos); // To get the real separator
                    }
                }
            }

            if (isInBlock) {
                item += c;
            }
        } else if ((c == '\"' || c == u'\'') && item.trimmed().isEmpty() && iCoteDefineBlock) {
            if (cote == ' ') {
                cote = c; // Set the real cote char
            }
            isInBlock = true;
            item.clear();
        } else if (QString(c) == realSeparator) {
            items.push_back(item);
            item.clear();
            isInBlock = false;
            // 320112 vvvv
            // Reset the block character to authorize mix
            cote = ' ';
            // 320112 ^^^^
        } else {
            item += c;
        }
    }

    if (!item.isEmpty() || (nb > 0 && iString.at(nb - 1) == realSeparator)) {
        items.push_back(item);
    }

    if (oRealSeparator != nullptr) {
        *oRealSeparator = realSeparator;
    }

    if (isInBlock) {
        items.clear();
    }

    return items;
}

QString SKGServices::getDateFormat(const QStringList &iDates)
{
    SKGTRACEINFUNC(2)
    bool f_YYYY_MM_DD = true;
    bool f_YYYYMMDD = true;
    bool f_DDMMYYYY = true;
    bool f_MMDDYYYY = true;
    bool f_MM_DD_YY = true;
    bool f_DD_MM_YY = true;
    bool f_MM_DD_YYYY = true;
    bool f_DD_MM_YYYY = true;
    bool f_DDMMMYYYY = true;
    bool f_DD_MMM_YY = true;
    bool f_DD_MMM_YYYY = true;

    // Build regexp
    static QRegularExpression rx(QStringLiteral("(.+)-(.+)-(.+)"));

    // Check all dates
    int nb = iDates.count();
    for (int i = 0; i < nb; ++i) {
        QString val = iDates.at(i).trimmed();
        if (val.size() > 10) {
            auto l = SKGServices::splitCSVLine(val, ' ');
            val = l[0];
        }
        if (!val.isEmpty()) {
            val = val.replace(' ', '0');
            val = val.replace('\\', '-');
            val = val.replace('/', '-');
            val = val.replace('.', '-');
            val = val.replace(QStringLiteral("'20"), QStringLiteral("-20"));
            val = val.replace(QStringLiteral("' "), QStringLiteral("-200"));
            val = val.replace('\'', QStringLiteral("-20"));
            val = val.replace(QStringLiteral("-90"), QStringLiteral("-1990"));
            val = val.replace(QStringLiteral("-91"), QStringLiteral("-1991"));
            val = val.replace(QStringLiteral("-92"), QStringLiteral("-1992"));
            val = val.replace(QStringLiteral("-93"), QStringLiteral("-1993"));
            val = val.replace(QStringLiteral("-94"), QStringLiteral("-1994"));
            val = val.replace(QStringLiteral("-95"), QStringLiteral("-1995"));
            val = val.replace(QStringLiteral("-96"), QStringLiteral("-1996"));
            val = val.replace(QStringLiteral("-97"), QStringLiteral("-1997"));
            val = val.replace(QStringLiteral("-98"), QStringLiteral("-1998"));
            val = val.replace(QStringLiteral("-99"), QStringLiteral("-1999"));
            auto match = rx.match(val);
            if (!match.hasMatch()) {
                f_YYYY_MM_DD = false;
                f_MM_DD_YY = false;
                f_DD_MM_YY = false;
                f_MM_DD_YYYY = false;
                f_DD_MM_YYYY = false;
                f_DD_MMM_YY = false;
                f_DD_MMM_YYYY = false;

                if (val.length() == 8) {
                    auto left2 = SKGServices::stringToInt(val.left(2));
                    if (left2 > 12) {
                        f_MMDDYYYY = false;
                    }
                    if (left2 > 31) {
                        f_DDMMYYYY = false;
                    }

                    auto mid2 = SKGServices::stringToInt(val.mid(2, 2));
                    if (mid2 > 12) {
                        f_DDMMYYYY = false;
                    }
                    if (mid2 > 31) {
                        f_MMDDYYYY = false;
                    }

                    auto mid4 = SKGServices::stringToInt(val.mid(4, 2));
                    if (mid4 > 12) {
                        f_YYYYMMDD = false;
                    }

                    auto right2 = SKGServices::stringToInt(val.right(2));
                    if (right2 > 31) {
                        f_YYYYMMDD = false;
                    }

                    f_DDMMMYYYY = false;
                } else if (val.length() == 9) {
                    f_MMDDYYYY = false;
                    f_DDMMYYYY = false;
                    f_YYYYMMDD = false;
                } else {
                    f_MMDDYYYY = false;
                    f_DDMMYYYY = false;
                    f_YYYYMMDD = false;
                    f_DDMMMYYYY = false;
                }
            } else {
                f_YYYYMMDD = false;
                f_DDMMYYYY = false;
                f_MMDDYYYY = false;
                f_DDMMMYYYY = false;

                QString v1 = match.captured(1);
                QString v2 = match.captured(2);
                QString v3 = match.captured(3);

                if (SKGServices::stringToInt(v1) > 12) {
                    f_MM_DD_YY = false;
                    f_MM_DD_YYYY = false;
                }

                if (SKGServices::stringToInt(v2) > 12) {
                    f_DD_MM_YY = false;
                    f_DD_MM_YYYY = false;
                }

                if (v2.length() > 2) {
                    f_MM_DD_YY = false;
                    f_MM_DD_YYYY = false;
                    f_DD_MM_YY = false;
                    f_DD_MM_YYYY = false;
                    f_YYYY_MM_DD = false;
                }

                if (v2.length() != 3) {
                    f_DD_MMM_YYYY = false;
                    f_DD_MMM_YY = false;
                }

                if (SKGServices::stringToInt(v1) > 31 || SKGServices::stringToInt(v2) > 31) {
                    f_MM_DD_YY = false;
                    f_MM_DD_YYYY = false;
                    f_DD_MM_YY = false;
                    f_DD_MM_YYYY = false;
                }

                if (SKGServices::stringToInt(v3) > 31) {
                    f_YYYY_MM_DD = false;
                }

                if (v1.length() == 4) {
                    f_MM_DD_YY = false;
                    f_DD_MM_YY = false;
                    f_MM_DD_YYYY = false;
                    f_DD_MM_YYYY = false;
                } else {
                    // To be more permissive and support mix of date: f_YYYY_MM_DD = false;
                }

                if (v3.length() == 4) {
                    f_YYYY_MM_DD = false;
                    f_MM_DD_YY = false;
                    f_DD_MM_YY = false;
                } else {
                    // To be more permissive and support mix of date: f_MM_DD_YYYY = false;
                    // To be more permissive and support mix of date: f_DD_MM_YYYY = false;
                }
            }
        }
    }

    if (f_YYYYMMDD) {
        return QStringLiteral("YYYYMMDD");
    }
    if (f_MMDDYYYY) {
        return QStringLiteral("MMDDYYYY");
    }
    if (f_DDMMYYYY) {
        return QStringLiteral("DDMMYYYY");
    }
    if (f_DD_MM_YY && f_MM_DD_YY) {
        QString sFormat = QLocale().dateFormat(QLocale::ShortFormat);
        if (sFormat.startsWith(QStringLiteral("%m")) || sFormat.startsWith(QStringLiteral("%n"))) {
            return QStringLiteral("MM-DD-YY");
        }
        return QStringLiteral("DD-MM-YY");
    }
    if (f_MM_DD_YY) {
        return QStringLiteral("MM-DD-YY");
    }
    if (f_DD_MM_YY) {
        return QStringLiteral("DD-MM-YY");
    }
    if (f_DD_MM_YYYY && f_MM_DD_YYYY) {
        QString sFormat = QLocale().dateFormat(QLocale::ShortFormat);
        if (sFormat.startsWith(QStringLiteral("%m")) || sFormat.startsWith(QStringLiteral("%n"))) {
            return QStringLiteral("MM-DD-YYYY");
        }
        return QStringLiteral("DD-MM-YYYY");
    }
    if (f_MM_DD_YYYY) {
        return QStringLiteral("MM-DD-YYYY");
    }
    if (f_DD_MM_YYYY) {
        return QStringLiteral("DD-MM-YYYY");
    }
    if (f_YYYY_MM_DD) {
        return QStringLiteral("YYYY-MM-DD");
    }
    if (f_DDMMMYYYY) {
        return QStringLiteral("DDMMMYYYY");
    }
    if (f_DD_MMM_YY) {
        return QStringLiteral("DD-MMM-YY");
    }
    if (f_DD_MMM_YYYY) {
        return QStringLiteral("DD-MMM-YYYY");
    }

    return QString();
}

QString SKGServices::toPercentageString(double iAmount, int iNbDecimal)
{
    QString currencyString = toCurrencyString(iAmount, QString(), iNbDecimal);
    return i18nc("Percent value; %1 is the value, % is the sign. Use the appropriate percent sign and positioning for your locale.", "%1 %", currencyString);
}

QString SKGServices::toCurrencyString(double iAmount, const QString &iSymbol, int iNbDecimal)
{
    static auto lc_monetary = (SKGServices::getEnvVariable(QStringLiteral("LC_MONETARY")).isEmpty()
                                   ? QString()
                                   : SKGServices::splitCSVLine(SKGServices::getEnvVariable(QStringLiteral("LC_MONETARY")), u'.').at(0));

    QLocale locale;
    QString localizedPercentSign = locale.percent();
    if (iSymbol == localizedPercentSign || iSymbol == QStringLiteral("%")) {
        return toPercentageString(iAmount, iNbDecimal);
    }
    if (!lc_monetary.isEmpty())
        locale = QLocale(lc_monetary);
    return locale.toCurrencyString(iAmount, iSymbol.isEmpty() ? QStringLiteral(" ") : iSymbol, iNbDecimal).trimmed();
}

QString SKGServices::dateToSqlString(const QString &iDate, const QString &iFormat)
{
    QString input = iDate;
    if (input.size() > 10) {
        auto l = SKGServices::splitCSVLine(input, ' ');
        input = l[0];
    }

    QString format = QStringLiteral("yyyy-MM-dd");
    QString YYYY = QStringLiteral("0000");
    QString MM = QStringLiteral("00");
    QString DD = QStringLiteral("00");
    if (iFormat == QStringLiteral("YYYYMMDD")) {
        YYYY = input.mid(0, 4);
        MM = input.mid(4, 2);
        DD = input.mid(6, 2);
    } else if (iFormat == QStringLiteral("DDMMYYYY") || iFormat == QStringLiteral("DDMMYY")) {
        YYYY = input.mid(4, 4);
        MM = input.mid(2, 2);
        DD = input.mid(0, 2);
    } else if (iFormat == QStringLiteral("DDMMMYYYY") || iFormat == QStringLiteral("DDMMMYY")) {
        YYYY = input.mid(5, 4);
        MM = input.mid(2, 3);
        DD = input.mid(0, 2);
        format = QStringLiteral("yyyy-MMM-dd");
    } else if (iFormat == QStringLiteral("MMDDYYYY") || iFormat == QStringLiteral("MMDDYY")) {
        YYYY = input.mid(4, 4);
        MM = input.mid(0, 2);
        DD = input.mid(2, 2);

    } else {
        QString val = input;
        val = val.replace(' ', '0');
        val = val.replace('\\', '-');
        val = val.replace('/', '-');
        val = val.replace('.', '-');
        val = val.replace(QStringLiteral("'20"), QStringLiteral("-20"));
        val = val.replace(QStringLiteral("' "), QStringLiteral("-200"));
        val = val.replace('\'', QStringLiteral("-20"));
        val = val.replace(QStringLiteral("-90"), QStringLiteral("-1990"));
        val = val.replace(QStringLiteral("-91"), QStringLiteral("-1991"));
        val = val.replace(QStringLiteral("-92"), QStringLiteral("-1992"));
        val = val.replace(QStringLiteral("-93"), QStringLiteral("-1993"));
        val = val.replace(QStringLiteral("-94"), QStringLiteral("-1994"));
        val = val.replace(QStringLiteral("-95"), QStringLiteral("-1995"));
        val = val.replace(QStringLiteral("-96"), QStringLiteral("-1996"));
        val = val.replace(QStringLiteral("-97"), QStringLiteral("-1997"));
        val = val.replace(QStringLiteral("-98"), QStringLiteral("-1998"));
        val = val.replace(QStringLiteral("-99"), QStringLiteral("-1999"));
        static QRegularExpression rx(QStringLiteral("(.+)-(.+)-(.+)"));
        auto match = rx.match(val);
        if (match.hasMatch()) {
            QString v1 = match.captured(1);
            QString v2 = match.captured(2);
            QString v3 = match.captured(3);
            if (iFormat == QStringLiteral("YYYY-MM-DD")) {
                YYYY = v1;
                MM = v2;
                DD = v3;
            } else if (iFormat == QStringLiteral("MM/DD/YY") || iFormat == QStringLiteral("MM-DD-YY") || iFormat == QStringLiteral("MM/DD/YYYY")
                       || iFormat == QStringLiteral("MM-DD-YYYY")) {
                MM = v1;
                DD = v2;
                YYYY = v3;
            } else if (iFormat == QStringLiteral("DD/MM/YY") || iFormat == QStringLiteral("DD-MM-YY") || iFormat == QStringLiteral("DD/MM/YYYY")
                       || iFormat == QStringLiteral("DD-MM-YYYY")) {
                DD = v1;
                MM = v2;
                YYYY = v3;
            } else if (iFormat == QStringLiteral("DD/MMM/YY") || iFormat == QStringLiteral("DD-MMM-YY") || iFormat == QStringLiteral("DD/MMM/YYYY")
                       || iFormat == QStringLiteral("DD-MMM-YYYY")) {
                DD = v1;
                MM = v2;
                YYYY = v3;
                format = QStringLiteral("yyyy-MMM-dd");
            }
        }
    }

    if (MM.length() == 1) {
        MM = u'0' % MM;
    }
    if (DD.length() == 1) {
        DD = u'0' % DD;
    }
    if (YYYY.length() == 1) {
        YYYY = u'0' % YYYY;
    }
    if (YYYY.length() == 2) {
        if (stringToInt(YYYY) > 70) {
            YYYY = QStringLiteral("19") % YYYY;
        } else {
            YYYY = QStringLiteral("20") % YYYY;
        }
    }

    QString date = YYYY % u'-' % MM % u'-' % DD;
    date.replace(' ', '0');
    return dateToSqlString(QDateTime::fromString(date, format));
}

QString SKGServices::getPeriodWhereClause(const QString &iPeriod, const QString &iDateAttribute, const QString &iComparator)
{
    QString output = QStringLiteral("1=0");
    if (iPeriod == QStringLiteral("ALL")) {
        output = QStringLiteral("1=1");
    } else if (iPeriod.length() == 4) {
        // 2014
        output = "STRFTIME('%Y'," + SKGServices::stringToSqlString(iDateAttribute) + ")" + iComparator + "'" + SKGServices::stringToSqlString(iPeriod) + u'\'';
    } else if (iPeriod.length() == 7 && iPeriod[4] == '-') {
        if (iPeriod[5] == 'S') {
            // 2014-S1
            output = "STRFTIME('%Y'," + SKGServices::stringToSqlString(iDateAttribute) + ")||'-S'||(CASE WHEN STRFTIME('%m',"
                + SKGServices::stringToSqlString(iDateAttribute) + ")<='06' THEN '1' ELSE '2' END)" + iComparator + "'"
                + SKGServices::stringToSqlString(iPeriod) + u'\'';
        } else if (iPeriod[5] == 'Q') {
            // 2014-Q1
            output = "STRFTIME('%Y'," + SKGServices::stringToSqlString(iDateAttribute) + ")||'-Q'||(CASE WHEN STRFTIME('%m',"
                + SKGServices::stringToSqlString(iDateAttribute) + ")<='03' THEN '1' WHEN STRFTIME('%m'," + SKGServices::stringToSqlString(iDateAttribute)
                + ")<='06' THEN '2' WHEN STRFTIME('%m'," + SKGServices::stringToSqlString(iDateAttribute) + ")<='09' THEN '3' ELSE '4' END)" + iComparator + "'"
                + SKGServices::stringToSqlString(iPeriod) + u'\'';
        } else {
            // 2014-07
            output = "STRFTIME('%Y-%m'," + SKGServices::stringToSqlString(iDateAttribute) + ")" + iComparator + "'" + SKGServices::stringToSqlString(iPeriod)
                + u'\'';
        }
    }
    if (iComparator == QStringLiteral("<") || iComparator == QStringLiteral("<=")) {
        output = "(" + output + " OR " + iDateAttribute + "='0000-00-00')";
    }
    return output;
}

QDate SKGServices::periodToDate(const QString &iPeriod)
{
    QDate output;

    if (iPeriod == QStringLiteral("ALL")) {
        output = QDate::currentDate();
    } else if (iPeriod.length() == 4) {
        // 2014
        output = QDate::fromString(iPeriod, QStringLiteral("yyyy")).addYears(1).addDays(-1);
    } else if (iPeriod.length() == 7) {
        if (iPeriod[5] == 'S') {
            // 2014-S1
            output = QDate::fromString(iPeriod, QStringLiteral("yyyy-SM"));
            output = output.addMonths(output.month() * 6 - output.month()); // convert semester in month
            output = output.addMonths(1).addDays(-1);
        } else if (iPeriod[5] == 'Q') {
            // 2014-Q1
            output = QDate::fromString(iPeriod, QStringLiteral("yyyy-QM"));
            output = output.addMonths(output.month() * 3 - output.month()); // convert quarter in month
            output = output.addMonths(1).addDays(-1);
        } else {
            // 2014-07
            output = QDate::fromString(iPeriod, QStringLiteral("yyyy-MM")).addMonths(1).addDays(-1);
        }
    }
    return output;
}

QString SKGServices::getNeighboringPeriod(const QString &iPeriod, int iDelta)
{
    QString output = QStringLiteral("1=0");
    if (iPeriod.length() == 4) {
        // 2014
        QDate date = QDate::fromString(iPeriod, QStringLiteral("yyyy")).addYears(iDelta);
        output = date.toString(QStringLiteral("yyyy"));
    } else if (iPeriod.length() == 7) {
        if (iPeriod[5] == 'S') {
            // 2014-S1
            QDate date2 = QDate::fromString(iPeriod, QStringLiteral("yyyy-SM"));
            date2 = date2.addMonths(date2.month() * 6 - date2.month()); // convert semester in month
            date2 = date2.addMonths(6 * iDelta);
            output = date2.toString(QStringLiteral("yyyy-S")) % (date2.month() <= 6 ? '1' : '2');
        } else if (iPeriod[5] == 'Q') {
            // 2014-Q1
            QDate date2 = QDate::fromString(iPeriod, QStringLiteral("yyyy-QM"));
            date2 = date2.addMonths(date2.month() * 3 - date2.month()); // convert quarter in month
            date2 = date2.addMonths(3 * iDelta);
            output = date2.toString(QStringLiteral("yyyy-Q")) % (date2.month() <= 3 ? '1' : (date2.month() <= 6 ? '2' : (date2.month() <= 9 ? '3' : '4')));
        } else {
            // 2014-07
            QDate date2 = QDate::fromString(iPeriod, QStringLiteral("yyyy-MM")).addMonths(iDelta);
            output = date2.toString(QStringLiteral("yyyy-MM"));
        }
    }
    return output;
}

QStringList SKGServices::tableToDump(const SKGStringListList &iTable, SKGServices::DumpMode iMode)
{
    SKGTRACEINFUNC(10)
    // initialisation
    QStringList oResult;

    // Compute max size of each column
    int *maxSizes = nullptr;
    int nbMaxSizes = 0;
    if (iMode == DUMP_TEXT) {
        int nb = iTable.count();
        for (int i = 0; i < nb; ++i) {
            const QStringList &line = iTable.at(i);
            int nb2 = line.size();

            if (maxSizes == nullptr) {
                nbMaxSizes = nb2;
                maxSizes = new int[nbMaxSizes];
                for (int j = 0; j < nbMaxSizes; ++j) {
                    maxSizes[j] = 0;
                }
            }

            for (int j = 0; j < nb2; ++j) {
                const QString &s = line.at(j);
                if (j < nbMaxSizes && s.length() > maxSizes[j]) {
                    maxSizes[j] = s.length();
                }
            }
        }
    }

    // dump
    int nb = iTable.count();
    oResult.reserve(nb);
    for (int i = 0; i < nb; ++i) {
        QString lineFormated;
        if (iMode == DUMP_TEXT && nbMaxSizes > 1) {
            lineFormated = QStringLiteral("| ");
        }

        const QStringList &line = iTable.at(i);
        int nb2 = line.size();
        for (int j = 0; j < nb2; ++j) {
            QString s = line.at(j);
            s.remove('\n');

            if (iMode == DUMP_CSV) {
                if (j > 0) {
                    lineFormated += ';';
                }
                lineFormated += stringToCsv(s);
            } else if (maxSizes != nullptr) {
                if (j < nbMaxSizes) {
                    s = s.leftJustified(maxSizes[j], ' ');
                }
                lineFormated += s;
                if (nbMaxSizes > 1) {
                    lineFormated += QStringLiteral(" | ");
                }
            }
        }
        oResult.push_back(lineFormated);
    }

    // delete
    if (maxSizes != nullptr) {
        delete[] maxSizes;
        maxSizes = nullptr;
    }

    return oResult;
}

QString SKGServices::getRealTable(const QString &iTable)
{
    QString output = iTable;
    if (output.length() > 2 && output.startsWith(QStringLiteral("v_"))) {
        output = output.mid(2, output.length() - 2);

        int pos = output.indexOf(QStringLiteral("_"));
        if (pos != -1) {
            output = output.left(pos);
        }
    }

    return output;
}

SKGError SKGServices::downloadToStream(const QUrl &iSourceUrl, QByteArray &oStream)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    QString tmpFile;
    if (iSourceUrl.isLocalFile()) {
        tmpFile = iSourceUrl.toLocalFile();
    } else {
        err = download(iSourceUrl, tmpFile);
    }
    IFOK(err)
    {
        // Open file
        QFile file(tmpFile);
        if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly))) {
            err.setReturnCode(ERR_FAIL).setMessage(i18nc("An information message", "Open file '%1' failed", tmpFile));
        } else {
            oStream = file.readAll();

            // close file
            file.close();
        }
        if (!iSourceUrl.isLocalFile()) {
            QFile(tmpFile).remove();
        }
    }
    return err;
}

SKGError SKGServices::download(const QUrl &iSourceUrl, QString &oTemporaryFile)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    if (tmpFile.open()) {
        err = upload(iSourceUrl, QUrl::fromLocalFile(tmpFile.fileName()));
        IFOK(err) oTemporaryFile = tmpFile.fileName();
    }
    return err;
}

SKGError SKGServices::upload(const QUrl &iSourceUrl, const QUrl &iDescUrl)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (iDescUrl != iSourceUrl) {
        if (iDescUrl.isLocalFile() && iSourceUrl.isLocalFile()) {
            QFile(iDescUrl.toLocalFile()).remove();
            if (!QFile::copy(iSourceUrl.toLocalFile(), iDescUrl.toLocalFile())) {
                err = SKGError(ERR_ABORT, i18nc("Error message", "Impossible to copy '%1' to '%2'", iSourceUrl.toDisplayString(), iDescUrl.toDisplayString()));
            }
        } else {
            KIO::FileCopyJob *getJob = KIO::file_copy(iSourceUrl, iDescUrl, -1, KIO::Overwrite | KIO::HideProgressInfo);
            if (!getJob->exec()) {
                err.setReturnCode(ERR_ABORT).setMessage(getJob->errorString());
                err.addError(ERR_ABORT, i18nc("Error message", "Impossible to copy '%1' to '%2'", iSourceUrl.toDisplayString(), iDescUrl.toDisplayString()));
            }
        }
    }
    return err;
}

SKGError SKGServices::cryptFile(const QString &iFileSource,
                                const QString &iFileTarget,
                                const QString &iPassword,
                                bool iEncrypt,
                                const QString &iHeaderFile,
                                bool &oModeSQLCipher)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(10) << "Input parameter [iFileSource]=[" << iFileSource << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iFileTarget]=[" << iFileTarget << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iPassword]  =[" << iPassword << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iHeaderFile]=[" << iHeaderFile << ']' << Qt::endl;

    oModeSQLCipher = false;

    // Read document
    QByteArray input;
    QByteArray uba;
    err = downloadToStream(QUrl::fromUserInput(iFileSource), input);
    IFOK(err)
    {
        bool isFileEncrypted = (input.startsWith(QByteArray((iHeaderFile % QStringLiteral("_ENCRYPT")).toLatin1())));
        bool sqliteMode = (input.left(15) == "SQLite format 3");
        SKGTRACEL(10) << "isFileEncrypted=[" << static_cast<unsigned int>(isFileEncrypted) << ']' << Qt::endl;

        // !!!!! Remove Cipher encryption to remove security hole (thank you to Vincent P) !!!!!
        // Only in sqlcipher mode. WARNING: in sqlite mode the issue is still there => add a message to push people to switch in sqlcipher mode
        if (iEncrypt && !sqliteMode) {
            // The input file is a sqlcipher file and we must save it
            // We just have to add a new header to the input file
            uba.reserve(input.length() + iHeaderFile.length() + 11);
            uba.append(iHeaderFile.toLatin1());
            uba.append(!iPassword.isEmpty() ? "_ENCRYPTE3-" : "_DECRYPTE3-");
            uba.append(input);
            oModeSQLCipher = true;
        } else if (!iEncrypt && input.startsWith(QByteArray((iHeaderFile % QStringLiteral("_ENCRYPTE3-")).toLatin1()))) {
            // This check is done to improve performances
            if (iPassword.isEmpty() || iPassword == QStringLiteral("DEFAULTPASSWORD")) {
                err = SKGError(ERR_ENCRYPTION, i18nc("Error message", "Wrong password"));
            } else {
                // The input file encrypter with the new mode
                // We just have to remove the header
                if (!iHeaderFile.isEmpty() && input.startsWith(iHeaderFile.toLatin1())) {
                    input = input.right(input.length() - iHeaderFile.length() - 11);
                }
                uba = input;
                oModeSQLCipher = true;
            }
        } else {
            // WARNING: This part is not really secured but is kept for compatibility
            SKGTRACEL(10) << "Mode not secured" << Qt::endl;
            // BUG 249955 vvv
            if (isFileEncrypted) {
                err = SKGError(ERR_ABORT,
                               i18nc("Error message about encrypting a file",
                                     "Old encrypted files are no more supported. You should use Skrooge <= 2.32.0 to remove the encryption."));
            }
            // BUG 249955 ^^^

            // Suppress header
            SKGTRACEL(10) << "input=[" << input.left(50) << "…]" << Qt::endl;
            if (!iHeaderFile.isEmpty() && input.startsWith(iHeaderFile.toLatin1())) {
                input = input.right(input.length() - iHeaderFile.length() - 11);
            }
            SKGTRACEL(10) << "input without header=[" << input.left(50) << "…]" << Qt::endl;
            uba = std::move(input);

            IFOK(err)
            {
                // Check if decryption is OK
                SKGTRACEL(10) << "output 1=[" << uba.left(50) << "…]" << Qt::endl;
                if (!iEncrypt) {
                    if (!uba.startsWith(QByteArray("SQLite format 3"))) {
                        if (!uba.startsWith(SQLCIPHERHEARDER)) {
                            if (isFileEncrypted) {
                                err = SKGError(ERR_ENCRYPTION, i18nc("Error message", "Wrong password"));
                            } else {
                                oModeSQLCipher = true;
                            }
                        } else {
                            uba = uba.right(uba.length() - QStringLiteral(SQLCIPHERHEARDER).size());
                            oModeSQLCipher = true;
                        }
                    }
                }
            }

            IFOK(err)
            {
                // Add headers
                if (iEncrypt && !iHeaderFile.isEmpty()) {
                    QByteArray h = (iHeaderFile % QStringLiteral("_DECRYPTED-")).toLatin1();
                    uba = uba.insert(0, h);
                }
            }
        }
        SKGTRACEL(10) << "output 2=[" << uba.left(50) << "…]" << Qt::endl;

        // output the results of that stage
        IFOK(err)
        {
            SKGTRACEIN(10, "SKGServices::cryptFile-save file")
            QSaveFile fileOutput(iFileTarget);
            if (!fileOutput.open(QIODevice::WriteOnly)) {
                err = SKGError(ERR_WRITEACCESS, i18nc("Error message: writing a file failed", "Write file '%1' failed", iFileTarget));
            } else {
                // Write document
                fileOutput.write(uba);

                // Close the file
                if (!fileOutput.commit()) {
                    IFOK(err)
                    {
                        err = SKGError(ERR_WRITEACCESS, i18nc("Error message: writing a file failed", "Write file '%1' failed", iFileTarget));
                    }
                }
            }
        }
    }
    SKGTRACEL(10) << "Output parameter [oModeSQLCipher]=[" << static_cast<unsigned int>(oModeSQLCipher) << ']' << Qt::endl;
    return err;
}

SKGError SKGServices::copySqliteDatabaseToXml(const QSqlDatabase &iDb, QDomDocument &oDocument, QVector<QString> *iIgnore)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    oDocument = QDomDocument(QStringLiteral("SKGML"));
    QDomElement document = oDocument.createElement(QStringLiteral("skrooge"));
    oDocument.appendChild(document);

    // Copy the tables
    QStringList listTables = iDb.tables();
    int nb = listTables.count();
    for (int i = 0; !err && i < nb; ++i) {
        const QString &tableName = listTables.at(i);
        if (!tableName.startsWith(QStringLiteral("sqlite_")) && !tableName.startsWith(QStringLiteral("vm_"))) {
            if (iIgnore == nullptr || !iIgnore->contains(tableName)) {
                QDomElement table = oDocument.createElement(QStringLiteral("list_") + tableName);
                document.appendChild(table);

                SKGStringListList listRows;
                err = SKGServices::executeSelectSqliteOrder(iDb, QStringLiteral("SELECT * FROM ") % tableName, listRows);
                int nbRows = listRows.count();
                if (nbRows != 0) {
                    const QStringList &titles = listRows.at(0);
                    for (int j = 1; !err && j < nbRows; ++j) { // Forget title
                        const QStringList &values = listRows.at(j);

                        QDomElement row = oDocument.createElement(tableName);
                        table.appendChild(row);

                        int nbVals = values.count();
                        for (int k = 0; k < nbVals; ++k) {
                            if (iIgnore == nullptr || !iIgnore->contains(tableName + "." + titles.at(k))) {
                                row.setAttribute(titles.at(k), values.at(k));
                            }
                        }
                    }
                }
            }
        }
    }
    return err;
}

SKGError SKGServices::copySqliteDatabase(const QSqlDatabase &iFileDb, const QSqlDatabase &iMemoryDb, bool iFromFileToMemory, const QString &iPassword)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(20) << "Input parameter [iFileDb]=[" << iFileDb.databaseName() << ']' << Qt::endl;
    SKGTRACEL(20) << "Input parameter [iMemoryDb]=[" << iMemoryDb.databaseName() << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iFromFileToMemory]=[" << (iFromFileToMemory ? "FILE->MEMORY" : "MEMORY->FILE") << ']' << Qt::endl;

    QString dbFileName = iFileDb.databaseName();
    // Copy the tables
    SKGStringListList listTables;
    int nb = 0;
    IFOK(err)
    {
        err = SKGServices::executeSelectSqliteOrder(
            (iFromFileToMemory ? iFileDb : iMemoryDb),
            QStringLiteral("SELECT sql, tbl_name FROM sqlite_master WHERE type='table' AND sql NOT NULL and name NOT LIKE 'sqlite_%'"),
            listTables);

        nb = listTables.count();
        for (int i = 1; !err && i < nb; ++i) { // Forget header
            QString val = listTables.at(i).at(0);
            err = SKGServices::executeSqliteOrder((iFromFileToMemory ? iMemoryDb : iFileDb), val);
        }
    }
    // Attach db
    IFOK(err)
    {
        QString add;
        if (!iPassword.isEmpty()) {
            add = QStringLiteral(" KEY '") % SKGServices::stringToSqlString(iPassword) % QStringLiteral("'");
        }
        err = SKGServices::executeSqliteOrder(iMemoryDb, QStringLiteral("ATTACH DATABASE '") % dbFileName % QStringLiteral("' as source") % add);
    }

    // Copy records
    IFOK(err)
    {
        err = SKGServices::executeSqliteOrder(iMemoryDb, QStringLiteral("BEGIN"));
        IFOK(err)
        {
            for (int i = 1; !err && i < nb; ++i) { // Forget header
                QString val = listTables.at(i).at(1);
                if (iFromFileToMemory) {
                    err =
                        SKGServices::executeSqliteOrder(iMemoryDb, QStringLiteral("insert into main.") % val % QStringLiteral(" select * from source.") % val);
                } else {
                    err =
                        SKGServices::executeSqliteOrder(iMemoryDb, QStringLiteral("insert into source.") % val % QStringLiteral(" select * from main.") % val);
                }
            }
        }
        SKGServices::executeSqliteOrder(iMemoryDb, QStringLiteral("COMMIT"));
    }

    // Detach
    {
        SKGError err2 = SKGServices::executeSqliteOrder(iMemoryDb, QStringLiteral("DETACH DATABASE source"));
        if (!err && err2) {
            err = err2;
        }
    }

    // Optimization
    IFOK(err)
    {
        QStringList optimization;
        optimization << QStringLiteral("PRAGMA case_sensitive_like=true") << QStringLiteral("PRAGMA journal_mode=MEMORY")
                     << QStringLiteral("PRAGMA temp_store=MEMORY")
                     // << QStringLiteral("PRAGMA locking_mode=EXCLUSIVE")
                     << QStringLiteral("PRAGMA synchronous = OFF") << QStringLiteral("PRAGMA recursive_triggers=true");
        err = SKGServices::executeSqliteOrders(iFromFileToMemory ? iMemoryDb : iFileDb, optimization);
    }

    // Copy the indexes
    IFOK(err)
    {
        SKGStringListList listSqlOrder;
        err = SKGServices::executeSelectSqliteOrder(
            (iFromFileToMemory ? iFileDb : iMemoryDb),
            QStringLiteral("SELECT sql FROM sqlite_master WHERE type='index' AND sql NOT NULL and name NOT LIKE 'sqlite_%'"),
            listSqlOrder);

        int nb2 = listSqlOrder.count();
        for (int i = 1; !err && i < nb2; ++i) { // Forget header
            QString val = listSqlOrder.at(i).at(0);
            err = SKGServices::executeSqliteOrder((iFromFileToMemory ? iMemoryDb : iFileDb), val);
        }
    }

    // Copy the views
    IFOK(err)
    {
        SKGStringListList listSqlOrder;
        err = SKGServices::executeSelectSqliteOrder(
            (iFromFileToMemory ? iFileDb : iMemoryDb),
            QStringLiteral("SELECT sql FROM sqlite_master WHERE type='view' AND sql NOT NULL and name NOT LIKE 'sqlite_%'"),
            listSqlOrder);

        int nb2 = listSqlOrder.count();
        for (int i = 1; !err && i < nb2; ++i) { // Forget header
            QString val = listSqlOrder.at(i).at(0);
            err = SKGServices::executeSqliteOrder((iFromFileToMemory ? iMemoryDb : iFileDb), val);
        }
    }

    // Copy the triggers, must be done after the views
    IFOK(err)
    {
        SKGStringListList listSqlOrder;
        err = SKGServices::executeSelectSqliteOrder(
            (iFromFileToMemory ? iFileDb : iMemoryDb),
            QStringLiteral("SELECT sql FROM sqlite_master WHERE type='trigger' AND sql NOT NULL and name NOT LIKE 'sqlite_%'"),
            listSqlOrder);

        int nb2 = listSqlOrder.count();
        for (int i = 1; !err && i < nb2; ++i) { // Forget header
            QString val = listSqlOrder.at(i).at(0);
            err = SKGServices::executeSqliteOrder((iFromFileToMemory ? iMemoryDb : iFileDb), val);
        }
    }

    // Check if created file exists
    if (!err && !iFromFileToMemory && !QFile(dbFileName).exists()) {
        err.setReturnCode(ERR_FAIL).setMessage(i18nc("An error message: creating a file failed", "Creation file '%1' failed", dbFileName));
    }
    IFKO(err)
    {
        err.addError(SQLLITEERROR + ERR_FAIL, i18nc("Error message: something failed", "%1 failed", QStringLiteral("SKGServices::copySqliteDatabase()")));
    }
    return err;
}

SKGError SKGServices::executeSqliteOrders(const QSqlDatabase &iDb, const QStringList &iSqlOrders)
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    int nb = iSqlOrders.count();
    for (int i = 0; !err && i < nb; ++i) {
        err = executeSqliteOrder(iDb, iSqlOrders.at(i));
    }
    return err;
}

SKGError SKGServices::executeSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, const QMap<QString, QVariant> &iBind, int *iLastId)
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(20) << "Input parameter [iSqlOrder]=[" << iSqlOrder << ']' << Qt::endl;

    QSqlQuery query(QString(), iDb);
    query.setForwardOnly(true);

    double elapse = 0;
    if (SKGServices::SKGSqlTraces != -1) {
        elapse = SKGServices::getMicroTime();
    }

    // Prepare sql order
    bool prep = query.prepare(iSqlOrder);

    // Bind values
    QMapIterator<QString, QVariant> i(iBind);
    while (i.hasNext()) {
        i.next();
        query.bindValue(i.key(), i.value());
    }

    if (!prep || !query.exec()) {
        QSqlError sqlError = query.lastError();
        if (sqlError.nativeErrorCode().toInt() != 19 /*SQLITE_CONSTRAINT*/ && sqlError.nativeErrorCode().toInt() != 2067 /*SQLITE_CONSTRAINT_UNIQUE*/
            && iSqlOrder != QStringLiteral("SELECT count(*) FROM sqlite_master") /*Test password*/) {
            SKGTRACE << "WARNING: " << iSqlOrder << Qt::endl;
            SKGTRACE << "         returns :" << sqlError.nativeErrorCode().toInt() << " - " << sqlError.text() << Qt::endl;
        }

        err = SKGError(SQLLITEERROR + sqlError.nativeErrorCode().toInt(), iSqlOrder);
        err.addError(SQLLITEERROR + sqlError.nativeErrorCode().toInt(), sqlError.text());

        if (sqlError.nativeErrorCode().toInt() == 19 && iSqlOrder.startsWith(QStringLiteral("INSERT "))) {
            err.addError(ERR_FAIL, i18nc("Error message", "Creation failed. The object already exists."));
        }
    } else {
        if (iLastId != nullptr) {
            *iLastId = query.lastInsertId().toInt();
        }
    }
    if (SKGServices::SKGSqlTraces != -1) {
        elapse = SKGServices::getMicroTime() - elapse;
        if (elapse >= SKGServices::SKGSqlTraces) {
            SKGTRACE << "executeSqliteOrder :" << iSqlOrder << " TIME=" << elapse << " ms" << Qt::endl;
        }
    }
    return err;
}

SKGError SKGServices::executeSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, int *iLastId)
{
    return executeSqliteOrder(iDb, iSqlOrder, QMap<QString, QVariant>(), iLastId);
}

SKGError SKGServices::dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, SKGServices::DumpMode iMode)
{
    return dumpSelectSqliteOrder(iDb, iSqlOrder, nullptr, iMode);
}

SKGError SKGServices::dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QTextStream *oStream, SKGServices::DumpMode iMode)
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(20) << "Input parameter [iSqlOrder]=[" << iSqlOrder << ']' << Qt::endl;

    // initialisation
    QStringList oResult;
    err = SKGServices::dumpSelectSqliteOrder(iDb, iSqlOrder, oResult, iMode);
    IFOK(err)
    {
        // dump
        int nb = oResult.size();
        for (int i = 0; i < nb; ++i) {
            if (oStream == nullptr) {
                SKGTRACESUITE << oResult.at(i) << Qt::endl;
            } else {
                *oStream << oResult.at(i) << Qt::endl;
            }
        }
    }
    return err;
}

SKGError SKGServices::dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QString &oResult, SKGServices::DumpMode iMode)
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // initialisation
    oResult = QString();

    QStringList oResultTmp;
    err = SKGServices::dumpSelectSqliteOrder(iDb, iSqlOrder, oResultTmp, iMode);
    IFOK(err)
    {
        // dump
        int nb = oResultTmp.size();
        for (int i = 0; i < nb; ++i) {
            oResult += oResultTmp.at(i) % '\n';
        }
    }
    return err;
}

SKGError SKGServices::dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QStringList &oResult, SKGServices::DumpMode iMode)
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)

    // Execution of sql order
    SKGStringListList oResultTmp;
    err = executeSelectSqliteOrder(iDb, iSqlOrder, oResultTmp);
    IFOK(err) oResult = tableToDump(oResultTmp, iMode);
    return err;
}

SKGError SKGServices::executeSingleSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QString &oResult)
{
    SKGStringListList result;
    SKGError err = executeSelectSqliteOrder(iDb, iSqlOrder, result);
    oResult = result.value(1).value(0);
    return err;
}

SKGError SKGServices::executeSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, SKGStringListList &oResult)
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // initialisation
    oResult.clear();

    QSqlQuery query(QString(), iDb);
    query.setForwardOnly(true);
    double elapse = 0;
    if (SKGServices::SKGSqlTraces != -1) {
        elapse = SKGServices::getMicroTime();
    }

    if (!query.exec(iSqlOrder)) {
        QSqlError sqlError = query.lastError();
        if (qApp->thread() == QThread::currentThread()) {
            SKGTRACE << "WARNING: " << iSqlOrder << Qt::endl;
            SKGTRACE << "         returns :" << sqlError.text() << Qt::endl;
        }
        err = SKGError(SQLLITEERROR + sqlError.nativeErrorCode().toInt(), iSqlOrder);
        err.addError(SQLLITEERROR + sqlError.nativeErrorCode().toInt(), sqlError.text());
    } else {
        double elapse1 = 0;
        if (SKGServices::SKGSqlTraces != -1) {
            elapse1 = SKGServices::getMicroTime() - elapse;
        }

        int nbRows = query.size();
        if (nbRows == -1)
            oResult.reserve(5000);
        if (nbRows > 0)
            oResult.reserve(nbRows);

        // Addition of column names
        QSqlRecord rec = query.record();
        QStringList line;
        int nbFields = rec.count();
        if (nbRows > 0)
            line.reserve(nbFields);

        int index = 0;
        while (index != -1) {
            QString val = rec.fieldName(index);
            if (!val.isEmpty()) {
                line.push_back(val);
                ++index;
            } else {
                index = -1;
            }
        }
        oResult.push_back(line);

        // Addition of rows
        while (query.next()) {
            QStringList line2;
            int index2 = 0;
            while (index2 != -1) {
                QVariant val = query.value(index2);
                if (val.isValid()) {
                    line2.push_back(val.toString());
                    ++index2;
                } else {
                    index2 = -1;
                }
            }
            oResult.push_back(line2);
        }
        if (SKGServices::SKGSqlTraces != -1) {
            double elapse2 = SKGServices::getMicroTime() - elapse;
            if (elapse1 >= SKGServices::SKGSqlTraces) {
                SKGTRACE << "executeSqliteOrder: " << iSqlOrder << " (" << oResult.count() << "x" << nbFields << ") TIME=" << elapse1
                         << " ms !!!,  (with fetch):" << elapse2 << " ms" << Qt::endl;
            } else if (elapse2 >= SKGServices::SKGSqlTraces) {
                SKGTRACE << "executeSqliteOrder: " << iSqlOrder << " (" << oResult.count() << "x" << nbFields << ") TIME=" << elapse1
                         << " ms,  (with fetch):" << elapse2 << " ms !!!" << Qt::endl;
            }
        }
    }
    return err;
}

SKGError SKGServices::readPropertyFile(const QString &iFileName, QHash<QString, QString> &oProperties)
{
    SKGError err;
    oProperties.clear();

    // Open file
    QFile file(iFileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        err = SKGError(ERR_FAIL, i18nc("An error message", "Open file '%1' failed", iFileName));
    } else {
        // Read file
        QTextStream stream(&file);
        while (!stream.atEnd() && !err) {
            // Read line
            QString line = stream.readLine().trimmed();
            if (!line.isEmpty() && !line.startsWith(QStringLiteral("#"))) {
                int pos = line.indexOf(QStringLiteral("="));
                if (pos != -1) {
                    oProperties[line.left(pos).trimmed().toLower()] = line.right(line.size() - pos - 1);
                }
            }
        }

        // close file
        file.close();
    }
    return err;
}

double SKGServices::getMicroTime()
{
#ifdef Q_OS_WIN
    return static_cast<double>(GetTickCount());
#else
    struct timeval tv {
    };
    struct timezone tz {
    };

    // get time
    gettimeofday(&tv, &tz);

    // return time
    return (static_cast<double>(1000.0 * tv.tv_sec)) + (static_cast<double>(tv.tv_usec / 1000));
#endif
}

SKGStringListList SKGServices::getBase100Table(const SKGStringListList &iTable)
{
    SKGTRACEINFUNC(10)

    // Build history
    SKGStringListList output;
    int nblines = iTable.count();
    int nbCols = 0;
    if (nblines != 0) {
        nbCols = iTable.at(0).count();
    }

    output.reserve(nblines + 1);
    output.push_back(iTable.at(0));

    // Create table
    for (int i = 1; i < nblines; ++i) {
        QStringList newLine;
        newLine.reserve(nbCols + 1);
        newLine.push_back(iTable.at(i).at(0));

        double valInitial = 0;

        for (int j = 1; j < nbCols; ++j) {
            double val = SKGServices::stringToDouble(iTable.at(i).at(j));
            if (j == 1) {
                valInitial = val;
                val = 100.0;
            } else {
                if (valInitial != 0.0) {
                    val = 100.0 * val / valInitial;
                }
            }
            newLine.push_back(SKGServices::doubleToString(val));
        }
        output.push_back(newLine);
    }

    return output;
}

SKGStringListList SKGServices::getPercentTable(const SKGStringListList &iTable, bool iOfColumns, bool iAbsolute)
{
    SKGTRACEINFUNC(10)

    // Build history
    SKGStringListList output;
    int nblines = iTable.count();
    int nbCols = 0;
    if (nblines != 0) {
        nbCols = iTable.at(0).count();
    }

    output.reserve(nblines + 1);
    output.push_back(iTable.at(0));

    // Compute sums
    QList<double> sums;
    if (iOfColumns) {
        // Compute sum of columns
        sums.reserve(nbCols);
        for (int j = 1; j < nbCols; ++j) {
            // Compute sum
            double sum = 0;
            for (int i = 1; i < nblines; ++i) {
                double v = SKGServices::stringToDouble(iTable.at(i).at(j));
                sum += (iAbsolute ? qAbs(v) : v);
            }

            sums.push_back(sum);
        }
    } else {
        // Compute sum of lines
        sums.reserve(nblines);
        for (int j = 1; j < nblines; ++j) {
            // Compute sum
            double sum = 0;
            for (int i = 1; i < nbCols; ++i) {
                double v = SKGServices::stringToDouble(iTable.at(j).at(i));
                sum += (iAbsolute ? qAbs(v) : v);
            }

            sums.push_back(sum);
        }
    }

    // Create table
    for (int i = 1; i < nblines; ++i) {
        QStringList newLine;
        newLine.reserve(nbCols + 1);
        newLine.push_back(iTable.at(i).at(0));

        for (int j = 1; j < nbCols; ++j) {
            double val = SKGServices::stringToDouble(iTable.at(i).at(j));
            val = (iAbsolute ? qAbs(val) : val);
            double sum = (iOfColumns ? sums.at(j - 1) : sums.at(i - 1));
            newLine.push_back(SKGServices::doubleToString(sum == 0.0 ? 0.0 : 100.0 * val / sum));
        }
        output.push_back(newLine);
    }

    return output;
}

SKGStringListList SKGServices::getHistorizedTable(const SKGStringListList &iTable)
{
    SKGTRACEINFUNC(10)

    // Build history
    SKGStringListList output;
    int nblines = iTable.count();
    int nbCols = 0;
    if (nblines != 0) {
        nbCols = iTable.at(0).count();
    }

    output.reserve(nblines + 1);
    output.push_back(iTable.at(0));
    for (int i = 1; i < nblines; ++i) {
        QStringList newLine;
        newLine.reserve(nbCols + 1);
        newLine.push_back(iTable.at(i).at(0));

        double sum = 0;
        for (int j = 1; j < nbCols; ++j) {
            sum += SKGServices::stringToDouble(iTable.at(i).at(j));
            newLine.push_back(SKGServices::doubleToString(sum));
        }
        output.push_back(newLine);
    }

    return output;
}

QString SKGServices::encodeForUrl(const QString &iString)
{
    return QUrl::toPercentEncoding(iString);
}

QIcon SKGServices::fromTheme(const QString &iName, const QStringList &iOverlays)
{
    QIcon output;
    if (!iOverlays.isEmpty()) {
        output = KDE::icon(iName, iOverlays);
    } else {
        output = KDE::icon(iName);
    }
    if (output.isNull() && !iName.isEmpty()) {
        static QHash<QString, QString> alternatives;
        if (alternatives.count() == 0) {
            // Build alternatives
            alternatives[QStringLiteral("arrow-down")] = QStringLiteral("go-down");
            alternatives[QStringLiteral("arrow-right")] = QStringLiteral("go-next");
            alternatives[QStringLiteral("arrow-up")] = QStringLiteral("go-up");
            alternatives[QStringLiteral("arrow-down-double")] = QStringLiteral("go-down");
            alternatives[QStringLiteral("arrow-up-double")] = QStringLiteral("go-up");
            alternatives[QStringLiteral("bookmark")] = QStringLiteral("bookmark-new");
            alternatives[QStringLiteral("bookmarks")] = QStringLiteral("bookmark-new");
            alternatives[QStringLiteral("checkbox")] = QStringLiteral("emblem-symbolic-link");
            alternatives[QStringLiteral("view-calendar-day")] = QStringLiteral("appointment");
            alternatives[QStringLiteral("configure")] = QStringLiteral("preferences-desktop");
            alternatives[QStringLiteral("dashboard-show")] = QStringLiteral("user-desktop");
            alternatives[QStringLiteral("dialog-cancel")] = QStringLiteral("process-stop");
            alternatives[QStringLiteral("dialog-close")] = QStringLiteral("process-stop");
            alternatives[QStringLiteral("dialog-ok")] = QString();
            alternatives[QStringLiteral("download-later")] = QStringLiteral("internet-services");
            alternatives[QStringLiteral("download")] = QStringLiteral("internet-services");
            alternatives[QStringLiteral("draw-freehand")] = QStringLiteral("accessories-text-editor");
            alternatives[QStringLiteral("edit-guides")] = QStringLiteral("text-x-generic");
            alternatives[QStringLiteral("edit-rename")] = QStringLiteral("accessories-text-editor");
            alternatives[QStringLiteral("emblem-locked")] = QStringLiteral("lock");
            alternatives[QStringLiteral("exchange-positions")] = QString();
            alternatives[QStringLiteral("format-fill-color")] = QString();
            alternatives[QStringLiteral("games-solve")] = QStringLiteral("application-certificate");
            alternatives[QStringLiteral("get-hot-new-stuff")] = QStringLiteral("applications-other");
            alternatives[QStringLiteral("irc-operator")] = QString();
            alternatives[QStringLiteral("ktip")] = QStringLiteral("dialog-information");
            alternatives[QStringLiteral("labplot-xy-plot-two-axes-centered-origin")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("layer-visible-off")] = QString();
            alternatives[QStringLiteral("layer-visible-on")] = QString();
            alternatives[QStringLiteral("merge")] = QString();
            alternatives[QStringLiteral("office-chart-area")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-area-stacked")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-bar-percentage")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-bar")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-bar-stacked")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-line")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-line-stacked")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-pie")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-ring")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("map-flat")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("office-chart-scatter")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("preview")] = QStringLiteral("document-print-preview");
            alternatives[QStringLiteral("quickopen")] = QStringLiteral("emblem-symbolic-link");
            alternatives[QStringLiteral("run-build-configure")] = QStringLiteral("media-playback-start");
            alternatives[QStringLiteral("run-build")] = QStringLiteral("media-playback-start");
            alternatives[QStringLiteral("show-menu")] = QStringLiteral("applications-system");
            alternatives[QStringLiteral("skrooge_category")] = QStringLiteral("folder-open");
            alternatives[QStringLiteral("split")] = QStringLiteral("edit-cut");
            alternatives[QStringLiteral("taxes-finances")] = QStringLiteral("fonts");
            alternatives[QStringLiteral("tools-wizard")] = QStringLiteral("applications-other");
            alternatives[QStringLiteral("user-group-properties")] = QStringLiteral("im-user");
            alternatives[QStringLiteral("user-properties")] = QStringLiteral("document-properties");
            alternatives[QStringLiteral("utilities-file-archiver")] = QStringLiteral("package-x-generic");
            alternatives[QStringLiteral("vcs-conflicting")] = QStringLiteral("dialog-warning");
            alternatives[QStringLiteral("vcs-normal")] = QStringLiteral("dialog-information");
            alternatives[QStringLiteral("view-financial-account-checking")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-bank")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-institution-edit")] = QStringLiteral("view-bank-account");
            alternatives[QStringLiteral("view-financial-account-savings")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-financial-account-asset")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-financial-account-investment")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-financial-account-loan")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-calendar-journal")] = QStringLiteral("x-office-calendar");
            alternatives[QStringLiteral("view-calendar-month")] = QStringLiteral("x-office-calendar");
            alternatives[QStringLiteral("view-calendar-day")] = QStringLiteral("x-office-calendar");
            alternatives[QStringLiteral("view-calendar-week")] = QStringLiteral("x-office-calendar");
            alternatives[QStringLiteral("view-calendar-whatsnext")] = QStringLiteral("x-office-calendar");
            alternatives[QStringLiteral("view-categories")] = QStringLiteral("applications-all-symbolic");
            alternatives[QStringLiteral("applications-all-symbolic")] = QStringLiteral("folder-open");
            alternatives[QStringLiteral("applications-all-symbolic-expenditures")] = QStringLiteral("face-sad");
            alternatives[QStringLiteral("applications-all-symbolic-incomes")] = QStringLiteral("face-smile");
            alternatives[QStringLiteral("view-file-columns")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-financial-list")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-investment")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-list-details")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-list-text")] = QStringLiteral("go-home");
            alternatives[QStringLiteral("view-pim-calendar")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("view-statistics")] = QStringLiteral("x-office-spreadsheet");
            alternatives[QStringLiteral("window-duplicate")] = QStringLiteral("edit-copy");
            alternatives[QStringLiteral("zoom-fit-width")] = QStringLiteral("media-playback-stop");
            alternatives[QStringLiteral("smallclock")] = QString();
            alternatives[QStringLiteral("edit_undo")] = QStringLiteral("edit-undo");
            alternatives[QStringLiteral("nextuntranslated")] = QStringLiteral("debug-execute-to-cursor");
            alternatives[QStringLiteral("format-precision-less")] = QStringLiteral("visibility");
            alternatives[QStringLiteral("format-indent-more")] = QStringLiteral("go-next");
            alternatives[QStringLiteral("format-indent-less")] = QStringLiteral("go-previous");
            alternatives[QStringLiteral("crosshairs")] = QStringLiteral("emblem-symbolic-link");
        }
        bool alternativeEmpty = false;
        if (alternatives.contains(iName)) {
            auto alternative = alternatives.value(iName);
            alternativeEmpty = (alternative.isEmpty());
            if (!alternativeEmpty) {
                if (!iOverlays.isEmpty()) {
                    output = KDE::icon(alternative, iOverlays);
                } else {
                    output = KDE::icon(alternative);
                }
                SKGTRACE << "WARNING: Icon [" << iName << "] replaced by [" << alternative << "]" << Qt::endl;
            }
        }
        if (output.isNull() && !alternativeEmpty) {
            SKGTRACE << "WARNING: Icon [" << iName << "] not found" << Qt::endl;
            output = KDE::icon(QStringLiteral("script-error"));
            if (output.isNull()) {
                output = KDE::icon(QStringLiteral("image-missing"));
            }
        }
    }
    return output;
}

QString SKGServices::getMajorVersion(const QString &iVersion)
{
    QString output = iVersion;
    int pos = output.indexOf(u'.');
    if (pos != -1) {
        pos = output.indexOf('.', pos + 1);
        if (pos != -1) {
            output = output.left(pos);
        }
    }
    return output;
}

QString SKGServices::getFullPathCommandLine(const QString &iCommandLine)
{
    QString output = iCommandLine;
    if (!output.isEmpty()) {
        auto pathWords = SKGServices::splitCSVLine(output, u' ');
        QString fullpath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, qApp->applicationName() % QStringLiteral("/") % pathWords.at(0));
        if (!fullpath.isEmpty()) {
            output = output.replace(pathWords.at(0), fullpath);
        }
    }
    return output;
}

QVector<KPluginMetaData> SKGServices::findDataPlugins(const QString &type)
{
    QVector<KPluginMetaData> output;
    const auto &servicePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, type, QStandardPaths::LocateDirectory);
    if (!servicePath.isEmpty()) {
        SKGTRACEL(5) << "Searching in:" << servicePath << Qt::endl;
        QDir dir(servicePath);
        const auto &tmp = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
        for (const QString &file : std::as_const(tmp)) {
            KPluginMetaData metadata = KPluginMetaData::fromJsonFile(dir.filePath(file));
            if (metadata.isValid()) {
                output << metadata;
            }
        }
    }

    return output;
}
