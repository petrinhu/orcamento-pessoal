/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for unit management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgunitpluginwidget.h"

#include <KNSWidgets/Dialog>
#include <klocalizedstring.h>
#include <kzip.h>
#include <qdir.h>
#include <qdom.h>
#include <qevent.h>
#include <qfile.h>
#include <qsortfilterproxymodel.h>
#include <qstandardpaths.h>
#include <qurl.h>
#include <qvalidator.h>

#include "skgbankincludes.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgunit_settings.h"

SKGUnitPluginWidget::SKGUnitPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument)
    : SKGTabPage(iParent, iDocument)
    , m_upload(nullptr)
    , m_unitValueGraphCmb(nullptr)
{
    SKGTRACEINFUNC(10)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);
    ui.kGraph->setShadowVisible(false);
    m_unitValueGraphCmb = new SKGComboBox(this);
    m_unitValueGraphCmb->addItem(i18nc("A mode of graph of unit values", "Unit values"));
    m_unitValueGraphCmb->addItem(i18nc("A mode of graph of unit values", "Amount owned"));
    connect(m_unitValueGraphCmb, static_cast<void (SKGComboBox::*)(int)>(&SKGComboBox::currentIndexChanged), this, &SKGUnitPluginWidget::onSelectionChanged);
    ui.kGraph->graph()->addToolbarWidget(m_unitValueGraphCmb);

    ui.kNameLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_name"))));
    ui.kDecimalLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("i_nbdecimal"))));
    ui.kCountyLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_country"))));
    ui.kSymbolLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_symbol"))));
    ui.kTypeLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_type"))));
    ui.kInternetLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_internet_code"))));
    ui.kDateLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("d_date"))));
    ui.kAmountLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("f_value"))));
    ui.kUnitLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_UNIT"))));
    ui.kUnitLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_UNIT"))));
    ui.kDownloadSourceLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_source"))));

    ui.kUnitTableViewEdition->getShowWidget()->addItem(QStringLiteral("all"),
                                                       i18nc("Noun, items to display", "All"),
                                                       QString(),
                                                       QString(),
                                                       QStringLiteral("currency;share;index;object"), // Check when checked
                                                       QStringLiteral("highlighted"), // Uncheck when checked
                                                       QString(),
                                                       QString(),
                                                       Qt::META | Qt::Key_A);
    ui.kUnitTableViewEdition->getShowWidget()->addItem(QStringLiteral("highlighted"),
                                                       i18n("Highlighted only"),
                                                       QStringLiteral("bookmarks"),
                                                       QStringLiteral("t_bookmarked='Y'"),
                                                       QString(),
                                                       QStringLiteral("all;currency;share;index;object"),
                                                       QStringLiteral("all"),
                                                       QString(),
                                                       Qt::META | Qt::Key_H);
    ui.kUnitTableViewEdition->getShowWidget()->addSeparator();
    ui.kUnitTableViewEdition->getShowWidget()->addItem(QStringLiteral("currency"),
                                                       i18nc("Noun, a country's currency", "Currency"),
                                                       QString(),
                                                       QStringLiteral("t_type IN ('1','2','C')"),
                                                       QString(), // Check when checked
                                                       QStringLiteral("highlighted"), // Uncheck when checked
                                                       QString(), // Check when unchecked
                                                       QStringLiteral("all"),
                                                       Qt::META | Qt::Key_C); // Uncheck when unchecked
    ui.kUnitTableViewEdition->getShowWidget()->addItem(QStringLiteral("share"),
                                                       i18nc("Noun, a financial share, as in a stock market", "Share"),
                                                       QString(),
                                                       QStringLiteral("t_type='S'"),
                                                       QString(),
                                                       QStringLiteral("highlighted"),
                                                       QString(),
                                                       QStringLiteral("all"),
                                                       Qt::META | Qt::Key_S);
    ui.kUnitTableViewEdition->getShowWidget()->addItem(QStringLiteral("index"),
                                                       i18nc("Noun, a financial index like the Dow Jones, NASDAQ, CAC40…", "Index"),
                                                       QString(),
                                                       QStringLiteral("t_type='I'"),
                                                       QString(),
                                                       QStringLiteral("highlighted"),
                                                       QString(),
                                                       QStringLiteral("all;current"),
                                                       Qt::META | Qt::Key_I);
    ui.kUnitTableViewEdition->getShowWidget()->addItem(QStringLiteral("object"),
                                                       i18nc("Noun, a physical object like a house or a car", "Object"),
                                                       QString(),
                                                       QStringLiteral("t_type='O'"),
                                                       QString(),
                                                       QStringLiteral("highlighted"),
                                                       QString(),
                                                       QStringLiteral("all"),
                                                       Qt::META | Qt::Key_O);

    ui.kUnitTableViewEdition->getShowWidget()->setDefaultState(QStringLiteral("all;currency;share;index;object"));

    ui.kGraph->getShowWidget()->setState(QStringLiteral("\"graph\""));
    ui.kGraph->setFilterVisibility(false);

    ui.kUnitCreatorUnit->setDocument(iDocument);
    ui.kUnitCreatorUnit->setWhereClauseCondition(QStringLiteral("t_type IN ('1','2','C')"));

    // Add Standard KDE Icons to buttons to Accounts
    ui.kUnitAdd->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));
    ui.kUnitUpdate->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kUnitValueDownload->setIcon(SKGServices::fromTheme(QStringLiteral("download")));
    ui.kDeleteSource->setIcon(SKGServices::fromTheme(QStringLiteral("edit-delete")));
    ui.kGetNewHotStuff->setIcon(SKGServices::fromTheme(QStringLiteral("get-hot-new-stuff")));

    auto newValidator = new QRegularExpressionValidator(QRegularExpression(QStringLiteral("^[\\w\\s]+$")), this);
    ui.kDownloadSource->setValidator(newValidator);

    QStringList overlays;
    overlays.push_back(QStringLiteral("list-add"));
    m_upload = new QAction(SKGServices::fromTheme(QStringLiteral("get-hot-new-stuff"), overlays), i18n("Upload"), this);
    connect(m_upload, &QAction::triggered, this, &SKGUnitPluginWidget::onPutNewHotStuff);

    auto menu = new QMenu(this);
    menu->addAction(m_upload);
    ui.kGetNewHotStuff->setMenu(menu);

    ui.kUnitOpen->setIcon(SKGServices::fromTheme(QStringLiteral("quickopen")));
    connect(ui.kUnitOpen, &QPushButton::clicked, this, &SKGUnitPluginWidget::onOpenURL);

    auto downloadLastAction = new QAction(SKGServices::fromTheme(QStringLiteral("download")), i18n("download last value only"), this);
    downloadLastAction->setData(static_cast<int>(SKGUnitObject::LAST));
    connect(downloadLastAction, &QAction::triggered, this, &SKGUnitPluginWidget::onDownloadUnitValue);

    auto downloadLastMonthlyAction =
        new QAction(SKGServices::fromTheme(QStringLiteral("download")), i18n("download monthly values since last known value"), this);
    downloadLastMonthlyAction->setData(static_cast<int>(SKGUnitObject::LAST_MONTHLY));
    connect(downloadLastMonthlyAction, &QAction::triggered, this, &SKGUnitPluginWidget::onDownloadUnitValue);

    auto downloadLastWeeklyAction =
        new QAction(SKGServices::fromTheme(QStringLiteral("download")), i18n("download weekly values since last known value"), this);
    downloadLastWeeklyAction->setData(static_cast<int>(SKGUnitObject::LAST_WEEKLY));
    connect(downloadLastWeeklyAction, &QAction::triggered, this, &SKGUnitPluginWidget::onDownloadUnitValue);

    auto downloadLastDailyAction = new QAction(SKGServices::fromTheme(QStringLiteral("download")), i18n("download daily values since last known value"), this);
    downloadLastDailyAction->setData(static_cast<int>(SKGUnitObject::LAST_DAILY));
    connect(downloadLastDailyAction, &QAction::triggered, this, &SKGUnitPluginWidget::onDownloadUnitValue);

    auto downloadMonthlyAction = new QAction(SKGServices::fromTheme(QStringLiteral("download")), i18n("download all monthly values"), this);
    downloadMonthlyAction->setData(static_cast<int>(SKGUnitObject::ALL_MONTHLY));
    connect(downloadMonthlyAction, &QAction::triggered, this, &SKGUnitPluginWidget::onDownloadUnitValue);

    auto downloadWeeklyAction = new QAction(SKGServices::fromTheme(QStringLiteral("download")), i18n("download all weekly values"), this);
    downloadWeeklyAction->setData(static_cast<int>(SKGUnitObject::ALL_WEEKLY));
    connect(downloadWeeklyAction, &QAction::triggered, this, &SKGUnitPluginWidget::onDownloadUnitValue);

    auto downloadDailyAction = new QAction(SKGServices::fromTheme(QStringLiteral("download")), i18n("download all daily values"), this);
    downloadDailyAction->setData(static_cast<int>(SKGUnitObject::ALL_DAILY));
    connect(downloadDailyAction, &QAction::triggered, this, &SKGUnitPluginWidget::onDownloadUnitValue);

    auto simplifyAction = new QAction(SKGServices::fromTheme(QStringLiteral("edit-delete")), i18n("simplify values"), this);
    connect(simplifyAction, &QAction::triggered, this, &SKGUnitPluginWidget::onSimplify);

    auto downloadMenu = new QMenu(this);
    downloadMenu->addAction(downloadLastAction);
    downloadMenu->addAction(downloadLastMonthlyAction);
    downloadMenu->addAction(downloadLastWeeklyAction);
    downloadMenu->addAction(downloadLastDailyAction);
    downloadMenu->addAction(downloadMonthlyAction);
    downloadMenu->addAction(downloadWeeklyAction);
    downloadMenu->addAction(downloadDailyAction);

    downloadMenu->addAction(simplifyAction);
    ui.kUnitValueDownload->setMenu(downloadMenu);
    connect(ui.kUnitValueDownload, &QToolButton::clicked, this, &SKGUnitPluginWidget::onDownloadUnitValue);
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.kStandardFrm);
        list.push_back(ui.kBtnFrm);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("dialog-ok")),
                                      i18n("Standard"),
                                      i18n("Display the edit panel for standard units"),
                                      list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.kManualFrm);
        list.push_back(ui.kBtnFrm);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("user-properties")),
                                      i18n("Manual / Share"),
                                      i18n("Display the edit panel for manual units"),
                                      list);
    }
    {
        SKGWidgetSelector::SKGListQWidget list;
        list.push_back(ui.kValuesFrm);
        list.push_back(ui.kBtnFrm);
        ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("taxes-finances")),
                                      i18n("Values"),
                                      i18n("Display the edit panel for values of units"),
                                      list);
    }
    connect(ui.kWidgetSelector, &SKGWidgetSelector::selectedModeChanged, this, &SKGUnitPluginWidget::onUnitCreatorModified);

    // Fill combo box for type
    ui.kTypeCreatorUnit->addItem(i18nc("Noun", "Primary currency"), static_cast<int>(SKGUnitObject::PRIMARY));
    ui.kTypeCreatorUnit->addItem(i18nc("Noun", "Secondary currency"), static_cast<int>(SKGUnitObject::SECONDARY));
    ui.kTypeCreatorUnit->addItem(i18nc("Noun, a country's currency", "Currency"), static_cast<int>(SKGUnitObject::CURRENCY));
    ui.kTypeCreatorUnit->addItem(i18nc("Noun, a financial share, as in a stock market", "Share"), static_cast<int>(SKGUnitObject::SHARE));
    ui.kTypeCreatorUnit->addItem(i18nc("Noun, a financial index like the Dow Jones, NASDAQ, CAC40…", "Index"), static_cast<int>(SKGUnitObject::INDEX));
    ui.kTypeCreatorUnit->addItem(i18nc("Noun, a physical object like a house or a car", "Object"), static_cast<int>(SKGUnitObject::OBJECT));
    bool primaryUnit, secondaryUnit;
    iDocument->existObjects(QStringLiteral("unit"), QStringLiteral("t_type='1'"), primaryUnit);
    iDocument->existObjects(QStringLiteral("unit"), QStringLiteral("t_type='2'"), secondaryUnit);
    if (primaryUnit) {
        if (secondaryUnit) {
            ui.kTypeCreatorUnit->setCurrentIndex(2);
        } else {
            ui.kTypeCreatorUnit->setCurrentIndex(1);
        }
    } else {
        ui.kTypeCreatorUnit->setCurrentIndex(0);
    }

    // Bind unit creation view
    {
        ui.kUnitTableViewEdition->setModel(
            new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()), QStringLiteral("v_unit_display"), QString(), this, QString(), false));

        connect(ui.kUnitTableViewEdition->getView(), &SKGTreeView::clickEmptyArea, this, &SKGUnitPluginWidget::cleanEditor);
        connect(ui.kUnitTableViewEdition->getView(),
                &SKGTreeView::doubleClicked,
                SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(),
                &QAction::trigger);
        connect(ui.kUnitTableViewEdition->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
            this->onSelectionChanged();
        });
    }

    // Bind unit creation view
    {
        auto objectModel2 = new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()),
                                               QStringLiteral("v_unitvalue_display"),
                                               QStringLiteral("1=0"),
                                               this,
                                               QString(),
                                               false);
        ui.kUnitValueTableViewEdition->setModel(objectModel2);
        connect(ui.kUnitValueTableViewEdition, &SKGTableView::selectionChangedDelayed, this, &SKGUnitPluginWidget::onSelectionValueChanged);
    }

    // Refresh
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGUnitPluginWidget::dataModified, Qt::QueuedConnection);
    ui.kWidgetSelector->setSelectedMode(0);

    // Fill combo box for reference currency
    fillSourceList();
    dataModified(QString(), 0);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    // Synchronize zooms of both tables
    connect(ui.kUnitTableViewEdition->getView(), &SKGTreeView::zoomChanged, ui.kUnitValueTableViewEdition, &SKGTreeView::setZoomPosition);
    connect(ui.kUnitValueTableViewEdition, &SKGTreeView::zoomChanged, ui.kUnitTableViewEdition->getView(), &SKGTreeView::setZoomPosition);

    // Other connects
    connect(ui.kNameCreatorUnit, &QLineEdit::textChanged, this, &SKGUnitPluginWidget::onUnitCreatorModified);
    connect(ui.kUnitAdd, &QPushButton::clicked, this, &SKGUnitPluginWidget::onAddUnit);
    connect(ui.kUnitUpdate, &QPushButton::clicked, this, &SKGUnitPluginWidget::onModifyUnit);
    connect(ui.kInternetCreatorUnit, &QLineEdit::textChanged, this, &SKGUnitPluginWidget::onUnitCreatorModified);
    connect(ui.kSymbolCreatorUnit, &QLineEdit::textChanged, this, &SKGUnitPluginWidget::onUnitCreatorModified);
    connect(ui.kObsolete, &QCheckBox::toggled, this, &SKGUnitPluginWidget::refreshUnitList);
    connect(ui.kAmountEdit, &SKGCalculatorEdit::textChanged, this, &SKGUnitPluginWidget::onUnitCreatorModified);
    connect(ui.kDeleteSource, &QToolButton::clicked, this, &SKGUnitPluginWidget::onDeleteSource);
    connect(ui.kDownloadSource, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::returnPressed), this, &SKGUnitPluginWidget::onAddSource);
    connect(ui.kDownloadSource, &SKGComboBox::editTextChanged, this, &SKGUnitPluginWidget::onSourceChanged);
    connect(ui.kGetNewHotStuff, &QToolButton::clicked, this, &SKGUnitPluginWidget::onGetNewHotStuff);

    // Fill combo box for currency
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGUnitPluginWidget::refreshUnitList);
    m_timer.start(500);

    onSourceChanged();
}

