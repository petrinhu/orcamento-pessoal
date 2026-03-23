/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin for monthly report.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgreportbank.h"

#include <qdir.h>
#include <qstandardpaths.h>
#include <qurl.h>

#include <kcolorscheme.h>

#include "skgaccountobject.h"
#include "skgdocumentbank.h"
#include "skgoperationobject.h"
#include "skgrecurrentoperationobject.h"
#include "skgruleobject.h"
#include "skgtraces.h"
#include "skgunitobject.h"
#include <utility>

SKGReportBank::SKGReportBank(SKGDocument *iDocument)
    : SKGReport(iDocument)
{
    SKGTRACEINFUNC(1)
    connect(this, &SKGReportBank::changed, this, &SKGReportBank::changed2);
}

SKGReportBank::~SKGReportBank(){SKGTRACEINFUNC(1)}

QVariantList SKGReportBank::getAlarms()
{
    QString cacheId = QStringLiteral("getAlarms");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        if (doc != nullptr) {
            SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();

            SKGObjectBase::SKGListSKGObjectBase rules;
            SKGError err = doc->getObjects(QStringLiteral("v_rule"), QStringLiteral("t_action_type='A' ORDER BY i_ORDER"), rules);
            int nb = rules.count();
            if (nb != 0) {
                for (int i = 0; !err && i < nb; ++i) {
                    SKGRuleObject rule(rules.at(i));
                    SKGRuleObject::SKGAlarmInfo alarm = rule.getAlarmInfo();

                    QVariantList item; // clazy:exclude=container-inside-loop
                    // Build the message
                    if (alarm.Message.contains(QStringLiteral("%3"))) {
                        alarm.Message = alarm.Message.arg(doc->formatMoney(alarm.Amount, primary, false),
                                                          doc->formatMoney(alarm.Limit, primary, false),
                                                          doc->formatMoney(alarm.Amount - alarm.Limit, primary, false));
                    } else if (alarm.Message.contains(QStringLiteral("%2"))) {
                        alarm.Message = alarm.Message.arg(doc->formatMoney(alarm.Amount, primary, false), doc->formatMoney(alarm.Limit, primary, false));
                    } else if (alarm.Message.contains(QStringLiteral("%1"))) {
                        alarm.Message = alarm.Message.arg(doc->formatMoney(alarm.Amount, primary, false));
                    }
                    item.push_back(alarm.Message);
                    item.push_back(alarm.Amount);
                    item.push_back(alarm.Limit);
                    item.push_back(alarm.Amount - alarm.Limit);
                    item.push_back(alarm.Raised);

                    table.push_back(item);
                }
            }
            m_cache[cacheId] = table;
        }
    }
    return table;
}

QVariantList SKGReportBank::getInterests()
{
    QString cacheId = QStringLiteral("getInterests");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        if (doc != nullptr) {
            // Build display
            int year = SKGServices::periodToDate(getPeriod()).year();
            SKGObjectBase::SKGListSKGObjectBase objs;
            auto sqlFilter = getSqlFilter();
            if (sqlFilter.isEmpty()) {
                sqlFilter = QStringLiteral("1=1");
            }
            SKGError err = doc->getObjects(
                QStringLiteral("v_account"),
                "(" % sqlFilter
                    % QStringLiteral(") AND t_close='N' AND EXISTS(select 1 from interest where interest.rd_account_id=v_account.id) ORDER BY t_name"),
                objs);
            IFOK(err)
            {
                int nb = objs.count();
                table.reserve(nb + 2);
                if (nb != 0) {
                    {
                        // Add header
                        QVariantList item;
                        item.push_back(false);
                        item.push_back(i18nc("Title", "Account"));
                        item.push_back(year);
                        table.push_back(item);
                    }

                    // Add items
                    double sum = 0;
                    for (int i = 0; i < nb; ++i) {
                        SKGAccountObject obj(objs.at(i));

                        SKGAccountObject::SKGInterestItemList oInterestList;
                        double oInterests = 0;
                        obj.getInterestItems(oInterestList, oInterests, year);
                        sum += oInterests;

                        QVariantList item; // clazy:exclude=container-inside-loop
                        item.push_back(false);
                        item.push_back(obj.getName());
                        item.push_back(oInterests);
                        table.push_back(item);
                    }
                    {
                        // Add sum
                        QVariantList item;
                        item.push_back(true);
                        item.push_back(i18nc("Noun, the numerical total of a sum of values", "Total"));
                        item.push_back(sum);
                        table.push_back(item);
                    }
                }
            }

            m_cache[cacheId] = table;
        }
    }
    return table;
}

QVariantList SKGReportBank::getBudgetTable()
{
    QString cacheId = QStringLiteral("getBudgetTable");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        table = doc != nullptr ? doc->getBudget(getPeriod()) : QVariantList();
        m_cache[cacheId] = table;
    }
    return table;
}

