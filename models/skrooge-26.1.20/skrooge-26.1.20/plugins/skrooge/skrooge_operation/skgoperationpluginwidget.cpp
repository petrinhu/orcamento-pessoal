/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for transaction management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgoperationpluginwidget.h"

#include <kcolorscheme.h>
#include <kstandardaction.h>

#include <qcompleter.h>
#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qheaderview.h>
#include <qinputdialog.h>
#include <qjsengine.h>
#include <qmap.h>
#include <qstandardpaths.h>
#include <qstringlistmodel.h>
#include <qtablewidget.h>

#include "skgbankincludes.h"
#include "skgcalculatoredit.h"
#include "skgmainpanel.h"
#include "skgobjectbase.h"
#include "skgobjectmodel.h"
#include "skgoperation_settings.h"
#include "skgpayeeobject.h"
#include "skgservices.h"
#include "skgshow.h"
#include "skgsplittabledelegate.h"
#include "skgtraces.h"
#include "skgtreeview.h"
#include <utility>

SKGOperationPluginWidget::SKGOperationPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument)
    : SKGTabPage(iParent, iDocument)
    , m_objectModel(nullptr)
    , m_fastEditionAction(nullptr)
    , m_lastFastEditionOperationFound(0)
    , m_showClosedAccounts(false)
    , m_numberFieldIsNotUptodate(true)
    , m_modeInfoZone(0)
    , m_tableDelegate(nullptr)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGOperationPluginWidget::onRefreshInformationZone, Qt::QueuedConnection);

    ui.setupUi(this);

    ui.kAccountLabel2->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_ACCOUNT"))));
    ui.kDateLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("d_date"))));
    ui.kAmountLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("f_CURRENTAMOUNT"))));
    ui.kPayeeLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_payee"))));
    ui.kTypeLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_mode"))));
    ui.kNumberEdit->setPlaceholderText(iDocument->getDisplay(QStringLiteral("t_number")));
    ui.kCategoryLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_CATEGORY"))));
    ui.kCommentLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_comment"))));
    ui.kTrackerLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_REFUND"))));

    ui.kUnitEdit->setDocument(iDocument);
    ui.kUnitShare->setDocument(iDocument);

    ui.kTitle->hide();
    ui.kReconciliatorFrame2->hide();
    ui.kReconciliateAccount->hide();

    m_attributesForSplit << QStringLiteral("d_date") << QStringLiteral("t_category") << QStringLiteral("f_value") << QStringLiteral("t_comment")
                         << QStringLiteral("t_refund");
    int nb = m_attributesForSplit.count();
    ui.kSubOperationsTable->setColumnCount(nb);
    for (int i = 0; i < nb; ++i) {
        QString att = m_attributesForSplit.at(i);
        auto item = new QTableWidgetItem(iDocument->getIcon(att), iDocument->getDisplay(att));
        ui.kSubOperationsTable->setHorizontalHeaderItem(i, item);
    }

    {
        // Bind transaction view
        m_objectModel = new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()),
                                           QStringLiteral("v_operation_display_all"),
                                           QStringLiteral("1=0"),
                                           this,
                                           QString(),
                                           false);
        ui.kOperationView->setModel(m_objectModel);

        // Add registered global action in contextual menu
        if (SKGMainPanel::getMainPanel() != nullptr) {
            m_fastEditionAction = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("fast_edition"));
        }

        connect(ui.kOperationView->getView(), &SKGTreeView::clickEmptyArea, this, &SKGOperationPluginWidget::cleanEditor);
        connect(ui.kOperationView->getView(), &SKGTreeView::doubleClicked, this, &SKGOperationPluginWidget::onDoubleClick);
        connect(ui.kOperationView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
            this->onSelectionChanged();
        });
    }

    // Add Standard KDE Icons to buttons to Transactions
    ui.kModifyOperationBtn->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kAddOperationBtn->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    ui.kCleanBtn->setIcon(SKGServices::fromTheme(QStringLiteral("edit-clear")));
    ui.kReconciliatorButton->setIcon(SKGServices::fromTheme(QStringLiteral("view-refresh")));
    ui.kValidate->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kValidate->setIconSize(QSize(48, 48));
    ui.kAutoPoint->setIcon(SKGServices::fromTheme(QStringLiteral("games-solve")));
    ui.kCreateFakeOperation->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    ui.kFastEditBtn->setIcon(SKGServices::fromTheme(QStringLiteral("games-solve")));

    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.SKGBasicSection);
        list.push_back(ui.SKGPayeeModeSection);
        list.push_back(ui.SKGSmallButtons);
        list.push_back(ui.SKGEditionButtonsWidget);
        list.push_back(ui.SKGSingleOpSection);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("dialog-ok")),
                                      i18n("Standard"),
                                      i18n("Display the edit panel for standard transactions"),
                                      list);
    }

    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.SKGBasicSection);
        list.push_back(ui.SKGPayeeModeSection);
        list.push_back(ui.SKGSmallButtons);
        list.push_back(ui.SKGEditionButtonsWidget);
        list.push_back(ui.SKGSplitOpSection);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("split")),
                                      i18n("Split"),
                                      i18n("Display the edit panel for split transactions"),
                                      list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.SKGBasicSection);
        list.push_back(ui.SKGPayeeModeSection);
        list.push_back(ui.SKGSmallButtons);
        list.push_back(ui.SKGEditionButtonsWidget);
        list.push_back(ui.SKGSingleOpSection);
        list.push_back(ui.kTargetAccountEdit);
        list.push_back(ui.kTargetAccountLabel);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("exchange-positions")),
                                      i18n("Transfer"),
                                      i18n("Display the edit panel for transfers between accounts"),
                                      list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.SKGBasicSection);
        list.push_back(ui.SKGPayeeModeSection);
        list.push_back(ui.SKGSmallButtons);
        list.push_back(ui.SKGEditionButtonsWidget);
        list.push_back(ui.SKGSharesSection);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("view-financial-account-savings")),
                                      i18n("Shares"),
                                      i18n("Display the edit panel for purchasing or selling shares"),
                                      list);
    }
    connect(ui.kWidgetSelector, &SKGWidgetSelector::selectedModeChanged, this, &SKGOperationPluginWidget::onBtnModeClicked);

    ui.kFreezeBtn->setIcon(SKGServices::fromTheme(QStringLiteral("emblem-locked")));

    // Fast edition
    connect(qApp, &QApplication::focusChanged, this, &SKGOperationPluginWidget::onFocusChanged);
    connect(m_fastEditionAction, &QAction::triggered, this, &SKGOperationPluginWidget::onFastEdition);

    // SubOperations
    connect(ui.kAmountEdit, &SKGCalculatorEdit::textChanged, this, &SKGOperationPluginWidget::onQuantityChanged);
    connect(ui.kDateEdit, &SKGDateEdit::dateChanged, this, &SKGOperationPluginWidget::onDateChanged);
    connect(ui.kSubOperationsTable, &SKGTableWidget::cellChanged, this, &SKGOperationPluginWidget::onSubopCellChanged);
    connect(ui.kSubOperationsTable->verticalHeader(), &QHeaderView::sectionClicked, this, &SKGOperationPluginWidget::onRemoveSubOperation);

    ui.kSubOperationsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui.kSubOperationsTable->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui.kSubOperationsTable->setWordWrap(false);
    m_tableDelegate = new SKGSplitTableDelegate(ui.kSubOperationsTable, getDocument(), m_attributesForSplit);
    m_tableDelegate->addParameterValue(QStringLiteral("total"), '0');
    ui.kSubOperationsTable->setItemDelegate(m_tableDelegate);
    ui.kSubOperationsTable->setTextElideMode(Qt::ElideMiddle);
    connect(ui.kSubOperationsTable, &SKGTableWidget::removeLine, this, &SKGOperationPluginWidget::onRemoveSubOperation);

    ui.kTargetAccountEdit->hide();
    ui.kTargetAccountLabel->hide();
    ui.SKGSplitOpSection->hide();
    ui.SKGSharesSection->hide();

    ui.kWidgetSelector->setSelectedMode(0);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    mainWidget()->installEventFilter(this);
    this->installEventFilter(this);

    // Set Event filters for locking widgets
    ui.kTypeEdit->lineEdit()->installEventFilter(this);
    ui.kTypeEdit->installEventFilter(this);
    ui.kUnitEdit->lineEdit()->installEventFilter(this);
    ui.kUnitEdit->installEventFilter(this);
    ui.kCategoryEdit->lineEdit()->installEventFilter(this);
    ui.kCategoryEdit->installEventFilter(this);
    ui.kCommentEdit->lineEdit()->installEventFilter(this);
    ui.kCommentEdit->installEventFilter(this);
    ui.kPayeeEdit->lineEdit()->installEventFilter(this);
    ui.kPayeeEdit->installEventFilter(this);
    ui.kTrackerEdit->lineEdit()->installEventFilter(this);
    ui.kTrackerEdit->installEventFilter(this);
    ui.kAccountEdit->installEventFilter(this);
    ui.kTargetAccountLabel->installEventFilter(this);
    ui.kAmountEdit->installEventFilter(this);
    ui.kNumberEdit->installEventFilter(this);

    connect(getDocument(), &SKGDocument::tableModified, this, &SKGOperationPluginWidget::dataModified, Qt::QueuedConnection);

    connect(ui.kUnitEdit, &SKGUnitComboBox::editTextChanged, this, &SKGOperationPluginWidget::refreshSubOperationAmount, Qt::QueuedConnection);
    connect(ui.kUnitEdit, &SKGUnitComboBox::editTextChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified, Qt::QueuedConnection);
    connect(ui.kUnitShare,
            static_cast<void (SKGUnitComboBox::*)(int)>(&SKGUnitComboBox::currentIndexChanged),
            this,
            &SKGOperationPluginWidget::onOperationCreatorModified,
            Qt::QueuedConnection);
    connect(ui.kAmountEdit, &SKGCalculatorEdit::textChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified, Qt::QueuedConnection);
    connect(ui.kAmountSharesEdit, &SKGCalculatorEdit::textChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified, Qt::QueuedConnection);
    connect(ui.kCommissionEdit, &SKGCalculatorEdit::textChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified, Qt::QueuedConnection);
    connect(ui.kTaxEdit, &SKGCalculatorEdit::textChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified, Qt::QueuedConnection);
    connect(ui.kAccountEdit,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGOperationPluginWidget::onOperationCreatorModified,
            Qt::QueuedConnection);
    connect(ui.kOperationView->getShowWidget(), &SKGShow::stateChanged, this, &SKGOperationPluginWidget::onFilterChanged, Qt::QueuedConnection);
    connect(ui.kPayeeEdit->lineEdit(), &QLineEdit::returnPressed, this, &SKGOperationPluginWidget::onPayeeChanged, Qt::QueuedConnection);

    connect(ui.kReconcilitorAmountEdit, &SKGCalculatorEdit::textChanged, this, &SKGOperationPluginWidget::onRefreshInformationZone);
    connect(ui.kAddOperationBtn, &QPushButton::clicked, this, &SKGOperationPluginWidget::onAddOperationClicked);
    connect(ui.kModifyOperationBtn, &QPushButton::clicked, this, &SKGOperationPluginWidget::onUpdateOperationClicked);
    connect(ui.kReconciliatorButton, &QToolButton::clicked, this, &SKGOperationPluginWidget::onRotateAccountTools);
    connect(ui.kValidate, &QToolButton::clicked, this, &SKGOperationPluginWidget::onValidateMarkedOperations);
    connect(ui.kCleanBtn, &QPushButton::clicked, this, &SKGOperationPluginWidget::cleanEditor);
    connect(ui.kSubOperationsTable, &SKGTableWidget::itemSelectionChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified);
    connect(ui.kAutoPoint, &QToolButton::clicked, this, &SKGOperationPluginWidget::onAutoPoint);
    connect(ui.kFreezeBtn, &QToolButton::clicked, this, &SKGOperationPluginWidget::onFreeze);
    connect(ui.kCreateFakeOperation, &QToolButton::clicked, this, &SKGOperationPluginWidget::onAddFakeOperation);
    connect(ui.kFastEditBtn, &QPushButton::clicked, this, &SKGOperationPluginWidget::onFastEdition);

    dataModified(QString(), 0);
    onOperationCreatorModified();

    setAllWidgetsEnabled();
}

SKGOperationPluginWidget::~SKGOperationPluginWidget()
{
    SKGTRACEINFUNC(1)
    m_objectModel = nullptr;
    m_fastEditionAction = nullptr;
}

QString SKGOperationPluginWidget::currentAccount()
{
    QStringList accounts = SKGServices::splitCSVLine(ui.kOperationView->getShowWidget()->getState());
    for (const auto &item : std::as_const(accounts)) {
        if (item.startsWith(QStringLiteral("##_"))) {
            return item.right(item.length() - 3);
        }
    }
    return QString();
}

bool SKGOperationPluginWidget::isWidgetEditionEnabled(QWidget *iWidget)
{
    return ((iWidget != nullptr) && (!iWidget->property("frozen").isValid() || !iWidget->property("frozen").toBool()));
}

void SKGOperationPluginWidget::setWidgetEditionEnabled(QWidget *iWidget, bool iEnabled)
{
    if ((iWidget != nullptr) && isWidgetEditionEnabled(iWidget) != iEnabled) {
        if (iEnabled) {
            iWidget->setStyleSheet(QStringLiteral("background-image:none;"));
            iWidget->setProperty("frozen", false);
        } else {
            auto color = KColorScheme(QPalette::Normal).background(KColorScheme::ActiveBackground).color().name().right(6);
            iWidget->setStyleSheet("background-color:#" % color);
            iWidget->setProperty("frozen", true);
        }

        QString addOn = i18nc("A tool tip", "This field is frozen (it will not be affected by Fast Edition). Double click to unfreeze it");
        QString t = iWidget->toolTip().remove('\n' % addOn).remove(addOn);
        if (!iEnabled) {
            t = iWidget->toolTip();
            if (!t.isEmpty()) {
                t += '\n';
            }
            t += addOn;
        }
        iWidget->setToolTip(t);

        // 348619: Freeze the unit when amount is frozen
        if (iWidget == ui.kAmountEdit) {
            setWidgetEditionEnabled(ui.kUnitEdit->lineEdit(), iEnabled);
        }
    }
}