SKGUnitPluginWidget::~SKGUnitPluginWidget()
{
    SKGTRACEINFUNC(10)
}

bool SKGUnitPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u && ui.kUnitAdd->isEnabled()) {
                ui.kUnitAdd->click();
            } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kUnitUpdate->isEnabled()) {
                ui.kUnitUpdate->click();
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

void SKGUnitPluginWidget::fillSourceList()
{
    // Get previous selected item
    QString current = ui.kDownloadSource->text();

    // Fill
    ui.kDownloadSource->clear();
    ui.kDownloadSource->addItems(SKGUnitObject::downloadSources());
    ui.kDeleteSource->hide();

    // Set previous selected itemData
    if (!current.isEmpty() && ui.kDownloadSource->contains(current)) {
        ui.kDownloadSource->setCurrentItem(current);
    }
}

void SKGUnitPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    // Mapping
    SKGUnitObject unit;

    int nbSelect = ui.kUnitTableViewEdition->getView()->getNbSelectedObjects();
    if (nbSelect == 1) {
        unit = ui.kUnitTableViewEdition->getView()->getFirstSelectedObject();
        ui.kNameCreatorUnit->setText(unit.getName());
        ui.kSymbolCreatorUnit->setText(unit.getSymbol());
        ui.kCountryCreatorUnit->setText(unit.getCountry());
        ui.kTypeCreatorUnit->setCurrentIndex(ui.kTypeCreatorUnit->findData(static_cast<int>(unit.getType())));
        ui.kInternetCreatorUnit->setText(unit.getInternetCode());
        ui.kUnitCreatorUnit->setText(unit.getAttribute(QStringLiteral("t_UNIT")));
        ui.kNbDecimal->setValue(unit.getNumberDecimal());
        ui.kDownloadSource->setText(unit.getDownloadSource());
    } else if (nbSelect > 1) {
        ui.kNameCreatorUnit->setText(NOUPDATE);
        ui.kSymbolCreatorUnit->setText(NOUPDATE);
        ui.kCountryCreatorUnit->setText(NOUPDATE);
        ui.kTypeCreatorUnit->setText(NOUPDATE);
        ui.kInternetCreatorUnit->setText(NOUPDATE);
        ui.kUnitCreatorUnit->setText(NOUPDATE);
        ui.kDownloadSource->setText(NOUPDATE);
    }
    ui.kUnitValueFrame->setEnabled(nbSelect == 1);
    ui.kUnitValueDownload->setEnabled(nbSelect > 0);
    ui.kUnitOpen->setEnabled(nbSelect > 0);

    // Fill values
    QString wc = "rd_unit_id=(select id from unit where t_name='" % SKGServices::stringToSqlString(ui.kNameCreatorUnit->text()) % "')";
    auto *objectModel = qobject_cast<SKGObjectModel *>(ui.kUnitValueTableViewEdition->model());
    if (objectModel != nullptr) {
        objectModel->setFilter(QString()); // Correction 2299600: to be sure that refresh will be done
        objectModel->setFilter(wc % " order by d_date desc");
        objectModel->refresh();
    }

    ui.kUnitOfUnitLbl->setText(ui.kUnitCreatorUnit->text());

    // Draw plot
    SKGStringListList table;
    getDocument()->getConsolidatedView(QStringLiteral("v_unitvalue_display"),
                                       QStringLiteral("d_date"),
                                       QStringLiteral("t_UNIT"),
                                       m_unitValueGraphCmb->currentIndex() == 0 ? QStringLiteral("f_quantity") : QStringLiteral("f_AMOUNTOWNED"),
                                       QStringLiteral("TOTAL"),
                                       wc % " AND d_date>(date('now', 'localtime', '-50 year')) AND d_date<(date('now', 'localtime', '+50 year'))",
                                       table,
                                       QString());

    SKGServices::SKGUnitInfo primaryUnit = qobject_cast<SKGDocumentBank *>(getDocument())->getPrimaryUnit();
    SKGServices::SKGUnitInfo secondaryUnit = qobject_cast<SKGDocumentBank *>(getDocument())->getSecondaryUnit();
    if (unit.getType() == SKGUnitObject::INDEX) {
        primaryUnit.Symbol = QString();
        secondaryUnit.Symbol = QString();
    } else {
        SKGUnitObject parentUnitObject;
        unit.getUnit(parentUnitObject);
        SKGServices::SKGUnitInfo parentUnit = parentUnitObject.getUnitInfo();
        if (primaryUnit.Symbol != parentUnit.Symbol) {
            secondaryUnit = primaryUnit;
            primaryUnit = std::move(parentUnit);

            secondaryUnit.Value = 1.0 / primaryUnit.Value;
            primaryUnit.Value = 1;
        }
    }
    ui.kGraph->setData(table, primaryUnit, secondaryUnit, SKGTableWithGraph::LIMITS);

    // Correction bug 2299394 vvv
    if (ui.kUnitValueTableViewEdition->isAutoResized()) {
        ui.kUnitValueTableViewEdition->resizeColumnsToContentsDelayed();
    }
    // Correction bug 2299394 ^^^

    onUnitCreatorModified();
    Q_EMIT selectionChanged();
}