QVariantList SKGReportBank::getPortfolio()
{
    QString cacheId = QStringLiteral("getPortfolio");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        QString period = getPeriod();
        if (!period.isEmpty()) {
            QDate date = qMin(SKGServices::periodToDate(period), QDate::currentDate().addDays(1 - QDate::currentDate().day()).addMonths(1).addDays(-1));
            auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
            if (doc != nullptr) {
                SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();

                // Get list of transactions
                SKGObjectBase::SKGListSKGObjectBase objs;
                auto sqlFilter = getSqlFilter();
                if (sqlFilter.isEmpty()) {
                    sqlFilter = QStringLiteral("1=1");
                }
                SKGError err = doc->getObjects(QStringLiteral("v_operation_display"),
                                               "d_date<'" % SKGServices::dateToSqlString(date)
                                                   % "' AND rc_unit_id IN (SELECT id FROM v_unit_display WHERE t_type='S' AND (" % sqlFilter
                                                   % ") AND f_QUANTITYOWNED>0.01) ORDER BY t_UNIT",
                                               objs);
                int nb = objs.count();
                if (!err && nb > 0) {
                    table.reserve(nb + 1);

                    QVariantList line;
                    line << doc->getDisplay(QStringLiteral("t_symbol")) << doc->getDisplay(QStringLiteral("t_UNIT")) << i18nc("Column table title", "Quantity")
                         << i18nc("Column table title", "Purchase amount") << i18nc("Column table title", "Initial amount")
                         << QLocale().toString(date, QLocale::ShortFormat) << i18nc("Column table title", "Variation")
                         << i18nc("Column table title", "Variation %");
                    table << QVariant(line);

                    QVector<unitValues> listUnitValues;
                    unitValues current;
                    current.initalAmount = 0.0;
                    current.purchaseAmount = 0.0;
                    current.currentAmount = 0.0;
                    current.quantity = 0.0;
                    listUnitValues.reserve(nb);
                    for (int i = 0; i < nb; ++i) {
                        SKGOperationObject obj(objs.at(i));
                        SKGUnitObject unit;
                        obj.getUnit(unit);
                        if (i != 0 && current.unit != unit) {
                            listUnitValues.push_back(current);
                            current.initalAmount = 0.0;
                            current.purchaseAmount = 0.0;
                            current.currentAmount = 0.0;
                            current.quantity = 0.0;
                        }

                        current.unit = unit;
                        current.initalAmount += obj.getAmount(obj.getDate());
                        current.currentAmount += obj.getAmount(date);
                        SKGObjectBase::SKGListSKGObjectBase oGroupedOperations;
                        obj.getGroupedOperations(oGroupedOperations);
                        oGroupedOperations.removeAll(obj);
                        if (oGroupedOperations.count() == 1) {
                            SKGOperationObject obj2(oGroupedOperations.at(0));
                            current.purchaseAmount += obj2.getAmount(obj.getDate());
                        }
                        current.quantity += SKGServices::stringToDouble(obj.getAttribute(QStringLiteral("f_QUANTITY")));
                    }
                    if (!current.unit.getName().isEmpty()) {
                        listUnitValues.push_back(current);
                    }

                    nb = listUnitValues.count();
                    for (int j = 0; j < nb; ++j) {
                        unitValues current2 = listUnitValues.at(j);

                        SKGServices::SKGUnitInfo ui = current2.unit.getUnitInfo();
                        ui.Value = 1;

                        QVariantList line2; // clazy:exclude=container-inside-loop
                        line2 << current2.unit.getSymbol() << current2.unit.getName() << doc->formatMoney(current2.quantity, ui, false)
                              << current2.purchaseAmount << current2.initalAmount << current2.currentAmount << current2.currentAmount - current2.initalAmount
                              << (current2.initalAmount == 0.0 ? 0.0 : 100.0 * (current2.currentAmount - current2.initalAmount) / current2.initalAmount);
                        table << QVariant(line2);
                    }
                }
            }
        }
        m_cache[cacheId] = table;
    }

    return table;
}