bool SKGOperationPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::MouseButtonDblClick) {
        auto *line = qobject_cast<QLineEdit *>(iObject);
        if (line != nullptr) {
            setWidgetEditionEnabled(line, !isWidgetEditionEnabled(line));
        }
    } else if ((iEvent != nullptr) && iEvent->type() == QEvent::FocusIn) {
        auto *line = qobject_cast<QLineEdit *>(iObject);
        if (line != nullptr) {
            m_previousValue = line->text();
        } else {
            auto *cmb = qobject_cast<SKGComboBox *>(iObject);
            if (cmb != nullptr) {
                m_previousValue = cmb->text();
            }
        }
    } else if ((iEvent != nullptr) && iEvent->type() == QEvent::FocusOut) {
        auto *line = qobject_cast<QLineEdit *>(iObject);
        if (line != nullptr) {
            if (m_previousValue != line->text() && !line->text().isEmpty()) {
                setWidgetEditionEnabled(line, false);
            }
        } else {
            auto *cmb = qobject_cast<SKGComboBox *>(iObject);
            if (cmb != nullptr) {
                if (m_previousValue != cmb->text() && !cmb->text().isEmpty()) {
                    setWidgetEditionEnabled(cmb->lineEdit(), false);
                }
            }
        }
    } else if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u && ui.kAddOperationBtn->isEnabled()) {
                ui.kAddOperationBtn->click();
            } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kModifyOperationBtn->isEnabled()) {
                ui.kModifyOperationBtn->click();
            }
        } else if (keyEvent && (keyEvent->key() == Qt::Key_Up || keyEvent->key() == Qt::Key_Down) && iObject == ui.kNumberEdit) {
            int c = SKGServices::stringToInt(ui.kNumberEdit->text());
            if (c != 0) {
                ui.kNumberEdit->setText(SKGServices::intToString(keyEvent->key() == Qt::Key_Up ? c + 1 : c - 1));
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

void SKGOperationPluginWidget::onFreeze()
{
    if (!ui.kFreezeBtn->isChecked()) {
        ui.kFreezeBtn->setIcon(SKGServices::fromTheme(QStringLiteral("emblem-locked")));
        // At least one field is already frozen ==> unfreeze
        setAllWidgetsEnabled();
    } else {
        QStringList overlay;
        overlay.push_back(QStringLiteral("edit-delete"));
        ui.kFreezeBtn->setIcon(SKGServices::fromTheme(QStringLiteral("emblem-locked"), overlay));
        // No widget frozen ==> freeze widget containing test
        if (!ui.kTypeEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kTypeEdit->lineEdit(), false);
        }
        if (!ui.kUnitEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kUnitEdit->lineEdit(), false);
        }
        if (!ui.kCategoryEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kCategoryEdit->lineEdit(), false);
        }
        if (!ui.kCommentEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kCommentEdit->lineEdit(), false);
        }
        if (!ui.kPayeeEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kPayeeEdit->lineEdit(), false);
        }
        if (!ui.kTrackerEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kTrackerEdit->lineEdit(), false);
        }
        // if(!ui.kAccountEdit->text().isEmpty()) setWidgetEditionEnabled(ui.kAccountEdit, false);
        if (!ui.kAmountEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kAmountEdit, false);
        }
        if (!ui.kNumberEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kNumberEdit, false);
        }
        if (!ui.kTargetAccountEdit->text().isEmpty()) {
            setWidgetEditionEnabled(ui.kTargetAccountEdit, false);
        }
    }
}

void SKGOperationPluginWidget::setAllWidgetsEnabled()
{
    SKGTRACEINFUNC(10)
    // Enable widgets
    setWidgetEditionEnabled(ui.kTypeEdit->lineEdit(), true);
    setWidgetEditionEnabled(ui.kUnitEdit->lineEdit(), true);
    setWidgetEditionEnabled(ui.kCategoryEdit->lineEdit(), true);
    setWidgetEditionEnabled(ui.kCommentEdit->lineEdit(), true);
    setWidgetEditionEnabled(ui.kPayeeEdit->lineEdit(), true);
    setWidgetEditionEnabled(ui.kTrackerEdit->lineEdit(), true);
    setWidgetEditionEnabled(ui.kAccountEdit, true);
    setWidgetEditionEnabled(ui.kTargetAccountEdit, true);
    setWidgetEditionEnabled(ui.kAmountEdit, true);
    setWidgetEditionEnabled(ui.kNumberEdit, true);
}

QString SKGOperationPluginWidget::getAttributeOfSelection(const QString &iAttribute)
{
    QString output;
    SKGObjectBase::SKGListSKGObjectBase selectedObjects = ui.kOperationView->getView()->getSelectedObjects();
    int nb = selectedObjects.count();
    for (int i = 0; i < nb; ++i) {
        const SKGObjectBase &obj = selectedObjects.at(i);
        QString val = obj.getAttribute(iAttribute);
        if (i > 0 && val != output) {
            output = NOUPDATE;
            break;
        }
        output = std::move(val);
    }

    return output;
}

void SKGOperationPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)

    int mode = ui.kWidgetSelector->getSelectedMode();

    // Enable widgets
    setAllWidgetsEnabled();
    ui.kFreezeBtn->setChecked(false);
    ui.kFreezeBtn->setIcon(SKGServices::fromTheme(QStringLiteral("emblem-locked")));

    // Mapping
    int nbSelect = ui.kOperationView->getView()->getNbSelectedObjects();
    bool onConsolidatedTable = false;
    if ((nbSelect != 0) && (m_objectModel != nullptr)) {
        SKGObjectBase objbase = ui.kOperationView->getView()->getFirstSelectedObject();
        SKGOperationObject obj;
        onConsolidatedTable = (objbase.getTable() == QStringLiteral("v_suboperation_consolidated"));
        if (onConsolidatedTable) {
            obj = SKGOperationObject(obj.getDocument(), SKGServices::stringToInt(obj.getAttribute(QStringLiteral("i_OPID"))));
        } else {
            obj = objbase;
        }

        ui.kDateEdit->setDate(SKGServices::stringToDate(objbase.getAttribute(QStringLiteral("d_date"))));
        m_previousDate = ui.kDateEdit->date();
        ui.kCommentEdit->setText(objbase.getAttribute(onConsolidatedTable ? QStringLiteral("t_REALCOMMENT") : QStringLiteral("t_comment")));
        QString number = objbase.getAttribute(QStringLiteral("t_number"));
        ui.kNumberEdit->setText(number);
        QString accountName = objbase.getAttribute(QStringLiteral("t_ACCOUNT"));
        if (!m_showClosedAccounts && !accountName.isEmpty() && !ui.kAccountEdit->contains(accountName)) {
            // Refresh list of accounts if a closed account is selected
            m_showClosedAccounts = true;
            dataModified(QString(), 0);
        }
        ui.kAccountEdit->setText(accountName);
        ui.kPayeeEdit->setText(objbase.getAttribute(QStringLiteral("t_PAYEE")));
        ui.kTypeEdit->setText(objbase.getAttribute(QStringLiteral("t_mode")));
        QString unit = objbase.getAttribute(QStringLiteral("t_UNIT"));
        ui.kUnitEdit->setText(unit);
        QString cat = objbase.getAttribute(QStringLiteral("t_REALCATEGORY"));
        if (cat.isEmpty()) {
            cat = objbase.getAttribute(QStringLiteral("t_CATEGORY"));
        }
        ui.kCategoryEdit->setText(cat);
        ui.kTrackerEdit->setText(objbase.getAttribute(onConsolidatedTable ? QStringLiteral("t_REALREFUND") : QStringLiteral("t_REFUND")));
        QString quantity = objbase.getAttribute(QStringLiteral("f_REALQUANTITY"));
        if (quantity.isEmpty()) {
            quantity = objbase.getAttribute(QStringLiteral("f_QUANTITY"));
        }
        double quantityVal = SKGServices::stringToDouble(quantity);
        SKGUnitObject unitObject = ui.kUnitEdit->getUnit();
        int nbDec = unitObject.getNumberDecimal();
        if (nbDec == 0) {
            nbDec = 2;
        }
        quantity = SKGServices::toCurrencyString(qAbs(quantityVal), QString(), nbDec);
        if (quantity.startsWith(QLocale().positiveSign())) {
            quantity = quantity.right(quantity.length() - 1);
        }
        if (quantityVal > 0) {
            quantity = u'+' % quantity;
        } else {
            quantity = u'-' % quantity;
        }
        ui.kAmountEdit->setText(quantity);

        if (nbSelect > 1) {
            // In case of multi selection
            if (mode >= 0) {
                ui.kWidgetSelector->setSelectedMode(0);
            }
            ui.kAccountEdit->setText(getAttributeOfSelection(QStringLiteral("t_ACCOUNT")));
            ui.kTypeEdit->setText(getAttributeOfSelection(QStringLiteral("t_mode")));
            ui.kUnitEdit->setText(getAttributeOfSelection(QStringLiteral("t_UNIT")));
            ui.kCategoryEdit->setText(getAttributeOfSelection(onConsolidatedTable ? QStringLiteral("t_REALCATEGORY") : QStringLiteral("t_CATEGORY")));
            ui.kTrackerEdit->setText(getAttributeOfSelection(onConsolidatedTable ? QStringLiteral("t_REALREFUND") : QStringLiteral("t_REFUND")));
            ui.kCommentEdit->setText(getAttributeOfSelection(onConsolidatedTable ? QStringLiteral("t_REALCOMMENT") : QStringLiteral("t_comment")));
            ui.kPayeeEdit->setText(getAttributeOfSelection(QStringLiteral("t_PAYEE")));

            QString d = getAttributeOfSelection(QStringLiteral("d_date"));
            if (d == NOUPDATE) {
                ui.kDateEdit->setCurrentText(NOUPDATE);
            }

            QString q = getAttributeOfSelection(onConsolidatedTable ? QStringLiteral("f_REALQUANTITY") : QStringLiteral("f_QUANTITY"));
            ui.kAmountEdit->setText(q != NOUPDATE ? quantity : NOUPDATE);
            ui.kNumberEdit->setText(QString());
        } else {
            if (obj.getStatus() == SKGOperationObject::MARKED) {
                displayReconciliationInfo();
            } else if (m_modeInfoZone != 1) {
                displayBalance();
            }

            // It is a single selection
            // Is it a split ?
            int nbSubOperations = obj.getNbSubOperations();
            if (nbSubOperations > 1 && !onConsolidatedTable) {
                // yes, it is a split
                if (mode >= 0) {
                    ui.kWidgetSelector->setSelectedMode(1);
                }

                displaySubOperations();
            } else {
                // Is it a transfer ?
                SKGOperationObject op2;
                if (obj.isTransfer(op2) && op2.exist()) {
                    // yes it is a transfer
                    SKGAccountObject account2;
                    op2.getParentAccount(account2);
                    QString accountName2 = account2.getName();
                    if (!m_showClosedAccounts && !ui.kTargetAccountEdit->contains(accountName2)) {
                        // Refresh list of accounts if a closed account is selected
                        m_showClosedAccounts = true;
                        dataModified(QString(), 0);
                    }
                    ui.kTargetAccountEdit->setText(accountName2);
                    if (mode >= 0) {
                        ui.kWidgetSelector->setSelectedMode(2);
                    }
                } else {
                    if (mode >= 0) {
                        ui.kWidgetSelector->setSelectedMode(0);
                    }
                }
            }
        }
    }

    ui.kNumberEdit->setEnabled(nbSelect <= 1);

    bool splitTest = nbSelect <= 1 && !onConsolidatedTable;
    ui.kWidgetSelector->setEnabledMode(1, splitTest);
    if (!splitTest && mode == 1) {
        ui.kWidgetSelector->setSelectedMode(0);
    }

    onOperationCreatorModified();

    Q_EMIT selectionChanged();
}

void SKGOperationPluginWidget::onOperationCreatorModified()
{
    SKGTRACEINFUNC(10)

    int mode = ui.kWidgetSelector->getSelectedMode();

    // Set icons
    if (!isTemplateMode()) {
        ui.kModifyOperationBtn->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
        ui.kAddOperationBtn->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    } else {
        QStringList overlay;
        overlay.push_back(QStringLiteral("edit-guides"));
        ui.kModifyOperationBtn->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok"), overlay));
        ui.kAddOperationBtn->setIcon(SKGServices::fromTheme(QStringLiteral("list-add"), overlay));
    }

    // Is it an existing unit ?
    QString unitName = ui.kUnitEdit->currentText();
    SKGUnitObject unit(getDocument());
    unit.setName(unitName);
    unit.setSymbol(unitName);
    if (unit.load().isSucceeded()) {
        ui.kWidgetSelector->setEnabledMode(3, true);
        if (mode == 3 && unit.getType() == SKGUnitObject::SHARE) {
            // Update units
            auto unit = ui.kUnitShare->getUnit();
            ui.kUnitCommission->setText(unit.getSymbol());
            ui.kUnitTax->setText(unit.getSymbol());

            // Update total in "purchase / sale share" page
            double total = ui.kAmountSharesEdit->value() + (ui.kCommissionEdit->value() + ui.kTaxEdit->value()) * (ui.kAmountEdit->value() > 0 ? 1 : -1);
            ui.KTotal->setText(SKGServices::toCurrencyString(total, unit.getSymbol(), unit.getNumberDecimal()));
        } else {
            // BUG 2692665
            auto unitShareName = ui.kUnitShare->currentText();
            if (unitShareName.isEmpty()) {
                ui.kUnitShare->setText(unitName);
                ui.kUnitCommission->setText(unitName);
                ui.kUnitTax->setText(unitName);
                ui.KTotal->setText(unitName);

            } else {
                ui.kUnitCommission->setText(unitShareName);
                ui.kUnitTax->setText(unitShareName);
                ui.KTotal->setText(unitShareName);
            }
        }

    } else {
        ui.kWidgetSelector->setEnabledMode(3, false);
        if (mode == 3) {
            ui.kWidgetSelector->setSelectedMode(0);
        }
    }

    bool activated = mode != -1 && !ui.kAccountEdit->currentText().isEmpty()
        && ((!ui.kAmountEdit->text().isEmpty() && (ui.kAmountEdit->valid() || ui.kAmountEdit->text() == NOUPDATE)) || !ui.kAmountEdit->isEnabled())
        && !unitName.isEmpty() && (mode != 3 || !ui.kAmountSharesEdit->text().isEmpty());

    int nbSelect = getNbSelectedObjects();

    ui.kAddOperationBtn->setEnabled(activated);
    ui.kModifyOperationBtn->setEnabled(
        activated && nbSelect > 0
        && (ui.kWidgetSelector->getSelectedMode() == 0 || ui.kWidgetSelector->getSelectedMode() == 1 || ui.kWidgetSelector->getSelectedMode() == 2));

    m_numberFieldIsNotUptodate = true;
    if (ui.kNumberEdit->hasFocus()) {
        fillNumber();
    }
}

