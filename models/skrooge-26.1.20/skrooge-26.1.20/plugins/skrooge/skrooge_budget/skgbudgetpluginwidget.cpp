/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to manage budgets.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgbudgetpluginwidget.h"

#include <qdom.h>
#include <qevent.h>

#include "skgbudgetdelegate.h"
#include "skgbudgetobject.h"
#include "skgbudgetruleobject.h"
#include "skgcategoryobject.h"
#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGBudgetPluginWidget::SKGBudgetPluginWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGTabPage(iParent, iDocument)
    , m_objectModel(nullptr)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    ui.kUseScheduledOperation->hide();

    ui.kTopBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-up-double")));
    ui.kUpBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-up")));
    ui.kDownBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-down")));
    ui.kBottomBtn->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-down-double")));

    ui.kPeriodLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_period"))));
    ui.kYearLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("i_year"))));
    ui.kMonthLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("i_month"))));
    ui.kAmountLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("f_value"))));
    ui.kCategoryLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_category"))));
    ui.kYearCheck->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("i_year"))));
    ui.kMonthCheck->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("i_month"))));
    ui.kCategoryCheck->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_category"))));
    ui.kPeriodLbl2->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_period"))));
    ui.kAmountLabel2->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("f_value"))));
    ui.kCategoryTransferCheck->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_category"))));

    ui.kYear->setValue(QDate::currentDate().year());
    ui.kYearAutoBase->setValue(QDate::currentDate().year());
    ui.kMonth->setValue(QDate::currentDate().month());
    ui.kPeriod->addItem(i18nc("Noun, how to define a budget period", "Monthly"));
    ui.kPeriod->addItem(i18nc("Noun, how to define a budget period", "Yearly"));
    ui.kPeriod->addItem(i18nc("Noun, how to define a budget period", "Individual"));

    ui.kView->getShowWidget()->addItem(QStringLiteral("all"),
                                       i18nc("Noun, budget items to display", "All"),
                                       QString(),
                                       QString(),
                                       QStringLiteral("current;currentYear;currentMonth;previousYear;previousMonth"),
                                       QStringLiteral("none"),
                                       QString(),
                                       QString(),
                                       Qt::META | Qt::Key_A);
    ui.kView->getShowWidget()->addItem(QStringLiteral("none"),
                                       i18nc("Noun, budget items to display", "None"),
                                       QString(),
                                       QStringLiteral("1=0"),
                                       QString(), // Check when checked
                                       QStringLiteral("all;current;currentYear;currentMonth;previousYear;previousMonth"), // Uncheck when checked
                                       QString(), // Check when unchecked
                                       QStringLiteral("all"), // Uncheck when unchecked
                                       Qt::META | Qt::Key_0);
    ui.kView->getShowWidget()->addSeparator();
    ui.kView->getShowWidget()->addItem(
        QStringLiteral("current"),
        i18nc("Noun, budget items to display", "Current"),
        QStringLiteral("view-calendar-whatsnext"),
        QStringLiteral("t_PERIOD>=STRFTIME('%Y-%m', date('now', 'localtime')) OR t_PERIOD=STRFTIME('%Y', date('now', 'localtime'))"),
        QStringLiteral("currentMonth"), // Check when checked
        QStringLiteral("none"), // Uncheck when checked
        QString(), // Check when unchecked
        QStringLiteral("all"), // Uncheck when unchecked
        Qt::META | Qt::Key_1);
    ui.kView->getShowWidget()->addSeparator();
    ui.kView->getShowWidget()->addItem(QStringLiteral("currentYear"),
                                       i18nc("Noun, budget items to display", "Current year"),
                                       QStringLiteral("view-calendar-month"),
                                       QStringLiteral("t_PERIOD LIKE STRFTIME('%Y', date('now', 'localtime'))||'%'"),
                                       QString(),
                                       QStringLiteral("none"),
                                       QString(),
                                       QStringLiteral("all"),
                                       Qt::META | Qt::Key_3);
    ui.kView->getShowWidget()->addItem(QStringLiteral("currentMonth"),
                                       i18nc("Noun, budget items to display", "Current month"),
                                       QStringLiteral("view-calendar-week"),
                                       QStringLiteral("t_PERIOD=STRFTIME('%Y-%m', date('now', 'localtime'))"),
                                       QString(),
                                       QStringLiteral("none"),
                                       QString(),
                                       QStringLiteral("all;current"),
                                       Qt::META | Qt::Key_2);
    ui.kView->getShowWidget()->addSeparator();
    ui.kView->getShowWidget()->addItem(QStringLiteral("previousYear"),
                                       i18nc("Noun, budget items to display", "Previous year"),
                                       QStringLiteral("view-calendar-month"),
                                       QStringLiteral("t_PERIOD LIKE STRFTIME('%Y', date('now', 'localtime','start of year','-1 day'))||'%'"),
                                       QString(),
                                       QStringLiteral("none"),
                                       QString(),
                                       QStringLiteral("all"),
                                       Qt::META | Qt::Key_5);
    ui.kView->getShowWidget()->addItem(QStringLiteral("previousMonth"),
                                       i18nc("Noun, budget items to display", "Previous month"),
                                       QStringLiteral("view-calendar-week"),
                                       QStringLiteral("t_PERIOD=STRFTIME('%Y-%m', date('now', 'localtime','start of month','-1 day'))"),
                                       QString(),
                                       QStringLiteral("none"),
                                       QString(),
                                       QStringLiteral("all"),
                                       Qt::META | Qt::Key_4);
    ui.kView->getShowWidget()->setDefaultState(QStringLiteral("current;currentMonth"));

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGBudgetPluginWidget::refreshInfoZone, Qt::QueuedConnection);

    ui.kConditionCmb->addItem(i18nc("Noun, condition item to a apply a transfer of budget", "All"), static_cast<int>(SKGBudgetRuleObject::ALL));
    ui.kConditionCmb->addItem(i18nc("Noun, condition item to a apply a transfer of budget", "Negative"), static_cast<int>(SKGBudgetRuleObject::NEGATIVE));
    ui.kConditionCmb->addItem(i18nc("Noun, condition item to a apply a transfer of budget", "Positive"), static_cast<int>(SKGBudgetRuleObject::POSITIVE));

    ui.kModeCmb->addItem(i18nc("Noun, mode item to a apply a transfer of budget", "Next"), static_cast<int>(SKGBudgetRuleObject::NEXT));
    ui.kModeCmb->addItem(i18nc("Noun, mode item to a apply a transfer of budget", "Current"), static_cast<int>(SKGBudgetRuleObject::CURRENT));
    ui.kModeCmb->addItem(i18nc("Noun, mode item to a apply a transfer of budget", "Current year"), static_cast<int>(SKGBudgetRuleObject::YEAR));

    ui.kView->getView()->setItemDelegate(new SKGBudgetDelegate(ui.kView->getView(), getDocument()));

    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    if (doc != nullptr) {
        ui.kUnitCmb->addItem(QStringLiteral("%"));
        ui.kUnitCmb->addItem(doc->getPrimaryUnit().Symbol);
        ui.kUnit->setText(doc->getPrimaryUnit().Symbol);

        // Bind transaction view
        m_objectModel = new SKGObjectModel(doc, QStringLiteral("v_budget_display"), QStringLiteral("1=0"), this, QString(), false);
        ui.kView->setModel(m_objectModel);
        ui.kSortButton->setVisible(false);
    }

    connect(ui.kView->getView(), &SKGTreeView::doubleClicked, SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(), &QAction::trigger);
    connect(ui.kView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
        this->onSelectionChanged();
    });

    // Add Standard KDE Icons to buttons to Transactions
    ui.kModifyBtn->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kAddBtn->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    QAction *processAction = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("tool_process_budget_rules"));
    if (processAction != nullptr) {
        ui.kProcessBtn->setIcon(processAction->icon());
        connect(ui.kProcessBtn, &QPushButton::clicked, processAction, &QAction::trigger);
    }

    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.SKGManualSection);
        list.push_back(ui.SKGEditionButtonsSection);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("user-properties")),
                                      i18n("Manual"),
                                      i18n("Display the edit panel for standard budget"),
                                      list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.SKGAutoSection);
        list.push_back(ui.SKGEditionButtonsSection);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("games-solve")),
                                      i18n("Auto"),
                                      i18n("Display the edit panel for automatic budgets"),
                                      list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.SKGRuleSection);
        list.push_back(ui.SKGEditionButtonsSection);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("run-build")), i18n("Rules"), i18n("Display the edit panel for rules"), list);
    }
    connect(ui.kWidgetSelector, &SKGWidgetSelector::selectedModeChanged, this, &SKGBudgetPluginWidget::onBtnModeClicked, Qt::QueuedConnection);
    connect(ui.kAddBtn, &QPushButton::clicked, this, &SKGBudgetPluginWidget::onAddClicked);
    connect(ui.kModifyBtn, &QPushButton::clicked, this, &SKGBudgetPluginWidget::onUpdateClicked);
    connect(ui.kAmountEdit, &SKGCalculatorEdit::textChanged, this, &SKGBudgetPluginWidget::onCreatorModified);
    connect(ui.kPeriod, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged), this, &SKGBudgetPluginWidget::onCreatorModified);
    connect(ui.kYearCheck, &QCheckBox::toggled, ui.kYearRule, &QSpinBox::setEnabled);
    connect(ui.kMonthCheck, &QCheckBox::toggled, ui.kMonthRule, &QSpinBox::setEnabled);
    connect(ui.kCategoryCheck, &QCheckBox::toggled, ui.kCategoryRule, &SKGComboBox::setEnabled);
    connect(ui.kCategoryTransferCheck, &QCheckBox::toggled, ui.kCategoryTransfer, &SKGComboBox::setEnabled);
    connect(ui.kModeCmb, static_cast<void (SKGComboBox::*)(int)>(&SKGComboBox::currentIndexChanged), this, &SKGBudgetPluginWidget::onCreatorModified);
    connect(ui.kAutoBudgetCheck, &QCheckBox::toggled, ui.kYearAutoBase, &QSpinBox::setEnabled);
    connect(ui.kAutoBudgetCheck, &QCheckBox::toggled, ui.kRemovePrevious, &QCheckBox::setEnabled);
    connect(ui.kAutoBudgetCheck, &QCheckBox::toggled, ui.kUseScheduledOperation, &QCheckBox::setEnabled);

    connect(ui.kTopBtn, &QToolButton::clicked, this, &SKGBudgetPluginWidget::onTop);
    connect(ui.kUpBtn, &QToolButton::clicked, this, &SKGBudgetPluginWidget::onUp);
    connect(ui.kDownBtn, &QToolButton::clicked, this, &SKGBudgetPluginWidget::onDown);
    connect(ui.kBottomBtn, &QToolButton::clicked, this, &SKGBudgetPluginWidget::onBottom);
    connect(ui.kYearAuto, &QSpinBox::textChanged, this, [this](const QString &text) {
        ui.kRemovePrevious->setText(i18nc("Option", "Remove existing budgets for %1", text));
    });
    ui.kYearAuto->setValue(QDate::currentDate().year());

    ui.kWidgetSelector->setSelectedMode(0);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    // Refresh
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGBudgetPluginWidget::dataModified, Qt::QueuedConnection);
    dataModified(QString(), 0);
}

