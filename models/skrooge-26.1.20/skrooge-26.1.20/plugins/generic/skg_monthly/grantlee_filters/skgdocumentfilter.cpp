/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * The grantlee's filter to get items of a document table.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgdocumentfilter.h"

#include <ktexttemplate/util.h>

#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgreport.h"
#include "skgtraces.h"

#include <QMetaProperty>

#include <KFormat>

QString fixSQL(SKGReport *report, const QString &iSql)
{
    auto sql = iSql;
    sql = sql.replace(QStringLiteral("##PERIOD##"), SKGServices::getPeriodWhereClause(report->getPeriod()));
    sql = sql.replace(QStringLiteral("##<PERIOD##"), SKGServices::getPeriodWhereClause(report->getPeriod(), QStringLiteral("d_date"), QStringLiteral("<")));
    sql = sql.replace(QStringLiteral("##>PERIOD##"), SKGServices::getPeriodWhereClause(report->getPeriod(), QStringLiteral("d_date"), QStringLiteral(">")));
    sql = sql.replace(QStringLiteral("##<=PERIOD##"), SKGServices::getPeriodWhereClause(report->getPeriod(), QStringLiteral("d_date"), QStringLiteral("<=")));
    sql = sql.replace(QStringLiteral("##>=PERIOD##"), SKGServices::getPeriodWhereClause(report->getPeriod(), QStringLiteral("d_date"), QStringLiteral(">=")));

    sql = sql.replace(QStringLiteral("##PREVIOUS_PERIOD##"), SKGServices::getPeriodWhereClause(report->getPreviousPeriod()));
    sql = sql.replace(QStringLiteral("##<PREVIOUS_PERIOD##"),
                      SKGServices::getPeriodWhereClause(report->getPreviousPeriod(), QStringLiteral("d_date"), QStringLiteral("<")));
    sql = sql.replace(QStringLiteral("##>PREVIOUS_PERIOD##"),
                      SKGServices::getPeriodWhereClause(report->getPreviousPeriod(), QStringLiteral("d_date"), QStringLiteral(">")));
    sql = sql.replace(QStringLiteral("##<=PREVIOUS_PERIOD##"),
                      SKGServices::getPeriodWhereClause(report->getPreviousPeriod(), QStringLiteral("d_date"), QStringLiteral("<=")));
    sql = sql.replace(QStringLiteral("##>=PREVIOUS_PERIOD##"),
                      SKGServices::getPeriodWhereClause(report->getPreviousPeriod(), QStringLiteral("d_date"), QStringLiteral(">=")));
    return sql;
}

QVariant SKGDocumentTableFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    SKGReport *report = nullptr;
    auto *doc = qobject_cast<SKGDocument *>(input.value<QObject *>());
    if (doc == nullptr) {
        report = qobject_cast<SKGReport *>(input.value<QObject *>());
        doc = report->getDocument();
    }
    if ((doc != nullptr) && argument.isValid()) {
        SKGObjectBase::SKGListSKGObjectBase objects;
        QString table = KTextTemplate::getSafeString(argument);
        QString wc;
        int pos = table.indexOf(QStringLiteral(","));
        if (pos != -1) {
            if (table.at(table.size() - 1) == QStringLiteral(",")) {
                table = table.left(table.size() - 1);
            } else {
                wc = table.right(table.size() - pos - 1);
                table = table.left(pos);
            }
        }
        if (report != nullptr) {
            table = fixSQL(report, table);
            wc = fixSQL(report, wc);
        }

        doc->getObjects(table, wc, objects);
        return QVariant::fromValue(objects);
    }

    return QVariant();
}

bool SKGDocumentTableFilter::isSafe() const
{
    return true;
}

QVariant SKGDocumentQueryFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    SKGReport *report = nullptr;
    auto *doc = qobject_cast<SKGDocument *>(input.value<QObject *>());
    if (doc == nullptr) {
        report = qobject_cast<SKGReport *>(input.value<QObject *>());
        doc = report->getDocument();
    }
    if ((doc != nullptr) && argument.isValid()) {
        QString sql = KTextTemplate::getSafeString(argument);
        if (report != nullptr) {
            sql = fixSQL(report, sql);
        }
        SKGStringListList result;
        doc->executeSelectSqliteOrder(sql, result);
        return QVariant::fromValue(result);
    }

    return QVariant();
}

bool SKGDocumentQueryFilter::isSafe() const
{
    return true;
}

QVariant SKGDocumentDisplayFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    auto *doc = qobject_cast<SKGDocument *>(input.value<QObject *>());
    if (doc != nullptr) {
        return doc->getDisplay(KTextTemplate::getSafeString(argument));
    }

    return QVariant();
}

bool SKGDocumentDisplayFilter::isSafe() const
{
    return true;
}

QVariant SKGPercentFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    Q_UNUSED(argument)
    SKGMainPanel *mainPanel = SKGMainPanel::getMainPanel();
    SKGDocument *doc = nullptr;
    if (mainPanel != nullptr) {
        doc = mainPanel->getDocument();
    }
    if (doc != nullptr) {
        QString s = KTextTemplate::getSafeString(input);
        return QVariant(doc->formatPercentage(SKGServices::stringToDouble(s)));
    }
    return QVariant();
}

bool SKGPercentFilter::isSafe() const
{
    return true;
}

