/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGRuleObject.
 *
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgruleobject.h"

#include <klocalizedstring.h>

#include <qdom.h>

#include "skgdocumentbank.h"
#include "skgoperationobject.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgunitvalueobject.h"

SKGRuleObject::SKGRuleObject()
    : SKGRuleObject(nullptr)
{
}

SKGRuleObject::SKGRuleObject(SKGDocument *iDocument, int iID)
    : SKGObjectBase(iDocument, QStringLiteral("v_rule"), iID)
{
}

SKGRuleObject::SKGRuleObject(const SKGRuleObject &iObject) = default;

SKGRuleObject::SKGRuleObject(const SKGObjectBase &iObject)
{
    if (iObject.getRealTable() == QStringLiteral("rule")) {
        copyFrom(iObject);
    } else {
        *this = SKGObjectBase(iObject.getDocument(), QStringLiteral("v_rule"), iObject.getID());
    }
}

SKGRuleObject &SKGRuleObject::operator=(const SKGObjectBase &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGRuleObject &SKGRuleObject::operator=(const SKGRuleObject &iObject)
{
    copyFrom(iObject);
    return *this;
}

SKGRuleObject::~SKGRuleObject() = default;

QString SKGRuleObject::getDisplayName() const
{
    return getSearchDescription();
}

SKGError SKGRuleObject::bookmark(bool iBookmark)
{
    return setAttribute(QStringLiteral("t_bookmarked"), iBookmark ? QStringLiteral("Y") : QStringLiteral("N"));
}

bool SKGRuleObject::isBookmarked() const
{
    return getAttribute(QStringLiteral("t_bookmarked")) == QStringLiteral("Y");
}

SKGError SKGRuleObject::save(bool iInsertOrUpdate, bool iReloadAfterSave)
{
    // Do the save
    SKGError err = SKGObjectBase::save(iInsertOrUpdate, iReloadAfterSave);

    // Raise alarm
    if (!err && getActionType() == ALARM) {
        err = execute();
    }

    return err;
}

SKGError SKGRuleObject::setXMLSearchDefinition(const QString &iXml)
{
    setSearchDescription(SKGRuleObject::getDescriptionFromXML(getDocument(), iXml, false, SEARCH));
    return setAttribute(QStringLiteral("t_definition"), iXml);
}

QString SKGRuleObject::getXMLSearchDefinition() const
{
    return getAttribute(QStringLiteral("t_definition"));
}

SKGError SKGRuleObject::setXMLActionDefinition(const QString &iXml)
{
    setActionDescription(SKGRuleObject::getDescriptionFromXML(getDocument(), iXml, false, getActionType()));
    return setAttribute(QStringLiteral("t_action_definition"), iXml);
}

QString SKGRuleObject::getXMLActionDefinition() const
{
    return getAttribute(QStringLiteral("t_action_definition"));
}

SKGError SKGRuleObject::setSearchDescription(const QString &iDescription)
{
    return setAttribute(QStringLiteral("t_description"), iDescription);
}

QString SKGRuleObject::getSearchDescription() const
{
    return getAttribute(QStringLiteral("t_description"));
}

SKGError SKGRuleObject::setActionDescription(const QString &iDescription)
{
    return setAttribute(QStringLiteral("t_action_description"), iDescription);
}

QString SKGRuleObject::getActionDescription() const
{
    return getAttribute(QStringLiteral("t_action_description"));
}

SKGError SKGRuleObject::setActionType(SKGRuleObject::ActionType iType)
{
    SKGError err =
        setAttribute(QStringLiteral("t_action_type"),
                     (iType == SEARCH ? QStringLiteral("S")
                                      : (iType == UPDATE ? QStringLiteral("U") : (iType == APPLYTEMPLATE ? QStringLiteral("T") : QStringLiteral("A")))));
    return err;
}

SKGRuleObject::ActionType SKGRuleObject::getActionType() const
{
    QString typeString = getAttribute(QStringLiteral("t_action_type"));
    return (typeString == QStringLiteral("S") ? SEARCH
                                              : (typeString == QStringLiteral("U") ? UPDATE : (typeString == QStringLiteral("T") ? APPLYTEMPLATE : ALARM)));
}

SKGError SKGRuleObject::setOrder(double iOrder)
{
    SKGError err;
    double order = iOrder;
    if (order == -1) {
        order = 1;
        SKGStringListList result;
        err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT max(f_sortorder) from rule"), result);
        if (!err && result.count() == 2) {
            order = SKGServices::stringToDouble(result.at(1).at(0)) + 1;
        }
    }
    IFOKDO(err, setAttribute(QStringLiteral("f_sortorder"), SKGServices::doubleToString(order)))
    return err;
}

QString SKGRuleObject::getSelectSqlOrder(const QString &iAdditionalCondition) const
{
    QString wc = iAdditionalCondition;
    QString wc2 = SKGRuleObject::getDescriptionFromXML(getDocument(), getXMLSearchDefinition(), true, SEARCH);
    if (!wc2.isEmpty()) {
        if (wc.isEmpty()) {
            wc = std::move(wc2);
        } else {
            wc = u'(' % wc % ") AND (" % wc2 % u')';
        }
    }
    if (wc.isEmpty()) {
        wc = QStringLiteral("1=1");
    }
    wc = "t_template='N' AND d_date!='0000-00-00' AND (" % wc % u')';
    return wc;
}

SKGRuleObject::SKGAlarmInfo SKGRuleObject::getAlarmInfo() const
{
    SKGTRACEINFUNC(10)
    SKGRuleObject::SKGAlarmInfo alarm;
    alarm.Raised = false;
    alarm.Message = QString();
    alarm.Amount = 0.0;
    alarm.Limit = 0.0;
    if (getActionType() == SKGRuleObject::ALARM) {
        // Alarm mode
        QString wc = getSelectSqlOrder();
        if (wc.isEmpty()) {
            wc = QStringLiteral("1=1");
        }

        SKGDocument *doc = getDocument();

        QStringList list = SKGRuleObject::getFromXML(doc, getXMLActionDefinition(), true, ALARM, false);
        if (!list.isEmpty()) {
            QString sql = list.at(0);
            sql.replace(QStringLiteral("#WC#"), wc);

            SKGStringListList result;
            doc->executeSelectSqliteOrder(sql, result);
            if (result.count() == 2) {
                const auto &r = result.at(1);
                alarm.Raised = (r.at(0) == QStringLiteral("1"));
                alarm.Message = r.at(3);
                alarm.Amount = SKGServices::stringToDouble(r.at(1));
                alarm.Limit = SKGServices::stringToDouble(r.at(2));
            }
        }
    }
    return alarm;
}

SKGError SKGRuleObject::execute(ProcessMode iMode)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (getActionType() == SKGRuleObject::UPDATE) {
        // Update mode
        QString addSql;
        if (iMode == IMPORTED) {
            addSql = QStringLiteral("t_imported!='N'");
        } else if (iMode == IMPORTEDNOTVALIDATE) {
            addSql = QStringLiteral("t_imported='P'");
        } else if (iMode == IMPORTING) {
            addSql = QStringLiteral("t_imported='T'");
        } else if (iMode == NOTCHECKED) {
            addSql = QStringLiteral("t_status!='Y'");
        }
        QString wc = getSelectSqlOrder(addSql);

        SKGDocument *doc = getDocument();
        if (doc != nullptr) {
            QStringList list = SKGRuleObject::getFromXML(doc, getXMLActionDefinition(), true, UPDATE, true); // SQL + SET clause
            int nb = list.count();
            err = doc->beginTransaction("#INTERNAL#" % i18nc("Progression step", "Apply rule"), nb);
            IFOK(err)
            {
                // All sql orders must be executed to be sure than i_tmp is reset
                SKGError err2;
                for (int i = 0; i < nb; ++i) {
                    QString sql = list.at(i);
                    sql.replace(QStringLiteral("#WC#"), wc);
                    err2 = doc->executeSqliteOrder(sql);
                    if (!err2) {
                        err2 = doc->stepForward(i + 1);
                    }
                    if (err2 && !err) {
                        err = err2;
                    }
                }
            }

            IFOK(err)
            {
                SKGStringListList result;
                err = doc->executeSelectSqliteOrder(QStringLiteral("SELECT changes()"), result);
                if (!err && result.count() == 2) {
                    int nbChanges = SKGServices::stringToInt(result.at(1).at(0));
                    if (nbChanges != 0) {
                        doc->sendMessage(
                            i18np("1 transaction modified by %2", "%1 transactions modified by %2", nbChanges, getAttribute(QStringLiteral("i_ORDER"))));
                    }
                }
            }

            SKGENDTRANSACTION(doc, err)
        }
    } else if (getActionType() == SKGRuleObject::ALARM) {
        // Alarm mode
        auto doc = qobject_cast<SKGDocumentBank *>(getDocument());
        if (doc != nullptr) {
            SKGRuleObject::SKGAlarmInfo alarm = getAlarmInfo();
            if (!alarm.Message.isEmpty()) {
                SKGServices::SKGUnitInfo unit = doc->getPrimaryUnit();

                // Build the message
                if (alarm.Message.contains(QStringLiteral("%3"))) {
                    alarm.Message = alarm.Message.arg(doc->formatMoney(alarm.Amount, unit, false),
                                                      doc->formatMoney(alarm.Limit, unit, false),
                                                      doc->formatMoney(alarm.Amount - alarm.Limit, unit, false));
                } else if (alarm.Message.contains(QStringLiteral("%2"))) {
                    alarm.Message = alarm.Message.arg(doc->formatMoney(alarm.Amount, unit, false), doc->formatMoney(alarm.Limit, unit, false));
                } else if (alarm.Message.contains(QStringLiteral("%1"))) {
                    alarm.Message = alarm.Message.arg(doc->formatMoney(alarm.Amount, unit, false));
                }
                getDocument()->sendMessage(alarm.Message);
            }
        }
    } else if (getActionType() == SKGRuleObject::APPLYTEMPLATE) {
        // Template mode
        QString addSql;
        if (iMode == IMPORTED) {
            addSql = QStringLiteral("t_imported!='N'");
        } else if (iMode == IMPORTEDNOTVALIDATE) {
            addSql = QStringLiteral("t_imported='P'");
        } else if (iMode == IMPORTING) {
            addSql = QStringLiteral("t_imported='T'");
        }
        QString wc = getSelectSqlOrder(addSql);

        auto doc = qobject_cast<SKGDocumentBank *>(getDocument());
        if (doc != nullptr) {
            QStringList list = SKGRuleObject::getFromXML(doc, getXMLActionDefinition(), true, APPLYTEMPLATE, false);
            if (!list.isEmpty()) {
                const QString &id = list.at(0);

                // Get template
                SKGOperationObject templateToApply(doc, SKGServices::stringToInt(id));

                // Get transactions to modify
                SKGObjectBase::SKGListSKGObjectBase objectsToModify;
                IFOKDO(err, doc->getObjects(QStringLiteral("v_operation_prop"), wc, objectsToModify))
                int nb = objectsToModify.count();
                for (int i = 0; !err && i < nb; ++i) {
                    SKGOperationObject operationObj(doc, SKGServices::stringToInt(objectsToModify.at(i).getAttribute(QStringLiteral("i_OPID"))));

                    SKGOperationObject op;
                    IFOKDO(err, templateToApply.duplicate(op))
                    IFOKDO(err, op.mergeAttribute(operationObj, SKGOperationObject::PROPORTIONAL, false))
                }
            }
        }
    }
    IFKO(err) err.addError(ERR_FAIL, i18nc("Error message", "Rule %1 failed", getAttribute(QStringLiteral("i_ORDER"))));
    return err;
}

