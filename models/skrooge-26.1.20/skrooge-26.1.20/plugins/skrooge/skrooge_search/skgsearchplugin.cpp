/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to search and process transactions
 *
 * @author Stephane MANKOWSKI
 */
#include "skgsearchplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include <qdom.h>

#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skgerror.h"
#include "skghtmlboardwidget.h"
#include "skgmainpanel.h"
#include "skgquerycreator.h"
#include "skgruleobject.h"
#include "skgsearch_settings.h"
#include "skgsearchpluginwidget.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGSearchPlugin, "metadata.json")

SKGSearchPlugin::SKGSearchPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
    , m_searchField(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGSearchPlugin::raiseAlarms, Qt::QueuedConnection);
}

SKGSearchPlugin::~SKGSearchPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

int SKGSearchPlugin::getNbDashboardWidgets()
{
    return 1;
}

QString SKGSearchPlugin::getDashboardWidgetTitle(int iIndex)
{
    Q_UNUSED(iIndex)
    return i18nc("Noun, alarms", "Alarms");
}

SKGBoardWidget *SKGSearchPlugin::getDashboardWidget(int iIndex)
{
    Q_UNUSED(iIndex)
    return new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                  m_currentBankDocument,
                                  getDashboardWidgetTitle(iIndex),
                                  QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/alarm.qml")),
                                  QStringList() << QStringLiteral("operation") << QStringLiteral("rule"));
}