QVariant SKGFileSizeFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    Q_UNUSED(argument)

    auto size = SKGServices::stringToInt(KTextTemplate::getSafeString(input));
    return QVariant(KFormat().formatByteSize(size));
}

bool SKGFileSizeFilter::isSafe() const
{
    return true;
}

QVariant SKGDumpFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    Q_UNUSED(argument)

    if (input.isValid()) {
        auto *obj = input.value<QObject *>();
        if (obj != nullptr) {
            const QMetaObject *metaObject = obj->metaObject();
            QString table = QStringLiteral("<table class=\"table table-striped table-condensed \">");
            int nb = metaObject->propertyCount();
            for (int i = 0; i < nb; ++i) {
                QVariant val = SKGDumpFilter::doFilter(obj->property(metaObject->property(i).name()), QVariant(), autoescape);
                table += QStringLiteral("<tr><td>") % metaObject->property(i).name() % "</td><td>" % val.toString() % "</td></tr>";
            }
            table += QStringLiteral("</table>");
            return QVariant(table);
        }
        if (input.canConvert<SKGObjectBase>()) {
            SKGObjectBase objectBase = input.value<SKGObjectBase>();
            SKGQStringQStringMap att = objectBase.getAttributes();
            QStringList keys = att.keys();
            QString table = QStringLiteral("<table class=\"table table-striped table-condensed \">");
            int nb = keys.count();
            for (int i = 0; i < nb; ++i) {
                table += QStringLiteral("<tr><td>") % keys.at(i) % "</td><td>" % att[keys.at(i)] % "</td></tr>";
            }
            table += QStringLiteral("</table>");
            return QVariant(table);
        }
        if (input.canConvert<QVariantList>()) {
            QVariantList l = input.toList();
            int nb = l.count();
            QString table = QStringLiteral("<table class=\"table table-striped table-condensed \">");
            for (int i = 0; i < nb; ++i) {
                QVariant val = SKGDumpFilter::doFilter(l.at(i), QVariant(), autoescape);
                table += QStringLiteral("<tr><td>") % SKGServices::intToString(i) % "</td><td>" % val.toString() % "</td></tr>";
            }
            table += QStringLiteral("</table>");
            return QVariant(table);
        }
        if (input.canConvert<SKGObjectBase::SKGListSKGObjectBase>()) {
            SKGObjectBase::SKGListSKGObjectBase l = input.value<SKGObjectBase::SKGListSKGObjectBase>();
            int nb = l.count();
            QString table;
            if (nb != 0) {
                table = QStringLiteral("<table class=\"table table-striped table-condensed \"><tr><th>#</th>");
                for (int i = 0; i < nb; ++i) {
                    const SKGObjectBase &objectBase = l.at(i);
                    SKGQStringQStringMap att = objectBase.getAttributes();
                    QStringList keys = att.keys();
                    QString line = QStringLiteral("<tr><td>") % SKGServices::intToString(i) % "</td>";
                    int nbc = keys.count();
                    for (int j = 0; j < nbc; ++j) {
                        if (i == 0) {
                            table += QStringLiteral("<th>") % keys.at(j) % "</th>";
                        }
                        line += QStringLiteral("<td>") % att[keys.at(j)] % "</td>";
                    }
                    if (i == 0) {
                        table += QStringLiteral("</tr>");
                    }
                    line += QStringLiteral("</tr>");

                    table += line;
                }
                table += QStringLiteral("</table>");
            }
            return QVariant(table);
        }
        return QVariant(input.toString());
    }

    return QVariant();
}

bool SKGDumpFilter::isSafe() const
{
    return true;
}

QVariant SKGMoneyFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    QString arg = KTextTemplate::getSafeString(argument);
    SKGMainPanel *mainPanel = SKGMainPanel::getMainPanel();
    SKGDocument *doc = nullptr;
    if (mainPanel != nullptr) {
        doc = mainPanel->getDocument();
    }
    if (doc != nullptr) {
        QStringList args = SKGServices::splitCSVLine(arg);
        SKGServices::SKGUnitInfo unit = doc->getUnit(args.contains(QStringLiteral("2")) ? QStringLiteral("secondary") : QStringLiteral("primary"));
        if (args.contains(QStringLiteral("nodecimal"))) {
            unit.NbDecimal = 0;
        }
        return QVariant(doc->formatMoney(SKGServices::stringToDouble(KTextTemplate::getSafeString(input)), unit, !args.contains(QStringLiteral("nocolor"))));
    }
    return QVariant();
}

bool SKGMoneyFilter::isSafe() const
{
    return true;
}

QVariant SKGUrlEncodeFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    Q_UNUSED(argument)
    return QVariant(SKGServices::encodeForUrl(KTextTemplate::getSafeString(input)));
}

bool SKGUrlEncodeFilter::isSafe() const
{
    return true;
}

QVariant SKGReplaceFilter::doFilter(const QVariant &input, const QVariant &argument, bool autoescape) const
{
    Q_UNUSED(autoescape)
    QString arg = KTextTemplate::getSafeString(argument);
    QStringList args = SKGServices::splitCSVLine(arg, u';', false);
    QString output = KTextTemplate::getSafeString(input);
    if (args.count() == 2) {
        output = output.replace(args.at(0), args.at(1));
    }
    return QVariant(output);
}

bool SKGReplaceFilter::isSafe() const
{
    return true;
}
