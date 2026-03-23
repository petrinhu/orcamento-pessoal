/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to search and process transactions.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgsearchpluginwidget.h"

#include <qdir.h>
#include <qdom.h>
#include <qevent.h>

#include "skgbankincludes.h"
#include "skgcategoryobject.h"
#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgruleobject.h"
#include "skgservices.h"
#include "skgtraces.h"

SKGSearchPluginWidget::SKGSearchPluginWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGTabPage(iParent, iDocument)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);
    auto msg = i18nc("Message template",
                     "Message to display when alarm is triggered (%1 is the total amount, %2 is the alarm amount, %3 the difference)",
                     "%1",
                     "%2",
                     "%3");
    ui.kAlarmMessage->setToolTip(msg);
    ui.kAlarmMessage->setStatusTip(msg);

    ui.kView->getShowWidget()->addItem(QStringLiteral("search"),
                                       i18nc("Noun, a search", "Search"),
                                       QStringLiteral("system-search"),
                                       QStringLiteral("t_action_type='S'"),
                                       QString(),
                                       QString(),
                                       QString(),
                                       QString(),
                                       Qt::META | Qt::Key_S);
    ui.kView->getShowWidget()->addItem(QStringLiteral("update"),
                                       i18nc("Noun, a modification", "Update"),
                                       QStringLiteral("view-refresh"),
                                       QStringLiteral("t_action_type='U'"),
                                       QString(),
                                       QString(),
                                       QString(),
                                       QString(),
                                       Qt::META | Qt::Key_U);
    ui.kView->getShowWidget()->addItem(QStringLiteral("alarm"),
                                       i18nc("Noun, an alarm", "Alarm"),
                                       QStringLiteral("dialog-warning"),
                                       QStringLiteral("t_action_type='A'"),
                                       QString(),
                                       QString(),
                                       QString(),
                                       QString(),
                                       Qt::META | Qt::Key_A);
    ui.kView->getShowWidget()->addItem(QStringLiteral("template"),
                                       i18nc("Noun, a modification by applying a template", "Template"),
                                       QStringLiteral("edit-guides"),
                                       QStringLiteral("t_action_type='T'"),
                                       QString(),
                                       QString(),
                                       QString(),
                                       QString(),
                                       Qt::META | Qt::Key_T);
    ui.kView->getShowWidget()->addSeparator();
    ui.kView->getShowWidget()->addItem(QStringLiteral("highlighted"),
                                       i18nc("Adjective, an highlighted item", "Highlighted"),
                                       QStringLiteral("bookmarks"),
                                       QStringLiteral("t_bookmarked='Y'"),
                                       QString(),
                                       QString(),
                                       QString(),
                                       QString(),
                                       Qt::META | Qt::Key_H);
    ui.kView->getShowWidget()->setDefaultState(QStringLiteral("search;update;alarm;template;highlighted"));

    // Add Standard KDE Icons to buttons
    ui.kUpdate->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kAdd->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    ui.kSearch->setIcon(SKGServices::fromTheme(QStringLiteral("system-search")));
    QStringList overlayopen;
    overlayopen.push_back(QStringLiteral("quickopen"));
    ui.kOpenReport->setIcon(SKGServices::fromTheme(QStringLiteral("view-statistics"), overlayopen));

    ui.kTopBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-up-double")));
    ui.kUpBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-up")));
    ui.kDownBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-down")));
    ui.kBottomBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-down-double")));

    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.kQueryGrp);
        list.push_back(ui.kBtnFrm);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("system-search")),
                                      i18n("Search"),
                                      i18n("Display the edit panel for searches"),
                                      list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.kQueryGrp);
        list.push_back(ui.kBtnFrm);
        list.push_back(ui.kActionGrp);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("view-refresh")), i18n("Update"), i18n("Display the edit panel for updates"), list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.kQueryGrp);
        list.push_back(ui.kBtnFrm);
        list.push_back(ui.kAlarmFrm);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("dialog-warning")), i18n("Alarm"), i18n("Display the edit panel for alarm"), list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.kQueryGrp);
        list.push_back(ui.kBtnFrm);
        list.push_back(ui.kTemplateFrm);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("edit-guides")),
                                      i18n("Template"),
                                      i18n("Display the edit panel for updates by templates"),
                                      list);
    }

    QStringList attributeForQuery;
    attributeForQuery.reserve(40);
    attributeForQuery << QStringLiteral("d_DATEOP") << QStringLiteral("t_number") << QStringLiteral("t_mode") << QStringLiteral("t_PAYEE")
                      << QStringLiteral("t_comment") << QStringLiteral("t_REALCOMMENT") << QStringLiteral("t_REALCATEGORY") << QStringLiteral("t_status")
                      << QStringLiteral("t_bookmarked") << QStringLiteral("t_imported") << QStringLiteral("t_TRANSFER") << QStringLiteral("t_UNIT")
                      << QStringLiteral("t_ACCOUNT") << QStringLiteral("t_BANK") << QStringLiteral("t_TOACCOUNT") << QStringLiteral("f_REALCURRENTAMOUNT")
                      << QStringLiteral("t_REALREFUND") << QStringLiteral("f_BALANCE") << QStringLiteral("i_NBSUBOPERATIONS");
    QStringList attributeForUpdate;
    attributeForUpdate.reserve(40);
    attributeForUpdate << QStringLiteral("d_DATEOP") << QStringLiteral("t_number") << QStringLiteral("t_mode") << QStringLiteral("t_PAYEE")
                       << QStringLiteral("t_comment") << QStringLiteral("t_status") << QStringLiteral("t_bookmarked") << QStringLiteral("t_imported")
                       << QStringLiteral("t_REALCOMMENT") << QStringLiteral("t_REALCATEGORY") << QStringLiteral("t_ACCOUNT") << QStringLiteral("t_REALREFUND")
                       << QStringLiteral("t_UNIT");
    // WARNING: trigger must be modified if this list is modifier

    // Adding properties
    QStringList properties;
    iDocument->getDistinctValues(QStringLiteral("parameters"),
                                 QStringLiteral("t_name"),
                                 QStringLiteral("(t_uuid_parent like '%-operation' OR t_uuid_parent like '%-suboperation') AND t_name NOT LIKE 'SKG_%'"),
                                 properties);
    int nb = properties.count();
    for (int i = 0; i < nb; ++i) {
        attributeForQuery.push_back("p_" % properties.at(i));
        attributeForUpdate.push_back("p_" % properties.at(i));
    }

    ui.kQueryCreator->setParameters(iDocument, QStringLiteral("v_suboperation_consolidated"), attributeForQuery);
    ui.kActionCreator->setParameters(iDocument, QStringLiteral("v_suboperation_consolidated"), attributeForUpdate, true);

    // Bind transaction view
    ui.kView->setModel(new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()),
                                          QStringLiteral("v_rule_display"),
                                          QStringLiteral("1=1 ORDER BY f_sortorder"),
                                          this,
                                          QString(),
                                          false));
    ui.kView->getView()->sortByColumn(0, Qt::AscendingOrder);

    // Add registered global action in contextual menu
    if (SKGMainPanel::getMainPanel() != nullptr) {
        auto menu = new QMenu(this);
        menu->setIcon(SKGServices::fromTheme(QStringLiteral("system-run")));
        menu->addAction(SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("execute_all")));
        menu->addAction(SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("execute_imported")));
        menu->addAction(SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("execute_not_validated")));
        menu->addAction(SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("execute_notchecked")));

        ui.kApply->setIcon(menu->icon());
        ui.kApply->setMenu(menu);
        ui.kApply->setPopupMode(QToolButton::InstantPopup);
    }

    ui.kWidgetSelector->setSelectedMode(0);

    connect(ui.kView->getView(), &SKGTreeView::clickEmptyArea, this, &SKGSearchPluginWidget::cleanEditor);
    connect(ui.kView->getView(), &SKGTreeView::doubleClicked, SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(), &QAction::trigger);
    connect(ui.kView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
        this->onSelectionChanged();
    });
    connect(ui.kQueryCreator, &SKGQueryCreator::search, this, &SKGSearchPluginWidget::onOpen);

    connect(ui.kAdd, &QPushButton::clicked, this, &SKGSearchPluginWidget::onAddRule);
    connect(ui.kUpdate, &QPushButton::clicked, this, &SKGSearchPluginWidget::onModifyRule);
    connect(ui.kTopBtn, &QToolButton::clicked, this, &SKGSearchPluginWidget::onTop);
    connect(ui.kUpBtn, &QToolButton::clicked, this, &SKGSearchPluginWidget::onUp);
    connect(ui.kDownBtn, &QToolButton::clicked, this, &SKGSearchPluginWidget::onDown);
    connect(ui.kBottomBtn, &QToolButton::clicked, this, &SKGSearchPluginWidget::onBottom);
    connect(ui.kOpenReport, &QPushButton::clicked, this, &SKGSearchPluginWidget::onOpen);
    connect(ui.kSearch, &QPushButton::clicked, this, &SKGSearchPluginWidget::onOpen);

    // Refresh
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGSearchPluginWidget::dataModified, Qt::QueuedConnection);
    dataModified(QString(), 0);

    onSelectionChanged();

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);
}