void SKGUnitPluginWidget::onSelectionValueChanged()
{
    SKGTRACEINFUNC(10)
    // Mapping
    QItemSelectionModel *selModel = ui.kUnitValueTableViewEdition->selectionModel();
    if (selModel != nullptr) {
        QModelIndexList indexes = selModel->selectedRows();
        int nbSelect = indexes.count();
        if (nbSelect != 0) {
            QModelIndex idx = indexes[indexes.count() - 1];

            auto *model = qobject_cast<SKGObjectModel *>(ui.kUnitValueTableViewEdition->model());
            if (model != nullptr) {
                SKGUnitValueObject unitValue(model->getObject(idx));
                SKGUnitObject unit;
                unitValue.getUnit(unit);

                ui.kDateEdit->setDate(unitValue.getDate());
                ui.kAmountEdit->setText(SKGServices::toCurrencyString(SKGServices::stringToDouble(unitValue.getAttribute(QStringLiteral("f_quantity"))),
                                                                      QString(),
                                                                      SKGServices::stringToInt(unit.getAttribute(QStringLiteral("i_nbdecimal")))));
            }
        } else {
            ui.kDateEdit->setDate(QDate::currentDate());
            ui.kAmountEdit->setText(QString());
        }
        Q_EMIT selectionChanged();
    }
}

