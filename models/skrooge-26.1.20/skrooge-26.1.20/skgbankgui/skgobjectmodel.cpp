/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGObjectModel.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgobjectmodel.h"

#include <math.h>

#include <kconfiggroup.h>
#include <klocalizedstring.h>

#include <qapplication.h>
#include <qcolor.h>
#include <qdir.h>
#include <qfont.h>
#include <qicon.h>
#include <qmimedata.h>
#include <qstandardpaths.h>

#include "skgaccountobject.h"
#include "skgbudgetobject.h"
#include "skgbudgetruleobject.h"
#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgoperationobject.h"
#include "skgpayeeobject.h"
#include "skgrecurrentoperationobject.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"
#include "skgtrackerobject.h"
#include "skgtransactionmng.h"
#include "skgunitobject.h"
#include "skgunitvalueobject.h"
#include <utility>

SKGObjectModel::SKGObjectModel(SKGDocumentBank *iDocument,
                               const QString &iTable,
                               const QString &iWhereClause,
                               QWidget *iParent,
                               const QString &iParentAttribute,
                               bool iResetOnCreation)
    : SKGObjectModelBase(iDocument, iTable, iWhereClause, iParent, iParentAttribute, false)
{
    SKGTRACEINFUNC(1)

    m_operationTable = false;
    m_recurrentoperationTable = false;
    m_trackerTable = false;
    m_accountTable = false;
    m_unitTable = false;
    m_unitvalueTable = false;
    m_suboperationTable = false;
    m_categoryTable = false;
    m_ruleTable = false;
    m_interestTable = false;
    m_interestResultTable = false;
    m_payeeTable = false;
    m_budgetTable = false;
    m_budgetRuleTable = false;
    m_isResetRealyNeeded = iResetOnCreation;
    refresh();
}

void SKGObjectModel::buidCache()
{
    SKGObjectModelBase::buidCache();
    m_operationTable = (getRealTable() == QStringLiteral("operation") || getRealTable() == QStringLiteral("suboperation"));
    m_payeeTable = (getRealTable() == QStringLiteral("payee"));
    m_trackerTable = (getRealTable() == QStringLiteral("refund"));
    m_recurrentoperationTable = (getRealTable() == QStringLiteral("recurrentoperation"));
    m_accountTable = (getRealTable() == QStringLiteral("account"));
    m_unitTable = (getRealTable() == QStringLiteral("unit"));
    m_unitvalueTable = (getRealTable() == QStringLiteral("unitvalue"));
    m_suboperationTable = (getTable() == QStringLiteral("v_suboperation_consolidated"));
    m_ruleTable = (getRealTable() == QStringLiteral("rule"));
    m_categoryTable = (getRealTable() == QStringLiteral("category"));
    m_interestTable = (getRealTable() == QStringLiteral("interest"));
    m_interestResultTable = (getRealTable() == QStringLiteral("interest_result"));
    m_budgetTable = (getRealTable() == QStringLiteral("budget"));
    m_budgetRuleTable = (getRealTable() == QStringLiteral("budgetrule"));

    if (m_unitvalueTable) {
        SKGUnitValueObject unitValObject(getObject(this->index(0, 0)));
        SKGUnitObject unitObject;
        unitValObject.getUnit(unitObject);
        SKGUnitObject parentUnit;
        unitObject.getUnit(parentUnit);
        if (parentUnit.exist()) {
            m_cacheUnit.Name = parentUnit.getName();
            m_cacheUnit.Symbol = parentUnit.getSymbol();
            m_cacheUnit.Value = 1;
            m_cacheUnit.NbDecimal = unitObject.getNumberDecimal();
        } else {
            m_cacheUnit.Name = QString();
            m_cacheUnit.Symbol = QString();
            m_cacheUnit.Value = 1;
            m_cacheUnit.NbDecimal = unitObject.getNumberDecimal();
        }
        if (m_cacheUnit.Symbol.isEmpty()) {
            m_cacheUnit.Symbol = ' ';
        }

        // Bug 209672 vvvv
        if (unitObject.getType() == SKGUnitObject::INDEX) {
            m_cacheUnit.Symbol = ' ';
            m_cacheUnit.Name = ' ';
        }
        // Bug 209672 ^^^^
    }

    if (m_operationTable) {
        // Read Setting
        KSharedConfigPtr config = KSharedConfig::openConfig();
        KConfigGroup pref = config->group(QStringLiteral("skrooge_operation"));
        m_fontFutureOperationsColor = QVariant::fromValue(pref.readEntry("fontFutureColor", QColor(Qt::gray)));
        m_fontNotValidatedOperationsColor = QVariant::fromValue(pref.readEntry("fontNotValidatedColor", QColor(Qt::blue)));
        m_fontSubOperationsColor = QVariant::fromValue(pref.readEntry("fontSubOperationColor", QColor(Qt::darkGreen)));
    }

    if (m_recurrentoperationTable) {
        // Read Setting
        KSharedConfigPtr config = KSharedConfig::openConfig();
        KConfigGroup pref = config->group(QStringLiteral("skrooge_scheduled"));
        m_fontDisabledScheduleColor = QVariant::fromValue(pref.readEntry("fontFutureColor", QColor(Qt::gray)));
    }

    m_iconFavorite = SKGServices::fromTheme(QStringLiteral("bookmarks"));

    if (m_operationTable || m_recurrentoperationTable) {
        m_iconTransfer = SKGServices::fromTheme(QStringLiteral("exchange-positions"));
        QStringList overlays;
        overlays.push_back(QStringLiteral("list-remove"));
        m_iconGroup = SKGServices::fromTheme(QStringLiteral("exchange-positions"), overlays);
        m_iconSplit = SKGServices::fromTheme(QStringLiteral("split"));
        m_iconImported = SKGServices::fromTheme(QStringLiteral("utilities-file-archiver"));
        {
            QStringList overlay;
            overlay.push_back(QStringLiteral("dialog-ok"));
            m_iconImportedChecked = SKGServices::fromTheme(QStringLiteral("utilities-file-archiver"), overlay);
        }

        m_iconRecurrent = SKGServices::fromTheme(QStringLiteral("view-calendar-day"));
        {
            QStringList overlay;
            overlay.push_back(QStringLiteral("bookmarks"));
            m_iconRecurrentMaster = SKGServices::fromTheme(QStringLiteral("view-calendar-day"), overlay);
        }
    }

    if (m_budgetTable) {
        m_iconGreen = SKGServices::fromTheme(QStringLiteral("security-high"));
        m_iconRed = SKGServices::fromTheme(QStringLiteral("security-low"));
        m_iconAnber = SKGServices::fromTheme(QStringLiteral("security-medium"));
    }

    if (m_unitTable) {
        m_iconMuchMore = SKGServices::fromTheme(QStringLiteral("skrooge_much_more"));
        m_iconMuchLess = SKGServices::fromTheme(QStringLiteral("skrooge_much_less"));
        m_iconMore = SKGServices::fromTheme(QStringLiteral("skrooge_more"));
        m_iconLess = SKGServices::fromTheme(QStringLiteral("skrooge_less"));
    }

    if (m_ruleTable) {
        m_iconSearch = SKGServices::fromTheme(QStringLiteral("system-search"));
        m_iconUpdate = SKGServices::fromTheme(QStringLiteral("view-refresh"));
        m_iconAlarm = SKGServices::fromTheme(QStringLiteral("dialog-warning"));
        m_iconTemplate = SKGServices::fromTheme(QStringLiteral("edit-guides"));
    }
    m_iconClosed = SKGServices::fromTheme(QStringLiteral("dialog-close"));

    if (m_categoryTable) {
        m_iconCategory = SKGServices::fromTheme(QStringLiteral("applications-all-symbolic"));
        m_iconCategoryMoins = SKGServices::fromTheme(QStringLiteral("applications-all-symbolic-expenditures"));
        m_iconCategoryPlus = SKGServices::fromTheme(QStringLiteral("applications-all-symbolic-incomes"));
    }
}