QVariantList SKGReportBank::getUnitTable()
{
    QString cacheId = QStringLiteral("getUnitTable");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        QString period = getPeriod();
        if (!period.isEmpty()) {
            QDate date1 = SKGServices::periodToDate(getPreviousPeriod());
            QDate date2 = SKGServices::periodToDate(period);

            auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
            if (doc != nullptr) {
                SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();

                SKGObjectBase::SKGListSKGObjectBase units;
                SKGError err = doc->getObjects(QStringLiteral("v_unit_display"), QStringLiteral("1=1 ORDER BY t_TYPENLS"), units);
                int nbUnits = units.count();
                if (nbUnits != 0) {
                    table.reserve(nbUnits + 1);
                    QVariantList line;
                    line << "sum" << doc->getDisplay(QStringLiteral("t_UNIT")) << QLocale().toString(date1, QLocale::ShortFormat)
                         << QLocale().toString(date2, QLocale::ShortFormat) << "%" << doc->getDisplay(QStringLiteral("t_symbol"));
                    table << QVariant(line);
                    for (const auto &item : std::as_const(units)) {
                        SKGUnitObject unit(item);
                        double v1 = unit.getAmount(date1);
                        double v2 = unit.getAmount(date2);
                        QVariantList line2; // clazy:exclude=container-inside-loop
                        line2 << false << unit.getName() << v1 << v2 << (v1 == 0 ? 0.0 : 100.0 * (v2 - v1) / qAbs(v1)) << unit.getSymbol();
                        table << QVariant(line2);
                    }
                }
            }
        }
        m_cache[cacheId] = table;
    }

    return table;
}

QVariantList SKGReportBank::getAccountTable()
{
    QString cacheId = QStringLiteral("getAccountTable");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        QString period = getPeriod();
        if (!period.isEmpty()) {
            QDate date1 = SKGServices::periodToDate(getPreviousPeriod());
            QDate date2 = SKGServices::periodToDate(period);
            QDate date3 = date2.addYears(-1);
            if (date3 == date1) {
                date3 = date3.addYears(-1);
            }

            auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
            if (doc != nullptr) {
                SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();

                SKGObjectBase::SKGListSKGObjectBase accounts;
                auto sqlFilter = getSqlFilter();
                if (sqlFilter.isEmpty()) {
                    sqlFilter = QStringLiteral("1=1");
                }
                SKGError err = doc->getObjects(QStringLiteral("v_account"), sqlFilter + QStringLiteral(" ORDER BY t_TYPENLS, t_BANK, t_name"), accounts);
                IFOK(err)
                {
                    table.push_back(QVariantList() << "sum" << doc->getDisplay(QStringLiteral("t_ACCOUNT")) << QLocale().toString(date1, QLocale::ShortFormat)
                                                   << QLocale().toString(date2, QLocale::ShortFormat) << "%" << QLocale().toString(date3, QLocale::ShortFormat)
                                                   << QLocale().toString(date2, QLocale::ShortFormat) << "%");
                    double sumTypeV1 = 0;
                    double sumTypeV2 = 0;
                    double sumTypeV3 = 0;
                    double sumV1 = 0;
                    double sumV2 = 0;
                    double sumV3 = 0;
                    int nbAdded = 0;
                    QString currentType;
                    int nb = accounts.count();
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGAccountObject account(accounts.at(i));
                        double v1 = account.getAmount(date1);
                        double v2 = account.getAmount(date2);
                        double v3 = account.getAmount(date3);
                        QString type = account.getAttribute(QStringLiteral("t_TYPENLS"));
                        bool closed = account.isClosed();
                        if (type != currentType) {
                            if (!currentType.isEmpty() && nbAdded > 0) {
                                table.push_back(QVariantList() << true << i18nc("Noun", "Total of %1", currentType) << sumTypeV1 << sumTypeV2
                                                               << (100.0 * (sumTypeV2 - sumTypeV1) / qAbs(sumTypeV1)) << sumTypeV3 << sumTypeV2
                                                               << (100.0 * (sumTypeV2 - sumTypeV3) / qAbs(sumTypeV3)) << "" << "");
                                sumTypeV1 = 0;
                                sumTypeV2 = 0;
                                sumTypeV3 = 0;
                                nbAdded = 0;
                            }
                            currentType = std::move(type);
                        }
                        if (!closed || qAbs(v1) > 0.01 || qAbs(v2) > 0.01 || qAbs(v3) > 0.01) {
                            QString icon = account.getAttribute(QStringLiteral("t_ICON"));
                            if (!icon.isEmpty()) {
                                QString iconfile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/images/logo/" % icon);
                                if (!iconfile.isEmpty()) {
                                    icon = std::move(iconfile);
                                }
                            }
                            if (v1 != 0 || v3 != 0)
                                table.push_back(QVariantList() << false << account.getName() << v1 << v2 << (100.0 * (v2 - v1) / qAbs(v1)) << v3 << v2
                                                               << (100.0 * (v2 - v3) / qAbs(v3)) << account.getAttribute(QStringLiteral("t_BANK")) << icon);
                            nbAdded++;
                        }
                        sumTypeV1 += v1;
                        sumTypeV2 += v2;
                        sumTypeV3 += v3;
                        sumV1 += v1;
                        sumV2 += v2;
                        sumV3 += v3;
                    }

                    if (sumTypeV1 != 0 || sumTypeV3 != 0)
                        table.push_back(QVariantList() << true << i18nc("Noun", "Total of %1", currentType) << sumTypeV1 << sumTypeV2
                                                       << (100.0 * (sumTypeV2 - sumTypeV1) / qAbs(sumTypeV1)) << sumTypeV3 << sumTypeV2
                                                       << (100.0 * (sumTypeV2 - sumTypeV3) / qAbs(sumTypeV3)) << "" << "");

                    if (sumV1 != 0 || sumV3 != 0)
                        table.push_back(QVariantList()
                                        << true << i18nc("Noun, the numerical total of a sum of values", "Total") << sumV1 << sumV2
                                        << (100.0 * (sumV2 - sumV1) / qAbs(sumV1)) << sumV3 << sumV2 << (100.0 * (sumV2 - sumV3) / qAbs(sumV3)) << "" << "");
                }
            }
        }
        m_cache[cacheId] = table;
    }
    return table;
}