SKGBudgetPluginWidget::~SKGBudgetPluginWidget()
{
    SKGTRACEINFUNC(1)
    m_objectModel = nullptr;
}

bool SKGBudgetPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u && ui.kAddBtn->isEnabled()) {
                ui.kAddBtn->click();
            } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kModifyBtn->isEnabled()) {
                ui.kModifyBtn->click();
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

QString SKGBudgetPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("currentPage"), SKGServices::intToString(ui.kWidgetSelector->getSelectedMode()));
    if ((m_objectModel != nullptr) && m_objectModel->getRealTable() == QStringLiteral("budget")) {
        root.setAttribute(QStringLiteral("view"), ui.kView->getState());
        root.setAttribute(QStringLiteral("viewRule"), m_viewRule);
    } else {
        root.setAttribute(QStringLiteral("view"), m_viewBudget);
        root.setAttribute(QStringLiteral("viewRule"), ui.kView->getState());
    }

    return doc.toString();
}

void SKGBudgetPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString currentPage = root.attribute(QStringLiteral("currentPage"));
    if (currentPage.isEmpty()) {
        currentPage = '0';
    }

    ui.kWidgetSelector->setSelectedMode(SKGServices::stringToInt(currentPage));

    m_viewBudget = root.attribute(QStringLiteral("view"));
    m_viewRule = root.attribute(QStringLiteral("viewRule"));
    if ((m_objectModel != nullptr) && m_objectModel->getRealTable() == QStringLiteral("budget")) {
        ui.kView->setState(m_viewBudget);
    } else {
        ui.kView->setState(m_viewRule);
    }
}