double SKGRuleObject::getOrder() const
{
    return SKGServices::stringToDouble(getAttribute(QStringLiteral("f_sortorder")));
}

QString SKGRuleObject::getDisplayForOperator(const QString &iOperator, const QString &iParam1, const QString &iParam2, const QString &iAtt2)
{
    QString output = iOperator;
    if (output == QStringLiteral("#ATT# LIKE '%#V1S#%'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "contains '#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT# NOT LIKE '%#V1S#%'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "does not contain '#V1S#'")
                     .replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT# LIKE '#V1S#%'")) {
        output =
            i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "starts with '#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT# NOT LIKE '#V1S#%'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "does not start with '#V1S#'")
                     .replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT# LIKE '%#V1S#'")) {
        output =
            i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "ends with '#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT# NOT LIKE '%#V1S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "does not end with '#V1S#'")
                     .replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#=''")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is empty");
    } else if (output == QStringLiteral("#ATT#!=''")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is not empty");
    } else if (output == QStringLiteral("#ATT#=REGEXPCAPTURE('#V1S#', #ATT2#, #V2#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=regexpcapture(#ATT2#, '#V1S#', #V2#)")
                     .replace(QStringLiteral("#V1S#"), iParam1)
                     .replace(QStringLiteral("#V2#"), iParam2)
                     .replace(QStringLiteral("#ATT2#"), iAtt2);
    } else if (output == QStringLiteral("REGEXP('#V1S#', #ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "regexp '#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("NOT(REGEXP('#V1S#', #ATT#))")) {
        output =
            i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "not regexp '#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("WILDCARD('#V1S#', #ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "wildcard '#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("NOT(WILDCARD('#V1S#', #ATT#))")) {
        output =
            i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "not wildcard '#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#=#V1#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=#V1#").replace(QStringLiteral("#V1#"), iParam1);
    } else if (output == QStringLiteral("#ATT#!=#V1#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "!=#V1#").replace(QStringLiteral("#V1#"), iParam1);
    } else if (output == QStringLiteral("#ATT#>#V1#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", ">#V1#").replace(QStringLiteral("#V1#"), iParam1);
    } else if (output == QStringLiteral("#ATT#<#V1#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "<#V1#").replace(QStringLiteral("#V1#"), iParam1);
    } else if (output == QStringLiteral("#ATT#>=#V1#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", ">=#V1#").replace(QStringLiteral("#V1#"), iParam1);
    } else if (output == QStringLiteral("#ATT#<=#V1#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "<=#V1#").replace(QStringLiteral("#V1#"), iParam1);
    } else if (output == QStringLiteral("#ATT#='#V1S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "='#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#!='#V1S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "!='#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#>'#V1S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", ">'#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#<'#V1S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "<'#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#>='#V1S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", ">='#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#<='#V1S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "<='#V1S#'").replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#>=#V1# AND #ATT#<=#V2#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is between #V1# and #V2#")
                     .replace(QStringLiteral("#V1#"), iParam1)
                     .replace(QStringLiteral("#V2#"), iParam2);
    } else if (output == QStringLiteral("((#ATT#>='#V1S#' AND #ATT#<='#V2S#') OR (#ATT#>='#V2S#' AND #ATT#<='#V1S#'))")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is between '#V1S#' and '#V2S#'")
                     .replace(QStringLiteral("#V1S#"), iParam1)
                     .replace(QStringLiteral("#V2S#"), iParam2);
    } else if (output == QStringLiteral("#ATT#=lower(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is set to lower");
    } else if (output == QStringLiteral("#ATT#=upper(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is set to upper");
    } else if (output == QStringLiteral("#ATT#=capitalize(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is set to capitalize");
    } else if (output == QStringLiteral("#ATT#!=lower(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is not lower");
    } else if (output == QStringLiteral("#ATT#!=upper(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is not upper");
    } else if (output == QStringLiteral("#ATT#!=capitalize(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is not capitalize");
    } else if (output == QStringLiteral("#ATT#= lower(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is lower");
    } else if (output == QStringLiteral("#ATT#= upper(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is upper");
    } else if (output == QStringLiteral("#ATT#= capitalize(#ATT#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is capitalize");
    } else if (output == QStringLiteral("#ATT#=replace(#ATT2#,'#V1S#','#V2S#')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=#ATT2# with '#V1S#' replaced by '#V2S#'")
                     .replace(QStringLiteral("#V1S#"), iParam1)
                     .replace(QStringLiteral("#V2S#"), iParam2)
                     .replace(QStringLiteral("#ATT2#"), iAtt2);
    } else if (output == QStringLiteral("#ATT#=substr(#ATT2#,'#V1#','#V2#')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=substring of #ATT2# from #V1# to #V2#")
                     .replace(QStringLiteral("#V1#"), iParam1)
                     .replace(QStringLiteral("#V2#"), iParam2)
                     .replace(QStringLiteral("#ATT2#"), iAtt2);
    } else if (output == QStringLiteral("#ATT#=#ATT2#")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=#ATT2#").replace(QStringLiteral("#ATT2#"), iAtt2);
    } else if (output == QStringLiteral("#ATT#=WORD(#ATT2#,#V1S#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=word(#ATT2#,#V1S#)")
                     .replace(QStringLiteral("#ATT2#"), iAtt2)
                     .replace(QStringLiteral("#V1S#"), iParam1);
    } else if (output == QStringLiteral("#ATT#=todate(#ATT2#,'#DF#')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=#ATT2# as date with format #DF#")
                     .replace(QStringLiteral("#ATT2#"), iAtt2)
                     .replace(QStringLiteral("#DF#"), iParam2);
    } else if (output == QStringLiteral("#ATT#=todate(WORD(#ATT2#,#V1S#),'#DF#')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "=word(#ATT2#,#V1S#) as date with format #DF#")
                     .replace(QStringLiteral("#ATT2#"), iAtt2)
                     .replace(QStringLiteral("#V1S#"), iParam1)
                     .replace(QStringLiteral("#DF#"), iParam2);
    } else if (output == QStringLiteral("STRFTIME('%Y-%m',#ATT#)=STRFTIME('%Y-%m',date('now', 'localtime'))")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in current month");
    } else if (output == QStringLiteral("STRFTIME('%Y-%m',#ATT#)=STRFTIME('%Y-%m',date('now', 'localtime','start of month','-1 month'))")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in previous month");
    } else if (output == QStringLiteral("STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime'))")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in current year");
    } else if (output == QStringLiteral("STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime','start of month','-1 year'))")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in previous year");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-30 day') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in last 30 days");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-3 month') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in last 3 months");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-6 month') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in last 6 months");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-12 month') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in last 12 months");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-2 year') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in last 2 years");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-3 year') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in last 3 years");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-5 year') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "is in last 5 years");
    } else if (output == QStringLiteral("ABS(TOTAL(#ATT#))#OP##V1#,ABS(TOTAL(#ATT#)), #V1#, '#V2S#'")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "If total(#ATT#)#OP##V1# then send '#V2S#'")
                     .replace(QStringLiteral("#V1#"), iParam1)
                     .replace(QStringLiteral("#V2S#"), iParam2)
                     .replace(QStringLiteral("#OP#"), iAtt2);
    } else if (output == QStringLiteral("APPLYTEMPLATE(#V1#)")) {
        output = i18nc("Description of a condition. Do not translate key words (#V1S#, #V1#, …)", "Apply the template '#V2S#'")
                     .replace(QStringLiteral("#V2S#"), iParam2);
    }

    return output;
}

QString SKGRuleObject::getToolTipForOperator(const QString &iOperator)
{
    QString output = iOperator;
    if (output == QStringLiteral("#ATT# LIKE '%#V1S#%'")) {
        output = i18nc("Help for a condition", "To find out if the attribute contains a given string");
    } else if (output == QStringLiteral("#ATT# NOT LIKE '%#V1S#%'")) {
        output = i18nc("Help for a condition", "To find out if the attribute doesn't contain a given string");
    } else if (output == QStringLiteral("#ATT# LIKE '#V1S#%'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is starting by a given string");
    } else if (output == QStringLiteral("#ATT# NOT LIKE '#V1S#%'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not starting by a given string");
    } else if (output == QStringLiteral("#ATT# LIKE '%#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is ending by a given string");
    } else if (output == QStringLiteral("#ATT# NOT LIKE '%#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not ending by a given string");
    } else if (output == QStringLiteral("#ATT#=''")) {
        output = i18nc("Help for a condition", "To find out if the attribute is empty");
    } else if (output == QStringLiteral("#ATT#!=''")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not empty");
    } else if (output == QStringLiteral("#ATT#=REGEXPCAPTURE('#V1S#', #ATT2#, #V2#)")) {
        output = i18nc("Help for a condition", "To get a captured value by a given regular expression (eg. \"(\\d+)(\\s*)(cm|inch(es)?)\")");
    } else if (output == QStringLiteral("REGEXP('#V1S#', #ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is matching a given regular expression (eg. \"^[H,h]ello$\")");
    } else if (output == QStringLiteral("NOT(REGEXP('#V1S#', #ATT#))")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not matching a given regular expression (eg. \"^[H,h]ello$\")");
    } else if (output == QStringLiteral("WILDCARD('#V1S#', #ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is matching a given wildcard expression (eg. \"_ello\")");
    } else if (output == QStringLiteral("NOT(WILDCARD('#V1S#', #ATT#))")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not matching a given wildcard expression (eg. \"_ello\")");
    } else if (output == QStringLiteral("#ATT#=#V1#")) {
        output = i18nc("Help for a condition", "To find out if the attribute is equal to a given value");
    } else if (output == QStringLiteral("#ATT#!=#V1#")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not equal to a given value");
    } else if (output == QStringLiteral("#ATT#>#V1#")) {
        output = i18nc("Help for a condition", "To find out if the attribute is greater than a given value");
    } else if (output == QStringLiteral("#ATT#<#V1#")) {
        output = i18nc("Help for a condition", "To find out if the attribute is smaller than a given value");
    } else if (output == QStringLiteral("#ATT#>=#V1#")) {
        output = i18nc("Help for a condition", "To find out if the attribute is greater or equal to a given value");
    } else if (output == QStringLiteral("#ATT#<=#V1#")) {
        output = i18nc("Help for a condition", "To set the attribute with a given value");
    } else if (output == QStringLiteral("#ATT#='#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is equal to a given string");
    } else if (output == QStringLiteral("#ATT#!='#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not equal to a given string");
    } else if (output == QStringLiteral("#ATT#>'#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is greater than a given string");
    } else if (output == QStringLiteral("#ATT#<'#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is smaller than a given string");
    } else if (output == QStringLiteral("#ATT#>='#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is greater or equal to a given string");
    } else if (output == QStringLiteral("#ATT#<='#V1S#'")) {
        output = i18nc("Help for a condition", "To find out if the attribute is smaller or equal to a given string");
    } else if (output == QStringLiteral("#ATT#>=#V1# AND #ATT#<=#V2#")) {
        output = i18nc("Help for a condition", "To find out if the attribute is between two given values");
    } else if (output == QStringLiteral("((#ATT#>='#V1S#' AND #ATT#<='#V2S#') OR (#ATT#>='#V2S#' AND #ATT#<='#V1S#'))")) {
        output = i18nc("Help for a condition", "To find out if the attribute is between two given strings");
    } else if (output == QStringLiteral("#ATT#=lower(#ATT#)")) {
        output = i18nc("Help for a condition", "To set the attribute in lower case (eg. hello)");
    } else if (output == QStringLiteral("#ATT#=upper(#ATT#)")) {
        output = i18nc("Help for a condition", "To set the attribute in upper case (eg. HELLO)");
    } else if (output == QStringLiteral("#ATT#=capitalize(#ATT#)")) {
        output = i18nc("Help for a condition", "To set the attribute in capitalized case (eg. Hello)");
    } else if (output == QStringLiteral("#ATT#!=lower(#ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not in lower case (eg. hello)");
    } else if (output == QStringLiteral("#ATT#!=upper(#ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not in upper case (eg. HELLO)");
    } else if (output == QStringLiteral("#ATT#!=capitalize(#ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is not in capitalized case (eg. Hello)");
    } else if (output == QStringLiteral("#ATT#= lower(#ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is in lower case (eg. hello)");
    } else if (output == QStringLiteral("#ATT#= upper(#ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is in upper case (eg. HELLO)");
    } else if (output == QStringLiteral("#ATT#= capitalize(#ATT#)")) {
        output = i18nc("Help for a condition", "To find out if the attribute is in capitalized case (eg. Hello)");
    } else if (output == QStringLiteral("#ATT#=replace(#ATT2#,'#V1S#','#V2S#')")) {
        output = i18nc("Help for a condition", "To set the attribute with the value of another attribute where a value is replaced by another one");
    } else if (output == QStringLiteral("#ATT#=substr(#ATT2#,'#V1#','#V2#')")) {
        output = i18nc("Help for a condition", "To set the attribute with a part of the value of another attribute");
    } else if (output == QStringLiteral("#ATT#=#ATT2#")) {
        output = i18nc("Help for a condition", "To set the attribute with the value of another attribute");
    } else if (output == QStringLiteral("#ATT#=WORD(#ATT2#,#V1S#)")) {
        output = i18nc("Help for a condition", "To set the attribute with a word of the value of another attribute converted in date format");
    } else if (output == QStringLiteral("#ATT#=todate(#ATT2#,'#DF#')")) {
        output = i18nc("Help for a condition", "To set the date attribute with the value of another attribute");
    } else if (output == QStringLiteral("#ATT#=todate(WORD(#ATT2#,#V1S#),'#DF#')")) {
        output = i18nc("Help for a condition", "To set the date attribute with a word of another attribute converted in date format");
    } else if (output == QStringLiteral("STRFTIME('%Y-%m',#ATT#)=STRFTIME('%Y-%m',date('now', 'localtime'))")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is today");
    } else if (output == QStringLiteral("STRFTIME('%Y-%m',#ATT#)=STRFTIME('%Y-%m',date('now', 'localtime','start of month','-1 month'))")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in previous month");
    } else if (output == QStringLiteral("STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime'))")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in current year");
    } else if (output == QStringLiteral("STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime','start of month','-1 year'))")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in previous year");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-30 day') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in last 30 days");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-3 month') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in last 3 months");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-6 month') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in last 6 months");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-12 month') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in last 12 months");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-2 year') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in last 2 years");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-3 year') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in last 3 years");
    } else if (output == QStringLiteral("#ATT#>date('now', 'localtime','-5 year') AND #ATT#<=date('now', 'localtime')")) {
        output = i18nc("Help for a condition", "To find out if the date of the transaction is in last 5 years");
    }

    return output;
}