QVariantList SKGReportBank::getBankTable()
{
    QString cacheId = QStringLiteral("getBankTable");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        QString period = getPeriod();
        if (!period.isEmpty()) {
            QDate date1 = SKGServices::periodToDate(getPreviousPeriod());
            QDate date2 = SKGServices::periodToDate(period);
            QDate date3 = date2.addYears(-1);
            if (date3 == date1) {
                date3 = date3.addYears(-1);
            }
            auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
            if (doc != nullptr) {
                SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();

                SKGObjectBase::SKGListSKGObjectBase accounts;
                auto sqlFilter = getSqlFilter();
                if (sqlFilter.isEmpty()) {
                    sqlFilter = QStringLiteral("1=1");
                }
                SKGError err = doc->getObjects(QStringLiteral("v_account"), sqlFilter + QStringLiteral(" ORDER BY t_BANK"), accounts);
                IFOK(err)
                {
                    table.push_back(QVariantList() << "sum" << doc->getDisplay(QStringLiteral("t_BANK")) << QLocale().toString(date1, QLocale::ShortFormat)
                                                   << QLocale().toString(date2, QLocale::ShortFormat) << "%" << QLocale().toString(date3, QLocale::ShortFormat)
                                                   << QLocale().toString(date2, QLocale::ShortFormat) << "%");
                    double sumTypeV1 = 0;
                    double sumTypeV2 = 0;
                    double sumTypeV3 = 0;
                    double sumV1 = 0;
                    double sumV2 = 0;
                    double sumV3 = 0;
                    QString currentName;
                    QString currentIcon;
                    bool currentOpen = false;
                    int nb = accounts.count();
                    for (int i = 0; !err && i < nb; ++i) {
                        SKGAccountObject account(accounts.at(i));
                        double v1 = account.getAmount(date1);
                        double v2 = account.getAmount(date2);
                        double v3 = account.getAmount(date3);
                        QString name = account.getAttribute(QStringLiteral("t_BANK"));
                        QString icon = account.getAttribute(QStringLiteral("t_ICON"));
                        if (!icon.isEmpty()) {
                            QString iconfile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/images/logo/" % icon);
                            if (!iconfile.isEmpty()) {
                                icon = std::move(iconfile);
                            }
                        }
                        bool open = !account.isClosed();
                        if (name != currentName) {
                            if (!currentName.isEmpty() && currentOpen) {
                                table.push_back(QVariantList()
                                                << false << currentName << sumTypeV1 << sumTypeV2 << (100.0 * (sumTypeV2 - sumTypeV1) / qAbs(sumTypeV1))
                                                << sumTypeV3 << sumTypeV2 << (100.0 * (sumTypeV2 - sumTypeV3) / qAbs(sumTypeV3)) << currentIcon);
                                sumTypeV1 = 0;
                                sumTypeV2 = 0;
                                sumTypeV3 = 0;
                                currentOpen = open;
                            }
                            currentName = std::move(name);
                            currentIcon = std::move(icon);
                        }
                        currentOpen = currentOpen || open;

                        sumTypeV1 += v1;
                        sumTypeV2 += v2;
                        sumTypeV3 += v3;
                        sumV1 += v1;
                        sumV2 += v2;
                        sumV3 += v3;
                    }

                    if (currentOpen) {
                        table.push_back(QVariantList() << false << currentName << sumTypeV1 << sumTypeV2 << (100.0 * (sumTypeV2 - sumTypeV1) / qAbs(sumTypeV1))
                                                       << sumTypeV3 << sumTypeV2 << (100.0 * (sumTypeV2 - sumTypeV3) / qAbs(sumTypeV3)) << currentIcon);
                    }

                    if (sumV1 != 0 || sumV3 != 0) {
                        table.push_back(QVariantList() << true << i18nc("Noun, the numerical total of a sum of values", "Total") << sumV1 << sumV2
                                                       << (100.0 * (sumV2 - sumV1) / qAbs(sumV1)) << sumV3 << sumV2 << (100.0 * (sumV2 - sumV3) / qAbs(sumV3)));
                    }
                }
            }
        }
        m_cache[cacheId] = table;
    }
    return table;
}