SKGObjectModel::~SKGObjectModel(){SKGTRACEINFUNC(1)}

QVariant SKGObjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    _SKGTRACEINFUNC(10)

    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole || role == Qt::ToolTipRole) {
            QString att;
            if (section >= 0 && section < m_listAttibutes.count()) {
                att = m_listAttibutes[section];
            } else {
                att = SKGServices::intToString(section);
            }

            if (att == QStringLiteral("t_bookmarked") || att == QStringLiteral("i_NBRECURRENT") || att == QStringLiteral("t_status")
                || att == QStringLiteral("t_close") || att == QStringLiteral("t_imported")) {
                return (role == Qt::ToolTipRole) ? SKGObjectModelBase::headerData(section, orientation, Qt::DisplayRole) : "";
            }
        }
    }
    return SKGObjectModelBase::headerData(section, orientation, role);
}

QString SKGObjectModel::getAttributeForGrouping(const SKGObjectBase &iObject, const QString &iAttribute) const
{
    if (m_recurrentoperationTable && iAttribute == QStringLiteral("i_nb_times")) {
        if (iObject.getAttribute(QStringLiteral("t_times")) != QStringLiteral("Y")) {
            return QChar(8734);
        }
    } else if (m_ruleTable && iAttribute == QStringLiteral("t_action_type")) {
        QString val = iObject.getAttribute(iAttribute);
        if (val == QStringLiteral("S")) {
            val = i18nc("Noun, a search", "Search");
        } else if (val == QStringLiteral("U")) {
            val = i18nc("Noun, a modification", "Update");
        } else {
            val = i18nc("Noun, an alarm", "Alarm");
        }
        return val;
    } else if (iAttribute == QStringLiteral("t_bookmarked") || iAttribute == QStringLiteral("t_close")) {
        QString val = iObject.getAttribute(iAttribute);
        return val == QStringLiteral("Y") ? i18n("Yes") : i18n("No");
    } else if (iAttribute == QStringLiteral("t_status")) {
        QString val = iObject.getAttribute(iAttribute);
        return val == QStringLiteral("N") ? i18n("None") : val == QStringLiteral("P") ? i18n("Marked") : i18n("Checked");
    }
    return SKGObjectModelBase::getAttributeForGrouping(iObject, iAttribute);
}