QStringList SKGRuleObject::getListOfOperators(SKGServices::AttributeType iAttributeType, SKGRuleObject::ActionType iType)
{
    QStringList output;
    if (iType == UPDATE) {
        // Mode update
        if (iAttributeType == SKGServices::TEXT) {
            output.push_back(QStringLiteral("#ATT#='#V1S#'"));
            output.push_back(QStringLiteral("#ATT#=lower(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#=upper(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#=capitalize(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#=replace(#ATT2#,'#V1S#','#V2S#')"));
            output.push_back(QStringLiteral("#ATT#=REGEXPCAPTURE('#V1S#', #ATT2#, #V2#)"));
        } else if (iAttributeType == SKGServices::INTEGER || iAttributeType == SKGServices::FLOAT) {
            output.push_back(QStringLiteral("#ATT#=#V1#"));
        } else if (iAttributeType == SKGServices::DATE || iAttributeType == SKGServices::BOOL || iAttributeType == SKGServices::TRISTATE) {
            output.push_back(QStringLiteral("#ATT#='#V1S#'"));
        }
        if (iAttributeType == SKGServices::DATE) {
            output.push_back(QStringLiteral("#ATT#=todate(#ATT2#,'#DF#')"));
            output.push_back(QStringLiteral("#ATT#=todate(WORD(#ATT2#,#V1S#),'#DF#')"));
        } else if (iAttributeType != SKGServices::BOOL && iAttributeType != SKGServices::TRISTATE) {
            output.push_back(QStringLiteral("#ATT#=substr(#ATT2#,'#V1#','#V2#')"));
            output.push_back(QStringLiteral("#ATT#=#ATT2#"));
            output.push_back(QStringLiteral("#ATT#=WORD(#ATT2#,#V1S#)"));
        }
    } else if (iType == SEARCH) {
        // Mode query
        if (iAttributeType == SKGServices::TEXT) {
            output.push_back(QStringLiteral("#ATT# LIKE '%#V1S#%'"));
            output.push_back(QStringLiteral("#ATT# NOT LIKE '%#V1S#%'"));
            output.push_back(QStringLiteral("#ATT# LIKE '#V1S#%'"));
            output.push_back(QStringLiteral("#ATT# NOT LIKE '#V1S#%'"));
            output.push_back(QStringLiteral("#ATT# LIKE '%#V1S#'"));
            output.push_back(QStringLiteral("#ATT# NOT LIKE '%#V1S#'"));
            output.push_back(QStringLiteral("#ATT#=''"));
            output.push_back(QStringLiteral("#ATT#!=''"));
            output.push_back(QStringLiteral("#ATT#= lower(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#!=lower(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#= upper(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#!=upper(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#= capitalize(#ATT#)"));
            output.push_back(QStringLiteral("#ATT#!=capitalize(#ATT#)"));
            output.push_back(QStringLiteral("REGEXP('#V1S#', #ATT#)"));
            output.push_back(QStringLiteral("NOT(REGEXP('#V1S#', #ATT#))"));
            output.push_back(QStringLiteral("WILDCARD('#V1S#', #ATT#)"));
            output.push_back(QStringLiteral("NOT(WILDCARD('#V1S#', #ATT#))"));
        }

        if (iAttributeType == SKGServices::INTEGER || iAttributeType == SKGServices::FLOAT) {
            output.push_back(QStringLiteral("#ATT#=#V1#"));
            output.push_back(QStringLiteral("#ATT#!=#V1#"));
            output.push_back(QStringLiteral("#ATT#>#V1#"));
            output.push_back(QStringLiteral("#ATT#<#V1#"));
            output.push_back(QStringLiteral("#ATT#>=#V1#"));
            output.push_back(QStringLiteral("#ATT#<=#V1#"));
            output.push_back(QStringLiteral("#ATT#>=#V1# AND #ATT#<=#V2#"));
        }

        if (iAttributeType == SKGServices::BOOL || iAttributeType == SKGServices::TRISTATE || iAttributeType == SKGServices::TEXT
            || iAttributeType == SKGServices::DATE) {
            output.push_back(QStringLiteral("#ATT#='#V1S#'"));
        }

        if (iAttributeType == SKGServices::TRISTATE || iAttributeType == SKGServices::TEXT || iAttributeType == SKGServices::DATE) {
            output.push_back(QStringLiteral("#ATT#!='#V1S#'"));
        }

        if (iAttributeType == SKGServices::TEXT || iAttributeType == SKGServices::DATE) {
            output.push_back(QStringLiteral("#ATT#>'#V1S#'"));
            output.push_back(QStringLiteral("#ATT#<'#V1S#'"));
            output.push_back(QStringLiteral("#ATT#>='#V1S#'"));
            output.push_back(QStringLiteral("#ATT#<='#V1S#'"));
            output.push_back(QStringLiteral("((#ATT#>='#V1S#' AND #ATT#<='#V2S#') OR (#ATT#>='#V2S#' AND #ATT#<='#V1S#'))"));
        }

        if (iAttributeType == SKGServices::DATE) {
            output.push_back(QStringLiteral("STRFTIME('%Y-%m',#ATT#)=STRFTIME('%Y-%m',date('now', 'localtime'))"));
            output.push_back(QStringLiteral("STRFTIME('%Y-%m',#ATT#)=STRFTIME('%Y-%m',date('now', 'localtime','start of month','-1 month'))"));
            output.push_back(QStringLiteral("STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime'))"));
            output.push_back(QStringLiteral("STRFTIME('%Y',#ATT#)=STRFTIME('%Y',date('now', 'localtime','start of month','-1 year'))"));
            output.push_back(QStringLiteral("#ATT#>date('now', 'localtime','-30 day') AND #ATT#<=date('now', 'localtime')"));
            output.push_back(QStringLiteral("#ATT#>date('now', 'localtime','-3 month') AND #ATT#<=date('now', 'localtime')"));
            output.push_back(QStringLiteral("#ATT#>date('now', 'localtime','-6 month') AND #ATT#<=date('now', 'localtime')"));
            output.push_back(QStringLiteral("#ATT#>date('now', 'localtime','-12 month') AND #ATT#<=date('now', 'localtime')"));
            output.push_back(QStringLiteral("#ATT#>date('now', 'localtime','-2 year') AND #ATT#<=date('now', 'localtime')"));
            output.push_back(QStringLiteral("#ATT#>date('now', 'localtime','-3 year') AND #ATT#<=date('now', 'localtime')"));
            output.push_back(QStringLiteral("#ATT#>date('now', 'localtime','-5 year') AND #ATT#<=date('now', 'localtime')"));
        }
    } else if (iType == ALARM) {
        output.push_back(QStringLiteral("ABS(TOTAL(#ATT#))#OP##V1#,ABS(TOTAL(#ATT#)), #V1#, '#V2S#'"));
    } else if (iType == APPLYTEMPLATE) {
        output.push_back(QStringLiteral("APPLYTEMPLATE(#V1#)"));
    }
    return output;
}