SKGSearchPluginWidget::~SKGSearchPluginWidget()
{
    SKGTRACEINFUNC(1)
}

bool SKGSearchPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u && ui.kAdd->isEnabled()) {
                ui.kAdd->click();
            } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kUpdate->isEnabled()) {
                ui.kUpdate->click();
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

QString SKGSearchPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);
    root.setAttribute(QStringLiteral("currentPage"), SKGServices::intToString(ui.kWidgetSelector->getSelectedMode()));
    root.setAttribute(QStringLiteral("view"), ui.kView->getState());
    return doc.toString();
}

void SKGSearchPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString currentPage = root.attribute(QStringLiteral("currentPage"));
    QString xmlsearchcondition = root.attribute(QStringLiteral("xmlsearchcondition"));

    if (currentPage.isEmpty()) {
        currentPage = '0';
    }

    ui.kWidgetSelector->setSelectedMode(SKGServices::stringToInt(currentPage));
    ui.kQueryCreator->setXMLCondition(xmlsearchcondition);
    ui.kView->setState(root.attribute(QStringLiteral("view")));
}

QString SKGSearchPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGSEARCH_DEFAULT_PARAMETERS");
}

QWidget *SKGSearchPluginWidget::mainWidget()
{
    return ui.kView->getView();
}