bool SKGSearchPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_search"), title());
    setXMLFile(QStringLiteral("skrooge_search.rc"));

    // Create yours actions here
    // Execute on all operation
    auto actExecuteAll =
        new QAction(SKGServices::fromTheme(QStringLiteral("system-run")), i18nc("Verb, action to execute", "Execute on all transactions"), this);
    connect(actExecuteAll, &QAction::triggered, this, [this] {
        execute(SKGRuleObject::ALL);
    });
    registerGlobalAction(QStringLiteral("execute_all"), actExecuteAll, QStringList() << QStringLiteral("rule"), 1, -1, 501);

    // Execute on not checked
    {
        QStringList overlaycsv;
        overlaycsv.push_back(QStringLiteral("document-import"));
        auto actExecuteImported = new QAction(SKGServices::fromTheme(QStringLiteral("system-run"), overlaycsv),
                                              i18nc("Verb, action to execute", "Execute on not checked transactions"),
                                              this);
        connect(actExecuteImported, &QAction::triggered, this, [this] {
            execute(SKGRuleObject::NOTCHECKED);
        });
        registerGlobalAction(QStringLiteral("execute_notchecked"), actExecuteImported, QStringList() << QStringLiteral("rule"), 1, -1, 502);
    }

    // Execute on imported transaction
    {
        QStringList overlaycsv;
        overlaycsv.push_back(QStringLiteral("document-import"));
        auto actExecuteImported = new QAction(SKGServices::fromTheme(QStringLiteral("system-run"), overlaycsv),
                                              i18nc("Verb, action to execute", "Execute on imported transactions"),
                                              this);
        connect(actExecuteImported, &QAction::triggered, this, [this] {
            execute(SKGRuleObject::IMPORTED);
        });
        registerGlobalAction(QStringLiteral("execute_imported"), actExecuteImported, QStringList() << QStringLiteral("rule"), 1, -1, 502);
    }

    // Execute on not validated
    {
        QStringList overlaycsv;
        overlaycsv.push_back(QStringLiteral("dialog-ok"));
        auto actExecuteNotValidated = new QAction(SKGServices::fromTheme(QStringLiteral("system-run"), overlaycsv),
                                                  i18nc("Verb, action to execute", "Execute on not validated transactions"),
                                                  this);
        connect(actExecuteNotValidated, &QAction::triggered, this, [this] {
            execute(SKGRuleObject::IMPORTEDNOTVALIDATE);
        });
        registerGlobalAction(QStringLiteral("execute_not_validated"), actExecuteNotValidated, QStringList() << QStringLiteral("rule"), 1, -1, 503);
    }

    // Search
    QAction *actSearch = actionCollection()->addAction(KStandardAction::Find, QStringLiteral("edit_find"), this, SLOT(find()));
    registerGlobalAction(QStringLiteral("edit_find"), actSearch); // Global
    auto actSearch2 = new QAction(actSearch->icon(), actSearch->text(), this);
    connect(actSearch2, &QAction::triggered, this, &SKGSearchPlugin::find);
    registerGlobalAction(QStringLiteral("edit_find_ctx"),
                         actSearch2,
                         QStringList() << QStringLiteral("account") << QStringLiteral("category") << QStringLiteral("refund") << QStringLiteral("payee")
                                       << QStringLiteral("operation") << QStringLiteral("suboperation"),
                         1,
                         -1,
                         130); // For contextual menus

    // Search field
    QStringList attributeForQuery;
    attributeForQuery.reserve(40);
    attributeForQuery << QStringLiteral("d_DATEOP") << QStringLiteral("t_number") << QStringLiteral("t_mode") << QStringLiteral("t_PAYEE")
                      << QStringLiteral("t_comment") << QStringLiteral("t_REALCOMMENT") << QStringLiteral("t_REALCATEGORY") << QStringLiteral("t_status")
                      << QStringLiteral("t_bookmarked") << QStringLiteral("t_imported") << QStringLiteral("t_TRANSFER") << QStringLiteral("t_UNIT")
                      << QStringLiteral("t_ACCOUNT") << QStringLiteral("t_BANK") << QStringLiteral("t_TOACCOUNT") << QStringLiteral("f_REALCURRENTAMOUNT")
                      << QStringLiteral("t_REALREFUND") << QStringLiteral("f_BALANCE") << QStringLiteral("i_NBSUBOPERATIONS");

    // Adding properties
    QStringList properties;
    m_currentBankDocument->getDistinctValues(
        QStringLiteral("parameters"),
        QStringLiteral("t_name"),
        QStringLiteral("(t_uuid_parent like '%-operation' OR t_uuid_parent like '%-suboperation') AND t_name NOT LIKE 'SKG_%'"),
        properties);
    int nb = properties.count();
    for (int i = 0; i < nb; ++i) {
        attributeForQuery.push_back("p_" % properties.at(i));
    }

    m_searchField = new SKGQueryCreator(SKGMainPanel::getMainPanel(), true);
    m_searchField->setParameters(m_currentBankDocument, QStringLiteral("v_suboperation_consolidated"), attributeForQuery, false);
    connect(m_searchField, &SKGQueryCreator::search, this, &SKGSearchPlugin::onOpen);

    auto *m_searchFieldAct = new QWidgetAction(this);
    m_searchFieldAct->setDefaultWidget(m_searchField);
    registerGlobalAction(QStringLiteral("edit_search"), m_searchFieldAct); // Global

    return true;
}

void SKGSearchPlugin::onOpen()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)
    SKGRuleObject rule;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    rule = SKGRuleObject(m_currentBankDocument);
    IFOKDO(err, rule.setXMLSearchDefinition(m_searchField->getXMLCondition()))
    IFOKDO(err, rule.setOrder(-1))
    IFOKDO(err, rule.setActionType(SKGRuleObject::SEARCH))

    QString wc = "i_SUBOPID in (SELECT i_SUBOPID FROM v_operation_prop WHERE " % rule.getSelectSqlOrder() % u')';
    QString title = i18nc("Noun, a list of items", "Sub transactions corresponding to rule '%1'", rule.getSearchDescription());

    // Call transaction plugin
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGMainPanel::getMainPanel()->getDocument()->getParameter(QStringLiteral("SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS")));
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("parameters"));
        doc.appendChild(root);
    }

    root.setAttribute(QStringLiteral("operationWhereClause"), wc);
    root.setAttribute(QStringLiteral("title"), title);
    root.setAttribute(QStringLiteral("title_icon"), QStringLiteral("system-search"));

    root.setAttribute(QStringLiteral("operationTable"), QStringLiteral("v_suboperation_consolidated"));
    root.setAttribute(QStringLiteral("currentPage"), QStringLiteral("-1"));
    SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge operation plugin")),
                                           -1,
                                           doc.toString(),
                                           i18nc("Noun, a list of items", "Sub transactions"));

    QApplication::restoreOverrideCursor();

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGSearchPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    // Start alarm
    if ((m_currentBankDocument != nullptr) && m_currentBankDocument->getMainDatabase() != nullptr) {
        QString doc_id = m_currentBankDocument->getUniqueIdentifier();
        if (m_docUniqueIdentifier != doc_id) {
            m_docUniqueIdentifier = std::move(doc_id);

            raiseAlarms();
        }
    }
}