QStringList SKGRuleObject::getFromXML(SKGDocument *iDocument, const QString &iXML, bool iSQL, SKGRuleObject::ActionType iType, bool iFullUpdate)
{
    QStringList output;
    if (iFullUpdate) {
        // Add markers
        output.push_back(QStringLiteral("UPDATE v_operation_prop set i_tmp=1 WHERE #WC#"));
    }

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iXML);
    QDomElement root = doc.documentElement();
    if (root.tagName() == QStringLiteral("element") || iType != SEARCH) {
        // Mode advanced
        QDomNode l = root.firstChild();
        while (!l.isNull()) {
            QDomElement elementl = l.toElement();
            if (!elementl.isNull()) {
                QString lineDescription;

                QDomNode n = elementl.firstChild();
                bool parenthesisNeeded = false;
                while (!n.isNull()) {
                    QDomElement element = n.toElement();
                    if (!element.isNull()) {
                        // Build element description
                        QString elementDescription;
                        QString attribute = element.attribute(QStringLiteral("attribute"));
                        QString propName;
                        if (iSQL) {
                            attribute = SKGServices::stringToSqlString(attribute);
                            if (attribute.startsWith(QStringLiteral("p_"))) {
                                // Case property
                                propName = attribute.right(attribute.length() - 2);
                                if (iType == SEARCH) {
                                    attribute = "i_PROPPNAME='" % SKGServices::stringToSqlString(propName) % "' AND i_PROPVALUE";
                                } else {
                                    attribute = QStringLiteral("t_value");
                                }
                            }

                            QString part = element.attribute(QStringLiteral("operator"));
                            part = part.replace(QStringLiteral("#V1#"), SKGServices::stringToSqlString(element.attribute(QStringLiteral("value"))));
                            part = part.replace(QStringLiteral("#V1S#"), SKGServices::stringToSqlString(element.attribute(QStringLiteral("value"))));
                            part = part.replace(QStringLiteral("#V2#"), SKGServices::stringToSqlString(element.attribute(QStringLiteral("value2"))));
                            part = part.replace(QStringLiteral("#V2S#"), SKGServices::stringToSqlString(element.attribute(QStringLiteral("value2"))));
                            part = part.replace(QStringLiteral("#DF#"), SKGServices::stringToSqlString(element.attribute(QStringLiteral("value2"))));
                            part = part.replace(QStringLiteral("#OP#"), SKGServices::stringToSqlString(element.attribute(QStringLiteral("operator2"))));

                            elementDescription += part;
                        } else {
                            attribute = "operation." % attribute;
                            if (iDocument != nullptr) {
                                attribute = iDocument->getDisplay(attribute);
                            }

                            if (iType != ALARM && iType != APPLYTEMPLATE) {
                                elementDescription = attribute;
                                elementDescription += ' ';
                            }

                            QString tmp = element.attribute(QStringLiteral("att2s"));
                            if (tmp.isEmpty()) {
                                tmp = element.attribute(QStringLiteral("operator2"));
                            }
                            QString part = SKGRuleObject::getDisplayForOperator(element.attribute(QStringLiteral("operator")),
                                                                                element.attribute(QStringLiteral("value")),
                                                                                element.attribute(QStringLiteral("value2")),
                                                                                tmp);

                            elementDescription += part;
                        }

                        elementDescription = elementDescription.replace(QStringLiteral("#ATT#"), attribute);

                        // Att2
                        QString attribute2 = element.attribute(QStringLiteral("att2"));
                        if (iSQL) {
                            attribute2 = SKGServices::stringToSqlString(attribute2);
                            if (attribute2.startsWith(QStringLiteral("p_"))) {
                                QString propertyName = attribute2.right(attribute2.length() - 2);
                                attribute2 = "IFNULL((SELECT op2.i_PROPVALUE FROM v_operation_prop op2 WHERE op2.id=v_operation_prop.id AND op2.i_PROPPNAME='"
                                    % SKGServices::stringToSqlString(propertyName) % "'),'')";
                            }

                            if (element.attribute(QStringLiteral("attribute")).startsWith(QStringLiteral("p_"))) {
                                attribute2 = "(SELECT " % attribute2 % " FROM v_operation_prop WHERE i_PROPPID=parameters.id)";
                            }
                        }
                        elementDescription = elementDescription.replace(QStringLiteral("#ATT2#"), attribute2);

                        // Add it to line description
                        if (iSQL) {
                            if (iType == UPDATE) {
                                if (!iFullUpdate) {
                                    output.push_back(elementDescription);
                                } else {
                                    if (attribute == QStringLiteral("t_REALCATEGORY")) {
                                        elementDescription.replace(attribute, QStringLiteral("t_fullname"));

                                        QString parentcat;
                                        QString cat = element.attribute(QStringLiteral("value"));
                                        bool stop = false;
                                        while (!stop) {
                                            int posSeparator = cat.indexOf(OBJECTSEPARATOR);
                                            if (posSeparator == -1) {
                                                if (parentcat.isEmpty()) {
                                                    output.push_back(QStringLiteral(
                                                        "UPDATE category SET rd_category_id=0 WHERE rd_category_id IS NULL OR rd_category_id=''"));
                                                    output.push_back("INSERT OR IGNORE INTO category (t_name, rd_category_id) VALUES ('"
                                                                     % SKGServices::stringToSqlString(cat) % "', 0)");
                                                } else {
                                                    output.push_back("INSERT OR IGNORE INTO category (t_name, rd_category_id) VALUES ('"
                                                                     % SKGServices::stringToSqlString(cat) % "',(SELECT id FROM category WHERE t_fullname='"
                                                                     % SKGServices::stringToSqlString(parentcat) % "'))");
                                                }
                                                stop = true;
                                            } else {
                                                // Get first and second parts of the branch
                                                QString first = cat.mid(0, posSeparator);
                                                QString second =
                                                    cat.mid(posSeparator + OBJECTSEPARATOR.length(), cat.length() - posSeparator - OBJECTSEPARATOR.length());
                                                if (parentcat.isEmpty()) {
                                                    output.push_back(QStringLiteral(
                                                        "UPDATE category SET rd_category_id=0 WHERE rd_category_id IS NULL OR rd_category_id=''"));
                                                    output.push_back("INSERT OR IGNORE INTO category (t_name, rd_category_id) VALUES ('"
                                                                     % SKGServices::stringToSqlString(first) % "', 0)");
                                                } else {
                                                    output.push_back("INSERT OR IGNORE INTO category (t_name, rd_category_id) VALUES ('"
                                                                     % SKGServices::stringToSqlString(first) % "',(SELECT id FROM category WHERE t_fullname='"
                                                                     % SKGServices::stringToSqlString(parentcat) % "'))");
                                                }

                                                if (parentcat.isEmpty()) {
                                                    parentcat = std::move(first);
                                                } else {
                                                    parentcat = parentcat % OBJECTSEPARATOR % first;
                                                }
                                                cat = std::move(second);
                                            }
                                        }
                                        output.push_back("UPDATE suboperation set r_category_id=(SELECT id FROM category WHERE " % elementDescription
                                                         % ") WHERE i_tmp=1");
                                    } else if (element.attribute(QStringLiteral("attribute")).startsWith(QStringLiteral("p_"))) {
                                        output.push_back("INSERT OR IGNORE INTO parameters (t_uuid_parent, t_name, i_tmp) SELECT o.id||'-operation', '"
                                                         % SKGServices::stringToSqlString(propName) % "', 1 FROM operation o WHERE o.i_tmp=1");
                                        output.push_back("UPDATE parameters set " % elementDescription % " WHERE i_tmp=1 AND t_name='"
                                                         % SKGServices::stringToSqlString(propName) % "' AND NOT(" % elementDescription % u')');
                                        output.push_back("DELETE FROM parameters WHERE i_tmp=1 AND t_name='" % SKGServices::stringToSqlString(propName)
                                                         % "' AND t_value=''");
                                    } else {
                                        output.push_back("UPDATE v_operation_prop set " % elementDescription % " WHERE i_tmp=1 AND NOT(" % elementDescription
                                                         % u')');
                                    }
                                }
                            } else if (iType == ALARM) {
                                output.push_back("SELECT " % elementDescription % " FROM v_operation_prop WHERE #WC#");
                            } else if (iType == APPLYTEMPLATE) {
                                output.push_back(element.attribute(QStringLiteral("value")));
                            }
                        }

                        if (!lineDescription.isEmpty()) {
                            lineDescription += (iType == UPDATE ? QStringLiteral(" , ")
                                                                : (iSQL ? QStringLiteral(" AND ") : i18nc("logical operator in a search query", " and ")));
                            parenthesisNeeded = true;
                        }
                        lineDescription += elementDescription;
                    }
                    n = n.nextSibling();
                }

                if (!(iType != SEARCH && iSQL) && !lineDescription.isEmpty()) {
                    if (iType == SEARCH && parenthesisNeeded) {
                        lineDescription = u'(' % lineDescription % u')';
                    }
                    output.push_back(lineDescription);
                }
            }
            l = l.nextSibling();
        }
    } else {
        output.push_back(root.attribute(iSQL ? QStringLiteral("sql") : QStringLiteral("query")));
    }

    if (iFullUpdate) {
        // Remove markers
        output.push_back(QStringLiteral("UPDATE v_operation_prop set i_tmp=0 WHERE i_tmp=1"));
    }
    return output;
}