SKGObjectBase::SKGListSKGObjectBase SKGSearchPluginWidget::getSelectedObjects()
{
    SKGObjectBase::SKGListSKGObjectBase list = ui.kView->getView()->getSelectedObjects();

    // Sort selection by f_sortorder. It is mandatory for reorder functions
    std::stable_sort(list.begin(), list.end());

    return list;
}

int SKGSearchPluginWidget::getNbSelectedObjects()
{
    return ui.kView->getView()->getNbSelectedObjects();
}

void SKGSearchPluginWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    SKGTRACEINFUNC(1)
    Q_UNUSED(iIdTransaction)

    // Refresh account list
    if (iTableName == QStringLiteral("unit") || iTableName.isEmpty()) {
        ui.kAlarmUnit->setText(qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit().Symbol);
    }

    if (iTableName == QStringLiteral("operation") || iTableName.isEmpty()) {
        // Fill combo boxes
        SKGStringListList result;
        getDocument()->executeSelectSqliteOrder(
            QStringLiteral("SELECT id, t_displayname FROM v_operation_displayname WHERE t_template='Y' ORDER BY t_displayname"),
            result);
        int nb2 = result.count();
        for (int i = 1; i < nb2; ++i) { // Ignore header
            const QStringList &r = result.at(i);
            ui.kTemplate->addItem(r.at(1), r.at(0));
        }
    }
}