QVariant SKGObjectModel::computeData(const QModelIndex &iIndex, int iRole) const
{
    if (!iIndex.isValid()) {
        return QVariant();
    }
    _SKGTRACEINFUNC(10)
    SKGObjectBase *obj = getObjectPointer(iIndex);
    if (obj == nullptr || obj->getTable().isEmpty()) {
        return SKGObjectModelBase::computeData(iIndex, iRole);
    }

    switch (iRole) {
    case Qt::DisplayRole:
    case Qt::EditRole:
    case Qt::UserRole: {
        QString att = m_listAttibutes[iIndex.column()];
        QString val = obj->getAttribute(att);
        if (att == QStringLiteral("i_NBRECURRENT")) {
            if (iRole == Qt::UserRole) {
                if (val != QStringLiteral("0")) {
                    return QStringLiteral("Y");
                }
                if (obj->getAttribute(QStringLiteral("r_recurrentoperation_id")) != QStringLiteral("0")) {
                    return QStringLiteral("Y");
                }
                return QStringLiteral("N");
            }
            return "";
        }
        if (att == QStringLiteral("t_bookmarked") || att == QStringLiteral("t_status") || att == QStringLiteral("t_imported")
            || att == QStringLiteral("t_close") || att == QStringLiteral("t_action_type")) {
            if (iRole == Qt::UserRole) {
                if (m_ruleTable && att == QStringLiteral("t_action_type")) {
                    if (val == QStringLiteral("S")) {
                        return i18nc("Noun, a search", "Search");
                    }
                    if (val == QStringLiteral("U")) {
                        return i18nc("Noun, a modification", "Update");
                    }
                    return i18nc("Noun, an alarm", "Alarm");
                }
                return val;
            }
            return "";
        }
        if (m_interestTable && att == QStringLiteral("t_expenditure_value_date_mode")) {
            if (val == QStringLiteral("0")) {
                return i18nc("Noun", "Day -0");
            }
            if (val == QStringLiteral("1")) {
                return i18nc("Noun", "Day -1");
            }
            if (val == QStringLiteral("2")) {
                return i18nc("Noun", "Day -2");
            }
            if (val == QStringLiteral("3")) {
                return i18nc("Noun", "Day -3");
            }
            if (val == QStringLiteral("4")) {
                return i18nc("Noun", "Day -4");
            }
            if (val == QStringLiteral("5")) {
                return i18nc("Noun", "Day -5");
            }
            return i18nc("Noun", "Fifteen");
        }
        if (m_accountTable && att == QStringLiteral("d_reconciliationdate")) {
            if (val.isEmpty() && iRole == Qt::DisplayRole) {
                return i18nc("Noun", "Never");
            }
        } else if (m_interestTable && att == QStringLiteral("t_income_value_date_mode")) {
            if (val == QStringLiteral("0")) {
                return i18nc("Noun", "Day +0");
            }
            if (val == QStringLiteral("1")) {
                return i18nc("Noun", "Day +1");
            }
            if (val == QStringLiteral("2")) {
                return i18nc("Noun", "Day +2");
            }
            if (val == QStringLiteral("3")) {
                return i18nc("Noun", "Day +3");
            }
            if (val == QStringLiteral("4")) {
                return i18nc("Noun", "Day +4");
            }
            if (val == QStringLiteral("5")) {
                return i18nc("Noun", "Day +5");
            }
            return i18nc("Noun", "Fifteen");
        }
        if (getAttributeType(iIndex.column()) == SKGServices::FLOAT) {
            double dval = SKGServices::stringToDouble(val);
            if (iRole == Qt::DisplayRole) {
                if (val.isEmpty()) {
                    return "";
                }
                if (att.endsWith(QStringLiteral("_INCOME")) || att.endsWith(QStringLiteral("_EXPENSE"))
                    || (m_operationTable && obj->getAttribute(QStringLiteral("t_template")) == QStringLiteral("Y"))
                    || (m_categoryTable && (att == QStringLiteral("f_REALCURRENTAMOUNT") || att == QStringLiteral("f_SUMCURRENTAMOUNT")))) {
                    if (dval == 0) {
                        return "";
                    }
                }

                SKGServices::SKGUnitInfo unit;
                unit.Symbol = QString();
                unit.NbDecimal = 2;
                if (!att.contains(QStringLiteral("QUANTITY")) && !att.contains(QStringLiteral("f_BALANCE_ENTERED"))) {
                    unit = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit();
                    unit.NbDecimal = SKGServices::stringToInt(obj->getAttribute(QStringLiteral("i_NBDEC")));
                    if (unit.NbDecimal == 0) {
                        unit.NbDecimal = 2;
                    }
                    if (m_unitvalueTable && !m_cacheUnit.Symbol.isEmpty()) {
                        unit = m_cacheUnit;
                    }
                } else {
                    unit.NbDecimal = SKGServices::stringToInt(obj->getAttribute(QStringLiteral("i_NBDEC")));
                    if (unit.NbDecimal == 0) {
                        unit.NbDecimal = 2;
                    }
                    if (att != QStringLiteral("f_QUANTITYOWNED")) {
                        unit.Symbol = obj->getAttribute(QStringLiteral("t_UNIT"));
                    }
                }

                // Bug 209672 vvvv
                if (m_unitTable) {
                    if (obj->getAttribute(QStringLiteral("t_type")) == QStringLiteral("I")) {
                        unit.Symbol = ' ';
                    }
                }
                // Bug 209672 ^^^

                if (QString::compare(att, QStringLiteral("f_rate"), Qt::CaseInsensitive) == 0) {
                    unit.Symbol = '%';
                    unit.NbDecimal = 2;
                } else if (att == QStringLiteral("f_coef")) {
                    unit.Symbol = QString();
                    unit.NbDecimal = 2;
                }

                if (unit.Symbol.isEmpty()) {
                    unit.Symbol = ' ';
                }

                return SKGServices::toCurrencyString(dval, unit.Symbol, unit.NbDecimal);
            }
            return dval;
        }
        if (getAttributeType(iIndex.column()) == SKGServices::INTEGER) {
            if (m_recurrentoperationTable && att == QStringLiteral("i_nb_times")) {
                QString t_times = obj->getAttribute(QStringLiteral("t_times"));
                if (t_times != QStringLiteral("Y")) {
                    return QChar(8734);
                }
            } else if ((att == QStringLiteral("i_NBOPERATIONS") || att == QStringLiteral("i_SUMNBOPERATIONS")) && val == QStringLiteral("0")) {
                return "";
            }

            return SKGServices::stringToInt(val);
        }
        if (m_suboperationTable && att.startsWith(QStringLiteral("p_"))) {
            val = obj->getProperty(att.right(att.size() - 2));
            if (val.isEmpty()) {
                val = obj->getDocument()->getParameter(att.right(att.size() - 2), obj->getAttribute(QStringLiteral("i_OPID")) % "-operation");
            }
            return val;
        }
        if (m_payeeTable && att == QStringLiteral("t_CATEGORY") && val.isEmpty()) {
            auto c = qobject_cast<SKGDocumentBank *>(getDocument())->getCategoryForPayee(obj->getAttribute(QStringLiteral("t_name")));
            if (!c.isEmpty()) {
                return i18n("Auto: %1", c);
            }
        }
        break;
    }
    case Qt::DecorationRole: {
        // decoration
        QString att = m_listAttibutes[iIndex.column()];
        if (att == QStringLiteral("t_bookmarked")) {
            if (obj->getAttribute(QStringLiteral("t_bookmarked")) == QStringLiteral("Y")) {
                return m_iconFavorite;
            }
        } else if (m_operationTable || m_recurrentoperationTable) {
            if (att == QStringLiteral("t_mode")) {
                if (obj->getAttribute(QStringLiteral("t_TRANSFER")) == QStringLiteral("Y")) {
                    return m_iconTransfer;
                }
                if (obj->getAttribute(QStringLiteral("i_group_id")) != QStringLiteral("0")) {
                    return m_iconGroup;
                }
            } else if (att == QStringLiteral("t_CATEGORY")) {
                if (SKGServices::stringToInt(obj->getAttribute(QStringLiteral("i_NBSUBOPERATIONS"))) > 1) {
                    return m_iconSplit;
                }
            } else if (att == QStringLiteral("i_NBRECURRENT") && m_operationTable) {
                if (obj->getAttribute(QStringLiteral("i_NBRECURRENT")) != QStringLiteral("0")) {
                    return m_iconRecurrentMaster;
                }
                if (obj->getAttribute(QStringLiteral("r_recurrentoperation_id")) != QStringLiteral("0")) {
                    return m_iconRecurrent;
                }
            } else if (att == QStringLiteral("t_imported")) {
                QString impStatus = obj->getAttribute(QStringLiteral("t_imported"));
                if (impStatus == QStringLiteral("Y")) {
                    return m_iconImported;
                }
                if (impStatus == QStringLiteral("P")) {
                    return m_iconImportedChecked;
                }
            } else if (att == QStringLiteral("t_REFUND") || att == QStringLiteral("t_REALREFUND") || att == QStringLiteral("t_REFUNDDISPLAY")) {
                if (att == QStringLiteral("t_REFUNDDISPLAY")) {
                    if (obj->getAttribute(att).count(QStringLiteral("(")) > 1) {
                        att.clear();
                    } else {
                        att = QStringLiteral("t_REFUND");
                    }
                }
                if (!att.isEmpty()) {
                    QString trackerName;
                    trackerName = obj->getAttribute(att);
                    if (!trackerName.isEmpty()) {
                        SKGTrackerObject tracker(SKGObjectBase(getDocument(), QStringLiteral("refund"))); // Better performance if v_refund is not used
                        tracker.setName(trackerName);
                        tracker.load();

                        if (tracker.isClosed()) {
                            return m_iconClosed;
                        }
                    }
                }
            }
        } else if (m_ruleTable) {
            if (att == QStringLiteral("t_action_type")) {
                QString val = obj->getAttribute(att);
                if (val == QStringLiteral("S")) {
                    return m_iconSearch;
                }
                if (val == QStringLiteral("U")) {
                    return m_iconUpdate;
                }
                if (val == QStringLiteral("T")) {
                    return m_iconTemplate;
                }
                return m_iconAlarm;
            }
        } else if (m_unitTable) {
            if (att == QStringLiteral("f_CURRENTAMOUNT")) {
                SKGUnitObject unit(*obj);
                double amountOneYearBefore = unit.getAmount(QDate::currentDate().addYears(-1));
                double annualchange = amountOneYearBefore != 0 ? 0.0 : 100.0 * (unit.getAmount() - amountOneYearBefore) / amountOneYearBefore;
                if (annualchange >= 10) {
                    return m_iconMuchMore;
                }
                if (annualchange <= -10) {
                    return m_iconMuchLess;
                }
                if (annualchange > 0) {
                    return m_iconMore;
                }
                if (annualchange < 0) {
                    return m_iconLess;
                }
            }
        } else if (m_accountTable) {
            SKGAccountObject act(*obj);
            if (att == QStringLiteral("t_BANK")) {
                const bool wallet = (act.getType() == SKGAccountObject::WALLET);
                QString t_icon = wallet ? QStringLiteral("wallet-closed") : obj->getAttribute(QStringLiteral("t_icon"));
                if (t_icon.isEmpty()) {
                    t_icon = obj->getAttribute(QStringLiteral("t_ICON"));
                }
                if (!t_icon.isEmpty()) {
                    QString fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/images/logo/" % t_icon);
                    if (fileName.isEmpty()) {
                        fileName = std::move(t_icon);
                    }
                    return QVariant::fromValue(SKGServices::fromTheme(fileName));
                }
            } else if (att == QStringLiteral("f_importbalance")) {
                QString val = obj->getAttribute(att);
                if (val.isEmpty()) {
                    return "";
                }

                // Compute value
                auto soluces = act.getPossibleReconciliations(SKGServices::stringToDouble(val), false);
                return SKGServices::fromTheme(soluces.isEmpty() ? QStringLiteral("security-low") : QStringLiteral("security-high"));
            } else if (att == QStringLiteral("f_reconciliationbalance")) {
                QString val = obj->getAttribute(att);
                if (val.isEmpty()) {
                    return "";
                }

                // Compute value
                auto soluces = act.getPossibleReconciliations(SKGServices::stringToDouble(val), false);
                return SKGServices::fromTheme(soluces.isEmpty() ? QStringLiteral("security-low") : QStringLiteral("security-high"));
            }
        } else if (m_categoryTable) {
            if (iIndex.column() == 0) {
                QString t_TYPEEXPENSE = obj->getAttribute(QStringLiteral("t_TYPEEXPENSE"));
                if (t_TYPEEXPENSE == QStringLiteral("-")) {
                    return m_iconCategoryMoins;
                }
                if (t_TYPEEXPENSE == QStringLiteral("+")) {
                    return m_iconCategoryPlus;
                }
                return m_iconCategory;
            }
        } else if (m_budgetTable) {
            if (att == QStringLiteral("f_DELTA") || att == QStringLiteral("f_DELTABEFORETRANSFER")) {
                double val = SKGServices::stringToDouble(obj->getAttribute(att));
                if (val < -EPSILON) {
                    return m_iconRed;
                }
                if (val > EPSILON) {
                    return m_iconGreen;
                }
                double transferred = SKGServices::stringToDouble(obj->getAttribute(QStringLiteral("f_transferred")));
                if (transferred < -EPSILON) {
                    return m_iconAnber;
                }
                return m_iconGreen;
            }
        }
        break;
    }
    case Qt::ForegroundRole: {
        // Text color
        QString att = m_listAttibutes[iIndex.column()];
        if (m_recurrentoperationTable) {
            if (obj->getAttribute(QStringLiteral("i_nb_times")) == QStringLiteral("0")) {
                return m_fontDisabledScheduleColor;
            }
            if (att == QStringLiteral("d_date") && SKGServices::stringToDate(obj->getAttribute(QStringLiteral("d_date"))) <= QDate::currentDate()) {
                return m_fontNegativeColor;
            }
        } else if (m_operationTable) {
            if (SKGServices::stringToDate(obj->getAttribute(QStringLiteral("d_date"))) > QDate::currentDate()) {
                return m_fontFutureOperationsColor;
            }
            if (getAttributeType(iIndex.column()) != SKGServices::FLOAT) {
                if (obj->getAttribute(QStringLiteral("t_imported")) == QStringLiteral("P")) {
                    return m_fontNotValidatedOperationsColor;
                }
                if (m_suboperationTable) {
                    return m_fontSubOperationsColor;
                }
            }
        }
        break;
    }
    case Qt::TextAlignmentRole: {
        // Text alignment
        if (m_recurrentoperationTable) {
            QString att = m_listAttibutes[iIndex.column()];

            if (att == QStringLiteral("i_auto_write_days") || att == QStringLiteral("i_warn_days") || att == QStringLiteral("i_nb_times")) {
                return static_cast<int>(Qt::AlignVCenter | Qt::AlignLeft);
            }
        }
        break;
    }
    case Qt::CheckStateRole: {
        // CheckState
        QString att = m_listAttibutes[iIndex.column()];
        if (m_operationTable && att == QStringLiteral("t_status")) {
            QString cond = obj->getAttribute(QStringLiteral("t_status"));
            if (cond == QStringLiteral("P")) {
                return static_cast<int>(Qt::PartiallyChecked);
            }
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            if (cond == QStringLiteral("N")) {
                return static_cast<int>(Qt::Unchecked);
            }
        } else if (att == QStringLiteral("t_close")) {
            QString cond = obj->getAttribute(QStringLiteral("t_close"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_recurrentoperationTable && att == QStringLiteral("i_auto_write_days")) {
            QString cond = obj->getAttribute(QStringLiteral("t_auto_write"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_recurrentoperationTable && att == QStringLiteral("i_warn_days")) {
            QString cond = obj->getAttribute(QStringLiteral("t_warn"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_recurrentoperationTable && att == QStringLiteral("i_nb_times")) {
            QString cond = obj->getAttribute(QStringLiteral("t_times"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_budgetRuleTable && att == QStringLiteral("t_CATEGORYCONDITION")) {
            QString cond = obj->getAttribute(QStringLiteral("t_category_condition"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_budgetRuleTable && att == QStringLiteral("i_year")) {
            QString cond = obj->getAttribute(QStringLiteral("t_year_condition"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_budgetRuleTable && att == QStringLiteral("i_month")) {
            QString cond = obj->getAttribute(QStringLiteral("t_month_condition"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_budgetRuleTable && att == QStringLiteral("t_CATEGORY")) {
            QString cond = obj->getAttribute(QStringLiteral("t_category_target"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);

        } else if (m_budgetTable && att == QStringLiteral("t_CATEGORY")) {
            QString cond = obj->getAttribute(QStringLiteral("t_including_subcategories"));
            if (cond == QStringLiteral("Y")) {
                return static_cast<int>(Qt::Checked);
            }
            return static_cast<int>(Qt::Unchecked);
        }
        break;
    }
    case Qt::ToolTipRole: {
        // Tooltip
        QString toolTipString;
        QString att = m_listAttibutes[iIndex.column()];
        if (getAttributeType(iIndex.column()) == SKGServices::FLOAT) {
            // Add secondary unit
            if (!att.contains(QStringLiteral("QUANTITY")) && att != QStringLiteral("f_coef") && att != QStringLiteral("f_rate")) {
                SKGServices::SKGUnitInfo secondaryUnit = qobject_cast<SKGDocumentBank *>(getDocument())->getSecondaryUnit();
                if (!secondaryUnit.Symbol.isEmpty()) {
                    double val = SKGServices::stringToDouble(obj->getAttribute(att));
                    if ((!att.endsWith(QStringLiteral("_INCOME")) && !att.endsWith(QStringLiteral("_EXPENSE"))) || val != 0) {
                        if (secondaryUnit.Value != 0.0) {
                            toolTipString = SKGServices::toCurrencyString(val / secondaryUnit.Value, secondaryUnit.Symbol, secondaryUnit.NbDecimal);
                        }
                    }
                }
            }

            // Add balance
            if (m_operationTable && !m_suboperationTable) {
                SKGOperationObject op(*obj);
                if (!op.isTemplate()) {
                    SKGServices::SKGUnitInfo primaryUnit = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit();

                    // Add original amount
                    QString originalAmount = obj->getProperty(QStringLiteral("SKG_OP_ORIGINAL_AMOUNT"));
                    if (!originalAmount.isEmpty()) {
                        if (!toolTipString.isEmpty()) {
                            toolTipString += QStringLiteral("\n\n");
                        }
                        double val1 = SKGServices::stringToDouble(obj->getAttribute(QStringLiteral("f_CURRENTAMOUNT")));
                        double val2 = SKGServices::stringToDouble(originalAmount);
                        double gain = (val2 != 0 ? 100.0 * (val1 - val2) / val2 : 0);
                        double gainperyear = gain;

                        int nbDays = op.getDate().daysTo(QDate::currentDate());
                        if (nbDays != 0 && val2 != 0) {
                            double gainperday = 100.0 * expm1(log(val1 / val2) / static_cast<double>(nbDays));
                            gainperyear = 100.0 * (pow(1.0 + gainperday / 100.0, 365.0) - 1);
                        }
                        toolTipString += i18nc("Noun",
                                               "Original amount=%1 (%2 = %3 / year)",
                                               SKGServices::toCurrencyString(val2, primaryUnit.Symbol, primaryUnit.NbDecimal),
                                               (gain >= 0 ? "+" : "-") % SKGServices::toPercentageString(gain, 2),
                                               (gainperyear >= 0 ? "+" : "-") % SKGServices::toPercentageString(gainperyear, 2));
                    } else {
                        if (!toolTipString.isEmpty()) {
                            toolTipString += QStringLiteral("\n\n");
                        }
                        double val1 = SKGServices::stringToDouble(obj->getAttribute(QStringLiteral("f_CURRENTAMOUNT")));
                        double val2 = op.getAmount(op.getDate());
                        double gain = (val2 != 0 ? 100.0 * (val1 - val2) / val2 : 0);
                        double gainperyear = gain;

                        int nbDays = op.getDate().daysTo(QDate::currentDate());
                        if (nbDays != 0 && val2 != 0) {
                            double gainperday = 100.0 * expm1(log(val1 / val2) / static_cast<double>(nbDays));
                            gainperyear = 100.0 * (pow(1.0 + gainperday / 100.0, 365.0) - 1);
                        }

                        QString sval1 = SKGServices::toCurrencyString(val1, primaryUnit.Symbol, primaryUnit.NbDecimal);
                        QString sval2 = SKGServices::toCurrencyString(val2, primaryUnit.Symbol, primaryUnit.NbDecimal);
                        if (sval1 != sval2) {
                            toolTipString += i18nc("Noun",
                                                   "Amount at creation date=%1 (%2 = %3 / year)",
                                                   sval2,
                                                   (gain >= 0 ? "+" : "-") % SKGServices::toPercentageString(gain, 2),
                                                   (gainperyear >= 0 ? "+" : "-") % SKGServices::toPercentageString(gainperyear, 2));

                            toolTipString += '\n';
                        }
                    }
                    toolTipString +=
                        i18nc("Noun", "Account balance=%1", SKGServices::toCurrencyString(op.getBalance(), primaryUnit.Symbol, primaryUnit.NbDecimal));
                }
            }

            if (m_budgetTable) {
                if (att == QStringLiteral("f_DELTA")) {
                    double val = SKGServices::stringToDouble(obj->getAttribute(QStringLiteral("f_DELTABEFORETRANSFER")));

                    if (!toolTipString.isEmpty()) {
                        toolTipString += QStringLiteral("\n\n");
                    }

                    SKGServices::SKGUnitInfo primaryUnit = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit();
                    toolTipString += i18nc("Noun", "Original delta=%1", SKGServices::toCurrencyString(val, primaryUnit.Symbol, primaryUnit.NbDecimal));
                } else if (att == QStringLiteral("f_budgeted_modified")) {
                    QString reasons = obj->getAttribute(QStringLiteral("t_modification_reasons"));
                    if (!reasons.isEmpty()) {
                        if (!toolTipString.isEmpty()) {
                            toolTipString += QStringLiteral("\n\n");
                        }
                        toolTipString += reasons;
                    }
                }
            }

            if (m_unitTable) {
                if (att == QStringLiteral("f_CURRENTAMOUNT")) {
                    SKGUnitObject unit(*obj);
                    double amountOneYearBefore = unit.getAmount(QDate::currentDate().addYears(-1));
                    double annualchange = 100.0 * (unit.getAmount() - amountOneYearBefore) / amountOneYearBefore;

                    double amount6MonthBefore = unit.getAmount(QDate::currentDate().addMonths(-6));
                    double sixMonthChange = 100.0 * (unit.getAmount() - amount6MonthBefore) / amount6MonthBefore;

                    double amount1MonthBefore = unit.getAmount(QDate::currentDate().addMonths(-1));
                    double oneMonthChange = 100.0 * (unit.getAmount() - amount1MonthBefore) / amount1MonthBefore;
                    if (!toolTipString.isEmpty()) {
                        toolTipString += QStringLiteral("\n\n");
                    }
                    toolTipString += i18nc("Noun", "Variation on the last year = %1\n", SKGServices::toPercentageString(annualchange));
                    toolTipString += i18nc("Noun",
                                           "Variation on the last 6 months = %1 = %2 / year\n",
                                           SKGServices::toPercentageString(sixMonthChange),
                                           SKGServices::toPercentageString(100.0 * pow(1.0 + sixMonthChange / 100.0, 2.0) - 100.0));
                    toolTipString += i18nc("Noun",
                                           "Variation on the last month = %1 = %2 / year\n",
                                           SKGServices::toPercentageString(oneMonthChange),
                                           SKGServices::toPercentageString(100.0 * pow(1.0 + oneMonthChange / 100.0, 12.0) - 100.0));
                }
            }
        } else if (m_operationTable || m_recurrentoperationTable) {
            if (att == QStringLiteral("t_imported")) {
                if (!m_suboperationTable) {
                    SKGOperationObject op;
                    if (m_recurrentoperationTable) {
                        SKGRecurrentOperationObject rop(*obj);
                        rop.getParentOperation(op);
                    } else {
                        op = *obj;
                    }
                    toolTipString = op.getImportID();
                }
            } else if (att == QStringLiteral("t_REFUND") || att == QStringLiteral("t_REALREFUND") || att == QStringLiteral("t_REFUNDDISPLAY")) {
                if (att == QStringLiteral("t_REFUNDDISPLAY")) {
                    if (obj->getAttribute(att).count(QStringLiteral("(")) > 1) {
                        att.clear();
                    } else {
                        att = QStringLiteral("t_REFUND");
                    }
                }
                if (!att.isEmpty()) {
                    QString trackerName = obj->getAttribute(att);
                    if (!trackerName.isEmpty()) {
                        SKGTrackerObject tracker(getDocument());
                        tracker.setName(trackerName);
                        tracker.load();
                        SKGServices::SKGUnitInfo unit = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit();
                        toolTipString = SKGServices::toCurrencyString(tracker.getCurrentAmount(), unit.Symbol, unit.NbDecimal);
                    }
                }
            } else if (att == QStringLiteral("t_PAYEE")) {
                QString payeeName = obj->getAttribute(att);
                if (!payeeName.isEmpty()) {
                    SKGPayeeObject payee(getDocument());
                    payee.setName(payeeName);
                    payee.load();

                    auto address = payee.getAddress();
                    if (!address.isEmpty()) {
                        toolTipString += i18nc("Information", "Address= %1\n", address);
                    }

                    auto c = payee.getAttribute(QStringLiteral("t_CATEGORY"));
                    if (c.isEmpty()) {
                        c = qobject_cast<SKGDocumentBank *>(getDocument())->getCategoryForPayee(payeeName, false);
                    }
                    if (!c.isEmpty()) {
                        toolTipString += i18nc("Information", "Category= %1\n", c);
                    }
                }
            } else if (att == QStringLiteral("t_ACCOUNT") || att == QStringLiteral("t_TOACCOUNT")) {
                QString accountName = obj->getAttribute(att);
                if (!accountName.isEmpty()) {
                    SKGAccountObject account(getDocument());
                    account.setName(accountName);
                    account.load();

                    auto tmp = account.getAgencyNumber();
                    if (!tmp.isEmpty()) {
                        toolTipString += i18nc("Information", "Agency number= %1\n", tmp);
                    }

                    tmp = account.getNumber();
                    if (!tmp.isEmpty()) {
                        toolTipString += i18nc("Information", "Number= %1\n", tmp);
                    }

                    tmp = account.getAgencyAddress();
                    if (!tmp.isEmpty()) {
                        toolTipString += i18nc("Information", "Address= %1\n", tmp);
                    }

                    tmp = account.getComment();
                    if (!tmp.isEmpty()) {
                        toolTipString += i18nc("Information", "Comment= %1\n", tmp);
                    }
                }
            } else if (att == QStringLiteral("t_CATEGORY")) {
                SKGOperationObject op(*obj);
                if (m_recurrentoperationTable) {
                    SKGRecurrentOperationObject rop(*obj);
                    rop.getParentOperation(op);
                }
                if (SKGServices::stringToInt(op.getAttribute(QStringLiteral("i_NBSUBOPERATIONS"))) > 1) {
                    SKGObjectBase::SKGListSKGObjectBase subOps;
                    op.getSubOperations(subOps);
                    for (const auto &subOp : std::as_const(subOps)) {
                        toolTipString += subOp.getDisplayName() % '\n';
                    }
                }
            } else if (att == QStringLiteral("t_mode")) {
                SKGOperationObject op(*obj);
                if (m_recurrentoperationTable) {
                    SKGRecurrentOperationObject rop(*obj);
                    rop.getParentOperation(op);
                }
                if (op.getAttribute(QStringLiteral("i_group_id")) != QStringLiteral("0")) {
                    SKGOperationObject gop;
                    op.getGroupOperation(gop);

                    SKGObjectBase::SKGListSKGObjectBase gOps;
                    op.getGroupedOperations(gOps);
                    for (const auto &item : std::as_const(gOps)) {
                        SKGOperationObject gOp(item);
                        SKGAccountObject account;
                        gOp.getParentAccount(account);
                        toolTipString += account.getDisplayName() % '\n';
                    }
                }
            }

            if (m_operationTable && !m_suboperationTable && toolTipString.isEmpty()) {
                SKGOperationObject op(*obj);
                if (op.getStatus() == SKGOperationObject::MARKED) {
                    toolTipString = i18nc("A tool tip", "This transaction is marked but not checked yet.");
                    toolTipString += '\n';
                    toolTipString += i18nc("A tool tip", "You can use the reconciliation mode to validate marked transactions.");
                    toolTipString += '\n';
                    if (att == QStringLiteral("t_status")) {
                        toolTipString += i18nc("A tool tip", "Click in this column to switch back status.");
                    } else {
                        toolTipString += i18nc("A tool tip", "Click in the Status (checkmark) column to switch back status.");
                    }
                    toolTipString += '\n';
                    toolTipString += i18nc("A tool tip", "Ctrl+click to force checked status.");
                } else if (op.getStatus() == SKGOperationObject::CHECKED) {
                    toolTipString = i18nc("A tool tip", "This transaction is already checked.");
                } else if (op.getStatus() == SKGOperationObject::NONE) {
                    toolTipString = i18nc("A tool tip", "This transaction is not marked yet.");
                    toolTipString += '\n';
                    toolTipString += i18nc("A tool tip", "Click to set marked status.");
                    toolTipString += '\n';
                    toolTipString += i18nc("A tool tip", "Ctrl+click to force checked status.");
                }
            }
        } else if (m_ruleTable && att == QStringLiteral("t_action_type")) {
            QString val = obj->getAttribute(att);
            if (val == QStringLiteral("S")) {
                toolTipString = i18nc("Noun, a search", "Search");
            } else if (val == QStringLiteral("U")) {
                toolTipString = i18nc("Noun, a modification", "Update");
            } else {
                toolTipString = i18nc("Noun, an alarm", "Alarm");
            }
        }

        QString toolTipStringBase = SKGObjectModelBase::computeData(iIndex, iRole).toString();
        if (!toolTipStringBase.isEmpty()) {
            if (!toolTipString.isEmpty()) {
                toolTipString += QStringLiteral("\n\n");
            }
            toolTipString += toolTipStringBase;
        }
        return toolTipString;
    }
    default: {
    }
    }
    return SKGObjectModelBase::computeData(iIndex, iRole);
}

bool SKGObjectModel::setData(const QModelIndex &iIndex, const QVariant &iValue, int iRole)
{
    if (!iIndex.isValid()) {
        return false;
    }

    if (iRole == Qt::CheckStateRole) {
        SKGError err;
        {
            auto newState = static_cast<Qt::CheckState>(iValue.toInt());
            if (m_accountTable) {
                SKGAccountObject obj(getObject(iIndex));
                SKGBEGINLIGHTTRANSACTION(*getDocument(),
                                         (newState == Qt::Checked ? i18nc("Noun, name of the user action", "Close account '%1'", obj.getName())
                                                                  : i18nc("Noun, name of the user action", "Open account '%1'", obj.getName())),
                                         err);
                if (qAbs(obj.getCurrentAmount()) > 0.01 && newState == Qt::Checked) {
                    err = getDocument()->sendMessage(i18nc("An information message", "Warning, you closed an account with money"), SKGDocument::Warning);
                }
                IFOKDO(err, obj.setClosed(newState == Qt::Checked))
                IFOKDO(err, obj.save())
            } else if (m_trackerTable) {
                SKGTrackerObject obj(getObject(iIndex));
                SKGBEGINLIGHTTRANSACTION(*getDocument(),
                                         (newState == Qt::Checked ? i18nc("Noun, name of the user action", "Close tracker '%1'", obj.getName())
                                                                  : i18nc("Noun, name of the user action", "Open tracker '%1'", obj.getName())),
                                         err);
                err = obj.setClosed(newState == Qt::Checked);
                IFOKDO(err, obj.save())
            } else if (m_categoryTable) {
                SKGCategoryObject obj(getObject(iIndex));
                SKGBEGINLIGHTTRANSACTION(*getDocument(),
                                         (newState == Qt::Checked ? i18nc("Noun, name of the user action", "Close category '%1'", obj.getName())
                                                                  : i18nc("Noun, name of the user action", "Open category '%1'", obj.getName())),
                                         err);
                err = obj.setClosed(newState == Qt::Checked);
                IFOKDO(err, obj.save())
            } else if (m_payeeTable) {
                SKGPayeeObject obj(getObject(iIndex));
                SKGBEGINLIGHTTRANSACTION(*getDocument(),
                                         (newState == Qt::Checked ? i18nc("Noun, name of the user action", "Close payee '%1'", obj.getName())
                                                                  : i18nc("Noun, name of the user action", "Open payee '%1'", obj.getName())),
                                         err);
                err = obj.setClosed(newState == Qt::Checked);
                IFOKDO(err, obj.save())
            } else if (m_operationTable && !m_suboperationTable) {
                // Get the real object, not the object from the view
                SKGObjectBase *objtmp = getObjectPointer(iIndex);
                if (objtmp != nullptr) {
                    SKGOperationObject obj = SKGOperationObject(objtmp->getDocument(), objtmp->getID());
                    SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Change transaction status"), err)
                    SKGOperationObject::OperationStatus statusinitial = obj.getStatus();
                    SKGOperationObject::OperationStatus t_status = statusinitial;
                    if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u) {
                        // 2747379: NONE ==> CHECKED, MARKED ==> CHECKED, CHECKED ==> CHECKED
                        t_status = SKGOperationObject::CHECKED;
                        // t_status= ( t_status==SKGOperationObject::MARKED ? SKGOperationObject::NONE : ( t_status==SKGOperationObject::CHECKED ?
                        // SKGOperationObject::MARKED : SKGOperationObject::NONE ) );
                    } else {
                        // 2747379: NONE ==> MARKED, MARKED ==> NONE, CHECKED ==> MARKED
                        t_status = (t_status == SKGOperationObject::NONE
                                        ? SKGOperationObject::MARKED
                                        : (t_status == SKGOperationObject::MARKED ? SKGOperationObject::NONE : SKGOperationObject::MARKED));
                        // t_status=(t_status==SKGOperationObject::MARKED ? SKGOperationObject::CHECKED : (t_status==SKGOperationObject::CHECKED ?
                        // SKGOperationObject::CHECKED : SKGOperationObject::MARKED ));
                    }
                    if (t_status != statusinitial) {
                        err = obj.setStatus(t_status);
                        IFOKDO(err, obj.save())

                        // Send message
                        IFOKDO(err,
                               getDocument()->sendMessage(
                                   i18nc("An information to the user", "The status of the transaction '%1' has been changed", obj.getDisplayName()),
                                   SKGDocument::Hidden))
                    }
                }
            } else if (m_recurrentoperationTable) {
                QString att = m_listAttibutes[iIndex.column()];

                // Get the real object, not the object from the view
                SKGObjectBase *objtmp = getObjectPointer(iIndex);
                if (objtmp != nullptr) {
                    SKGRecurrentOperationObject obj = SKGRecurrentOperationObject(objtmp->getDocument(), objtmp->getID());

                    SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Recurrent transaction update"), err)
                    if (att == QStringLiteral("i_warn_days")) {
                        err = obj.warnEnabled(!obj.isWarnEnabled());
                    } else if (att == QStringLiteral("i_auto_write_days")) {
                        err = obj.autoWriteEnabled(!obj.isAutoWriteEnabled());
                    } else if (att == QStringLiteral("i_nb_times")) {
                        err = obj.timeLimit(!obj.hasTimeLimit());
                    }
                    IFOKDO(err, obj.save())

                    // Send message
                    IFOKDO(
                        err,
                        getDocument()->sendMessage(i18nc("An information to the user", "The recurrent transaction '%1' has been updated", obj.getDisplayName()),
                                                   SKGDocument::Hidden))
                }
            } else if (m_budgetRuleTable) {
                QString att = m_listAttibutes[iIndex.column()];

                // Get the real object, not the object from the view
                SKGObjectBase *objtmp = getObjectPointer(iIndex);
                if (objtmp != nullptr) {
                    SKGBudgetRuleObject obj = SKGBudgetRuleObject(objtmp->getDocument(), objtmp->getID());

                    SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget rule update"), err)
                    if (att == QStringLiteral("i_year")) {
                        err = obj.enableYearCondition(!obj.isYearConditionEnabled());
                    } else if (att == QStringLiteral("i_month")) {
                        err = obj.enableMonthCondition(!obj.isMonthConditionEnabled());
                    } else if (att == QStringLiteral("t_CATEGORYCONDITION")) {
                        err = obj.enableCategoryCondition(!obj.isCategoryConditionEnabled());
                    } else if (att == QStringLiteral("t_CATEGORY")) {
                        err = obj.enableCategoryChange(!obj.isCategoryChangeEnabled());
                    }
                    IFOKDO(err, obj.save())

                    // Send message
                    IFOKDO(err,
                           getDocument()->sendMessage(i18nc("An information to the user", "The budget rule '%1' has been updated", obj.getDisplayName()),
                                                      SKGDocument::Hidden))
                }
            } else if (m_budgetTable) {
                QString att = m_listAttibutes[iIndex.column()];

                // Get the real object, not the object from the view
                SKGObjectBase *objtmp = getObjectPointer(iIndex);
                if (objtmp != nullptr) {
                    SKGBudgetObject obj = SKGBudgetObject(objtmp->getDocument(), objtmp->getID());

                    SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget update"), err)
                    if (att == QStringLiteral("t_CATEGORY")) {
                        IFOKDO(err, obj.enableSubCategoriesInclusion(!obj.isSubCategoriesInclusionEnabled()))
                        IFOKDO(err, obj.save())

                        // Send message
                        IFOKDO(err,
                               getDocument()->sendMessage(i18nc("An information to the user", "The budget '%1' have been updated", obj.getDisplayName()),
                                                          SKGDocument::Hidden))
                    }
                }
            }
        }

        SKGMainPanel::displayErrorMessage(err);
        return !err;
    }
    return SKGObjectModelBase::setData(iIndex, iValue, iRole);
}

Qt::ItemFlags SKGObjectModel::flags(const QModelIndex &iIndex) const
{
    _SKGTRACEINFUNC(10)

    Qt::ItemFlags f = SKGObjectModelBase::flags(iIndex);

    if (iIndex.isValid()) {
        QString att = m_listAttibutes[iIndex.column()];
        if (att == QStringLiteral("t_bookmarked") || m_ruleTable || m_recurrentoperationTable || m_interestTable || m_interestResultTable) {
            f = f & ~Qt::ItemIsEditable;
        }
    }

    if (m_categoryTable || m_payeeTable || m_accountTable || m_unitTable || m_trackerTable) {
        if (iIndex.isValid()) {
            f |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
        } else {
            f |= Qt::ItemIsDropEnabled;
        }
    }

    return f;
}

Qt::DropActions SKGObjectModel::supportedDragActions() const
{
    if (m_categoryTable || m_payeeTable || m_accountTable || m_unitTable || m_trackerTable) {
        return Qt::MoveAction;
    }
    return SKGObjectModelBase::supportedDragActions();
}

Qt::DropActions SKGObjectModel::supportedDropActions() const
{
    return SKGObjectModelBase::supportedDropActions();
}

bool SKGObjectModel::dropMimeData(const QMimeData *iData, Qt::DropAction iAction, int iRow, int iColumn, const QModelIndex &iParent)
{
    if (SKGObjectModelBase::dropMimeData(iData, iAction, iRow, iColumn, iParent)) {
        return true;
    }
    if (iAction == Qt::IgnoreAction) {
        return true;
    }
    if ((iData == nullptr)
        || !(iData->hasFormat(QStringLiteral("application/skg.category.ids")) || iData->hasFormat(QStringLiteral("application/skg.payee.ids"))
             || iData->hasFormat(QStringLiteral("application/skg.account.ids")) || iData->hasFormat(QStringLiteral("application/skg.refund.ids"))
             || iData->hasFormat(QStringLiteral("application/skg.unit.ids")))) {
        return false;
    }
    if (iColumn > 0) {
        return false;
    }

    SKGError err;
    if (iData->hasFormat(QStringLiteral("application/skg.category.ids"))) {
        QByteArray encodedData = iData->data(QStringLiteral("application/skg.category.ids"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        SKGCategoryObject parentCategory;
        if (iParent.isValid()) {
            parentCategory = getObject(iParent);
        }
        {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Move category"), err)

            while (!stream.atEnd() && !err) {
                int o_id;
                QString o_table;
                stream >> o_table;
                stream >> o_id;

                SKGCategoryObject child(getDocument(), o_id);
                err = child.load();
                QString oldName = child.getDisplayName();
                IFOK(err)
                {
                    if (iParent.isValid()) {
                        err = child.setParentCategory(parentCategory);
                    } else {
                        err = child.removeParentCategory();
                    }
                }
                IFOKDO(err, child.save())

                // Send message
                IFOKDO(
                    err,
                    getDocument()->sendMessage(i18nc("An information to the user", "The category '%1' has been moved to '%2'", oldName, child.getDisplayName()),
                                               SKGDocument::Hidden))
            }
        }
    } else if (iData->hasFormat(QStringLiteral("application/skg.payee.ids"))) {
        QByteArray encodedData = iData->data(QStringLiteral("application/skg.payee.ids"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        if (iParent.isValid()) {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Merge payees"), err)
            SKGPayeeObject parentPayee(getObject(iParent));
            while (!stream.atEnd() && !err) {
                int o_id;
                QString o_table;
                stream >> o_table;
                stream >> o_id;

                SKGPayeeObject child(getDocument(), o_id);
                err = child.load();

                // Send message
                IFOKDO(err,
                       getDocument()->sendMessage(i18nc("An information to the user",
                                                        "The payee '%1' has been merged with payee '%2'",
                                                        child.getDisplayName(),
                                                        parentPayee.getDisplayName()),
                                                  SKGDocument::Hidden))

                IFOKDO(err, parentPayee.merge(child))
            }
        }
    } else if (iData->hasFormat(QStringLiteral("application/skg.account.ids"))) {
        QByteArray encodedData = iData->data(QStringLiteral("application/skg.account.ids"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        if (iParent.isValid()) {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Merge accounts"), err)
            SKGAccountObject parentAccount(getObject(iParent));
            while (!stream.atEnd() && !err) {
                int o_id;
                QString o_table;
                stream >> o_table;
                stream >> o_id;

                SKGAccountObject child(getDocument(), o_id);
                err = child.load();

                // Send message
                IFOKDO(err,
                       getDocument()->sendMessage(i18nc("An information to the user",
                                                        "The account '%1' has been merged with account '%2'",
                                                        child.getDisplayName(),
                                                        parentAccount.getDisplayName()),
                                                  SKGDocument::Hidden))

                double balancebefore = 0.0;
                double balanceafter = 0.0;
                SKGUnitObject unit2;
                IFOKDO(err, parentAccount.getInitialBalance(balancebefore, unit2))
                IFOKDO(err, parentAccount.merge(child, !(QApplication::keyboardModifiers() & Qt::ControlModifier)))
                IFOKDO(err, parentAccount.getInitialBalance(balanceafter, unit2))
                if (balanceafter != balancebefore) {
                    getDocument()->sendMessage(i18nc("Warning message",
                                                     "The initial balance of the target account '%1' has been change to %2.\nIf you want to do the merge "
                                                     "without changing the initial balance, you must keep the Ctrl key pressed.",
                                                     parentAccount.getDisplayName(),
                                                     balanceafter),
                                               SKGDocument::Warning);
                }
            }
        }
    } else if (iData->hasFormat(QStringLiteral("application/skg.unit.ids"))) {
        QByteArray encodedData = iData->data(QStringLiteral("application/skg.unit.ids"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        if (iParent.isValid()) {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Merge units"), err)
            SKGUnitObject parentUnit(getObject(iParent));
            while (!stream.atEnd() && !err) {
                int o_id;
                QString o_table;
                stream >> o_table;
                stream >> o_id;

                SKGUnitObject child(getDocument(), o_id);
                err = child.load();

                // Send message
                IFOKDO(err,
                       getDocument()->sendMessage(i18nc("An information to the user",
                                                        "The unit '%1' has been merged with unit '%2'",
                                                        child.getDisplayName(),
                                                        parentUnit.getDisplayName()),
                                                  SKGDocument::Hidden))

                IFOKDO(err, parentUnit.merge(child))
            }
        }
    } else if (iData->hasFormat(QStringLiteral("application/skg.refund.ids"))) {
        QByteArray encodedData = iData->data(QStringLiteral("application/skg.refund.ids"));
        QDataStream stream(&encodedData, QIODevice::ReadOnly);

        if (iParent.isValid()) {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Merge trackers"), err)
            SKGTrackerObject parentTracker(getObject(iParent));
            while (!stream.atEnd() && !err) {
                int o_id;
                QString o_table;
                stream >> o_table;
                stream >> o_id;

                SKGTrackerObject child(getDocument(), o_id);
                err = child.load();

                // Send message
                IFOKDO(err,
                       parentTracker.getDocument()->sendMessage(i18nc("An information to the user",
                                                                      "The tracker '%1' has been merged with tracker '%2'",
                                                                      child.getDisplayName(),
                                                                      parentTracker.getDisplayName()),
                                                                SKGDocument::Hidden))

                IFOKDO(err, parentTracker.merge(child))
            }
        }
    }
    SKGMainPanel::displayErrorMessage(err);
    return !err;
}

void SKGObjectModel::dataModified(const QString &iTableName, int iIdTransaction)
{
    if (getRealTable() == iTableName || iTableName.isEmpty() || getRealTable() == QStringLiteral("doctransaction")) {
        SKGTRACEINFUNC(1)
        if (iTableName == QStringLiteral("category")) {
            // Full refresh
            m_isResetRealyNeeded = true;
            refresh();
        } else {
            SKGObjectModelBase::dataModified(iTableName, iIdTransaction);
        }
    } else {
        SKGObjectModelBase::dataModified(iTableName, iIdTransaction);
    }
}

QString SKGObjectModel::formatMoney(double iValue) const
{
    return getDocument()->formatMoney(iValue, qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit(), false);
}