QString SKGBudgetPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGBUDGET_DEFAULT_PARAMETERS");
}

QWidget *SKGBudgetPluginWidget::mainWidget()
{
    return ui.kView->getView();
}

void SKGBudgetPluginWidget::refresh()
{
    SKGTRACEINFUNC(1)

    QSqlDatabase *db = getDocument()->getMainDatabase();
    setEnabled(db != nullptr);
    if (db != nullptr) {
        // Refresh yours widgets here
    }
}

void SKGBudgetPluginWidget::dataModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    // Refresh widgets
    if (iTableName == QStringLiteral("budget") || iTableName.isEmpty()) {
        // Refresh info area
        m_timer.start(300);
    }

    if (!iLightTransaction) {
        if (iTableName == QStringLiteral("category") || iTableName.isEmpty()) {
            // Set type category
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kCategoryEdit << ui.kCategoryRule << ui.kCategoryTransfer,
                                                getDocument(),
                                                QStringLiteral("category"),
                                                QStringLiteral("t_fullname"),
                                                QString());
        }
    }
}

void SKGBudgetPluginWidget::onBtnModeClicked(int mode)
{
    SKGTRACEINFUNC(10)
    if (m_objectModel == nullptr) {
        return;
    }

    if (mode == 2 && m_objectModel->getTable() != QStringLiteral("v_budgetrule_display")) {
        ui.kView->getShowWidget()->setEnabled(false);
        m_viewBudget = ui.kView->getState();
        m_objectModel->setFilter(QString());
        m_objectModel->setTable(QStringLiteral("v_budgetrule_display"));
        ui.kSortButton->setVisible(true);
        ui.kView->setState(m_viewRule);
    } else if (mode != 2 && m_objectModel->getTable() != QStringLiteral("v_budget_display")) {
        ui.kView->getShowWidget()->setEnabled(true);
        m_viewRule = ui.kView->getState();
        m_objectModel->setTable(QStringLiteral("v_budget_display"));
        ui.kSortButton->setVisible(false);
        ui.kView->setState(m_viewBudget);
    }

    onCreatorModified();
}