void SKGOperationPluginWidget::onPayeeChanged()
{
    if (skgoperation_settings::setCategoryForPayee() && ui.kCategoryEdit->text().isEmpty()) {
        ui.kCategoryEdit->setText(qobject_cast<SKGDocumentBank *>(getDocument())->getCategoryForPayee(ui.kPayeeEdit->text(), false));
    }
}

void SKGOperationPluginWidget::onUpdateOperationClicked()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

    int nb = selection.count();
    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Transaction update"), err, nb)
        err = updateSelection(selection);
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transaction updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Transaction update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kOperationView->getView()->setFocus();
}

SKGError SKGOperationPluginWidget::updateSelection(const SKGObjectBase::SKGListSKGObjectBase &iSelection, bool iForceCreation)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Initialisation
    double ratio = -1;
    bool refreshSubOperation = true;

    // Get Selection
    int nb = iSelection.count();
    double rate = -1;

    for (int i = 0; !err && i < nb; ++i) {
        const SKGObjectBase &obj = iSelection.at(i);
        SKGOperationObject operationObj;
        if (obj.getTable() == QStringLiteral("v_suboperation_consolidated")) {
            operationObj = SKGOperationObject(obj.getDocument(), SKGServices::stringToInt(obj.getAttribute(QStringLiteral("i_OPID"))));
        } else {
            operationObj = SKGOperationObject(obj.getDocument(), obj.getID());
        }

        SKGObjectBase::SKGListSKGObjectBase gops;
        IFOKDO(err, operationObj.getGroupedOperations(gops))
        if (gops.count() == 2 && ui.kWidgetSelector->getSelectedMode() < 2) {
            getDocument()->sendMessage(i18nc("An information message", "You modified one part of a transfer"), SKGDocument::Warning);
        }

        // Update transaction if single selection
        if (ui.kWidgetSelector->getSelectedMode() == 0) {
            // Get subop
            SKGSubOperationObject subOp;
            int nbSubop = 0;
            if (obj.getTable() == QStringLiteral("v_suboperation_consolidated")) {
                // It is a sub operation
                subOp = SKGSubOperationObject(obj.getDocument(), SKGServices::stringToInt(obj.getAttribute(QStringLiteral("i_SUBOPID"))));
                nbSubop = 1;
            } else {
                // It is a real operation, we take the first one
                SKGObjectBase::SKGListSKGObjectBase subOps;
                IFOKDO(err, operationObj.getSubOperations(subOps))
                nbSubop = subOps.count();
                if (nbSubop != 0) {
                    subOp = subOps[0];
                }
            }

            QString trackerName = ui.kTrackerEdit->text().trimmed();
            if (!err && trackerName != NOUPDATE) {
                SKGTrackerObject tracker;
                err = SKGTrackerObject::createTracker(qobject_cast<SKGDocumentBank *>(getDocument()), trackerName, tracker, true);
                IFOKDO(err, subOp.setTracker(tracker))
            }

            SKGCategoryObject cat;
            QString catName = ui.kCategoryEdit->text().trimmed();
            if (!err && catName != NOUPDATE) {
                err = SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), catName, cat, true);
                IFOKDO(err, subOp.setCategory(cat))
            } else {
                // Get current category to be able to find the appropriate sign
                subOp.getCategory(cat);
            }

            if (!err && ui.kAmountEdit->text() != NOUPDATE) {
                if (nbSubop > 1) {
                    err = SKGError(25, i18nc("Error message", "Cannot update a split transaction"));
                } else {
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
                    err = subOp.setQuantity(val);
                }
            }

            if (!err && ui.kCommentEdit->text() != NOUPDATE) {
                err = subOp.setComment(ui.kCommentEdit->text());
            }
            IFOKDO(err, subOp.save())
        } else if (ui.kWidgetSelector->getSelectedMode() == 1) {
            // Case split
            refreshSubOperation = false;
            int nbsubop = ui.kSubOperationsTable->rowCount();
            QList<int> listIdSubOp; // clazy:exclude=container-inside-loop
            listIdSubOp.reserve(nbsubop);
            for (int j = 0; !err && j < nbsubop; ++j) {
                // Get values
                QTableWidgetItem *item = ui.kSubOperationsTable->item(j, m_attributesForSplit.indexOf(QStringLiteral("t_category")));
                int id = (iForceCreation ? 0 : item->data(Qt::UserRole).toInt());
                QString catName = item->text();

                item = ui.kSubOperationsTable->item(j, m_attributesForSplit.indexOf(QStringLiteral("t_comment")));
                QString comment = item->text();

                item = ui.kSubOperationsTable->item(j, m_attributesForSplit.indexOf(QStringLiteral("f_value")));
                double val = item->data(101).toDouble();
                QString formula = item->toolTip();

                item = ui.kSubOperationsTable->item(j, m_attributesForSplit.indexOf(QStringLiteral("t_refund")));
                QString trackerName = item->text();

                item = ui.kSubOperationsTable->item(j, m_attributesForSplit.indexOf(QStringLiteral("d_date")));
                QDate date = SKGServices::stringToDate(item->toolTip());
                if (!date.isValid()) {
                    date = ui.kDateEdit->date();
                }

                SKGSubOperationObject subOperation;
                if (id != 0) {
                    // Update existing sub op
                    subOperation = SKGSubOperationObject(qobject_cast<SKGDocumentBank *>(getDocument()), id);
                } else {
                    // Create new sub op
                    err = operationObj.addSubOperation(subOperation);
                }

                // Create sub transaction object
                IFOK(err)
                {
                    SKGCategoryObject cat;
                    err = SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), catName, cat, true);
                    IFOKDO(err, subOperation.setCategory(cat))
                }
                IFOK(err)
                {
                    SKGTrackerObject tracker;
                    err = SKGTrackerObject::createTracker(qobject_cast<SKGDocumentBank *>(getDocument()), trackerName, tracker, true);
                    IFOKDO(err, subOperation.setTracker(tracker))
                }
                IFOKDO(err, subOperation.setOrder(ui.kSubOperationsTable->visualRow(j)))
                IFOKDO(err, subOperation.setDate(date))
                IFOKDO(err, subOperation.setComment(comment))
                IFOKDO(err, subOperation.setQuantity(val))
                if (formula.startsWith(QStringLiteral("=")) && !err) {
                    err = subOperation.setFormula(formula);
                }
                IFOKDO(err, subOperation.save())

                // The sub transaction created or updated mustn't be removed
                listIdSubOp.push_back(subOperation.getID());
            }

            // Remove useless subop
            IFOK(err)
            {
                SKGObjectBase::SKGListSKGObjectBase subOps;
                err = operationObj.getSubOperations(subOps);
                int nbsubop2 = subOps.count();
                for (int j = 0; !err && j < nbsubop2; ++j) {
                    const SKGObjectBase &sop = subOps.at(j);
                    if (!listIdSubOp.contains(sop.getID())) {
                        err = sop.remove(false);
                    }
                }
            }
        } else if (ui.kWidgetSelector->getSelectedMode() == 2) {
            // Case transfer
            // Create sub transaction object
            double operationQuantity = ui.kAmountEdit->value();

            SKGSubOperationObject subOperation;
            SKGObjectBase::SKGListSKGObjectBase subOps;
            IFOKDO(err, operationObj.getSubOperations(subOps))
            IFOK(err)
            {
                subOperation = subOps.at(0);

                double oldQuantity = subOperation.getQuantity();

                if (ui.kAmountEdit->sign() == 0) {
                    operationQuantity = qAbs(operationQuantity);
                } else if (ui.kAmountEdit->sign() > 0) {
                    operationQuantity = -qAbs(operationQuantity);
                } else {
                    operationQuantity = qAbs(operationQuantity);
                    if (oldQuantity == 0) {
                        err = getDocument()->sendMessage(i18nc("An information message", "Absolute value has been used for transfer creation."));
                    }
                }

                if (ui.kAmountEdit->text().trimmed() != NOUPDATE) {
                    err = subOperation.setQuantity(-operationQuantity);
                } else {
                    operationQuantity = -subOperation.getQuantity();
                }
            }

            SKGTrackerObject tracker;
            QString trackerName = ui.kTrackerEdit->text().trimmed();
            if (!err) {
                if (trackerName != NOUPDATE) {
                    err = SKGTrackerObject::createTracker(qobject_cast<SKGDocumentBank *>(getDocument()), trackerName, tracker, true);
                    IFOKDO(err, subOperation.setTracker(tracker))
                } else {
                    err = subOperation.getTracker(tracker);
                }
            }

            SKGCategoryObject cat;
            QString catName = ui.kCategoryEdit->text().trimmed();
            if (!err) {
                if (catName != NOUPDATE) {
                    err = SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), ui.kCategoryEdit->text(), cat, true);
                    IFOKDO(err, subOperation.setCategory(cat))
                } else {
                    err = subOperation.getCategory(cat);
                }
            }
            IFOKDO(err, subOperation.setComment(operationObj.getComment()))
            IFOKDO(err, subOperation.save())

            // Get account
            SKGAccountObject accountObj2(getDocument());
            IFOKDO(err, accountObj2.setName(ui.kTargetAccountEdit->currentText()))
            IFOKDO(err, accountObj2.load())

            // Check unit of target account
            SKGUnitObject unit;
            IFOKDO(err, operationObj.getUnit(unit))

            // Get date
            QDate operationDate;
            if (ui.kDateEdit->currentText().trimmed() != NOUPDATE) {
                operationDate = ui.kDateEdit->date();
            } else {
                operationDate = operationObj.getDate();
            }

            // Correction bug 2299303 vvv
            SKGUnitObject unitTargetAccount;
            IFOKDO(err, accountObj2.getUnit(unitTargetAccount))
            if (!err && unitTargetAccount.exist() && unit != unitTargetAccount) {
                // The unit of the transaction is not compliant with the unit of the target account
                // We ask to the user if he wants to continue or convert into the target account
                bool ok = false;
                QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                int decimal = qMax(unit.getNumberDecimal(), unitTargetAccount.getNumberDecimal());
                double newval;
                double defaultnewval = SKGUnitObject::convert(operationQuantity, unit, unitTargetAccount, operationDate);
                if (nb == 1) {
                    newval = QInputDialog::getDouble(SKGMainPanel::getMainPanel(),
                                                     i18nc("Question", "Confirmation"),
                                                     i18nc("Question",
                                                           "The operation's unit is not compatible with the target account.\n"
                                                           "Click Cancel if you want to continue anyway; "
                                                           "otherwise, enter the value in the target account's unit (%1):",
                                                           unitTargetAccount.getSymbol()),
                                                     defaultnewval,
                                                     -std::numeric_limits<double>::max(),
                                                     std::numeric_limits<double>::max(),
                                                     decimal,
                                                     &ok);
                } else {
                    if (rate == -1) {
                        newval = QInputDialog::getDouble(
                            SKGMainPanel::getMainPanel(),
                            i18nc("Question", "Confirmation"),
                            i18nc("Question",
                                  "Some operation's units are not compatible with the target account.\n"
                                  "The first selected transaction is:%1\n"
                                  "Click Cancel if you want to continue anyway; "
                                  "otherwise, enter the value in the target account's unit (%2):\nThe same rate will be applied to all transactions.",
                                  operationObj.getDisplayName(),
                                  unitTargetAccount.getSymbol()),
                            defaultnewval,
                            -std::numeric_limits<double>::max(),
                            std::numeric_limits<double>::max(),
                            decimal,
                            &ok);
                        if (ok) {
                            rate = defaultnewval / newval;
                        } else {
                            // Mode cancel for all
                            rate = -2;
                        }
                    } else {
                        if (rate != -2) {
                            ok = true;
                            newval = defaultnewval / rate;
                        }
                    }
                }
                QApplication::restoreOverrideCursor();
                if (ok) {
                    operationQuantity = newval;
                    unit = unitTargetAccount;
                } else {
                    // Mode cancel for all
                    rate = -2;
                }
            }
            // Correction bug 2299303 ^^^

            // create transferred operation
            SKGOperationObject operation2;

            IFOK(err)
            {
                if (gops.count() == 2) {
                    operation2 = (obj == SKGOperationObject(gops.at(0)) ? gops.at(1) : gops.at(0));
                    if (ui.kTargetAccountEdit->text() != NOUPDATE) {
                        err = operation2.setParentAccount(accountObj2);
                    }
                } else {
                    err = accountObj2.addOperation(operation2);
                }
            }
            if (nb == 1) {
                IFOKDO(err, operation2.setMode(ui.kTypeEdit->currentText()))
                QString payeeName = ui.kPayeeEdit->currentText();
                if (!err && payeeName != NOUPDATE) {
                    SKGPayeeObject payeeObject;
                    err = SKGPayeeObject::createPayee(qobject_cast<SKGDocumentBank *>(getDocument()), payeeName, payeeObject, true);
                    IFOKDO(err, operation2.setPayee(payeeObject))
                }
                IFOKDO(err, operation2.setNumber(ui.kNumberEdit->text()))
                IFOKDO(err, operation2.setComment(ui.kCommentEdit->text()))
                IFOKDO(err, operation2.setDate(operationDate, refreshSubOperation))
                IFOKDO(err, operation2.setUnit(unit))

            } else {
                IFOKDO(err, operation2.setMode(operationObj.getMode()))
                IFOKDO(err, operation2.setAttribute(QStringLiteral("r_payee_id"), operationObj.getAttribute(QStringLiteral("r_payee_id"))))
                IFOKDO(err, operation2.setNumber(operationObj.getNumber()))
                IFOKDO(err, operation2.setComment(operationObj.getComment()))
                IFOKDO(err, operation2.setDate(operationDate, refreshSubOperation))
                IFOKDO(err, operation2.setUnit(unit))
            }
            IFOKDO(err, operation2.setGroupOperation(operationObj))
            IFOKDO(err, operationObj.load())
            IFOKDO(err, operation2.setTemplate(isTemplateMode()))
            IFOKDO(err, operation2.save())

            // Create sub transaction object
            SKGSubOperationObject subOperation2;

            SKGObjectBase::SKGListSKGObjectBase subops;
            IFOKDO(err, operation2.getSubOperations(subops))
            IFOK(err)
            {
                if (!subops.isEmpty()) {
                    subOperation2 = subops.at(0);
                } else {
                    err = operation2.addSubOperation(subOperation2);
                }
            }
            IFOKDO(err, subOperation2.setQuantity(operationQuantity))
            IFOKDO(err, subOperation2.setTracker(tracker))
            IFOKDO(err, subOperation2.setCategory(cat))
            IFOKDO(err, subOperation2.setComment(operationObj.getComment()))
            IFOKDO(err, subOperation2.save())
        }

        IFOKDO(err, operationObj.setTemplate(isTemplateMode()))
        if (ui.kDateEdit->currentText() != NOUPDATE) {
            IFOKDO(err, operationObj.setDate(ui.kDateEdit->date(), refreshSubOperation))
        }

        if (nb == 1) {
            IFOKDO(err, operationObj.setNumber(ui.kNumberEdit->text()))
        }
        if (!err && ui.kCommentEdit->text() != NOUPDATE) {
            if (obj.getTable() != QStringLiteral("v_suboperation_consolidated")
                || obj.getAttribute(QStringLiteral("i_NBSUBOPERATIONS")) == QStringLiteral("1")) {
                err = operationObj.setComment(ui.kCommentEdit->text());
            }
        }

        if (!err && ui.kAccountEdit->text() != NOUPDATE) {
            SKGAccountObject account(getDocument());
            err = account.setName(ui.kAccountEdit->text());
            IFOKDO(err, account.load())
            IFOKDO(err, operationObj.setParentAccount(account))
        }
        if (!err && ui.kTypeEdit->text() != NOUPDATE) {
            err = operationObj.setMode(ui.kTypeEdit->text());
        }
        QString payeeName = ui.kPayeeEdit->currentText();
        if (!err && payeeName != NOUPDATE) {
            SKGPayeeObject payeeObject;
            err = SKGPayeeObject::createPayee(qobject_cast<SKGDocumentBank *>(getDocument()), payeeName, payeeObject, true);
            IFOKDO(err, operationObj.setPayee(payeeObject))
        }
        if (!err && ui.kUnitEdit->text() != NOUPDATE) {
            // Correction bug 282983 vvv
            // Check unit of target account
            SKGAccountObject account;
            IFOKDO(err, operationObj.getParentAccount(account))
            SKGUnitObject unitTargetAccount;
            IFOKDO(err, account.getUnit(unitTargetAccount))

            SKGUnitObject unit = ui.kUnitEdit->getUnit();

            if (!err && unitTargetAccount.exist() && unit != unitTargetAccount) {
                // Correction bug 283842 vvvv
                bool ok = false;
                if (ratio == -1) {
                    // The unit of the transaction is not compliant with the unit of the target account
                    // We ask to the user if he wants to continue or convert into the target account
                    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                    double currentAmount = ui.kAmountEdit->value();
                    int decimal = qMax(unit.getNumberDecimal(), unitTargetAccount.getNumberDecimal());
                    double newval = QInputDialog::getDouble(SKGMainPanel::getMainPanel(),
                                                            i18nc("Question", "Confirmation"),
                                                            i18nc("Question",
                                                                  "The operation's unit is not compatible with the target account.\n"
                                                                  "Click Cancel if you want to continue anyway; "
                                                                  "otherwise, enter the value in the target account's unit (%1):",
                                                                  unitTargetAccount.getSymbol()),
                                                            SKGUnitObject::convert(currentAmount, unit, unitTargetAccount, ui.kDateEdit->date()),
                                                            -std::numeric_limits<double>::max(),
                                                            std::numeric_limits<double>::max(),
                                                            decimal,
                                                            &ok);
                    ratio = newval / currentAmount;
                    QApplication::restoreOverrideCursor();
                }
                if (ok) {
                    // Apply ratio to all operation
                    SKGObjectBase::SKGListSKGObjectBase subops;
                    err = operationObj.getSubOperations(subops);
                    int nbsubops = subops.count();
                    for (int j = 0; !err && j < nbsubops; ++j) {
                        SKGSubOperationObject subop(subops.at(j));
                        err = subop.setQuantity(subop.getQuantity() * ratio);
                        IFOKDO(err, subop.save(true, false))
                    }

                    // Change unit
                    unit = unitTargetAccount;
                } else {
                    err = getDocument()->sendMessage(
                        i18nc("Warning message", "You created a transaction in %1 in an account in %2.", unit.getSymbol(), unitTargetAccount.getSymbol()),
                        SKGDocument::Warning);
                }
                // Correction bug 283842 ^^^^
            }
            // Correction bug 282983 ^^^
            IFOKDO(err, operationObj.setUnit(unit))
        }

        // Save
        IFOKDO(err, operationObj.save())

        // Send message
        if (!iForceCreation) {
            IFOKDO(
                err,
                operationObj.getDocument()->sendMessage(i18nc("An information message", "The transaction '%1' has been updated", operationObj.getDisplayName()),
                                                        SKGDocument::Hidden))
        }

        IFOKDO(err, getDocument()->stepForward(i + 1))
    }

    return err;
}