QVariantList SKGReportBank::getScheduledOperations()
{
    QString cacheId = QStringLiteral("getScheduledOperations");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        SKGObjectBase::SKGListSKGObjectBase objs;
        auto scheduled_operation_days_max = m_parameters.value(QStringLiteral("scheduled_operation_days_max"), QStringLiteral("30")).toString();
        SKGError err = m_document->getObjects(QStringLiteral("v_recurrentoperation_display"),
                                              QStringLiteral("i_nb_times!=0 AND d_date<=date('now', 'localtime','+") + scheduled_operation_days_max
                                                  + " day') ORDER BY d_date",
                                              objs);
        QDate d = QDate::currentDate().addDays(SKGServices::stringToInt(scheduled_operation_days_max));
        QString dateFormatShort = QLocale().dateFormat(QLocale::ShortFormat);
        IFOK(err)
        {
            int nb = objs.count();
            if (nb != 0) {
                table.reserve(nb);
                for (int i = 0; i < nb; ++i) {
                    SKGRecurrentOperationObject obj(objs.at(i));
                    bool first = true;
                    auto obj_date = obj.getDate().toString(dateFormatShort);
                    while (true) {
                        if (obj.getDate() > d || (obj.hasTimeLimit() && obj.getTimeLimit() == 0)) {
                            break;
                        } else {
                            bool bold = false;
                            if (obj.isWarnEnabled() && QDate::currentDate() >= obj.getDate().addDays(-obj.getWarnDays())) {
                                bold = true;
                            }
                            auto name = obj.getDisplayName();
                            if (!first) {
                                name = name.replace(obj_date, obj.getDate().toString(dateFormatShort));
                            }
                            table.push_back(QVariantList() << bold << name << (first ? obj.getUniqueID() : QString()) << obj.getDate());
                            first = false;
                            obj.setDate(obj.getNextDate());
                            if (obj.hasTimeLimit()) {
                                obj.setTimeLimit(obj.getTimeLimit() - 1);
                            }
                        }
                    }
                }
            }
            std::sort(table.begin(), table.end(), [](const QVariant &v1, const QVariant &v2) {
                return v1.toList().at(3).toDate() < v2.toList().at(3).toDate();
            });
            m_cache[cacheId] = table;
        }
    }
    return table;
}

QVariantList SKGReportBank::getMainCategoriesForPeriod()
{
    QString cacheId = QStringLiteral("getMainCategoriesForPeriod");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        table = doc != nullptr ? doc->getMainCategories(getPeriod(), 5) : QVariantList();
        m_cache[cacheId] = table;
    }
    return table;
}

QVariantList SKGReportBank::getMainCategoriesForPreviousPeriod()
{
    QString cacheId = QStringLiteral("getMainCategoriesForPreviousPeriod");
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        table = doc != nullptr ? doc->getMainCategories(getPreviousPeriod(), 5) : QVariantList();
        m_cache[cacheId] = table;
    }
    return table;
}

QStringList SKGReportBank::get5MainCategoriesVariation()
{
    QString cacheId = QStringLiteral("get5MainCategoriesVariation");
    QStringList table = m_cache.value(cacheId).toStringList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        table = doc != nullptr ? doc->get5MainCategoriesVariationList(getPeriod(), getPreviousPeriod(), false) : QStringList();
        m_cache[cacheId] = table;
    }
    return table;
}

QStringList SKGReportBank::get5MainCategoriesVariationIssue()
{
    QString cacheId = QStringLiteral("get5MainCategoriesVariationIssue");
    QStringList table = m_cache.value(cacheId).toStringList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        table = doc != nullptr ? doc->get5MainCategoriesVariationList(getPeriod(), getPreviousPeriod(), true) : QStringList();
        m_cache[cacheId] = table;
    }
    return table;
}