void SKGSearchPluginWidget::onAddRule()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)
    {
        SKGRuleObject rule;
        {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Search and process creation"), err)
            rule = SKGRuleObject(getDocument());
            IFOKDO(err, rule.setXMLSearchDefinition(ui.kQueryCreator->getXMLCondition()))
            IFOKDO(err, rule.setOrder(-1))

            QString xml = getXMLActionDefinition();
            IFOKDO(err, rule.setActionType(xml.isEmpty() ? SKGRuleObject::SEARCH : SKGRuleObject::ActionType(ui.kWidgetSelector->getSelectedMode())))
            IFOKDO(err, rule.setXMLActionDefinition(xml))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(err,
                   rule.getDocument()->sendMessage(i18nc("An information to the user", "The search rule '%1' have been added", rule.getDisplayName()),
                                                   SKGDocument::Hidden))
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Search and process created"));
            ui.kView->getView()->selectObject(rule.getUniqueID());
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Search and process creation failed"));
        }
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGSearchPluginWidget::onOpen()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)
    SKGRuleObject rule;
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    rule = SKGRuleObject(getDocument());
    IFOKDO(err, rule.setXMLSearchDefinition(ui.kQueryCreator->getXMLCondition()))
    IFOKDO(err, rule.setOrder(-1))

    QString xml = getXMLActionDefinition();
    IFOKDO(err, rule.setActionType(xml.isEmpty() ? SKGRuleObject::SEARCH : SKGRuleObject::ActionType(ui.kWidgetSelector->getSelectedMode())))
    IFOKDO(err, rule.setXMLActionDefinition(xml))
    IFOK(err) open(rule, (sender() == ui.kOpenReport ? SKGSearchPluginWidget::REPORT : SKGSearchPluginWidget::TABLE));
    QApplication::restoreOverrideCursor();

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGSearchPluginWidget::onModifyRule()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)
    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Search and process update"), err)
        SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
        if (rules.count() == 1) {
            SKGRuleObject rule(rules.at(0));
            IFOKDO(err, rule.setXMLSearchDefinition(ui.kQueryCreator->getXMLCondition()))
            QString xml = getXMLActionDefinition();
            IFOKDO(err, rule.setActionType(xml.isEmpty() ? SKGRuleObject::SEARCH : SKGRuleObject::ActionType(ui.kWidgetSelector->getSelectedMode())))
            IFOKDO(err, rule.setXMLActionDefinition(xml))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(err,
                   rule.getDocument()->sendMessage(i18nc("An information to the user", "The search rule '%1' have been updated", rule.getDisplayName()),
                                                   SKGDocument::Hidden))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Search and process updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Search and process update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kView->getView()->setFocus();
}

void SKGSearchPluginWidget::onSelectionChanged()
{
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();
    int nbSel = selection.count();

    ui.kTopBtn->setEnabled(nbSel > 0);
    ui.kUpBtn->setEnabled(nbSel > 0);
    ui.kDownBtn->setEnabled(nbSel > 0);
    ui.kBottomBtn->setEnabled(nbSel > 0);
    ui.kUpdate->setEnabled(nbSel == 1);
    ui.kApply->setEnabled(nbSel > 0);

    if (nbSel > 0) {
        SKGRuleObject rule(selection.at(0));
        ui.kQueryCreator->setXMLCondition(rule.getXMLSearchDefinition());

        int index = qMax(0, static_cast<int>(rule.getActionType()));
        if (ui.kWidgetSelector->getSelectedMode() != -1) {
            ui.kWidgetSelector->setSelectedMode(index);
        }
        if (index == 1) {
            // Set update mode
            ui.kActionCreator->setXMLCondition(rule.getXMLActionDefinition());
        } else if (index == 2) {
            // Set alarm mode
            QDomDocument doc(QStringLiteral("SKGML"));
            doc.setContent(rule.getXMLActionDefinition());

            QDomElement element = doc.documentElement();
            QDomElement elementLine = element.firstChild().toElement();
            QDomElement elementElement = elementLine.firstChild().toElement();
            ui.kAlarmAmount->setValue(SKGServices::stringToDouble(elementElement.attribute(QStringLiteral("value"))));
            ui.kAlarmMessage->setText(elementElement.attribute(QStringLiteral("value2")));
        } else if (index == 3) {
            // Set template mode
            QDomDocument doc(QStringLiteral("SKGML"));
            doc.setContent(rule.getXMLActionDefinition());

            QDomElement element = doc.documentElement();
            QDomElement elementLine = element.firstChild().toElement();
            QDomElement elementElement = elementLine.firstChild().toElement();
            ui.kTemplate->setCurrentIndex(ui.kTemplate->findData(elementElement.attribute(QStringLiteral("value"))));
        }
    }

    onEditorModified();

    Q_EMIT selectionChanged();
}