void SKGBudgetPluginWidget::onAddClicked()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)

    if (ui.kWidgetSelector->getSelectedMode() == 2) {
        // Creation of a rule
        QStringList uniqueIDs;
        {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget rule creation"), err)
            SKGBudgetRuleObject budgetRule(getDocument());
            IFOKDO(err, updateBudgetRule(budgetRule))
            IFOKDO(err, budgetRule.setOrder(-1))
            IFOKDO(err, budgetRule.save())
            uniqueIDs.push_back(budgetRule.getUniqueID());

            // Send message
            IFOKDO(
                err,
                budgetRule.getDocument()->sendMessage(i18nc("An information to the user", "The budget rule '%1' has been added", budgetRule.getDisplayName()),
                                                      SKGDocument::Hidden))
        }
        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Budget rule created"));
            ui.kView->getView()->selectObjects(uniqueIDs);
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Budget rule creation failed"));
        }
    } else {
        // Creation of a budget
        QStringList uniqueIDs;
        {
            SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget creation"), err, 2)
            if (ui.kWidgetSelector->getSelectedMode() == 0) {
                // Manual creation
                int mode = ui.kPeriod->currentIndex();
                if (mode == 0) { // Monthly
                    for (int m = 1; !err && m <= 12; ++m) {
                        SKGBudgetObject budget(getDocument());
                        IFOKDO(err, updateBudget(budget, m))

                        // Send message
                        IFOKDO(err,
                               getDocument()->sendMessage(i18nc("An information to the user", "The budget '%1' has been added", budget.getDisplayName()),
                                                          SKGDocument::Hidden))

                        uniqueIDs.push_back(budget.getUniqueID());
                    }
                } else if (mode == 1) { // Yearly
                    SKGBudgetObject budget(getDocument());

                    IFOKDO(err, updateBudget(budget, 0))

                    // Send message
                    IFOKDO(err,
                           getDocument()->sendMessage(i18nc("An information to the user", "The budget '%1' has been added", budget.getDisplayName()),
                                                      SKGDocument::Hidden))

                    uniqueIDs.push_back(budget.getUniqueID());
                } else { // Individual
                    SKGBudgetObject budget(getDocument());
                    IFOKDO(err, updateBudget(budget))

                    // Send message
                    IFOKDO(err,
                           getDocument()->sendMessage(i18nc("An information to the user", "The budget '%1' has been added", budget.getDisplayName()),
                                                      SKGDocument::Hidden))

                    uniqueIDs.push_back(budget.getUniqueID());
                }
            } else {
                // Automatic creation
                if (ui.kAutoBudgetCheck->isChecked()) {
                    err = SKGBudgetObject::createAutomaticBudget(qobject_cast<SKGDocumentBank *>(getDocument()),
                                                                 ui.kYearAuto->value(),
                                                                 ui.kYearAutoBase->value(),
                                                                 ui.kUseScheduledOperation->isChecked(),
                                                                 ui.kRemovePrevious->isChecked());
                }
                IFOKDO(err, getDocument()->stepForward(1))

                IFOKDO(err,
                       SKGBudgetObject::balanceBudget(qobject_cast<SKGDocumentBank *>(getDocument()),
                                                      ui.kYearAuto->value(),
                                                      (ui.kBalancingMonthly->isChecked() ? 0 : -1),
                                                      ui.kBalancingAnnual->isChecked()));
                IFOKDO(err, getDocument()->stepForward(2))
            }
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Budget created"));
            ui.kView->getView()->selectObjects(uniqueIDs);
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Budget creation failed"));
        }
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGBudgetPluginWidget::onUpdateClicked()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();
    if (ui.kWidgetSelector->getSelectedMode() == 2) {
        {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget rule update"), err)
            SKGBudgetRuleObject rule(selection.at(0));
            IFOKDO(err, updateBudgetRule(rule))

            // Send message
            IFOKDO(err,
                   rule.getDocument()->sendMessage(i18nc("An information to the user", "The budget rule '%1' has been updated", rule.getDisplayName()),
                                                   SKGDocument::Hidden))
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Budget rule updated"));
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Budget rule update failed"));
        }
    } else {
        {
            int nb = selection.count();
            SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget update"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGBudgetObject budget(selection.at(i));
                int mode = ui.kPeriod->currentIndex();
                if (mode == 1) { // Yearly
                    err = updateBudget(budget, 0);
                } else { // Individual
                    err = updateBudget(budget);
                }

                IFOKDO(err, getDocument()->stepForward(i + 1))
            }
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Budget updated"));
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Budget update failed"));
        }
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kView->getView()->setFocus();
}