QVariantMap SKGReportBank::getPersonalFinanceScoreDetails(bool iTransfer, bool iTracker)
{
    KColorScheme scheme(QPalette::Normal, KColorScheme::Window);

    QVariantMap output;
    double pfs = getPersonalFinanceScore(iTransfer, iTracker);
    output[QStringLiteral("value")] = pfs;
    if (pfs < 0) {
        output[QStringLiteral("level")] = QStringLiteral("danger");
        output[QStringLiteral("message")] = i18nc("An advice", "You must try to get out of debt.");
        output[QStringLiteral("color")] = scheme.foreground(KColorScheme::NegativeText).color().name().right(6);
    } else if (pfs >= 25) {
        output[QStringLiteral("level")] = QStringLiteral("success");
        output[QStringLiteral("message")] = i18nc("An advice", "Congratulations, you are now financially independent.");
        output[QStringLiteral("color")] = scheme.foreground(KColorScheme::PositiveText).color().name().right(6);
    } else if (pfs >= 10) {
        output[QStringLiteral("level")] = QStringLiteral("success");
        output[QStringLiteral("message")] = i18nc("An advice", "Congratulations, You saved up ten year’s worth of expenses.");
        output[QStringLiteral("color")] = scheme.foreground(KColorScheme::PositiveText).color().name().right(6);
    } else if (pfs >= 2) {
        output[QStringLiteral("level")] = QStringLiteral("warning");
        output[QStringLiteral("message")] =
            i18nc("An advice", "You saved up %1 year’s worth of expenses. You should continue your effort.", SKGServices::intToString(pfs));
        output[QStringLiteral("color")] = scheme.foreground(KColorScheme::NeutralText).color().name().right(6);
    } else if (pfs >= 1) {
        output[QStringLiteral("level")] = QStringLiteral("warning");
        output[QStringLiteral("message")] = i18nc("An advice", "You saved up one year’s worth of expenses. You should maintain your effort.");
        output[QStringLiteral("color")] = scheme.foreground(KColorScheme::NeutralText).color().name().right(6);
    } else {
        output[QStringLiteral("level")] = QStringLiteral("warning");
        output[QStringLiteral("message")] = i18nc("An advice", "You do not have debt but you have no margin. You must maintain your effort.");
        output[QStringLiteral("color")] = scheme.foreground(KColorScheme::NeutralText).color().name().right(6);
    }
    return output;
}

double SKGReportBank::getPersonalFinanceScore(bool iTransfer, bool iTracker)
{
    double as = getAnnualSpending(iTransfer, iTracker);
    return (as == 0.0 ? 0.0 : getNetWorth(iTransfer, iTracker) / as);
}

double SKGReportBank::getAnnualSpending(bool iTransfer, bool iTracker)
{
    QString cacheId =
        QStringLiteral("getAnnualSpending-") % (iTransfer ? QStringLiteral("Y") : QStringLiteral("N")) % (iTracker ? QStringLiteral("Y") : QStringLiteral("N"));
    double output = m_cache.value(cacheId).toDouble();
    if (!m_cache.contains(cacheId)) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        if (doc != nullptr) {
            QString result;
            QDate d2 = SKGServices::periodToDate(getPeriod());
            QDate d1 = d2.addYears(-1);
            doc->executeSingleSelectSqliteOrder("SELECT TOTAL(f_REALCURRENTAMOUNT) FROM v_suboperation_consolidated WHERE d_date BETWEEN '"
                                                    % SKGServices::dateToSqlString(d1) % "' AND '" % SKGServices::dateToSqlString(d2)
                                                    % "' AND t_TYPEEXPENSE='-'" % (iTransfer ? "" : " AND t_TRANSFER='N'")
                                                    % (iTracker ? "" : " AND t_REFUND=''"),
                                                result);
            output = -SKGServices::stringToDouble(result);
            m_cache[cacheId] = output;
        }
    }
    return output;
}

double SKGReportBank::getNetWorth(bool iTransfer, bool iTracker)
{
    QString cacheId =
        QStringLiteral("getNetWorth-") % (iTransfer ? QStringLiteral("Y") : QStringLiteral("N")) % (iTracker ? QStringLiteral("Y") : QStringLiteral("N"));
    double output = m_cache.value(cacheId).toDouble();
    if (!m_cache.contains(cacheId)) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        if (doc != nullptr) {
            QString result;
            QDate d = SKGServices::periodToDate(getPeriod());
            doc->executeSingleSelectSqliteOrder("SELECT TOTAL(f_REALCURRENTAMOUNT) FROM v_suboperation_consolidated WHERE d_date<='"
                                                    % SKGServices::dateToSqlString(d) % "' " % (iTransfer ? "" : " AND t_TRANSFER='N'")
                                                    % (iTracker ? "" : " AND t_REFUND=''"),
                                                result);
            output = SKGServices::stringToDouble(result);
            m_cache[cacheId] = output;
        }
    }
    return output;
}