void SKGUnitPluginWidget::onUnitCreatorModified()
{
    SKGTRACEINFUNC(10)

    bool activated = ui.kWidgetSelector->getSelectedMode() != -1 && !ui.kNameCreatorUnit->text().isEmpty() && !ui.kSymbolCreatorUnit->text().isEmpty();

    int nbSelect = getNbSelectedObjects();

    ui.kUnitAdd->setEnabled((activated && (ui.kAmountEdit->valid() || ui.kWidgetSelector->getSelectedMode() != 2))
                            || ui.kWidgetSelector->getSelectedMode() == 0);
    ui.kUnitUpdate->setEnabled(activated && nbSelect > 0 && ui.kWidgetSelector->getSelectedMode() == 1);

    ui.kWidgetSelector->setEnabledMode(2, nbSelect == 1);
    if (!(activated && nbSelect > 0) && ui.kWidgetSelector->getSelectedMode() == 2) {
        ui.kWidgetSelector->setSelectedMode(0);
    }
}

void SKGUnitPluginWidget::onAddUnit()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    int mode = ui.kWidgetSelector->getSelectedMode();

    if (mode == 0) {
        QString untiname = ui.kCurrencyList->text();
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Unit creation '%1'", untiname), err)
        SKGUnitObject oUnit;
        err = SKGUnitObject::createCurrencyUnit(qobject_cast<SKGDocumentBank *>(getDocument()), ui.kCurrencyList->text(), oUnit);
    } else if (mode == 1) {
        QString untiname = ui.kNameCreatorUnit->text();
        SKGUnitObject unitObj(getDocument());
        {SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Unit creation '%1'", untiname), err)

         // Create unit object
         IFOKDO(err, unitObj.setName(untiname)) IFOKDO(err, unitObj.setSymbol(ui.kSymbolCreatorUnit->text()))
             IFOKDO(err, unitObj.setCountry(ui.kCountryCreatorUnit->text())) IFOKDO(err, unitObj.setInternetCode(ui.kInternetCreatorUnit->text()))
                 IFOKDO(err, unitObj.setType(static_cast<SKGUnitObject::UnitType>(ui.kTypeCreatorUnit->itemData(ui.kTypeCreatorUnit->currentIndex()).toInt())))
                     IFOKDO(err, unitObj.setNumberDecimal(ui.kNbDecimal->value())) IFOKDO(err, unitObj.setUnit(ui.kUnitCreatorUnit->getUnit()))
                         IFOKDO(err, unitObj.setDownloadSource(ui.kDownloadSource->text())) IFOKDO(err, unitObj.save())}

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Unit '%1' created", untiname));
            ui.kUnitTableViewEdition->getView()->selectObject(unitObj.getUniqueID());
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Unit creation failed"));
        }
    } else if (mode == 2) {
        QString untiname = ui.kNameCreatorUnit->text();
        SKGUnitValueObject unitValueObject;
        {SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Unit value creation for '%1'", untiname), err) IFOKDO(
            err,
            qobject_cast<SKGDocumentBank *>(getDocument())->addOrModifyUnitValue(untiname, ui.kDateEdit->date(), ui.kAmountEdit->value(), &unitValueObject))}
        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Unit value created for '%1'", untiname));
            // BUG: doesn't work because of unit table is modified an refreshed
            ui.kUnitValueTableViewEdition->selectObject(unitValueObject.getUniqueID());
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Unit value creation failed"));
        }
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGUnitPluginWidget::onModifyUnit()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    SKGObjectBase::SKGListSKGObjectBase selection = ui.kUnitTableViewEdition->getView()->getSelectedObjects();

    int nb = selection.count();

    {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Unit update"), err, nb)
        auto name = ui.kNameCreatorUnit->text();
        if (name != NOUPDATE && !name.startsWith(QStringLiteral("="))) {
            // Is this name already existing?
            bool messageSent = false;
            SKGUnitObject p(getDocument());
            p.setName(name);
            IFOK(p.load())
            {
                if (selection.indexOf(p) == -1) {
                    // We will have to merge with the existing unit
                    selection.insert(0, p);
                    nb++;

                    getDocument()->sendMessage(
                        i18nc("Information message", "You tried to modify names of selected units to an existing unit. Units have been merged."));
                    messageSent = true;
                }
            }

            // Is it a massive modification of payees to merge them ?
            if (nb > 1) {
                if (!messageSent) {
                    getDocument()->sendMessage(i18nc("Information message", "You tried to modify all names of selected units. Units have been merged."));
                }

                // Do the merge
                SKGUnitObject unitObj1(selection[0]);
                for (int i = 1; !err && i < nb; ++i) {
                    SKGUnitObject unitObj(selection.at(i));
                    err = unitObj1.merge(unitObj);
                }

                // Change selection for the rest of the transaction
                selection.clear();
                selection.push_back(unitObj1);
                nb = 1;
            }
        }

        for (int i = 0; !err && i < nb; ++i) {
            // Modification of unit object
            SKGUnitObject unitObj(selection.at(i));
            IFOKDO(err, unitObj.setName(name))
            IFOKDO(err, unitObj.setSymbol(ui.kSymbolCreatorUnit->text()))
            IFOKDO(err, unitObj.setCountry(ui.kCountryCreatorUnit->text()))
            IFOKDO(err, unitObj.setInternetCode(ui.kInternetCreatorUnit->text()))
            if (!err && ui.kTypeCreatorUnit->text() != NOUPDATE) {
                err = unitObj.setType(static_cast<SKGUnitObject::UnitType>(ui.kTypeCreatorUnit->itemData(ui.kTypeCreatorUnit->currentIndex()).toInt()));
            }
            IFOKDO(err, unitObj.setNumberDecimal(ui.kNbDecimal->value()))
            if (!err && ui.kUnitCreatorUnit->text() != NOUPDATE) {
                err = unitObj.setUnit(ui.kUnitCreatorUnit->getUnit());
            }
            if (!err && ui.kDownloadSource->text() != NOUPDATE) {
                err = unitObj.setDownloadSource(ui.kDownloadSource->text());
            }
            IFOKDO(err, unitObj.save())

            // Send message
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The unit '%1' has been updated", unitObj.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Unit updated")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Unit update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kUnitTableViewEdition->getView()->setFocus();
}