SKGError SKGBudgetPluginWidget::updateBudget(SKGBudgetObject &iBudget, int iMonth)
{
    SKGError err;
    if (!err && ui.kYear->isEnabled()) {
        err = iBudget.setYear(ui.kYear->value());
    }
    if (!err && ui.kMonth->isEnabled()) {
        err = iBudget.setMonth(iMonth != -1 ? iMonth : ui.kMonth->value());
    }

    SKGCategoryObject cat;
    QString catName = ui.kCategoryEdit->text().trimmed();
    IFOKDO(err, SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), catName, cat, true))
    IFOKDO(err, iBudget.setCategory(cat))
    IFOKDO(err, iBudget.enableSubCategoriesInclusion(ui.kIncludingSubCategories->isChecked()))

    double val = ui.kAmountEdit->value();
    // Is the sign forced ?
    if (ui.kAmountEdit->sign() == 0) {
        // No
        SKGObjectBase cat2(cat.getDocument(), QStringLiteral("v_category_display"), cat.getID());

        // Are we able to find to sign with the category ?
        if (cat2.getAttribute(QStringLiteral("t_TYPEEXPENSE")) == QStringLiteral("-")) {
            val = -val;
        }
    }
    IFOKDO(err, iBudget.setBudgetedAmount(val))

    IFOKDO(err, iBudget.save())
    return err;
}