QVariantList SKGReportBank::getIncomeVsExpenditure(bool iOnSubOperation,
                                                   bool iGrouped,
                                                   bool iTransfer,
                                                   bool iTracker,
                                                   const QString &iWhereClause1,
                                                   const QString &iWhereClause2)
{
    QString cacheId = QStringLiteral("getIncomeVsExpenditure-") % (iOnSubOperation ? QStringLiteral("Y") : QStringLiteral("N"))
        % (iGrouped ? QStringLiteral("Y") : QStringLiteral("N")) % (iTransfer ? QStringLiteral("Y") : QStringLiteral("N"))
        % (iTracker ? QStringLiteral("Y") : QStringLiteral("N")) % iWhereClause1 % iWhereClause2;
    QVariantList table = m_cache.value(cacheId).toList();
    if (table.isEmpty()) {
        SKGTRACEINFUNC(10)
        auto *doc = qobject_cast<SKGDocumentBank *>(m_document);
        if (doc != nullptr) {
            QString tableDb = (iOnSubOperation ? QStringLiteral("v_suboperation_consolidated") : QStringLiteral("v_operation_display"));
            QString amount = (iOnSubOperation ? QStringLiteral("f_REALCURRENTAMOUNT") : QStringLiteral("f_CURRENTAMOUNT"));

            SKGStringListList listTmp;
            auto wc1 = iWhereClause1;
            if (wc1.isEmpty()) {
                wc1 = SKGServices::getPeriodWhereClause(getPeriod());
            }
            auto wc2 = iWhereClause2;
            if (wc2.isEmpty()) {
                wc2 = SKGServices::getPeriodWhereClause(getPreviousPeriod());
            }

            auto sqlFilter = getSqlFilter();
            if (sqlFilter.isEmpty()) {
                sqlFilter = QStringLiteral("1=1");
            }
            SKGError err = doc->executeSelectSqliteOrder(
                "SELECT TOTAL(" % amount % "), '1'  from " % tableDb % " WHERE " % wc1 % " AND " % sqlFilter % (iGrouped ? "" : " AND i_group_id=0")
                    % (iTransfer ? "" : " AND t_TRANSFER='N'") % (iTracker ? "" : (iOnSubOperation ? " AND t_REALREFUND=''" : " AND t_REFUND=''"))
                    % " group by t_TYPEEXPENSE " % " UNION ALL " % "SELECT TOTAL(" % amount % "), '2'  from " % tableDb % " WHERE " % wc2 % " AND " % sqlFilter
                    % (iGrouped ? "" : " AND i_group_id=0") % (iTransfer ? "" : " AND t_TRANSFER='N'")
                    % (iTracker ? "" : (iOnSubOperation ? " AND t_REALREFUND=''" : " AND t_REFUND=''")) % " group by t_TYPEEXPENSE ",
                listTmp);
            IFOK(err)
            {
                double income_previous_period = 0;
                double expense_previous_period = 0;
                double income_period = 0;
                double expense_period = 0;

                int nbval = listTmp.count();
                for (int i = 1; i < nbval; ++i) { // Ignore header
                    QString m = listTmp.at(i).at(1);
                    double v = SKGServices::stringToDouble(listTmp.at(i).at(0));
                    if (v > 0 && m == QStringLiteral("1")) {
                        income_period = v;
                    } else if (v < 0 && m == QStringLiteral("1")) {
                        expense_period = v;
                    } else if (v > 0 && m == QStringLiteral("2")) {
                        income_previous_period = v;
                    } else if (v < 0 && m == QStringLiteral("2")) {
                        expense_previous_period = v;
                    }
                }

                double saving_previous_period = income_previous_period + expense_previous_period;
                double saving_period = income_period + expense_period;

                table.push_back(QVariantList() << QStringLiteral("sum") << QString() << getPreviousPeriod() << getPeriod() << QStringLiteral("max"));
                table.push_back(QVariantList() << false << doc->getDisplay(QStringLiteral("f_CURRENTAMOUNT_INCOME")) << qAbs(income_previous_period)
                                               << qAbs(income_period));
                table.push_back(QVariantList() << false << doc->getDisplay(QStringLiteral("f_CURRENTAMOUNT_EXPENSE")) << qAbs(expense_previous_period)
                                               << qAbs(expense_period));
                table.push_back(QVariantList() << true << i18nc("Noun", "Savings possible") << saving_previous_period << saving_period);
                table.push_back(QVariantList() << true << i18nc("Noun", "Max") << qMax(qAbs(income_previous_period), qAbs(expense_previous_period))
                                               << qMax(qAbs(income_period), qAbs(expense_period)));
            }
            m_cache[cacheId] = table;
        }
    }
    return table;
}