SKGUnitObject::UnitDownloadMode SKGUnitPluginWidget::getDownloadModeFromSettings()
{
    SKGUnitObject::UnitDownloadMode mode = SKGUnitObject::LAST;
    if (skgunit_settings::last()) {
        mode = SKGUnitObject::LAST;
    } else if (skgunit_settings::last_monthly()) {
        mode = SKGUnitObject::LAST_MONTHLY;
    } else if (skgunit_settings::last_weekly()) {
        mode = SKGUnitObject::LAST_WEEKLY;
    } else if (skgunit_settings::last_daily()) {
        mode = SKGUnitObject::LAST_DAILY;
    } else if (skgunit_settings::all_monthly()) {
        mode = SKGUnitObject::ALL_MONTHLY;
    } else if (skgunit_settings::all_weekly()) {
        mode = SKGUnitObject::ALL_WEEKLY;
    } else if (skgunit_settings::all_daily()) {
        mode = SKGUnitObject::ALL_DAILY;
    }
    return mode;
}

SKGError SKGUnitPluginWidget::downloadUnitValue(const SKGUnitObject &iUnit, SKGUnitObject::UnitDownloadMode iMode)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    QString unitname = iUnit.getName();
    QString code = iUnit.getInternetCode();
    auto *doc = qobject_cast<SKGDocumentBank *>(iUnit.getDocument());
    if (!code.isEmpty() && (doc != nullptr)) {
        SKGBEGINTRANSACTION(*doc, i18nc("Noun, name of the user action", "Download values for [%1 (%2)]", unitname, code), err)
        err = const_cast<SKGUnitObject *>(&iUnit)->downloadUnitValue(iMode, skgunit_settings::nb_loaded_values());
    }

    return err;
}