void SKGOperationPluginWidget::onAddOperationClicked()
{
    SKGError err;
    SKGTRACEINFUNC(10)

    // Get parameters
    QString accountName = ui.kAccountEdit->currentText();
    SKGOperationObject operation;
    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Transaction creation"), err)

        // Check entries
        if (ui.kAccountEdit->text() == NOUPDATE || ui.kTypeEdit->text() == NOUPDATE || ui.kUnitEdit->text() == NOUPDATE || ui.kCategoryEdit->text() == NOUPDATE
            || ui.kTrackerEdit->text() == NOUPDATE || ui.kCommentEdit->text() == NOUPDATE || ui.kAmountEdit->text() == NOUPDATE
            || ui.kDateEdit->currentText() == NOUPDATE || ui.kPayeeEdit->text() == NOUPDATE) {
            err = SKGError(ERR_FAIL, i18nc("Error message", "Impossible to create a transaction with one attribute valuated with %1", NOUPDATE));
        }

        // Get account
        SKGAccountObject accountObj(getDocument());
        IFOKDO(err, accountObj.setName(accountName))
        IFOKDO(err, accountObj.load())

        // Create transaction object
        IFOKDO(err, accountObj.addOperation(operation))
        IFOKDO(err, operation.setMode(ui.kTypeEdit->currentText()))
        SKGPayeeObject payeeObject;
        IFOK(err)
        {
            QString payeeName = ui.kPayeeEdit->currentText();
            err = SKGPayeeObject::createPayee(qobject_cast<SKGDocumentBank *>(getDocument()), payeeName, payeeObject, true);
            IFOKDO(err, operation.setPayee(payeeObject))
        }
        IFOKDO(err, operation.setNumber(ui.kNumberEdit->text()))
        IFOKDO(err, operation.setComment(ui.kCommentEdit->text()))
        IFOKDO(err, operation.setDate(ui.kDateEdit->date()))
        IFOKDO(err, operation.setTemplate(isTemplateMode()))
        SKGUnitObject unit = ui.kUnitEdit->getUnit();
        IFOKDO(err, operation.setUnit(unit))
        if (skgoperation_settings::automaticPointInReconciliation() && m_modeInfoZone == 1) {
            IFOKDO(err, operation.setStatus(SKGOperationObject::MARKED))
        }
        IFOKDO(err, operation.save())

        if (ui.kWidgetSelector->getSelectedMode() <= 2) {
            // STD OPERATION (SPLIT , TRANSFER OR NOT)
            // We must create a suboperation, just be sure to be able to update it
            SKGSubOperationObject subOperation;
            IFOKDO(err, operation.addSubOperation(subOperation))
            IFOKDO(err, subOperation.setQuantity(0))
            IFOKDO(err, subOperation.save())

            SKGObjectBase::SKGListSKGObjectBase list;
            list << operation;
            IFOKDO(err, updateSelection(list, true))

        } else if (ui.kWidgetSelector->getSelectedMode() == 3) {
            // PURCHASE OR SALE SHARE
            // Create sub transaction object
            SKGSubOperationObject subOperation;
            double val = ui.kAmountEdit->value();
            IFOKDO(err, operation.addSubOperation(subOperation))
            IFOKDO(err, subOperation.setQuantity(val))
            IFOKDO(err, subOperation.save())

            if (!err && val > 0) {
                err = operation.setProperty(QStringLiteral("SKG_OP_ORIGINAL_AMOUNT"), SKGServices::doubleToString(ui.kAmountSharesEdit->value()));
            }
            IFOKDO(err, operation.save())

            // Get account
            SKGAccountObject accountObj2(getDocument());
            IFOKDO(err, accountObj2.setName(ui.kPaymentAccountEdit->currentText()))
            IFOKDO(err, accountObj2.load())

            SKGUnitObject unit = ui.kUnitShare->getUnit();

            SKGUnitObject unitTargetAccount;
            IFOKDO(err, accountObj2.getUnit(unitTargetAccount))
            double ratio = 1.0;
            if (!err && unitTargetAccount.exist() && unit != unitTargetAccount) {
                // The unit of the transaction is not compliant with the unit of the target account
                // We ask to the user if he wants to continue or convert into the target account
                bool ok = false;
                QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                int decimal = qMax(unit.getNumberDecimal(), unitTargetAccount.getNumberDecimal());
                double defaultnewval = SKGUnitObject::convert(ui.kAmountSharesEdit->value(), unit, unitTargetAccount, ui.kDateEdit->date());
                double newval = QInputDialog::getDouble(SKGMainPanel::getMainPanel(),
                                                        i18nc("Question", "Confirmation"),
                                                        i18nc("Question",
                                                              "The payment's unit (%1) is not compatible with the target account (%2).\n"
                                                              "Click Cancel if you want to continue anyway; "
                                                              "otherwise, enter the value in the target account's unit (%3):",
                                                              unit.getSymbol(),
                                                              accountObj2.getName(),
                                                              unitTargetAccount.getSymbol()),
                                                        defaultnewval,
                                                        -std::numeric_limits<double>::max(),
                                                        std::numeric_limits<double>::max(),
                                                        decimal,
                                                        &ok);
                if (ok) {
                    ratio = newval / ui.kAmountSharesEdit->value();
                    unit = unitTargetAccount;
                }
                QApplication::restoreOverrideCursor();
            }

            // create payment transaction for shares
            SKGOperationObject operation2;
            IFOKDO(err, accountObj2.addOperation(operation2))
            IFOKDO(err, operation2.setMode(ui.kTypeEdit->currentText()))
            IFOKDO(err, operation2.setPayee(payeeObject))
            IFOKDO(err, operation2.setNumber(ui.kNumberEdit->text()))
            IFOKDO(err, operation2.setComment(ui.kCommentEdit->text()))
            IFOKDO(err, operation2.setDate(ui.kDateEdit->date()))
            IFOKDO(err, operation2.setUnit(unit))
            IFOKDO(err, operation2.setGroupOperation(operation))
            IFOKDO(err, operation2.setTemplate(isTemplateMode()))
            IFOKDO(err, operation2.save())

            // Create main sub operation
            SKGSubOperationObject subOperation2;
            IFOKDO(err, operation2.addSubOperation(subOperation2))
            IFOKDO(err, subOperation2.setComment(i18nc("Noun", "Shares")))
            IFOKDO(err, subOperation2.setQuantity(ui.kAmountSharesEdit->value() * (val > 0 ? -1 : 1) * ratio))
            IFOKDO(err, subOperation2.save())

            // Create commission sub operation
            if (ui.kCommissionEdit->value() != 0.0) {
                SKGSubOperationObject subOperation3;
                IFOKDO(err, operation2.addSubOperation(subOperation3))
                IFOKDO(err, subOperation3.setComment(skgoperation_settings::commentCommissionOperation()))

                QString category = skgoperation_settings::categoryCommissionOperation();
                if (!category.isEmpty()) {
                    SKGCategoryObject c;
                    IFOKDO(err, SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), category, c, true))
                    IFOKDO(err, subOperation3.setCategory(c))
                }

                IFOKDO(err, subOperation3.setQuantity(-ui.kCommissionEdit->value() * ratio))
                IFOKDO(err, subOperation3.save())
            }

            // Create tax sub operation
            if (ui.kTaxEdit->value() != 0.0) {
                SKGSubOperationObject subOperation4;
                IFOKDO(err, operation2.addSubOperation(subOperation4))
                IFOKDO(err, subOperation4.setComment(skgoperation_settings::commentTaxOperation()))

                QString category = skgoperation_settings::categoryTaxOperation();
                if (!category.isEmpty()) {
                    SKGCategoryObject c;
                    IFOKDO(err, SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), category, c, true))
                    IFOKDO(err, subOperation4.setCategory(c))
                }

                IFOKDO(err, subOperation4.setQuantity(-ui.kTaxEdit->value() * ratio))
                IFOKDO(err, subOperation4.save())
            }
        }

        // Send message
        IFOKDO(err,
               operation.getDocument()->sendMessage(
                   i18nc("An information to the user that something was added", "The transaction '%1' has been added", operation.getDisplayName()),
                   SKGDocument::Hidden))
    }

    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("Successful message after an user action", "Transaction created"));
        ui.kOperationView->getView()->selectObject(operation.getUniqueID());
    }
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Transaction creation failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on date
    ui.kDateEdit->setFocus();
}

SKGTreeView *SKGOperationPluginWidget::getTableView()
{
    return ui.kOperationView->getView();
}

QString SKGOperationPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root;
    if (m_lastState.hasChildNodes()) {
        doc = m_lastState;
        root = doc.documentElement();
    } else {
        root = doc.createElement(QStringLiteral("parameters"));
        doc.appendChild(root);
    }

    root.setAttribute(QStringLiteral("currentPage"), SKGServices::intToString(ui.kWidgetSelector->getSelectedMode()));
    root.setAttribute(QStringLiteral("modeInfoZone"), SKGServices::intToString(m_modeInfoZone));
    root.setAttribute(QStringLiteral("reconcilitorAmount"), ui.kReconcilitorAmountEdit->text());
    root.removeAttribute(QStringLiteral("account"));

    // Memorize table settings
    root.setAttribute(QStringLiteral("view"), ui.kOperationView->getState());

    return doc.toString();
}

void SKGOperationPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString account = root.attribute(QStringLiteral("account"));
    QString currentPage = root.attribute(QStringLiteral("currentPage"));
    QString title = root.attribute(QStringLiteral("title"));
    QString title_icon = root.attribute(QStringLiteral("title_icon"));
    QString modeInfoZoneS = root.attribute(QStringLiteral("modeInfoZone"));
    QString reconcilitorAmountS = root.attribute(QStringLiteral("reconcilitorAmount"));
    QString operationTable = root.attribute(QStringLiteral("operationTable"));
    QString selection = root.attribute(QStringLiteral("selection"));
    QString templates = root.attribute(QStringLiteral("template"));
    m_operationWhereClause = root.attribute(QStringLiteral("operationWhereClause"));

    // Default values in case of reset
    if (currentPage.isEmpty()) {
        currentPage = '0';
    }
    if (operationTable.isEmpty()) {
        if (m_operationWhereClause.isEmpty()) {
            operationTable = QStringLiteral("v_operation_display_all");
        } else {
            operationTable = QStringLiteral("v_operation_display");
        }
    }

    // Set
    SKGAccountObject acc;
    SKGNamedObject::getObjectByName(getDocument(), QStringLiteral("v_account"), account, acc);
    if (acc.isClosed() && !m_showClosedAccounts) {
        m_showClosedAccounts = true;
        dataModified(QString(), 0);
    }
    bool previous = ui.kReconcilitorAmountEdit->blockSignals(true);
    ui.kReconcilitorAmountEdit->setText(reconcilitorAmountS);
    ui.kReconcilitorAmountEdit->blockSignals(previous);
    ui.kWidgetSelector->setSelectedMode(SKGServices::stringToInt(currentPage));
    if (!title.isEmpty()) {
        QFontMetrics fm(fontMetrics());
        ui.kTitle->setComment("<html><body><b>" % SKGServices::stringToHtml(fm.elidedText(title, Qt::ElideMiddle, 2000)) % "</b></body></html>");
        ui.kTitle->setToolTip(title);
        ui.kTitle->show();
    }
    if (!title_icon.isEmpty()) {
        ui.kTitle->setIcon(SKGServices::fromTheme(title_icon), KTitleWidget::ImageLeft);
    }

    if (m_objectModel != nullptr) {
        m_objectModel->setTable(operationTable);
    }
    if ((m_objectModel != nullptr) && !m_operationWhereClause.isEmpty()) {
        ui.kOperationView->getShowWidget()->setEnabled(false);
        m_objectModel->setFilter(m_operationWhereClause);
    }
    if (!operationTable.isEmpty() || !m_operationWhereClause.isEmpty()) {
        // We keep a copy of given state in case of bookmark
        m_lastState = doc;
    } else {
        m_lastState = QDomDocument();
    }

    // Update model
    if (m_objectModel != nullptr) {
        previous = m_objectModel->blockRefresh(true);
        onAccountChanged();
        m_objectModel->blockRefresh(previous);
    }

    // !!! Must be done here after onFilterChanged
    QString v = root.attribute(QStringLiteral("view"));
    if (!v.isEmpty()) {
        ui.kOperationView->setState(v);
    }
    if (!account.isEmpty()) {
        QStringList parameters = SKGServices::splitCSVLine(ui.kOperationView->getShowWidget()->getState());
        if (parameters.isEmpty()) {
            parameters.push_back(QString());
            parameters.push_back(QStringLiteral("transactions"));
            parameters.push_back(QStringLiteral("hide"));
        }
        parameters[0] = "##_" % account;
        ui.kOperationView->getShowWidget()->setState(SKGServices::stringsToCsv(parameters));
    }

    if (templates == QStringLiteral("Y")) {
        QStringList parameters = SKGServices::splitCSVLine(ui.kOperationView->getShowWidget()->getState());
        parameters.removeAll(QStringLiteral("transactions"));
        parameters.push_back(QStringLiteral("templates"));
        ui.kOperationView->getShowWidget()->setState(SKGServices::stringsToCsv(parameters));
    }

    QStringList parameters = SKGServices::splitCSVLine(ui.kOperationView->getShowWidget()->getState());
    if (!parameters.contains(QStringLiteral("transactions")) && !parameters.contains(QStringLiteral("templates"))) {
        parameters.push_back(QStringLiteral("transactions"));
        ui.kOperationView->getShowWidget()->setState(SKGServices::stringsToCsv(parameters));
    }

    if (!selection.isEmpty()) {
        QStringList uuids = SKGServices::splitCSVLine(selection);
        ui.kOperationView->getView()->selectObjects(uuids, true); // FIXME // TODO(Stephane MANKOWSKI)
        onSelectionChanged();
    }

    // Refresh of the information zone
    if (!modeInfoZoneS.isEmpty()) {
        m_modeInfoZone = SKGServices::stringToInt(modeInfoZoneS) - 1;
    } else {
        m_modeInfoZone = -1;
    }
    onRotateAccountTools();
}

QString SKGOperationPluginWidget::getDefaultStateAttribute()
{
    if ((m_objectModel != nullptr) && m_objectModel->getTable() == QStringLiteral("v_suboperation_consolidated")) {
        return QStringLiteral("SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS");
    }

    if (!m_operationWhereClause.isEmpty()) {
        return QString();
    }

    return QStringLiteral("SKGOPERATION_DEFAULT_PARAMETERS");
}

void SKGOperationPluginWidget::fillTargetAccount()
{
    int nbAccounts = ui.kAccountEdit->count();
    QString current = ui.kAccountEdit->text();
    QString currentTarget = ui.kTargetAccountEdit->text();
    QString currentRecon = ui.kReconciliateAccount->text();
    ui.kTargetAccountEdit->clear();
    ui.kReconciliateAccount->clear();
    ui.kReconciliateAccount->addItem(QString());
    for (int i = 0; i < nbAccounts; ++i) {
        if (ui.kAccountEdit->itemText(i) != current) {
            ui.kTargetAccountEdit->addItem(ui.kAccountEdit->itemIcon(i), ui.kAccountEdit->itemText(i));
            ui.kReconciliateAccount->addItem(ui.kAccountEdit->itemIcon(i), ui.kAccountEdit->itemText(i));
        }
    }
    if (ui.kTargetAccountEdit->contains(currentTarget)) {
        ui.kTargetAccountEdit->setText(currentTarget);
    }

    SKGError err;
    SKGAccountObject currentActObj(getDocument());
    IFOKDO(err, currentActObj.setName(current))
    IFOKDO(err, currentActObj.load())

    SKGAccountObject linkedActObj;
    IFOKDO(err, currentActObj.getLinkedAccount(linkedActObj))
    if (linkedActObj.getID() != 0) {
        currentRecon = linkedActObj.getName();
    }

    if (ui.kReconciliateAccount->contains(currentRecon)) {
        ui.kReconciliateAccount->setText(currentRecon);
    }
}

void SKGOperationPluginWidget::dataModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    // Refresh widgets
    QSqlDatabase *db = getDocument()->getMainDatabase();
    setEnabled(db != nullptr);
    if (db != nullptr) {
        if (iTableName == QStringLiteral("account") || iTableName.isEmpty()) {
            SKGShow *showWidget = ui.kOperationView->getShowWidget();
            QString current = currentAccount();
            QString currentState = showWidget->getState();

            // Disconnect combo filter account
            disconnect(showWidget, &SKGShow::stateChanged, this, &SKGOperationPluginWidget::onAccountChanged);
            disconnect(showWidget, &SKGShow::stateChanged, this, &SKGOperationPluginWidget::onRefreshInformationZoneDelayed);
            disconnect(showWidget, &SKGShow::stateChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified);
            disconnect(ui.kAccountEdit,
                       static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
                       this,
                       &SKGOperationPluginWidget::fillTargetAccount);

            // Clear
            ui.kAccountEdit->clear();
            ui.kPaymentAccountEdit->clear();

            SKGStringListList listAccount;
            getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT t_ICON, t_name, t_bookmarked, id, t_type from v_account_display ")
                                                        % (m_showClosedAccounts ? "" : "where t_close='N' ") % "order by t_bookmarked DESC, t_name ASC",
                                                    listAccount);

            int nbAccounts = listAccount.count() - 1;
            if (!m_lastState.hasChildNodes()) {
                ui.kTitle->hide();
            }

            // Set show widget
            showWidget->clear();

            if (nbAccounts > 1) {
                showWidget->addGroupedItem(QStringLiteral("all"),
                                           i18nc("Option to for display of transactions", "All"),
                                           QString(),
                                           QStringLiteral("1=1"),
                                           QStringLiteral("account"),
                                           Qt::META | Qt::Key_A);
                showWidget->addSeparator();
            }

            QString uncheck;
            bool accountSeparatorAdded = false;
            for (int i = 1; i <= nbAccounts; ++i) { // Ignore header
                const bool wallet = (listAccount.at(i).at(4) == QStringLiteral("W"));
                QString iconName = wallet ? QStringLiteral("wallet-closed")
                                          : QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/images/logo/" % listAccount.at(i).at(0));
                if (iconName.isEmpty()) {
                    iconName = listAccount.at(i).at(0);
                }
                QIcon icon(iconName);
                QString text = listAccount.at(i).at(1);
                QString id = "##_" % text;
                if (nbAccounts == 1) {
                    QStringList items = SKGServices::splitCSVLine(currentState);
                    if (items.isEmpty()) {
                        items.push_back(QStringLiteral("all"));
                    }
                    if (items[0] == QStringLiteral("all") || !items[0].startsWith(QStringLiteral("##_"))) {
                        items[0] = id;
                    }
                    if (items.count() == 1) {
                        items.push_back(QStringLiteral("transactions"));
                        items.push_back(QStringLiteral("hide"));
                    }
                    currentState = SKGServices::stringsToCsv(items);
                }
                ui.kAccountEdit->addItem(icon, text);
                ui.kPaymentAccountEdit->addItem(icon, text);

                if (!accountSeparatorAdded && listAccount.at(i).at(2) == QStringLiteral("N")) {
                    if (i > 1) {
                        showWidget->addSeparator();
                    }
                    accountSeparatorAdded = true;
                }

                QString account_id = listAccount.at(i).at(3);
                showWidget->addGroupedItem(id,
                                           text,
                                           iconName,
                                           "rd_account_id=" + account_id,
                                           QStringLiteral("account"),
                                           (i < 10 ? QKeySequence::fromString("Meta+" % SKGServices::intToString(i)) : QKeySequence()));

                SKGStringListList listLinkedAccounts;
                getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT id FROM account where t_close='N' AND r_account_id=") + account_id,
                                                        listLinkedAccounts);
                int nbLinkedAccount = listLinkedAccounts.count();
                if (nbLinkedAccount > 1) {
                    QString f = account_id;
                    for (int j = 1; j < nbLinkedAccount; ++j) { // Ignore header
                        f += ",'" + listLinkedAccounts.at(j).at(0) + '\'';
                    }

                    showWidget->addGroupedItem(id + "_cc",
                                               i18nc("Information", "%1 + its credit cards", text),
                                               iconName,
                                               "rd_account_id IN(" + f + ')',
                                               QStringLiteral("account"),
                                               QKeySequence());
                }

                if (!uncheck.isEmpty()) {
                    uncheck = uncheck % ";";
                }
                uncheck = uncheck % id;
            }

            int nb = showWidget->count();
            for (int i = 1; i < nb; ++i) {
                showWidget->setListIdToUncheckWhenChecked(i, uncheck % ";all");
            }
            if (nbAccounts > 1) {
                showWidget->setListIdToUncheckWhenChecked(0, uncheck);
            }

            showWidget->addSeparator();
            showWidget->addGroupedItem(QStringLiteral("transactions"),
                                       i18nc("Option to for display of transactions", "Transactions"),
                                       QStringLiteral("view-institution-edit"),
                                       QStringLiteral("d_date!='0000-00-00' AND t_template='N'"),
                                       QStringLiteral("type"),
                                       Qt::META | Qt::Key_O);
            showWidget->addGroupedItem(QStringLiteral("templates"),
                                       i18nc("Option to for display of transactions", "Templates"),
                                       QStringLiteral("edit-guides"),
                                       QStringLiteral("d_date!='0000-00-00' AND t_template='Y'"),
                                       QStringLiteral("type"),
                                       Qt::META | Qt::Key_T);
            showWidget->addSeparator();
            showWidget->addItem(QStringLiteral("hidepointed"),
                                i18nc("Option to for display of transactions", "Hide marked transactions"),
                                QStringLiteral("dialog-ok"),
                                QStringLiteral("t_status<>'P'"),
                                QString(),
                                QString(),
                                QString(),
                                QString(),
                                Qt::META | Qt::Key_P);
            showWidget->addItem(QStringLiteral("hide"),
                                i18nc("Option to for display of transactions", "Hide checked transactions"),
                                QStringLiteral("dialog-ok"),
                                QStringLiteral("t_status<>'Y'"),
                                QString(),
                                QString(),
                                QString(),
                                QString(),
                                Qt::META | Qt::Key_C);
            showWidget->addSeparator();
            showWidget->addPeriodItem(QStringLiteral("period"));
            showWidget->setMode(SKGShow::AND);
            if (currentState.isEmpty()) {
                showWidget->setDefaultState(QStringLiteral("all;transactions;hide"));
            } else {
                showWidget->setState(currentState);
            }

            if (!current.isEmpty()) {
                ui.kAccountEdit->setText(current);
            }

            // Connect combo filter account
            connect(ui.kAccountEdit,
                    static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
                    this,
                    &SKGOperationPluginWidget::fillTargetAccount,
                    Qt::QueuedConnection);
            connect(showWidget, &SKGShow::stateChanged, this, &SKGOperationPluginWidget::onAccountChanged, Qt::QueuedConnection);
            connect(showWidget, &SKGShow::stateChanged, this, &SKGOperationPluginWidget::onRefreshInformationZoneDelayed, Qt::QueuedConnection);
            connect(showWidget, &SKGShow::stateChanged, this, &SKGOperationPluginWidget::onOperationCreatorModified, Qt::QueuedConnection);

            fillTargetAccount();

            if (nbAccounts == 0) {
                ui.kTitle->setText(i18nc("Message", "First, you have to create an account."));
                ui.kTitle->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-information")), KTitleWidget::ImageLeft);
                ui.kTitle->show();
                showWidget->hide();
            } else {
                showWidget->show();
            }
        }

        if (iTableName == QStringLiteral("refund") || iTableName.isEmpty()) {
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kTrackerEdit,
                                                getDocument(),
                                                QStringLiteral("refund"),
                                                QStringLiteral("t_name"),
                                                QStringLiteral("t_close='N'"));
        }
        if (!iLightTransaction) {
            if (iTableName == QStringLiteral("category") || iTableName.isEmpty()) {
                SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kCategoryEdit,
                                                    getDocument(),
                                                    QStringLiteral("category"),
                                                    QStringLiteral("t_fullname"),
                                                    QStringLiteral("t_close='N'"));
            }
            if (iTableName == QStringLiteral("payee") || iTableName.isEmpty()) {
                SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kPayeeEdit,
                                                    getDocument(),
                                                    QStringLiteral("payee"),
                                                    QStringLiteral("t_name"),
                                                    QStringLiteral("t_close='N'"));
            }
            if (iTableName == QStringLiteral("operation") || iTableName.isEmpty()) {
                SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kTypeEdit,
                                                    getDocument(),
                                                    QStringLiteral("operation"),
                                                    QStringLiteral("t_mode"),
                                                    QString(),
                                                    true);
                SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kCommentEdit,
                                                    getDocument(),
                                                    QStringLiteral("v_operation_all_comment"),
                                                    QStringLiteral("t_comment"),
                                                    QString(),
                                                    true);

                // Set type number
                m_numberFieldIsNotUptodate = true;

                // Correction 215658: vvv because the table is modified, the selection is modified
                onSelectionChanged();
                // Correction 215658: ^^^

                onRefreshInformationZoneDelayed();
            }
        } else if (iTableName == QStringLiteral("operation") || iTableName.isEmpty()) {
            onRefreshInformationZoneDelayed();
        }
    }
}