void SKGSearchPlugin::raiseAlarms()
{
    if (m_currentBankDocument != nullptr) {
        SKGObjectBase::SKGListSKGObjectBase rules;
        SKGError err = m_currentBankDocument->getObjects(QStringLiteral("v_rule"), QStringLiteral("t_action_type='A' ORDER BY i_ORDER"), rules);
        int nb = rules.count();
        if (!err && (nb != 0)) {
            for (int i = 0; !err && i < nb; ++i) {
                SKGRuleObject rule(rules.at(i));
                err = rule.execute();
            }
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);

        m_timer.start(skgsearch_settings::alarm_frequency() * 60 * 1000);
    }
}

void SKGSearchPlugin::execute(SKGRuleObject::ProcessMode iMode)
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = SKGMainPanel::getMainPanel()->getSelectedObjects();

    int nb = rules.count();
    if (m_currentBankDocument != nullptr) {
        SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Process execution"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGRuleObject rule(rules.at(i));
            err = rule.execute(iMode);
            IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Process executed")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Process execution failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGSearchPlugin::find()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();

        QString xmlsearchcondition;
        int nb = selection.count();
        if (nb > 0) {
            QString table = selection.at(0).getRealTable();
            if (table == QStringLiteral("account")) {
                QDomDocument doc(QStringLiteral("SKGML"));
                QDomElement element = doc.createElement(QStringLiteral("element"));
                doc.appendChild(element);
                for (int i = 0; i < nb; ++i) {
                    QDomElement elementLine = doc.createElement(QStringLiteral("element"));
                    element.appendChild(elementLine);

                    QDomElement elementElement = doc.createElement(QStringLiteral("element"));
                    elementLine.appendChild(elementElement);

                    elementElement.setAttribute(QStringLiteral("attribute"), QStringLiteral("t_ACCOUNT"));
                    elementElement.setAttribute(QStringLiteral("operator"), QStringLiteral("#ATT#='#V1S#'"));
                    elementElement.setAttribute(QStringLiteral("value"), selection.at(i).getAttribute(QStringLiteral("t_name")));
                }
                xmlsearchcondition = doc.toString();
            } else if (table == QStringLiteral("category")) {
                QDomDocument doc(QStringLiteral("SKGML"));
                QDomElement element = doc.createElement(QStringLiteral("element"));
                doc.appendChild(element);
                for (int i = 0; i < nb; ++i) {
                    QDomElement elementLine = doc.createElement(QStringLiteral("element"));
                    element.appendChild(elementLine);

                    QDomElement elementElement = doc.createElement(QStringLiteral("element"));
                    elementLine.appendChild(elementElement);

                    elementElement.setAttribute(QStringLiteral("attribute"), QStringLiteral("t_REALCATEGORY"));
                    elementElement.setAttribute(QStringLiteral("operator"), QStringLiteral("#ATT#='#V1S#'"));
                    SKGCategoryObject cat(selection.at(i));
                    elementElement.setAttribute(QStringLiteral("value"), cat.getFullName());
                }
                xmlsearchcondition = doc.toString();
            } else if (table == QStringLiteral("refund")) {
                QDomDocument doc(QStringLiteral("SKGML"));
                QDomElement element = doc.createElement(QStringLiteral("element"));
                doc.appendChild(element);
                for (int i = 0; i < nb; ++i) {
                    QDomElement elementLine = doc.createElement(QStringLiteral("element"));
                    element.appendChild(elementLine);

                    QDomElement elementElement = doc.createElement(QStringLiteral("element"));
                    elementLine.appendChild(elementElement);

                    elementElement.setAttribute(QStringLiteral("attribute"), QStringLiteral("t_REALREFUND"));
                    elementElement.setAttribute(QStringLiteral("operator"), QStringLiteral("#ATT#='#V1S#'"));
                    elementElement.setAttribute(QStringLiteral("value"), selection.at(i).getAttribute(QStringLiteral("t_name")));
                }
                xmlsearchcondition = doc.toString();
            } else if (table == QStringLiteral("payee")) {
                QDomDocument doc(QStringLiteral("SKGML"));
                QDomElement element = doc.createElement(QStringLiteral("element"));
                doc.appendChild(element);
                for (int i = 0; i < nb; ++i) {
                    QDomElement elementLine = doc.createElement(QStringLiteral("element"));
                    element.appendChild(elementLine);

                    QDomElement elementElement = doc.createElement(QStringLiteral("element"));
                    elementLine.appendChild(elementElement);

                    elementElement.setAttribute(QStringLiteral("attribute"), QStringLiteral("t_PAYEE"));
                    elementElement.setAttribute(QStringLiteral("operator"), QStringLiteral("#ATT#='#V1S#'"));
                    elementElement.setAttribute(QStringLiteral("value"), selection.at(i).getAttribute(QStringLiteral("t_name")));
                }
                xmlsearchcondition = doc.toString();
            } else if (table == QStringLiteral("operation") || table == QStringLiteral("suboperation")) {
                QStringList attributeForQuery;
                attributeForQuery << QStringLiteral("d_date") << QStringLiteral("t_number") << QStringLiteral("t_mode") << QStringLiteral("t_PAYEE")
                                  << QStringLiteral("t_comment") << QStringLiteral("t_REALCATEGORY") << QStringLiteral("t_status")
                                  << QStringLiteral("t_bookmarked") << QStringLiteral("t_imported") << QStringLiteral("t_ACCOUNT")
                                  << QStringLiteral("f_REALCURRENTAMOUNT") << QStringLiteral("t_REALREFUND");

                QDomDocument doc(QStringLiteral("SKGML"));
                QDomElement element = doc.createElement(QStringLiteral("element"));
                doc.appendChild(element);
                for (int i = 0; i < nb; ++i) {
                    QDomElement elementLine = doc.createElement(QStringLiteral("element"));
                    element.appendChild(elementLine);

                    for (int j = 0; j < attributeForQuery.count(); ++j) {
                        const QString &att = attributeForQuery.at(j);
                        QString attRead =
                            (att == QStringLiteral("t_REALCATEGORY")
                                 ? QStringLiteral("t_CATEGORY")
                                 : (att == QStringLiteral("f_REALCURRENTAMOUNT") ? QStringLiteral("f_CURRENTAMOUNT")
                                                                                 : (att == QStringLiteral("t_REALREFUND") ? QStringLiteral("t_REFUND") : att)));

                        SKGObjectBase op(selection.at(i).getDocument(), QStringLiteral("v_operation_display_all"), selection.at(i).getID());
                        op.load();

                        QString val = op.getAttribute(attRead);
                        if (!val.isEmpty()) {
                            QDomElement elementElement = doc.createElement(QStringLiteral("element"));
                            elementLine.appendChild(elementElement);

                            elementElement.setAttribute(QStringLiteral("attribute"), att);
                            elementElement.setAttribute(QStringLiteral("operator"),
                                                        att.startsWith(QStringLiteral("f_")) || att.startsWith(QStringLiteral("i_"))
                                                            ? QStringLiteral("#ATT#=#V1#")
                                                            : QStringLiteral("#ATT#='#V1S#'"));
                            elementElement.setAttribute(QStringLiteral("value"), val);
                        }
                    }
                }
                xmlsearchcondition = doc.toString();
            }
        }

        // Call search plugin
        SKGMainPanel::getMainPanel()->openPage("skg://skrooge_search_plugin/?currentPage=0&xmlsearchcondition="
                                               % SKGServices::encodeForUrl(xmlsearchcondition));
    }
}