void SKGUnitPluginWidget::onDownloadUnitValue()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    SKGUnitObject::UnitDownloadMode mode = SKGUnitObject::LAST;
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        mode = static_cast<SKGUnitObject::UnitDownloadMode>(act->data().toInt());
    } else {
        mode = SKGUnitPluginWidget::getDownloadModeFromSettings();
    }

    SKGObjectBase::SKGListSKGObjectBase selection = ui.kUnitTableViewEdition->getView()->getSelectedObjects();
    int nb = selection.count();
    if (nb != 0) {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Download values"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGUnitObject unit(selection.at(i));
            err = downloadUnitValue(unit, mode);

            // Send message
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The unit '%1' has been downloaded", unit.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Download done")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Download failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGUnitPluginWidget::onSimplify()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    SKGObjectBase::SKGListSKGObjectBase selection = ui.kUnitTableViewEdition->getView()->getSelectedObjects();
    int nb = selection.count();
    if (nb != 0) {
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Simplify unit values"), err, nb)
        for (int i = 0; !err && i < nb; ++i) {
            SKGUnitObject unit(selection.at(i));
            err = unit.simplify();

            // Send message
            IFOKDO(err,
                   getDocument()->sendMessage(i18nc("An information to the user", "The unit '%1' has been simplified", unit.getDisplayName()),
                                              SKGDocument::Hidden))

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }
    }

    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Simplification done")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Simplification failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