SKGError SKGBudgetPluginWidget::updateBudgetRule(SKGBudgetRuleObject &iRule)
{
    SKGError err;
    SKGCategoryObject cat;
    QString catName = ui.kCategoryRule->text().trimmed();
    IFOKDO(err, SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), catName, cat, true))
    SKGCategoryObject catchange;
    QString catchangeName = ui.kCategoryTransfer->text().trimmed();
    IFOKDO(err, SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), catchangeName, catchange, true))
    IFOKDO(err, iRule.enableCategoryCondition(ui.kCategoryCheck->isChecked()))
    IFOKDO(err, iRule.setBudgetCategory(cat))
    IFOKDO(err, iRule.enableYearCondition(ui.kYearCheck->isChecked()))
    IFOKDO(err, iRule.setBudgetYear(ui.kYearRule->value()))
    IFOKDO(err, iRule.enableMonthCondition(ui.kMonthCheck->isChecked()))
    IFOKDO(err, iRule.setBudgetMonth(ui.kMonthRule->value()))
    IFOK(err)
    {
        bool absolute = (ui.kUnitCmb->currentIndex() == 1);
        double val = ui.kAmountEdit2->value();
        if (!absolute) {
            val = qMin(qMax(static_cast<double>(0), val), static_cast<double>(100));
        }
        err = iRule.setQuantity(val, absolute);
    }
    IFOKDO(err, iRule.setCondition(static_cast<SKGBudgetRuleObject::Condition>(ui.kConditionCmb->itemData(ui.kConditionCmb->currentIndex()).toInt())))
    IFOKDO(err, iRule.enableCategoryChange(ui.kCategoryTransferCheck->isChecked()))
    IFOKDO(err, iRule.setTransfer(static_cast<SKGBudgetRuleObject::Mode>(ui.kModeCmb->itemData(ui.kModeCmb->currentIndex()).toInt()), catchange))
    IFOKDO(err, iRule.save())
    return err;
}

void SKGBudgetPluginWidget::onCreatorModified()
{
    bool test = !ui.kAmountEdit->text().isEmpty() && !ui.kYear->text().isEmpty();
    ui.kAddBtn->setEnabled(test || ui.kWidgetSelector->getSelectedMode() != 0);
    ui.kModifyBtn->setEnabled((test && ui.kPeriod->currentIndex() != 0 && ui.kWidgetSelector->getSelectedMode() == 0 && (getNbSelectedObjects() != 0))
                              || (ui.kWidgetSelector->getSelectedMode() == 2 && getNbSelectedObjects() == 1));

    bool monthCondition = (ui.kPeriod->currentIndex() == 2 || ui.kWidgetSelector->getSelectedMode() == 2);
    ui.kMonthLbl->setVisible(monthCondition);
    ui.kMonth->setVisible(monthCondition);
}

void SKGBudgetPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    if (m_objectModel == nullptr) {
        return;
    }

    SKGObjectBase::SKGListSKGObjectBase objs = getSelectedObjects();
    int nb = objs.count();
    int mode = ui.kWidgetSelector->getSelectedMode();
    if (nb != 0) {
        if (m_objectModel->getRealTable() == QStringLiteral("budget")) {
            SKGBudgetObject budget(objs.at(0));
            ui.kYear->setValue(budget.getYear());
            ui.kMonth->setValue(budget.getMonth());
            ui.kAmountEdit->setValue(budget.getBudgetedAmount());
            ui.kCategoryEdit->setText(budget.getAttribute(QStringLiteral("t_CATEGORY")));
            ui.kPeriod->setCurrentIndex(budget.getMonth() == 0 ? 1 : 2); // Set yearly or individual
            ui.kIncludingSubCategories->setChecked(budget.isSubCategoriesInclusionEnabled());

            if (mode > 0) {
                ui.kWidgetSelector->setSelectedMode(0);
            }
        } else {
            SKGBudgetRuleObject rule(objs.at(0));
            ui.kYearCheck->setChecked(rule.isYearConditionEnabled());
            ui.kYearRule->setValue(rule.getBudgetYear());
            ui.kMonthCheck->setChecked(rule.isMonthConditionEnabled());
            ui.kMonthRule->setValue(rule.getBudgetMonth());
            ui.kCategoryCheck->setChecked(rule.isCategoryConditionEnabled());
            ui.kCategoryRule->setText(rule.getAttribute(QStringLiteral("t_CATEGORYCONDITION")));
            ui.kCategoryTransferCheck->setChecked(rule.isCategoryChangeEnabled());
            ui.kCategoryTransfer->setText(rule.getAttribute(QStringLiteral("t_CATEGORY")));
            ui.kUnitCmb->setCurrentIndex(rule.isAbolute() ? 1 : 0);
            ui.kAmountEdit2->setValue(rule.getQuantity());
            ui.kModeCmb->setCurrentIndex(ui.kModeCmb->findData(static_cast<int>(rule.getTransferMode())));
            ui.kConditionCmb->setCurrentIndex(ui.kConditionCmb->findData(static_cast<int>(rule.getCondition())));
        }
    }

    ui.kPeriod->setEnabled(nb <= 1);
    ui.kYear->setEnabled(nb <= 1);
    ui.kMonth->setEnabled(nb <= 1);

    onCreatorModified();
    refreshInfoZone();
    Q_EMIT selectionChanged();
}