SKGTabPage *SKGSearchPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGSearchPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QWidget *SKGSearchPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    auto w = new QWidget();
    ui.setupUi(w);
    return w;
}

KConfigSkeleton *SKGSearchPlugin::getPreferenceSkeleton()
{
    return skgsearch_settings::self();
}

QString SKGSearchPlugin::title() const
{
    return i18nc("Noun", "Search and process");
}

QString SKGSearchPlugin::icon() const
{
    return QStringLiteral("system-search");
}

QString SKGSearchPlugin::toolTip() const
{
    return i18nc("Noun", "Search and process management");
}

int SKGSearchPlugin::getOrder() const
{
    return 35;
}

QStringList SKGSearchPlugin::tips() const
{
    QStringList output;
    output.push_back(
        i18nc("Description of a tips", "<p>… skrooge can <a href=\"skg://skrooge_search_plugin\">search</a> and automatically process some transactions.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can create alarms based on <a href=\"skg://skrooge_search_plugin\">searches</a>.</p>"));
    return output;
}

bool SKGSearchPlugin::isInPagesChooser() const
{
    return true;
}

SKGAdviceList SKGSearchPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;

    // Alarms
    if (!iIgnoredAdvice.contains(QStringLiteral("skgsearchplugin_alarm"))) {
        SKGObjectBase::SKGListSKGObjectBase rules;
        SKGError err = m_currentBankDocument->getObjects(QStringLiteral("v_rule"), QStringLiteral("t_action_type='A' ORDER BY i_ORDER"), rules);
        int nb = rules.count();
        if (nb != 0) {
            SKGServices::SKGUnitInfo primary = m_currentBankDocument->getPrimaryUnit();
            SKGAdvice::SKGAdviceActionList autoCorrections;
            for (int i = 0; !err && i < nb; ++i) {
                SKGRuleObject rule(rules.at(i));
                SKGRuleObject::SKGAlarmInfo alarm = rule.getAlarmInfo();
                if (alarm.Raised) {
                    double percent = 100 * alarm.Amount / alarm.Limit;
                    if (percent >= 70) {
                        SKGAdvice ad;
                        ad.setUUID("skgsearchplugin_alarm|" % SKGServices::intToString(rule.getID()));
                        ad.setPriority(percent >= 90 ? 9 : 6);

                        QString msg = alarm.Message;
                        // Build the message
                        if (alarm.Message.contains(QStringLiteral("%3"))) {
                            msg = alarm.Message.arg(m_currentBankDocument->formatMoney(alarm.Amount, primary, false),
                                                    m_currentBankDocument->formatMoney(alarm.Limit, primary, false),
                                                    m_currentBankDocument->formatMoney(alarm.Amount - alarm.Limit, primary, false));
                        } else if (alarm.Message.contains(QStringLiteral("%2"))) {
                            msg = alarm.Message.arg(m_currentBankDocument->formatMoney(alarm.Amount, primary, false),
                                                    m_currentBankDocument->formatMoney(alarm.Limit, primary, false));
                        } else if (alarm.Message.contains(QStringLiteral("%1"))) {
                            msg = alarm.Message.arg(m_currentBankDocument->formatMoney(alarm.Amount, primary, false));
                        }

                        ad.setShortMessage(msg);
                        ad.setLongMessage(i18nc("Advice on making the best (long)", "Take care to your alarms.<br> %1.", msg));
                        autoCorrections.resize(0);
                        {
                            SKGAdvice::SKGAdviceAction a;
                            a.Title = i18nc("Advice on making the best (action)", "Open transactions corresponding to this alarm");
                            a.IconName = QStringLiteral("quickopen");
                            a.IsRecommended = false;
                            autoCorrections.push_back(a);
                        }
                        ad.setAutoCorrections(autoCorrections);
                        output.push_back(ad);
                    }
                }
            }
        }
    }

    return output;
}

SKGError SKGSearchPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgsearchplugin_alarm|"))) {
        // Get parameters
        QString id = iAdviceIdentifier.right(iAdviceIdentifier.length() - 22);
        SKGSearchPluginWidget::open(SKGRuleObject(m_currentBankDocument, SKGServices::stringToInt(id)));
        return SKGError();
    }

    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

#include <skgsearchplugin.moc>