SKGObjectBase::SKGListSKGObjectBase SKGUnitPluginWidget::getSelectedObjects()
{
    SKGObjectBase::SKGListSKGObjectBase output;
    if (ui.kUnitValueTableViewEdition->hasFocus()) {
        output = ui.kUnitValueTableViewEdition->getSelectedObjects();
    }
    if (output.isEmpty()) {
        output = ui.kUnitTableViewEdition->getView()->getSelectedObjects();
    }
    return output;
}

int SKGUnitPluginWidget::getNbSelectedObjects()
{
    int output = 0;
    if (ui.kUnitValueTableViewEdition->hasFocus()) {
        output = ui.kUnitValueTableViewEdition->getNbSelectedObjects();
    }
    if (output == 0) {
        output = ui.kUnitTableViewEdition->getView()->getNbSelectedObjects();
    }
    return output;
}

QString SKGUnitPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("splitter1State"), QString(ui.kMainSplitter->saveState().toHex()));
    root.setAttribute(QStringLiteral("splitter2State"), QString(ui.kValuesSplitter->saveState().toHex()));

    // Memorize table settings
    root.setAttribute(QStringLiteral("unitview"), ui.kUnitTableViewEdition->getState());
    root.setAttribute(QStringLiteral("unitvalueview"), ui.kUnitValueTableViewEdition->getState());
    root.setAttribute(QStringLiteral("currentPage"), SKGServices::intToString(ui.kWidgetSelector->getSelectedMode()));
    root.setAttribute(QStringLiteral("obsolete"), ui.kObsolete->isChecked() ? QStringLiteral("Y") : QStringLiteral("N"));
    root.setAttribute(QStringLiteral("graphSettings"), ui.kGraph->getState());
    root.setAttribute(QStringLiteral("unitvaluegraphmode"), m_unitValueGraphCmb->currentIndex());

    return doc.toString();
}

void SKGUnitPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString splitter1State = root.attribute(QStringLiteral("splitter1State"));
    QString splitter2State = root.attribute(QStringLiteral("splitter2State"));
    QString currentPage = root.attribute(QStringLiteral("currentPage"));
    QString obsolete = root.attribute(QStringLiteral("obsolete"));
    QString unitvaluegraphmode = root.attribute(QStringLiteral("unitvaluegraphmode"));

    if (currentPage.isEmpty()) {
        currentPage = '0';
    }

    if (!splitter1State.isEmpty()) {
        ui.kMainSplitter->restoreState(QByteArray::fromHex(splitter1State.toLatin1()));
    }
    if (!splitter2State.isEmpty()) {
        ui.kValuesSplitter->restoreState(QByteArray::fromHex(splitter2State.toLatin1()));
    }
    ui.kWidgetSelector->setSelectedMode(SKGServices::stringToInt(currentPage));
    ui.kObsolete->setChecked(obsolete == QStringLiteral("Y"));

    ui.kUnitTableViewEdition->setState(root.attribute(QStringLiteral("unitview")));
    ui.kUnitValueTableViewEdition->setState(root.attribute(QStringLiteral("unitvalueview")));
    ui.kGraph->setState(root.attribute(QStringLiteral("graphSettings")));
    ui.kGraph->setGraphType(SKGTableWithGraph::LINE);
    if (!unitvaluegraphmode.isEmpty()) {
        m_unitValueGraphCmb->setCurrentIndex(SKGServices::stringToInt(unitvaluegraphmode));
    }
}