void SKGSearchPluginWidget::onTop()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Search update"), err, nb)
        for (int i = nb - 1; !err && i >= 0; --i) {
            SKGRuleObject rule(rules.at(i));

            double order = 1;
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT min(f_sortorder) from rule"), result);
            if (!err && result.count() == 2) {
                order = SKGServices::stringToDouble(result.at(1).at(0)) - 1;
            }

            IFOKDO(err, rule.setOrder(order))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(
                err,
                getDocument()->sendMessage(i18nc("An information to the user", "The search '%1' has been updated", rule.getDisplayName()), SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Search updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Search update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGSearchPluginWidget::onUp()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Search update"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGRuleObject rule(rules.at(i));

            double order = rule.getOrder();
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder("SELECT f_sortorder from rule where f_sortorder<" % SKGServices::doubleToString(order)
                                                              % " ORDER BY f_sortorder DESC",
                                                          result);
            IFOK(err)
            {
                if (result.count() == 2) {
                    order = SKGServices::stringToDouble(result.at(1).at(0)) - 1;
                } else if (result.count() >= 2) {
                    order = (SKGServices::stringToDouble(result.at(1).at(0)) + SKGServices::stringToDouble(result.at(2).at(0))) / 2;
                }
            }

            IFOKDO(err, rule.setOrder(order))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(
                err,
                getDocument()->sendMessage(i18nc("An information to the user", "The search '%1' has been updated", rule.getDisplayName()), SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Search updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Search update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGSearchPluginWidget::onDown()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Search update"), err, nb)
        for (int i = nb - 1; !err && i >= 0; --i) {
            SKGRuleObject rule(rules.at(i));

            double order = rule.getOrder();
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder("SELECT f_sortorder from rule where f_sortorder>" % SKGServices::doubleToString(order)
                                                              % " ORDER BY f_sortorder ASC",
                                                          result);
            IFOK(err)
            {
                if (result.count() == 2) {
                    order = SKGServices::stringToDouble(result.at(1).at(0)) + 1;
                } else if (result.count() >= 2) {
                    order = (SKGServices::stringToDouble(result.at(1).at(0)) + SKGServices::stringToDouble(result.at(2).at(0))) / 2;
                }
            }

            IFOKDO(err, rule.setOrder(order))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(
                err,
                getDocument()->sendMessage(i18nc("An information to the user", "The search '%1' has been updated", rule.getDisplayName()), SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Search updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Search update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGSearchPluginWidget::onBottom()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Search update"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGRuleObject rule(rules.at(i));

            double order = 1;
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT max(f_sortorder) from rule"), result);
            if (!err && result.count() == 2) {
                order = SKGServices::stringToDouble(result.at(1).at(0)) + 1;
            }

            IFOKDO(err, rule.setOrder(order))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(
                err,
                getDocument()->sendMessage(i18nc("An information to the user", "The search '%1' has been updated", rule.getDisplayName()), SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Search updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Search update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGSearchPluginWidget::open(const SKGRuleObject &iRule, OpenMode iMode)
{
    _SKGTRACEINFUNC(10)

    // Build where clause and title
    QString wc = "i_SUBOPID in (SELECT i_SUBOPID FROM v_operation_prop WHERE " % iRule.getSelectSqlOrder() % u')';
    QString title = i18nc("Noun, a list of items", "Sub transactions corresponding to rule '%1'", iRule.getSearchDescription());

    // Call transaction plugin
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGMainPanel::getMainPanel()->getDocument()->getParameter(iMode == TABLE ? QStringLiteral("SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS")
                                                                                            : QStringLiteral("SKGREPORT_DEFAULT_PARAMETERS")));
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement(QStringLiteral("parameters"));
        doc.appendChild(root);
    }

    root.setAttribute(QStringLiteral("operationWhereClause"), wc);
    root.setAttribute(QStringLiteral("title"), title);
    root.setAttribute(QStringLiteral("title_icon"), QStringLiteral("system-search"));

    if (iMode == TABLE) {
        root.setAttribute(QStringLiteral("operationTable"), QStringLiteral("v_suboperation_consolidated"));
        root.setAttribute(QStringLiteral("currentPage"), QStringLiteral("-1"));
        SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge operation plugin")),
                                               -1,
                                               doc.toString(),
                                               i18nc("Noun, a list of items", "Sub transactions"));
    } else {
        root.setAttribute(QStringLiteral("period"), QStringLiteral("0"));
        SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge report plugin")), -1, doc.toString());
    }
}

void SKGSearchPluginWidget::onEditorModified()
{
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();
    int nbSelect = selection.count();
    ui.kUpdate->setEnabled(nbSelect == 1);
    ui.kQueryInfo->setText(QString());

    if (nbSelect == 1) {
        SKGRuleObject rule(selection.at(0));

        // Build where clause and title
        QString wc = rule.getSelectSqlOrder();

        SKGStringListList result;
        int vAll = 0;
        getDocument()->executeSelectSqliteOrder("SELECT count(distinct(id)) from v_operation_prop WHERE " % wc, result);
        if (result.count() == 2) {
            vAll = SKGServices::stringToInt(result.at(1).at(0));
        }

        int vNotChecked = 0;
        getDocument()->executeSelectSqliteOrder("SELECT count(distinct(id)) from v_operation_prop WHERE t_status!='Y' AND " % wc, result);
        if (result.count() == 2) {
            vNotChecked = SKGServices::stringToInt(result.at(1).at(0));
        }

        int vImported = 0;
        getDocument()->executeSelectSqliteOrder("SELECT count(distinct(id)) from v_operation_prop WHERE t_imported!='N' AND " % wc, result);
        if (result.count() == 2) {
            vImported = SKGServices::stringToInt(result.at(1).at(0));
        }

        int vNotValidatedl = 0;
        getDocument()->executeSelectSqliteOrder("SELECT count(distinct(id)) from v_operation_prop WHERE t_imported='P' AND " % wc, result);
        if (result.count() == 2) {
            vNotValidatedl = SKGServices::stringToInt(result.at(1).at(0));
        }

        ui.kQueryInfo->setText(i18np("%1 transaction found (%2 imported, %3 not yet validated, %4 not checked).",
                                     "%1 transactions found (%2 imported, %3 not yet validated, %4 not checked).",
                                     vAll,
                                     vImported,
                                     vNotValidatedl,
                                     vNotChecked));
    }
}

void SKGSearchPluginWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0) {
        ui.kQueryCreator->clearContents();
        ui.kActionCreator->clearContents();
    }
}

QString SKGSearchPluginWidget::getXMLActionDefinition()
{
    QString output;
    if (ui.kWidgetSelector->getSelectedMode() == 1) {
        // Mode update
        output = ui.kActionCreator->getXMLCondition();
    } else if (ui.kWidgetSelector->getSelectedMode() == 2) {
        // Mode alarm
        QDomDocument doc(QStringLiteral("SKGML"));
        QDomElement element = doc.createElement(QStringLiteral("element"));
        doc.appendChild(element);

        QDomElement elementLine = doc.createElement(QStringLiteral("element"));
        element.appendChild(elementLine);

        QDomElement elementElement = doc.createElement(QStringLiteral("element"));
        elementLine.appendChild(elementElement);

        elementElement.setAttribute(QStringLiteral("attribute"), QStringLiteral("f_REALCURRENTAMOUNT"));
        elementElement.setAttribute(QStringLiteral("operator"), QStringLiteral("ABS(TOTAL(#ATT#))#OP##V1#,ABS(TOTAL(#ATT#)), #V1#, '#V2S#'"));
        elementElement.setAttribute(QStringLiteral("operator2"), QStringLiteral(">="));
        elementElement.setAttribute(QStringLiteral("value"), SKGServices::doubleToString(ui.kAlarmAmount->value()));
        elementElement.setAttribute(QStringLiteral("value2"), ui.kAlarmMessage->text());

        output = doc.toString();
    } else if (ui.kWidgetSelector->getSelectedMode() == 3) {
        // Mode template
        QDomDocument doc(QStringLiteral("SKGML"));
        QDomElement element = doc.createElement(QStringLiteral("element"));
        doc.appendChild(element);

        QDomElement elementLine = doc.createElement(QStringLiteral("element"));
        element.appendChild(elementLine);

        QDomElement elementElement = doc.createElement(QStringLiteral("element"));
        elementLine.appendChild(elementElement);

        elementElement.setAttribute(QStringLiteral("attribute"), QStringLiteral("id"));
        elementElement.setAttribute(QStringLiteral("operator"), QStringLiteral("APPLYTEMPLATE(#V1#)"));
        elementElement.setAttribute(QStringLiteral("value"), ui.kTemplate->itemData(ui.kTemplate->currentIndex()).toString());
        elementElement.setAttribute(QStringLiteral("value2"), ui.kTemplate->currentText());

        output = doc.toString();
    }
    return output;
}

bool SKGSearchPluginWidget::isEditor()
{
    return true;
}

void SKGSearchPluginWidget::activateEditor()
{
    if (ui.kWidgetSelector->getSelectedMode() == -1) {
        ui.kWidgetSelector->setSelectedMode(0);
    }
}