void SKGOperationPluginWidget::onDoubleClick()
{
    _SKGTRACEINFUNC(10)

    // Get selection
    SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();
    if (selection.count() == 1) {
        SKGOperationObject op(selection.at(0));

        if (op.isTemplate() && selection.at(0).getRealTable() == QStringLiteral("operation")) {
            // this is a template, we have to create an operation
            SKGError err;
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Transaction creation"), err)
            SKGOperationObject operation;
            err = op.duplicate(operation);
            if (skgoperation_settings::automaticPointInReconciliation() && m_modeInfoZone == 1) {
                IFOKDO(err, operation.setStatus(SKGOperationObject::MARKED))
                IFOKDO(err, operation.save())
            }

            // Send message
            IFOKDO(err,
                   operation.getDocument()->sendMessage(
                       i18nc("An information to the user that something was added", "The transaction '%1' has been added", operation.getDisplayName()),
                       SKGDocument::Hidden))

            // status bar
            IFOK(err)
            {
                setTemplateMode(false);
                err = SKGError(0, i18nc("Successful message after an user action", "Transaction created"));
                ui.kOperationView->getView()->selectObject(operation.getUniqueID());
            }
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message", "Transaction creation failed"));
            }

            // Display error
            SKGMainPanel::displayErrorMessage(err);

        } else {
            // This is not a template, we have to open it
            SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open"))->trigger();
        }
    }
}

void SKGOperationPluginWidget::onRefreshInformationZoneDelayed()
{
    m_timer.start(300);
}