void SKGReportBank::addItemsInMapping(QVariantHash &iMapping)
{
    SKGReport::addItemsInMapping(iMapping);
    iMapping.insert(QStringLiteral("about_forumpage"), QStringLiteral("https://forum.kde.org/viewforum.php?f=210"));
    iMapping.insert(QStringLiteral("about_newspage"), QStringLiteral("https://skrooge.org/news"));
    iMapping.insert(QStringLiteral("about_operationpage"), QStringLiteral("skg://Skrooge_operation_plugin/"));
    iMapping.insert(QStringLiteral("about_accountpage"), QStringLiteral("skg://Skrooge_bank_plugin/"));
    iMapping.insert(QStringLiteral("about_importurl"), QStringLiteral("skg://import_operation/"));
    iMapping.insert(QStringLiteral("about_maintext"),
                    i18nc("The main text of skrooge",
                          "Skrooge allows you to keep a hold on your expenses, by tracking and budgeting them.<br/>"
                          "What should you do now ?<br/>"
                          "<ul>"
                          "<li>Create at least one <a href=\"%1\">account</a></li>"
                          "<li>Add some transactions, using <a href=\"%3\">import</a> or <a href=\"%2\">manual input</a></li>"
                          "<li>Categorize them</li>"
                          "</ul>"
                          "<p>You may come back to this page any time by closing all tabs.<br/>"
                          "For more information about using Skrooge, check the <a href=\"https://skrooge.org\">Skrooge website</a>.</p>"
                          "<p>We hope that you will enjoy Skrooge</p>"
                          "    The Skrooge Team",
                          iMapping[QStringLiteral("about_accountpage")].toString(),
                          iMapping[QStringLiteral("about_operationpage")].toString(),
                          iMapping[QStringLiteral("about_importurl")].toString()));
    iMapping.insert(QStringLiteral("logo"),
                    QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("icons/breeze/apps/48/skrooge.svg"))).url());
    iMapping.insert(
        QStringLiteral("logo_black"),
        QUrl::fromLocalFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("icons/breeze/apps/48/skrooge-black.svg"))).url());

    iMapping.insert(QStringLiteral("title_main"), i18nc("A monthly report title", "Report for %1", getPeriod()));
    iMapping.insert(QStringLiteral("title_budget"), i18nc("A monthly report title", "Budget"));
    iMapping.insert(QStringLiteral("title_main_categories"), i18nc("A monthly report title", "5 main categories of expenditure"));
    iMapping.insert(QStringLiteral("title_variations"), i18nc("A monthly report title", "5 main variations"));
    iMapping.insert(QStringLiteral("title_account"), i18nc("A monthly report title", "Amounts in accounts"));
    iMapping.insert(QStringLiteral("title_unit"), i18nc("A monthly report title", "Amounts of units"));
    iMapping.insert(QStringLiteral("title_advice"), i18nc("A monthly report title", "Advice"));
    iMapping.insert(QStringLiteral("title_portfolio"), i18nc("A monthly report title", "Stock portfolio"));
    iMapping.insert(QStringLiteral("title_interests"), i18nc("A monthly report title", "Estimated interests"));
    iMapping.insert(QStringLiteral("title_alarms"), i18nc("A monthly report title", "Alarms"));
    iMapping.insert(QStringLiteral("title_highlighted"), i18nc("A monthly report title", "Highlighted transactions"));
    iMapping.insert(QStringLiteral("title_networth"), i18nc("A monthly report title", "Net Worth"));
    iMapping.insert(QStringLiteral("title_annual_spending"), i18nc("A monthly report title", "Annual Spending"));
    iMapping.insert(QStringLiteral("title_personal_finance_score"), i18nc("A monthly report title", "Personal Finance Score"));
    iMapping.insert(QStringLiteral("title_incomes"), i18nc("Title of incomes", "Incomes:"));
    iMapping.insert(QStringLiteral("title_expenditures"), i18nc("Title of expenditures", "Expenditures:"));
    iMapping.insert(QStringLiteral("title_savings"), i18nc("Title of savings", "Savings:"));

    iMapping.insert(QStringLiteral("msg_no_variation"), i18nc("A monthly report message", "No variation found."));
    iMapping.insert(QStringLiteral("msg_no_scheduled"),
                    i18nc("A monthly report message",
                          R"(No scheduled transactions defined on the "<a href="%1">Scheduled transactions</a>" page.)",
                          "skg://Skrooge_scheduled_plugin/"));
    iMapping.insert(QStringLiteral("msg_no_highlighted"),
                    i18nc("A monthly report message",
                          R"(No highlighted transactions defined on the "<a href="%1">Transactions</a>" page.)",
                          "skg://Skrooge_operation_plugin/"));
    iMapping.insert(QStringLiteral("msg_no_budget"),
                    i18nc("A monthly report message", R"(No budget defined on the "<a href="%1">Budget</a>" page.)", "skg://Skrooge_budget_plugin/"));
    iMapping.insert(QStringLiteral("msg_no_share"),
                    i18nc("A monthly report message", R"(No share defined on the "<a href="%1">Unit</a>" page.)", "skg://Skrooge_unit_plugin/"));
    iMapping.insert(QStringLiteral("msg_amount_unit_date"),
                    i18nc("A monthly report message", "All amounts are calculated using the unit rates of the last day of the corresponding period."));
}