QString SKGUnitPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGUNIT_DEFAULT_PARAMETERS");
}

void SKGUnitPluginWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    if (iTableName == QStringLiteral("unitvalue") || iTableName.isEmpty()) {
        // Correction bug 2299394 vvv
        if (ui.kUnitValueTableViewEdition->isAutoResized()) {
            ui.kUnitValueTableViewEdition->resizeColumnsToContentsDelayed();
        }
        // Correction bug 2299394 ^^^
    }
}

void SKGUnitPluginWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0) {
        ui.kNameCreatorUnit->setText(QString());
        ui.kSymbolCreatorUnit->setText(QString());
        ui.kCountryCreatorUnit->setText(QString());
        ui.kInternetCreatorUnit->setText(QString());
        ui.kUnitCreatorUnit->setText(QString());
    }
}

bool SKGUnitPluginWidget::isEditor()
{
    return true;
}

void SKGUnitPluginWidget::activateEditor()
{
    if (ui.kWidgetSelector->getSelectedMode() == -1) {
        ui.kWidgetSelector->setSelectedMode(0);
    }
    ui.kCurrencyList->setFocus();
}

void SKGUnitPluginWidget::refreshUnitList()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    // Clean list
    ui.kCurrencyList->clear();

    // Add items
    QStringList list = SKGUnitObject::getListofKnownCurrencies(ui.kObsolete->isChecked());
    ui.kCurrencyList->addItems(list);

    // Completion
    KCompletion *comp = ui.kCurrencyList->completionObject();
    if (comp != nullptr) {
        comp->setIgnoreCase(true);
        comp->clear();
        comp->insertItems(list);
    }
    QApplication::restoreOverrideCursor();
}

QWidget *SKGUnitPluginWidget::mainWidget()
{
    if (ui.kUnitValueTableViewEdition->hasFocus()) {
        return ui.kUnitValueTableViewEdition;
    }
    return ui.kUnitTableViewEdition->getView();
}

void SKGUnitPluginWidget::onAddSource()
{
    QString source = ui.kDownloadSource->text().trimmed();
    if (!source.isEmpty() && (!SKGUnitObject::downloadSources().contains(source) || SKGUnitObject::isWritable(source))) {
        // This is a new source
        SKGError err = SKGUnitObject::addSource(source);
        onSourceChanged();

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGUnitPluginWidget::onDeleteSource()
{
    QString source = ui.kDownloadSource->text();
    if (!source.isEmpty() && SKGUnitObject::downloadSources().contains(source)) {
        // This is a new source
        SKGError err = SKGUnitObject::deleteSource(source);
        IFOK(err) ui.kDownloadSource->removeItem(ui.kDownloadSource->findText(source));

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGUnitPluginWidget::onSourceChanged()
{
    QString source = ui.kDownloadSource->text().trimmed();
    bool local = !source.isEmpty() && SKGUnitObject::isWritable(source);
    ui.kDeleteSource->setVisible(local);
    m_upload->setEnabled(local);

    static QString tooltipOrigin;
    if (tooltipOrigin.isEmpty()) {
        tooltipOrigin = ui.kInternetCreatorUnit->toolTip();
    }

    QString tooltip = tooltipOrigin;
    auto help = SKGUnitObject::getCommentFromSource(source);
    if (!help.isEmpty()) {
        tooltip += "<br/>" + i18nc("Help meeage in tooltip", "Here is the help for the selected source '%1':<br/>%2", source, help);
    }
    ui.kSourceHelp->setText(help);

    // Set tooltip on internet code
    ui.kInternetCreatorUnit->setToolTip(tooltip);
}

void SKGUnitPluginWidget::onGetNewHotStuff()
{
    QPointer<KNSWidgets::Dialog> dialog = new KNSWidgets::Dialog(QStringLiteral("skrooge_unit.knsrc"), this);
    dialog->exec();
    fillSourceList();
}

void SKGUnitPluginWidget::onPutNewHotStuff()
{
    QString source = ui.kDownloadSource->text().trimmed();

    // Create zip file
    QString sourceFileName = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u'/' + QStringLiteral("skrooge/quotes/") % source % ".txt";
    QString zipFileName = QDir::tempPath() % "/" % source % ".zip";
    KZip zip(zipFileName);
    if (zip.open(QIODevice::WriteOnly)) {
        zip.addLocalFile(sourceFileName, source % ".txt");
        zip.close();

        // Open dialog
        SKGMainPanel::getMainPanel()->displayMessage(
            i18nc("Upload message", "The package is ready. You can find it here %1. You can now upload it manually.", zipFileName));
    }
}

void SKGUnitPluginWidget::onOpenURL()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    SKGObjectBase::SKGListSKGObjectBase selection = ui.kUnitTableViewEdition->getView()->getSelectedObjects();
    int nb = selection.count();
    if (nb != 0) {
        for (int i = 0; !err && i < nb; ++i) {
            SKGUnitObject unit(selection.at(i));
            err = unit.openURL();
        }
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}