QString SKGRuleObject::getDescriptionFromXML(SKGDocument *iDocument, const QString &iXML, bool iSQL, SKGRuleObject::ActionType iType)
{
    QString output;

    QStringList list = getFromXML(iDocument, iXML, iSQL, iType);
    int nb = list.count();
    for (int i = 0; i < nb; ++i) {
        output.append(list.at(i));
        if (i < nb - 1) {
            output.append(iType != SEARCH ? QStringLiteral(" , ") : (iSQL ? QStringLiteral(" OR ") : i18nc("logical operator in a search query", " or ")));
        }
    }
    return output;
}

SKGError SKGRuleObject::createPayeeCategoryRule(SKGDocument *iDocument, const QString &iPayee, const QString &iCategory, SKGRuleObject &oRule)
{
    SKGError err;
    oRule = SKGRuleObject(iDocument);
    // TODO(Stephane MANKOWSKI): escape values
    IFOKDO(err, oRule.setActionType(SKGRuleObject::UPDATE))
    IFOKDO(err,
           oRule.setSearchDescription(iDocument->getDisplay(QStringLiteral("t_PAYEE")) % QLatin1Char(' ')
                                      % getDisplayForOperator(QStringLiteral("#ATT#='#V1S#'"), iPayee, QString(), QString())))
    IFOKDO(err,
           oRule.setXMLSearchDefinition("<!DOCTYPE SKGML><element> <!--OR--> <element>  <!--AND-->  <element value=\"" % iPayee
                                        % "\" attribute=\"t_PAYEE\" operator=\"#ATT#='#V1S#'\"/> </element></element>"))
    IFOKDO(err,
           oRule.setActionDescription(iDocument->getDisplay(QStringLiteral("t_REALCATEGORY")) % QLatin1Char(' ')
                                      % getDisplayForOperator(QStringLiteral("#ATT#='#V1S#'"), iCategory, QString(), QString())))
    IFOKDO(err,
           oRule.setXMLActionDefinition("<!DOCTYPE SKGML><element> <!--OR--> <element>  <!--AND-->  <element value=\"" % iCategory
                                        % "\" attribute=\"t_REALCATEGORY\" operator=\"#ATT#='#V1S#'\"/> </element></element>"))
    IFOKDO(err, oRule.save())

    return err;
}