void SKGBudgetPluginWidget::activateEditor()
{
    if (ui.kWidgetSelector->getSelectedMode() == -1) {
        ui.kWidgetSelector->setSelectedMode(0);
    }
    ui.kAmountEdit->setFocus();
}

bool SKGBudgetPluginWidget::isEditor()
{
    return true;
}

void SKGBudgetPluginWidget::refreshInfoZone()
{
    SKGTRACEINFUNC(10)
    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    if ((doc != nullptr) && ui.kWidgetSelector->getSelectedMode() != 2) {
        SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();
        SKGServices::SKGUnitInfo secondary = doc->getSecondaryUnit();
        // Refresh info area with selection
        SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

        double budgeted = 0;
        double modified = 0;
        int nb = selection.count();
        for (int i = 0; i < nb; ++i) {
            SKGBudgetObject budget(selection.at(i));
            budgeted += budget.getBudgetedAmount();
            modified += budget.getBudgetedModifiedAmount();
        }

        QString budgetedS = doc->formatMoney(budgeted, primary);
        QString modifiedS = doc->formatMoney(modified, primary);
        QString v = (budgetedS == modifiedS ? budgetedS : modifiedS % " <s><small>" % budgetedS % "</small></s>");
        if (nb != 0) {
            ui.kInfo->setText(i18np("Selection: %1 budget for %2", "Selection: %1 budgets for %2", nb, v));
            if (!secondary.Symbol.isEmpty() && (secondary.Value != 0.0)) {
                budgetedS = doc->formatMoney(budgeted, secondary);
                modifiedS = doc->formatMoney(modified, secondary);
                v = (budgetedS == modifiedS ? budgetedS : std::move(modifiedS) % " <s><small>" % budgetedS % "</small></s>");
            }
            ui.kInfo->setToolTip(i18np("Selection: %1 budget for %2", "Selection: %1 budgets for %2", nb, v));
        } else {
            ui.kInfo->setText(i18nc("Noun", "Selection: none"));
            ui.kInfo->setToolTip(i18nc("Noun", "Selection: none"));
        }
    }
}

void SKGBudgetPluginWidget::onTop()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget rule update"), err, nb)
        for (int i = nb - 1; !err && i >= 0; --i) {
            SKGBudgetRuleObject rule(rules.at(i));

            double order = 1;
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT min(f_sortorder) from budgetrule"), result);
            if (!err && result.count() == 2) {
                order = SKGServices::stringToDouble(result.at(1).at(0)) - 1;
            }

            IFOKDO(err, rule.setOrder(order))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The budget rule '%1' has been updated", rule.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Budget rule updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Budget rule update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBudgetPluginWidget::onUp()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget rule update"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGBudgetRuleObject rule(rules.at(i));

            double order = rule.getOrder();
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder("SELECT f_sortorder from budgetrule where f_sortorder<" % SKGServices::doubleToString(order)
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
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The budget rule '%1' has been updated", rule.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Budget rule updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Budget rule update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBudgetPluginWidget::onDown()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget rule update"), err, nb)
        for (int i = nb - 1; !err && i >= 0; --i) {
            SKGBudgetRuleObject rule(rules.at(i));

            double order = rule.getOrder();
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder("SELECT f_sortorder from budgetrule where f_sortorder>" % SKGServices::doubleToString(order)
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
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The budget rule '%1' has been updated", rule.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Budget rule updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Budget rule update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGBudgetPluginWidget::onBottom()
{
    SKGError err;
    SKGTRACEINFUNCRC(1, err)

    // Get rules
    SKGObjectBase::SKGListSKGObjectBase rules = getSelectedObjects();
    int nb = rules.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Budget rule update"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGBudgetRuleObject rule(rules.at(i));

            double order = 1;
            SKGStringListList result;
            err = getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT max(f_sortorder) from budgetrule"), result);
            if (!err && result.count() == 2) {
                order = SKGServices::stringToDouble(result.at(1).at(0)) + 1;
            }

            IFOKDO(err, rule.setOrder(order))
            IFOKDO(err, rule.save())

            // Send message
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The budget rule '%1' has been updated", rule.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Budget rule updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Budget rule update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}