void SKGOperationPluginWidget::onRefreshInformationZone()
{
    SKGTRACEINFUNC(1)
    ui.kReconciliateAccount->hide();

    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    QString current = currentAccount();
    if (doc != nullptr && ui.kInfo != nullptr && SKGMainPanel::getMainPanel()) {
        if (m_modeInfoZone == 0) {
            // Refresh info area
            // Compute where clause
            QString filter = QStringLiteral("1=1");
            if (!current.isEmpty()) {
                filter = "t_name='" % SKGServices::stringToSqlString(current) % u'\'';
            }
            ui.kInfo->setText(i18nc("Message", "Computing…"));
            doc->concurrentExecuteSelectSqliteOrder(
                "SELECT TOTAL(f_CURRENTAMOUNT), TOTAL(f_CHECKED), TOTAL(f_COMING_SOON), TOTAL(f_COMING_SOON_FROM_LINKED_ACCOUNT) from v_account_display WHERE "
                    % filter,
                [this, doc, current](const SKGStringListList &iResult) {
                    if (iResult.count() == 2 && SKGMainPanel::getMainPanel()->pageIndex(this) != -1) {
                        SKGServices::SKGUnitInfo unit1 = doc->getPrimaryUnit();
                        SKGServices::SKGUnitInfo unit2 = doc->getSecondaryUnit();
                        if (!current.isEmpty()) {
                            SKGAccountObject account(getDocument());
                            if (account.setName(current).isSucceeded()) {
                                if (account.load().isSucceeded()) {
                                    SKGUnitObject unitAccount;
                                    if (account.getUnit(unitAccount).isSucceeded()) {
                                        if (!unitAccount.getSymbol().isEmpty()) {
                                            unit1.Symbol = unitAccount.getSymbol();
                                            unit1.Value = SKGServices::stringToDouble(unitAccount.getAttribute(QStringLiteral("f_CURRENTAMOUNT")));

                                            if (unit1.Symbol != qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit().Symbol) {
                                                unit2 = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit();
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        double v1 = SKGServices::stringToDouble(iResult.at(1).at(0));
                        double v2 = SKGServices::stringToDouble(iResult.at(1).at(1));
                        double v3 = SKGServices::stringToDouble(iResult.at(1).at(2));
                        double v4 = SKGServices::stringToDouble(iResult.at(1).at(3));
                        QString s1 = doc->formatMoney(v1, unit1);
                        QString s2 = doc->formatMoney(v2, unit1);
                        QString s3 = doc->formatMoney(v3, unit1);
                        QString s4 = doc->formatMoney(v4, unit1);
                        QString zero = doc->formatMoney(0, unit1);
                        ui.kInfo->setText(i18nc("Message",
                                                "Balance: %1     Checked: %2     To be Checked: %3",
                                                s1,
                                                s2,
                                                !current.isEmpty() && s4 != zero ? s3 % " + " % s4 : s3));
                        if (!unit2.Symbol.isEmpty() && (unit2.Value != 0.0)) {
                            s1 = doc->formatMoney(v1, unit2);
                            s2 = doc->formatMoney(v2, unit2);
                            s3 = doc->formatMoney(v3, unit2);
                            s4 = doc->formatMoney(v4, unit2);
                        }
                        ui.kInfo->setToolTip(i18nc("Message",
                                                   "<p>Balance: %1</p><p>Checked: %2</p><p>To be Checked: %3</p>",
                                                   s1,
                                                   s2,
                                                   !current.isEmpty() && s4 != zero ? s3 % " + " % s4 : s3));
                    }
                });
        } else if (m_modeInfoZone == 1) {
            // Refresh reconciliation area
            SKGServices::SKGUnitInfo unit1 = doc->getPrimaryUnit();
            SKGServices::SKGUnitInfo unit2 = doc->getSecondaryUnit();
            // Compute where clause
            QString filter = '\'' % SKGServices::stringToSqlString(currentAccount()) % u'\'';
            SKGStringListList listTmp;
            getDocument()->executeSelectSqliteOrder(
                "SELECT ABS(TOTAL(f_CURRENTAMOUNT_EXPENSE)),TOTAL(f_CURRENTAMOUNT_INCOME) FROM v_operation_display WHERE t_status='P' AND t_ACCOUNT=" % filter,
                listTmp);
            if (listTmp.count() == 2) {
                SKGAccountObject::AccountType accountType = SKGAccountObject::OTHER;
                if (!current.isEmpty()) {
                    SKGAccountObject account(getDocument());
                    if (account.setName(current).isSucceeded()) {
                        if (account.load().isSucceeded()) {
                            accountType = account.getType();

                            SKGUnitObject unitAccount;
                            if (account.getUnit(unitAccount).isSucceeded()) {
                                if (!unitAccount.getSymbol().isEmpty()) {
                                    unit1.Symbol = unitAccount.getSymbol();
                                    unit1.Value = SKGServices::stringToDouble(unitAccount.getAttribute(QStringLiteral("f_CURRENTAMOUNT")));

                                    if (unit1.Symbol != qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit().Symbol) {
                                        unit2 = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit();
                                    }
                                }
                            }
                        }
                    }
                }
                if (accountType == SKGAccountObject::SKGAccountObject::CREDITCARD) {
                    ui.kReconciliationTitle->setText(i18nc("A title", "Total amount:"));
                    ui.kReconciliateAccount->show();
                } else {
                    ui.kReconciliationTitle->setText(i18nc("A title", "Final balance:"));
                }

                ui.kAutoPoint->setVisible(accountType != SKGAccountObject::WALLET);

                SKGStringListList listTmp2;
                double diff = 0;
                getDocument()->executeSelectSqliteOrder("SELECT TOTAL(f_CHECKEDANDPOINTED) from v_account_display WHERE t_name=" % filter, listTmp2);
                if (listTmp2.count() == 2) {
                    diff = SKGServices::stringToDouble(listTmp2.at(1).at(0)) - ui.kReconcilitorAmountEdit->value() * unit1.Value;
                }

                // Set tooltip
                if (current.isEmpty()) {
                    ui.kReconciliatorInfo->setText(i18nc("Description", "You must select only one account to use reconciliation."));
                    ui.kReconciliatorInfo->setToolTip(ui.kReconciliatorInfo->text());
                } else {
                    double v1 = SKGServices::stringToDouble(listTmp.at(1).at(0));
                    double v2 = SKGServices::stringToDouble(listTmp.at(1).at(1));
                    QString sdiff = doc->formatMoney(diff, unit1);
                    QString s1 = doc->formatMoney(v1, unit1);
                    QString s2 = doc->formatMoney(v2, unit1);
                    ui.kReconciliatorInfo->setText(i18nc("Message", "%1 - Delta: %2     Expenditure: %3     Income: %4", unit1.Symbol, sdiff, s1, s2));

                    // Comparison
                    QString zero = doc->formatMoney(0, unit1);
                    QString negativezero = doc->formatMoney(-EPSILON, unit1);
                    ui.kValidate->setVisible(sdiff == zero || sdiff == negativezero);
                    ui.kCreateFakeOperation->setVisible(!ui.kValidate->isVisible());
                    ui.kAutoPoint->setVisible(!ui.kValidate->isVisible());

                    if (!unit2.Symbol.isEmpty() && (unit2.Value != 0.0)) {
                        sdiff = doc->formatMoney(diff, unit2);
                        s1 = doc->formatMoney(v1, unit2);
                        s2 = doc->formatMoney(v2, unit2);
                    }
                    ui.kReconciliatorInfo->setToolTip(i18nc("Message", "<p>Delta: %1</p><p>Expenditure: %2</p><p>Income: %3</p>", sdiff, s1, s2));
                }
            }
        }
    }
}

void SKGOperationPluginWidget::onAccountChanged()
{
    SKGTRACEINFUNC(1)
    if (!currentAccount().isEmpty() && ui.kOperationView->getView()->getNbSelectedObjects() == 0) {
        // Get account object
        SKGAccountObject act(getDocument());
        SKGError err = act.setName(currentAccount());
        IFOKDO(err, act.load())

        // Get unit
        SKGUnitObject unit;
        IFOKDO(err, act.getUnit(unit))
        if (!err && !unit.getSymbol().isEmpty()) {
            ui.kUnitEdit->setText(unit.getSymbol());
        }
    }
    onFilterChanged();
}

void SKGOperationPluginWidget::onFilterChanged()
{
    SKGTRACEINFUNC(1)
    if (!isEnabled()) {
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Enable/ disable widgets
    bool onOneAccount = (!currentAccount().isEmpty());
    ui.kReconciliatorFrame2->setEnabled(onOneAccount);
    if (!onOneAccount && m_modeInfoZone == 1) {
        ui.kReconciliatorFrame2->hide();
        ui.kInfo->show();
        m_modeInfoZone = 0;
    }

    QString current = currentAccount();
    if (!current.isEmpty() && ui.kOperationView->getView()->getNbSelectedObjects() == 0) {
        ui.kAccountEdit->setText(current);
    }

    QApplication::restoreOverrideCursor();
}

void SKGOperationPluginWidget::fillNumber()
{
    SKGTRACEINFUNC(10)
    QStringList list;
    QString account = ui.kAccountEdit->text();
    QString wc;
    if (!account.isEmpty()) {
        wc = "rd_account_id IN (SELECT id FROM account WHERE t_name='" + SKGServices::stringToSqlString(account) + "')";
    }
    getDocument()->getDistinctValues(QStringLiteral("v_operation_next_numbers"), QStringLiteral("t_number"), wc, list);

    // Fill completion
    auto comp = new QCompleter(list);
    comp->setFilterMode(Qt::MatchContains);
    ui.kNumberEdit->setCompleter(comp);

    m_numberFieldIsNotUptodate = false;
}

void SKGOperationPluginWidget::onFocusChanged()
{
    _SKGTRACEINFUNC(10)
    if (!qApp->closingDown()) {
        if ((SKGMainPanel::getMainPanel() != nullptr) && SKGMainPanel::getMainPanel()->currentPage() == this) {
            if (m_numberFieldIsNotUptodate && ui.kNumberEdit->hasFocus()) {
                fillNumber();
            }

            bool test = ui.kTypeEdit->hasFocus() ||
                //                  ui.kAmountEdit->hasFocus() ||
                //              ui.kNumberEdit->hasFocus() ||
                ui.kUnitEdit->hasFocus() || ui.kCategoryEdit->hasFocus() || ui.kTrackerEdit->hasFocus() || ui.kCommentEdit->hasFocus()
                || ui.kPayeeEdit->hasFocus();
            if (m_fastEditionAction != nullptr) {
                m_fastEditionAction->setEnabled(test);
            }
        }
    }
}

void SKGOperationPluginWidget::onFastEdition()
{
    if (SKGMainPanel::getMainPanel()->currentPage() != this) {
        return;
    }
    SKGTRACEINFUNC(10)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    SKGError err;

    // Get widget item
    QWidget *w = QApplication::focusWidget();
    auto *cmb = qobject_cast<SKGComboBox *>(w);
    if (cmb != nullptr) {
        setWidgetEditionEnabled(cmb->lineEdit(), false);
    } else {
        setWidgetEditionEnabled(w, false);
    }

    // Build the where clause
    QString wc;
    if (ui.kTypeEdit->hasFocus()) {
        wc = "t_mode LIKE '" % SKGServices::stringToSqlString(ui.kTypeEdit->text()) % "%'";
    } else if (ui.kUnitEdit->hasFocus()) {
        wc = "t_UNIT LIKE '" % SKGServices::stringToSqlString(ui.kUnitEdit->text()) % "%'";
    } else if (ui.kCategoryEdit->hasFocus()) {
        wc = "t_CATEGORY LIKE '" % SKGServices::stringToSqlString(ui.kCategoryEdit->text()) % "%'";
    } else if (ui.kTrackerEdit->hasFocus()) {
        wc = "t_REFUND LIKE '" % SKGServices::stringToSqlString(ui.kTrackerEdit->text()) % "%'";
    } else if (ui.kCommentEdit->hasFocus()) {
        wc = "t_comment LIKE '" % SKGServices::stringToSqlString(ui.kCommentEdit->text()) % "%'";
    } else if (ui.kPayeeEdit->hasFocus()) {
        wc = "t_PAYEE LIKE '" % SKGServices::stringToSqlString(ui.kPayeeEdit->text()) % "%'";
    }

    if (!wc.isEmpty()) {
        QString accountName = ui.kAccountEdit->currentText();
        if (!accountName.isEmpty() && skgoperation_settings::oncurrentaccountonly()) {
            wc += " AND t_ACCOUNT LIKE '" % SKGServices::stringToSqlString(accountName) % "%'";
        }

        // Read Setting
        QString fasteditmode = skgoperation_settings::fasteditmode();

        /*
        0-Search in templates only
        1-Search first in templates and after in transactions
        2-Search in transactions only
        3-Search first in transactions and after in templates
        */
        if (fasteditmode == QStringLiteral("0")) {
            wc += QStringLiteral(" AND t_template='Y'");
        } else if (fasteditmode == QStringLiteral("2")) {
            wc += QStringLiteral(" AND t_template='N'");
        }

        if (wc != m_lastFastEditionWhereClause) {
            m_lastFastEditionWhereClause = wc;
            m_lastFastEditionOperationFound = 0;
        }

        // Look for last operation
        if (m_lastFastEditionOperationFound != 0) {
            wc += " AND id<" % SKGServices::intToString(m_lastFastEditionOperationFound);
        }

        // Add order by
        wc += QStringLiteral(" ORDER BY ");
        if (fasteditmode == QStringLiteral("1")) {
            wc += QStringLiteral(" t_template DESC, ");
        } else if (fasteditmode == QStringLiteral("3")) {
            wc += QStringLiteral(" t_template ASC, ");
        }
        wc += QStringLiteral("d_date DESC, id DESC LIMIT 1");

        SKGObjectBase::SKGListSKGObjectBase transactions;
        err = getDocument()->getObjects(QStringLiteral("v_operation_display_all"), wc, transactions);
        if (!err && !transactions.isEmpty()) {
            SKGOperationObject op(transactions.at(0));

            m_lastFastEditionOperationFound = op.getID();
            if (isWidgetEditionEnabled(ui.kTypeEdit->lineEdit())) {
                ui.kTypeEdit->setText(op.getMode());
            }
            if (isWidgetEditionEnabled(ui.kUnitEdit->lineEdit())) {
                ui.kUnitEdit->setText(op.getAttribute(QStringLiteral("t_UNIT")));
            }
            if (isWidgetEditionEnabled(ui.kCategoryEdit->lineEdit())) {
                ui.kCategoryEdit->setText(op.getAttribute(QStringLiteral("t_CATEGORY")));
            }
            if (isWidgetEditionEnabled(ui.kCommentEdit->lineEdit())) {
                ui.kCommentEdit->setText(op.getComment());
            }
            if (isWidgetEditionEnabled(ui.kPayeeEdit->lineEdit())) {
                ui.kPayeeEdit->setText(op.getAttribute(QStringLiteral("t_PAYEE")));
            }
            if (isWidgetEditionEnabled(ui.kTrackerEdit->lineEdit())) {
                ui.kTrackerEdit->setText(op.getAttribute(QStringLiteral("t_REFUND")));
            }
            if (currentAccount().isEmpty()) {
                ui.kAccountEdit->setText(op.getAttribute(QStringLiteral("t_ACCOUNT")));
            }
            if (isWidgetEditionEnabled(ui.kAmountEdit)) {
                QString quantity = op.getAttribute(QStringLiteral("f_QUANTITY"));

                double quantityVal = SKGServices::stringToDouble(quantity);
                SKGUnitObject unitObject = ui.kUnitEdit->getUnit();
                int nbDec = unitObject.getNumberDecimal();
                if (nbDec == 0) {
                    nbDec = 2;
                }
                quantity = SKGServices::toCurrencyString(qAbs(quantityVal), QString(), nbDec);
                if (quantity.startsWith(QLocale().positiveSign())) {
                    quantity = quantity.right(quantity.length() - 1);
                }
                if (quantityVal > 0) {
                    quantity = u'+' % quantity;
                } else {
                    quantity = u'-' % quantity;
                }
                ui.kAmountEdit->setText(quantity);
            }

            // set next number
            if (isWidgetEditionEnabled(ui.kNumberEdit)) {
                int number = SKGServices::stringToInt(op.getNumber());
                if (number == 0) {
                    ui.kNumberEdit->setText(QString());
                } else {
                    if (m_numberFieldIsNotUptodate) {
                        fillNumber();
                    }

                    QCompleter *comp = ui.kNumberEdit->completer();
                    if (comp != nullptr) {
                        QStringList list = qobject_cast<QStringListModel *>(comp->model())->stringList();
                        int nb = list.count();
                        int cpt = 0;
                        while (nb >= 0 && cpt >= 0 && cpt < 1000) {
                            ++number;

                            if (list.contains(SKGServices::intToString(number))) {
                                cpt = -2;
                            }
                            ++cpt;
                        }

                        if (cpt < 0) {
                            ui.kNumberEdit->setText(SKGServices::intToString(number));
                        }
                    }
                }
            }

            // Get nb transaction linked
            SKGObjectBase::SKGListSKGObjectBase groupedOperations;
            op.getGroupedOperations(groupedOperations);
            int nbGroupedOp = groupedOperations.count();

            // Get nb sub op
            SKGObjectBase::SKGListSKGObjectBase subOperations;
            op.getSubOperations(subOperations);
            int nbSupOp = subOperations.count();

            if (nbSupOp > 1) {
                // It is a SPLIT operation
                ui.kWidgetSelector->setSelectedMode(1);
                QDate d = ui.kDateEdit->date();
                if (!d.isValid()) {
                    d = QDate::currentDate();
                }
                displaySubOperations(op, false, d);

            } else {
                if (nbGroupedOp > 1) {
                    // It is a TRANSFER
                    SKGOperationObject op2(groupedOperations.at(0));
                    if (op2 == op) {
                        op2 = groupedOperations.at(1);
                    }

                    SKGAccountObject targetAccount;
                    op2.getParentAccount(targetAccount);

                    if (isWidgetEditionEnabled(ui.kTargetAccountEdit)) {
                        ui.kTargetAccountEdit->setText(targetAccount.getName());
                    }
                } else {
                    ui.kWidgetSelector->setSelectedMode(0);
                }
            }

        } else {
            m_lastFastEditionWhereClause = QString();
            m_lastFastEditionOperationFound = 0;
        }
    }

    if (w != nullptr) {
        w->setFocus(Qt::OtherFocusReason);
    }
    QApplication::restoreOverrideCursor();

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

bool SKGOperationPluginWidget::isTemplateMode()
{
    QAction *act = ui.kOperationView->getShowWidget()->getAction(QStringLiteral("templates"));
    return ((act != nullptr) && act->isChecked());
}

void SKGOperationPluginWidget::setTemplateMode(bool iTemplate)
{
    SKGTRACEINFUNC(10)

    if (iTemplate != isTemplateMode()) {
        QAction *act = ui.kOperationView->getShowWidget()->getAction(QStringLiteral("templates"));
        if (act != nullptr) {
            act->setChecked(iTemplate);
        }

        act = ui.kOperationView->getShowWidget()->getAction(QStringLiteral("transactions"));
        if (act != nullptr) {
            act->setChecked(!iTemplate);
        }
    }
}

void SKGOperationPluginWidget::onBtnModeClicked(int mode)
{
    SKGTRACEINFUNC(10)
    if (mode != 1 && mode != -1) {
        ui.kSubOperationsTable->setRowCount(0);
        ui.kSubOperationsTable->clearContents();
    }

    if (mode == 1) {
        if (ui.kSubOperationsTable->rowCount() == 0) {
            addSubOperationLine(0,
                                ui.kDateEdit->date(),
                                ui.kCategoryEdit->text(),
                                ui.kTrackerEdit->text(),
                                ui.kCommentEdit->text(),
                                ui.kAmountEdit->value(),
                                nullptr);
        }
    }
    onOperationCreatorModified();
}

void SKGOperationPluginWidget::displaySubOperations(const SKGOperationObject &iOperation, bool iKeepId, QDate iSubOperationsDate)
{
    SKGTRACEINFUNC(10)
    ui.kSubOperationsTable->setRowCount(0);
    ui.kSubOperationsTable->clearContents();

    int nbSubOperations = 0;

    SKGObjectBase::SKGListSKGObjectBase subOperations;
    SKGError err = iOperation.getSubOperations(subOperations);
    nbSubOperations = subOperations.count();
    for (int i = 0; i < nbSubOperations; ++i) {
        SKGSubOperationObject subOperation(subOperations.at(i));

        SKGCategoryObject category;
        subOperation.getCategory(category);

        SKGTrackerObject tracker;
        subOperation.getTracker(tracker);

        addSubOperationLine(i,
                            iSubOperationsDate.isValid() ? iSubOperationsDate : subOperation.getDate(),
                            category.getFullName(),
                            tracker.getName(),
                            subOperation.getComment(),
                            subOperation.getQuantity(),
                            subOperation.getFormula(),
                            (iKeepId ? subOperation.getID() : 0));
    }

    onQuantityChanged();
}

void SKGOperationPluginWidget::displaySubOperations()
{
    SKGTRACEINFUNC(10)
    SKGOperationObject operation;
    if (getSelectedOperation(operation).isSucceeded()) {
        displaySubOperations(operation);
    }
}

double SKGOperationPluginWidget::getRemainingQuantity()
{
    SKGTRACEINFUNC(10)
    double sumQuantities = 0;
    int nbSubOperations = ui.kSubOperationsTable->rowCount();

    for (int i = 0; i < nbSubOperations; ++i) {
        QTableWidgetItem *quantityItem = ui.kSubOperationsTable->item(i, m_attributesForSplit.indexOf(QStringLiteral("f_value")));
        if (quantityItem != nullptr) {
            sumQuantities = sumQuantities + quantityItem->data(101).toDouble();
        }
    }

    return ui.kAmountEdit->value() - sumQuantities;
}

void SKGOperationPluginWidget::onDateChanged(QDate iDate)
{
    SKGTRACEINFUNC(10)
    bool previous = ui.kSubOperationsTable->blockSignals(true); // Disable signals so that filling cell doesn't create new lines
    if (sender() == ui.kDateEdit && iDate.isValid() && m_previousDate.isValid()) {
        // Refresh dates
        int nbSubOperations = ui.kSubOperationsTable->rowCount();
        for (int i = 0; i < nbSubOperations; ++i) {
            QTableWidgetItem *dateItem = ui.kSubOperationsTable->item(i, m_attributesForSplit.indexOf(QStringLiteral("d_date")));
            if (dateItem != nullptr) {
                auto datestring = dateItem->toolTip();
                QDate previousSubDate = SKGServices::stringToDate(datestring);
                if (previousSubDate.isValid()) {
                    int delta = m_previousDate.daysTo(iDate);

                    auto newDate = previousSubDate.addDays(delta);
                    dateItem->setText(SKGMainPanel::dateToString(newDate));
                    dateItem->setToolTip(SKGServices::dateToSqlString(newDate));
                }
            }
        }
    }
    m_previousDate = iDate;
    ui.kSubOperationsTable->blockSignals(previous); // Reenable signals
}

void SKGOperationPluginWidget::refreshSubOperationAmount()
{
    SKGTRACEINFUNC(10)
    bool previous = ui.kSubOperationsTable->blockSignals(true); // Disable signals so that filling cell doesn't create new lines

    int nbSubOperations = ui.kSubOperationsTable->rowCount();

    // Refresh computed amounts
    auto unit = ui.kUnitEdit->getUnit().getUnitInfo();
    unit.Value = 1.0;
    for (int i = 0; i < nbSubOperations; ++i) {
        QTableWidgetItem *quantityItem = ui.kSubOperationsTable->item(i, m_attributesForSplit.indexOf(QStringLiteral("f_value")));
        if (quantityItem != nullptr) {
            QString formula = quantityItem->toolTip();
            if (formula.startsWith(QStringLiteral("="))) {
                formula = formula.right(formula.length() - 1);
                formula.replace(',', '.'); // Replace comma by a point in case of typo
                formula.remove(' ');
                formula.replace(QStringLiteral("total"), SKGServices::doubleToString(ui.kAmountEdit->value()));

                QJSEngine myEngine;
                auto result = myEngine.evaluate(formula);
                if (result.isNumber()) {
                    auto value = result.toNumber();
                    quantityItem->setText(getDocument()->formatMoney(value, unit, false));
                    quantityItem->setData(101, value);
                }
            } else {
                auto value = quantityItem->data(101).toDouble();
                quantityItem->setText(getDocument()->formatMoney(value, unit, false));
            }
        }
    }
    ui.kSubOperationsTable->blockSignals(previous); // Reenable signals
}

void SKGOperationPluginWidget::onQuantityChanged()
{
    SKGTRACEINFUNC(10)
    int nbSubOperations = ui.kSubOperationsTable->rowCount();

    bool previous = ui.kSubOperationsTable->blockSignals(true); // Disable signals so that filling cell doesn't create new lines
    if (sender() == ui.kAmountEdit) {
        // Update the total amount
        m_tableDelegate->addParameterValue(QStringLiteral("total"), ui.kAmountEdit->value());

        // Refresh computed amounts
        refreshSubOperationAmount();
    }

    // This code put the remaining quantity on the all sub transactions with the same ratios ^^^
    // Specific code for the last one to avoid "round" error
    QTableWidgetItem *remainingQuantityItem = ui.kSubOperationsTable->item(nbSubOperations - 1, m_attributesForSplit.indexOf(QStringLiteral("f_value")));
    if (remainingQuantityItem != nullptr) {
        // 348490 vvv
        double remain = remainingQuantityItem->data(101).toDouble() + getRemainingQuantity();
        if (qAbs(remain) < 1e-10) {
            onRemoveSubOperation(nbSubOperations - 1);
        } else {
            auto unit = ui.kUnitEdit->getUnit().getUnitInfo();
            unit.Value = 1.0;
            remainingQuantityItem->setText(getDocument()->formatMoney(remain, unit, false));
            remainingQuantityItem->setData(101, remain);
            remainingQuantityItem->setToolTip(SKGServices::doubleToString(remain));
        }
        // 348490 ^^^
    }
    ui.kSubOperationsTable->blockSignals(previous); // Reenable signals
}

void SKGOperationPluginWidget::onSubopCellChanged(int row, int column)
{
    SKGTRACEINFUNC(10)
    QTableWidgetItem *subop_cell = ui.kSubOperationsTable->item(row, column);
    QBrush base_brush = ui.kSubOperationsTable->item(row, 0)->foreground();

    if (column == m_attributesForSplit.indexOf(QStringLiteral("f_value"))) {
        // If the quantity in the last line is edited, we add a new
        // line with the new remaining quantity
        addSubOperationLine(ui.kSubOperationsTable->rowCount(), ui.kDateEdit->date(), QString(), QString(), QString(), 0, QString());

        if (subop_cell->data(101).toDouble() != 0) {
            onQuantityChanged();
        } else {
            base_brush = KColorScheme(QPalette::Normal).foreground(KColorScheme::NegativeText);
        }
        subop_cell->setForeground(base_brush);

        refreshSubOperationAmount();
    }
}

void SKGOperationPluginWidget::onRemoveSubOperation(int iRow)
{
    SKGTRACEINFUNC(10)
    bool previous = ui.kSubOperationsTable->blockSignals(true);
    ui.kSubOperationsTable->removeRow(iRow);

    // If all rows removed, add an empty line
    if (ui.kSubOperationsTable->rowCount() == 0) {
        addSubOperationLine(0, ui.kDateEdit->date(), QString(), QString(), QString(), 0, QString());
    }

    if (!previous) {
        onQuantityChanged();
    }
    ui.kSubOperationsTable->blockSignals(previous);
}

void SKGOperationPluginWidget::onRotateAccountTools()
{
    SKGTRACEINFUNC(10)
    if (m_modeInfoZone == 0) {
        displayReconciliationInfo();
    } else {
        displayBalance();
    }
}

void SKGOperationPluginWidget::displayBalance()
{
    if (m_modeInfoZone != 0) {
        ui.kReconciliatorFrame2->hide();
        ui.kInfo->show();
        m_modeInfoZone = 0;
        onRefreshInformationZoneDelayed();
    }
}

void SKGOperationPluginWidget::displayReconciliationInfo()
{
    if (!currentAccount().isEmpty()) {
        // Only show reconciliation info if only one account is displayed
        ui.kReconciliatorFrame2->show();
        ui.kInfo->hide();
        m_modeInfoZone = 1;
        onRefreshInformationZoneDelayed();
    } else {
        // If more than one account is displayed, skip reconciliation mode
        // (it doesn't make sense to reconcile several accounts at once)
        // and move to the next modeInfoZone
        m_modeInfoZone = 1;
        onRotateAccountTools();
    }
}

void SKGOperationPluginWidget::onAutoPoint()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Auto mark account"), err)
        SKGAccountObject act(getDocument());
        err = act.setName(currentAccount());
        IFOKDO(err, act.load())
        IFOKDO(err, act.autoReconcile(ui.kReconcilitorAmountEdit->value()))

        // Send message
        IFOKDO(
            err,
            act.getDocument()->sendMessage(i18nc("An information message", "The account '%1' has been auto marked", act.getDisplayName()), SKGDocument::Hidden))
    }
    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Account auto marked.")))

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPluginWidget::onAddFakeOperation()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Create fake transaction"), err)

        SKGAccountObject accountObj(getDocument());
        IFOKDO(err, accountObj.setName(currentAccount()))
        IFOKDO(err, accountObj.load())

        SKGOperationObject op;
        IFOKDO(err, accountObj.addOperation(op))
        IFOKDO(err, op.setDate(QDate::currentDate()))
        IFOKDO(err, op.setComment(skgoperation_settings::commentFakeOperation()))
        QString payee = skgoperation_settings::payeeFakeOperation();
        if (!payee.isEmpty()) {
            SKGPayeeObject p;
            IFOKDO(err, SKGPayeeObject::createPayee(qobject_cast<SKGDocumentBank *>(getDocument()), payee, p, true))
            IFOKDO(err, op.setPayee(p))
        }

        SKGUnitObject unit;
        IFOKDO(err, accountObj.getUnit(unit))
        IFOKDO(err, op.setUnit(unit))
        if (skgoperation_settings::automaticPointInReconciliation() && m_modeInfoZone == 1) {
            IFOKDO(err, op.setStatus(SKGOperationObject::MARKED))
        }
        IFOKDO(err, op.save())

        SKGSubOperationObject sop;
        IFOKDO(err, op.addSubOperation(sop))

        SKGStringListList listTmp2;
        double diff = 0;
        getDocument()->executeSelectSqliteOrder("SELECT f_CHECKEDANDPOINTED from v_account_display WHERE t_name='"
                                                    % SKGServices::stringToSqlString(currentAccount()) % u'\'',
                                                listTmp2);
        if (listTmp2.count() == 2) {
            const auto div = unit.getAmount();
            if (div != 0)
                diff = SKGServices::stringToDouble(listTmp2.at(1).at(0)) / div - ui.kReconcilitorAmountEdit->value();
        }

        IFOKDO(err, sop.setQuantity(-diff))
        IFOKDO(err, sop.setComment(skgoperation_settings::commentFakeOperation()))
        QString category = skgoperation_settings::categoryFakeOperation();
        if (!category.isEmpty()) {
            SKGCategoryObject c;
            IFOKDO(err, SKGCategoryObject::createPathCategory(qobject_cast<SKGDocumentBank *>(getDocument()), category, c, true))
            IFOKDO(err, sop.setCategory(c))
        }
        IFOKDO(err, sop.save())

        // Send message
        IFOKDO(err,
               op.getDocument()->sendMessage(
                   i18nc("An information to the user that something was added", "The transaction '%1' has been added", op.getDisplayName()),
                   SKGDocument::Hidden))
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Fake transaction created.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Creation failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPluginWidget::onValidateMarkedOperations()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    QString account = currentAccount();
    if (!account.isEmpty()) {
        // Get reconciled account
        SKGAccountObject act(getDocument());
        IFOKDO(err, act.setName(account))
        IFOKDO(err, act.load())

        QString bindAccount = ui.kReconciliateAccount->currentText();

        if (act.getType() == SKGAccountObject::CREDITCARD && !bindAccount.isEmpty()) {
            //
            IFOK(err)
            {
                SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Switch to checked"), err, 3)
                SKGAccountObject accountObj2(getDocument());
                IFOKDO(err, accountObj2.setName(bindAccount))
                IFOKDO(err, accountObj2.load())
                IFOKDO(err, getDocument()->stepForward(1))

                IFOKDO(err, act.transferDeferredOperations(accountObj2))
                IFOKDO(err, getDocument()->stepForward(2))

                // Change reconciliation date of the account
                IFOKDO(err, act.setReconciliationDate(QDate::currentDate()))
                IFOKDO(err, act.setReconciliationBalance(ui.kReconcilitorAmountEdit->value()))
                IFOKDO(err, act.setLinkedAccount(accountObj2))
                IFOKDO(err, act.save())

                // Send message
                IFOKDO(err,
                       act.getDocument()->sendMessage(i18nc("An information message", "The account '%1' is reconciled", act.getDisplayName()),
                                                      SKGDocument::Positive))

                IFOKDO(err, getDocument()->stepForward(3))
            }
        } else {
            // Change state of all transactions
            SKGObjectBase::SKGListSKGObjectBase list;
            IFOKDO(err,
                   getDocument()->getObjects(QStringLiteral("v_operation_display"),
                                             "t_status='P' AND t_ACCOUNT='" % SKGServices::stringToSqlString(account) % QLatin1Char('\''),
                                             list))
            int nb = list.count();
            IFOK(err)
            {
                SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Switch to checked"), err, nb + 1)

                if (act.getType() == SKGAccountObject::CREDITCARD && bindAccount.isEmpty()) {
                    // Reset the linked account
                    IFOKDO(err, act.setLinkedAccount(SKGAccountObject()))
                    IFOKDO(err, act.save())
                }

                for (int i = 0; !err && i < nb; ++i) {
                    // Set transaction checked
                    SKGOperationObject op(list.at(i));
                    err = op.setStatus(SKGOperationObject::CHECKED);
                    IFOKDO(err, op.save())

                    // Send message
                    IFOKDO(err,
                           op.getDocument()->sendMessage(i18nc("An information message", "The transaction '%1' has been checked", op.getDisplayName()),
                                                         SKGDocument::Hidden))

                    IFOKDO(err, getDocument()->stepForward(i + 1))
                }

                // Change reconciliation date of the account
                IFOKDO(err, act.setReconciliationDate(QDate::currentDate()))
                IFOKDO(err, act.setReconciliationBalance(ui.kReconcilitorAmountEdit->value()))
                IFOKDO(err, act.save())

                // Send message
                IFOKDO(err,
                       act.getDocument()->sendMessage(i18nc("An information message", "The account '%1' is reconciled", act.getDisplayName()),
                                                      SKGDocument::Positive))

                IFOKDO(err, getDocument()->stepForward(nb + 1))
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transaction checked.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Switch failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPluginWidget::addSubOperationLine(int row,
                                                   QDate date,
                                                   const QString &category,
                                                   const QString &tracker,
                                                   const QString &comment,
                                                   double quantity,
                                                   const QString &formula,
                                                   int id)
{
    SKGTRACEINFUNC(10)
    bool previous = ui.kSubOperationsTable->blockSignals(true);

    ui.kSubOperationsTable->insertRow(row);

    // Add a delete icon on the line:
    auto hitem = new QTableWidgetItem(SKGServices::fromTheme(QStringLiteral("edit-delete")), QString());
    ui.kSubOperationsTable->setVerticalHeaderItem(row, hitem);
    QHeaderView *headerView = ui.kSubOperationsTable->verticalHeader();
    headerView->setSectionsMovable(true);

    // Category
    auto categoryItem = new QTableWidgetItem(category);
    categoryItem->setToolTip(category);
    categoryItem->setData(Qt::UserRole, id);
    ui.kSubOperationsTable->setItem(row, m_attributesForSplit.indexOf(QStringLiteral("t_category")), categoryItem);

    // Comment
    auto commentItem = new QTableWidgetItem(comment);
    commentItem->setToolTip(comment);
    ui.kSubOperationsTable->setItem(row, m_attributesForSplit.indexOf(QStringLiteral("t_comment")), commentItem);

    // Quantity
    auto unit = ui.kUnitEdit->getUnit().getUnitInfo();
    unit.Value = 1.0;
    auto quantityItem = new QTableWidgetItem(getDocument()->formatMoney(quantity, unit, false));
    quantityItem->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
    quantityItem->setData(101, quantity);
    quantityItem->setToolTip(formula.isEmpty() ? SKGServices::doubleToString(quantity) : formula);
    ui.kSubOperationsTable->setItem(row, m_attributesForSplit.indexOf(QStringLiteral("f_value")), quantityItem);

    // Refund
    auto trackerItem = new QTableWidgetItem(tracker);
    trackerItem->setToolTip(tracker);
    categoryItem->setData(Qt::UserRole, id);
    ui.kSubOperationsTable->setItem(row, m_attributesForSplit.indexOf(QStringLiteral("t_refund")), trackerItem);

    // Date
    auto dateItem = new QTableWidgetItem(SKGMainPanel::dateToString(date));
    dateItem->setToolTip(SKGServices::dateToSqlString(date));
    ui.kSubOperationsTable->setItem(row, m_attributesForSplit.indexOf(QStringLiteral("d_date")), dateItem);

    ui.kSubOperationsTable->blockSignals(previous);

    ui.kSubOperationsTable->resizeColumnsToContents();
    ui.kSubOperationsTable->horizontalHeader()->setStretchLastSection(true);
    if (row == 0 && category.isEmpty()) {
        ui.kSubOperationsTable->horizontalHeader()->resizeSection(0, 300);
    }
}

QWidget *SKGOperationPluginWidget::mainWidget()
{
    return ui.kOperationView->getView();
}

SKGError SKGOperationPluginWidget::getSelectedOperation(SKGOperationObject &operation)
{
    SKGError err;
    SKGObjectBase::SKGListSKGObjectBase selectedOperations = getSelectedObjects();
    if (!selectedOperations.isEmpty()) {
        operation = selectedOperations.at(0);
        err.setReturnCode(0);
    } else {
        err.setReturnCode(1).setMessage(i18nc("Error message", "No Transaction Selected"));
    }
    return err;
}

void SKGOperationPluginWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0 || sender() == ui.kCleanBtn) {
        ui.kOperationView->getView()->clearSelection();
        ui.kDateEdit->setDate(QDate::currentDate());
        ui.kPayeeEdit->setText(QString());
        ui.kCategoryEdit->setText(QString());
        ui.kTrackerEdit->setText(QString());
        ui.kAmountEdit->setText(QString());
        ui.kTypeEdit->setText(QString());
        ui.kCommentEdit->setText(QString());
        ui.kNumberEdit->setText(QString());

        if (!currentAccount().isEmpty()) {
            ui.kAccountEdit->setText(currentAccount());
        }

        // BUG 376025 vvvv
        ui.kUnitEdit->setDocument(qobject_cast<SKGDocumentBank *>(getDocument()));
        // BUG 376025 ^^^^
        ui.kUnitShare->setDocument(qobject_cast<SKGDocumentBank *>(getDocument()));

        setAllWidgetsEnabled();
        m_previousDate = QDate::currentDate();
    }
    if (sender() == ui.kCleanBtn) {
        ui.kWidgetSelector->setSelectedMode(0);
    }
}

bool SKGOperationPluginWidget::isEditor()
{
    return true;
}

void SKGOperationPluginWidget::activateEditor()
{
    if (ui.kWidgetSelector->getSelectedMode() == -1) {
        ui.kWidgetSelector->setSelectedMode(0);
    }
    ui.kPayeeEdit->setFocus();
}
