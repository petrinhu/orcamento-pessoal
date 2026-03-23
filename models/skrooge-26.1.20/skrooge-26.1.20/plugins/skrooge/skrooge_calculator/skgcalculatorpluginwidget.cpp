/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to calculate.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgcalculatorpluginwidget.h"

#include <qdir.h>
#include <qdom.h>
#include <qevent.h>
#include <qmath.h>
#include <qstandardpaths.h>

#include "skgaccountobject.h"
#include "skgdocumentbank.h"
#include "skginterestobject.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgunitobject.h"

SKGCalculatorPluginWidget::SKGCalculatorPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument)
    : SKGTabPage(iParent, iDocument)
    , m_objectModel(nullptr)
{
    SKGTRACEINFUNC(1)
    if (iDocument == nullptr) {
        return;
    }

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGCalculatorPluginWidget::onAmortizationComputation, Qt::QueuedConnection);

    ui.setupUi(this);
    ui.kUnitEdit->setDocument(iDocument);
    ui.kUnitEdit->setWhereClauseCondition(QStringLiteral("t_type IN ('1','2','C')"));

    ui.kUpdate->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kAdd->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));

    ui.kLoadSummaryTitle->setIcon(SKGServices::fromTheme(QStringLiteral("configure")), KTitleWidget::ImageLeft);
    ui.kPaymentSummaryTitle->setIcon(SKGServices::fromTheme(QStringLiteral("bookmarks")), KTitleWidget::ImageLeft);
    ui.kAmortizationTableTitle->setIcon(SKGServices::fromTheme(QStringLiteral("view-pim-calendar")), KTitleWidget::ImageLeft);

    ui.kInterestResultsTitle->setIcon(SKGServices::fromTheme(QStringLiteral("bookmarks")), KTitleWidget::ImageLeft);
    ui.kInterestValuesTitle->setIcon(SKGServices::fromTheme(QStringLiteral("configure")), KTitleWidget::ImageLeft);
    ui.kDefineValueTitle->setIcon(SKGServices::fromTheme(QStringLiteral("configure")), KTitleWidget::ImageLeft);

    ui.kCreditValueDate->addItem(i18nc("A period of 15 days", "Fifteen"));
    ui.kDebitValueDate->addItem(i18nc("A period of 15 days", "Fifteen"));
    for (int i = 0; i <= 5; ++i) {
        ui.kCreditValueDate->addItem(i18nc("Used for configurating when interests are paid on an account : %s days after being calculated", "Day +%1", i));
        ui.kDebitValueDate->addItem(i18nc("Used for configurating when interests are paid on an account : %s days after being calculated", "Day -%1", i));
    }

    ui.kMode->addItem(i18nc("24 fifteen is the name of a financial method to compute interests on an account", "24 fifteen"));
    ui.kMode->addItem(i18nc("A period of 360 days", "360 days"));
    ui.kMode->addItem(i18nc("A period of 365 days", "365 days"));

    ui.kAmortizationTable->verticalHeader()->setDefaultSectionSize(ui.kAmortizationTable->verticalHeader()->minimumSectionSize());

    SKGWidgetSelector::SKGListQWidget list;
    list.push_back(ui.kInterestFrm);
    list.push_back(ui.kBtnFrm);
    ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("taxes-finances")), i18n("Interest"), i18n("Interests calculator"), list);
    ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("dialog-scripts")),
                                  i18n("Amortization Table"),
                                  i18n("Loan amortization table calculator"),
                                  ui.kAmortizationFrm);
    connect(ui.kWidgetSelector, &SKGWidgetSelector::selectedModeChanged, this, &SKGCalculatorPluginWidget::onBtnModeClicked);

    // Bind account creation view
    m_objectModel = new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()), QStringLiteral("v_interest"), QString(), this, QString(), false);
    ui.kInterestView->setModel(m_objectModel);

    auto resultModel = new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()), QStringLiteral("interest_result"), QString(), this, QString(), false);
    ui.kInterestResultTable->setModel(resultModel);

    connect(ui.kInterestView, &SKGTableView::selectionChangedDelayed, this, &SKGCalculatorPluginWidget::onSelectionChanged);
    connect(m_objectModel, &SKGObjectModelBase::beforeReset, ui.kInterestView, &SKGTreeView::saveSelection);
    connect(m_objectModel, &SKGObjectModelBase::afterReset, ui.kInterestView, &SKGTreeView::resetSelection);

    ui.kWidgetSelector->setSelectedMode(0);

    // Refresh
    connect(ui.kDisplayAccountCombo,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGCalculatorPluginWidget::onFilterChanged,
            Qt::QueuedConnection);
    connect(ui.KYearEdit,
            static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
            this,
            &SKGCalculatorPluginWidget::onFilterChanged,
            Qt::QueuedConnection);

    connect(getDocument(), &SKGDocument::tableModified, this, &SKGCalculatorPluginWidget::dataModified, Qt::QueuedConnection);
    dataModified(QString(), 0);

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    // Synchronize zooms of both tables
    connect(ui.kInterestResultTable, &SKGTableView::zoomChanged, ui.kInterestView, &SKGTableView::setZoomPosition);
    connect(ui.kInterestView, &SKGTableView::zoomChanged, ui.kInterestResultTable, &SKGTableView::setZoomPosition);

    // Other connects
    connect(ui.kAdd, &QPushButton::clicked, this, &SKGCalculatorPluginWidget::onAdd);
    connect(ui.kUpdate, &QPushButton::clicked, this, &SKGCalculatorPluginWidget::onUpdate);
    connect(ui.kLoanEdit, &SKGCalculatorEdit::textChanged, this, &SKGCalculatorPluginWidget::onAmortizationComputationDelayed);
    connect(ui.kUnitEdit,
            static_cast<void (SKGUnitComboBox::*)(const QString &)>(&SKGUnitComboBox::currentTextChanged),
            this,
            &SKGCalculatorPluginWidget::onAmortizationComputationDelayed);
    connect(ui.kLenghtEdit, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SKGCalculatorPluginWidget::onAmortizationComputationDelayed);
    connect(ui.kAnnualRateEdit,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,
            &SKGCalculatorPluginWidget::onAmortizationComputationDelayed);
    connect(ui.kInsuranceRateEdit,
            static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged),
            this,
            &SKGCalculatorPluginWidget::onAmortizationComputationDelayed);
}

SKGCalculatorPluginWidget::~SKGCalculatorPluginWidget()
{
    SKGTRACEINFUNC(1)
    m_objectModel = nullptr;
}

bool SKGCalculatorPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
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

QString SKGCalculatorPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("currentPage"), SKGServices::intToString(ui.kWidgetSelector->getSelectedMode()));
    root.setAttribute(QStringLiteral("account"), ui.kDisplayAccountCombo->currentText());
    root.setAttribute(QStringLiteral("year"), ui.KYearEdit->text());

    root.setAttribute(QStringLiteral("amortizationLoan"), ui.kLoanEdit->text());
    root.setAttribute(QStringLiteral("amortizationUnit"), ui.kUnitEdit->text());
    root.setAttribute(QStringLiteral("amortizationRate"), SKGServices::doubleToString(ui.kAnnualRateEdit->value()));
    root.setAttribute(QStringLiteral("amortizationLenght"), SKGServices::intToString(ui.kLenghtEdit->value()));
    root.setAttribute(QStringLiteral("amortizationInsuranceRate"), SKGServices::doubleToString(ui.kInsuranceRateEdit->value()));

    // Memorize table settings
    root.setAttribute(QStringLiteral("view"), ui.kInterestView->getState());
    root.setAttribute(QStringLiteral("viewResult"), ui.kInterestResultTable->getState());

    return doc.toString();
}

void SKGCalculatorPluginWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString account = root.attribute(QStringLiteral("account"));
    QString currentPage = root.attribute(QStringLiteral("currentPage"));
    QString year = root.attribute(QStringLiteral("year"));
    QString amortizationLoan = root.attribute(QStringLiteral("amortizationLoan"));
    QString amortizationUnit = root.attribute(QStringLiteral("amortizationUnit"));
    QString amortizationRate = root.attribute(QStringLiteral("amortizationRate"));
    QString amortizationInsuranceRate = root.attribute(QStringLiteral("amortizationInsuranceRate"));
    QString amortizationLenght = root.attribute(QStringLiteral("amortizationLenght"));

    if (!currentPage.isEmpty()) {
        ui.kWidgetSelector->setSelectedMode(SKGServices::stringToInt(currentPage));
    }
    if (!account.isEmpty()) {
        bool previous = ui.kDisplayAccountCombo->blockSignals(true);
        ui.kDisplayAccountCombo->setText(account);
        ui.kDisplayAccountCombo->blockSignals(previous);
    }
    if (!amortizationLoan.isEmpty()) {
        ui.kLoanEdit->setText(amortizationLoan);
    }
    if (!amortizationUnit.isEmpty()) {
        ui.kUnitEdit->setText(amortizationUnit);
    }
    if (!amortizationRate.isEmpty()) {
        ui.kAnnualRateEdit->setValue(SKGServices::stringToDouble(amortizationRate));
    }
    if (!amortizationInsuranceRate.isEmpty()) {
        ui.kInsuranceRateEdit->setValue(SKGServices::stringToDouble(amortizationInsuranceRate));
    }
    if (!amortizationLenght.isEmpty()) {
        ui.kLenghtEdit->setValue(SKGServices::stringToInt(amortizationLenght));
    }

    if (!year.isEmpty()) {
        ui.KYearEdit->setText(year);
    }

    // Update model
    if (m_objectModel != nullptr) {
        bool previous = m_objectModel->blockRefresh(true);
        onFilterChanged();
        m_objectModel->blockRefresh(previous);
    }

    // !!! Must be done here after onFilterChanged
    ui.kInterestView->setState(root.attribute(QStringLiteral("view")));
    ui.kInterestResultTable->setState(root.attribute(QStringLiteral("viewResult")));
}

QString SKGCalculatorPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGCALCULATOR_DEFAULT_PARAMETERS");
}

void SKGCalculatorPluginWidget::onBtnModeClicked(int mode)
{
    Q_UNUSED(mode)

    // Save zoom position of previous page
    int zoom = zoomPosition();

    // Save zoom position on new page
    setZoomPosition(zoom);

    Q_EMIT selectionChanged();
}

void SKGCalculatorPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    // Mapping
    QItemSelectionModel *selModel = ui.kInterestView->selectionModel();
    if (selModel != nullptr) {
        QModelIndexList indexes = selModel->selectedRows();
        if (!indexes.isEmpty() && (m_objectModel != nullptr)) {
            QModelIndex idx = indexes[indexes.count() - 1];

            SKGInterestObject interest(m_objectModel->getObject(idx));

            ui.kDateEdit->setDate(interest.getDate());
            ui.kRateEdit->setValue(interest.getRate());
            ui.kCreditValueDate->setCurrentIndex(static_cast<int>(interest.getIncomeValueDateMode()));
            ui.kDebitValueDate->setCurrentIndex(static_cast<int>(interest.getExpenditueValueDateMode()));
            ui.kMode->setCurrentIndex(static_cast<int>(interest.getInterestComputationMode()));
        }

        Q_EMIT selectionChanged();
    }
}

void SKGCalculatorPluginWidget::dataModified(const QString &iTableName, int iIdTransaction)

{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Refresh widgets
    QSqlDatabase *db = getDocument()->getMainDatabase();
    setEnabled(db != nullptr);
    if (db != nullptr && (iTableName == QStringLiteral("interest") || iTableName.isEmpty())) {
        // Correction bug 2299394 vvv
        if (ui.kInterestView->isAutoResized()) {
            ui.kInterestView->resizeColumnsToContentsDelayed();
        }
        // Correction bug 2299394 ^^^

        // Correction 215658: vvv because the table is modified, the selection is modified
        onSelectionChanged();
        // Correction 215658: ^^^
    }

    // Refresh interest parameter view
    QString current = ui.kDisplayAccountCombo->currentText();
    if (db != nullptr && (iTableName == QStringLiteral("v_account_display") || iTableName.isEmpty())) {
        // Clear
        ui.kDisplayAccountCombo->clear();

        SKGStringListList listAccount;
        getDocument()->executeSelectSqliteOrder(QStringLiteral("SELECT t_ICON, t_name from v_account_display where t_close='N' order by t_name"), listAccount);

        int nbAccounts = listAccount.count();
        if (nbAccounts >= 0) {
            for (int i = 1; i < nbAccounts; ++i) { // Ignore header
                QString fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/images/logo/" % listAccount.at(i).at(0));
                if (fileName.isEmpty()) {
                    fileName = listAccount.at(i).at(0);
                }
                QIcon icon(fileName);
                QString text = listAccount.at(i).at(1);
                ui.kDisplayAccountCombo->addItem(icon, text);
            }
        }

        int posText = ui.kDisplayAccountCombo->findText(current);
        if (posText == -1) {
            posText = 0;
        }
        ui.kDisplayAccountCombo->setCurrentIndex(posText);

        ui.kAdd->setEnabled(ui.kDisplayAccountCombo->count() > 0);
    }

    // Fill years
    if (db != nullptr && (iTableName == QStringLiteral("v_operation_display") || iTableName.isEmpty())) {
        disconnect(ui.KYearEdit,
                   static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
                   this,
                   &SKGCalculatorPluginWidget::onFilterChanged);

        int lastYear = QDate::currentDate().year();
        int firstYear = lastYear;

        QStringList list;
        getDocument()->getDistinctValues(QStringLiteral("v_operation_display"), QStringLiteral("d_DATEYEAR"), QString(), list);
        if (!list.isEmpty()) {
            firstYear = SKGServices::stringToInt(list.at(0));
        }

        QString year = ui.KYearEdit->text();
        ui.KYearEdit->clear();
        for (int i = lastYear; i >= firstYear; --i) {
            QString v = SKGServices::intToString(i);
            if (year.isEmpty()) {
                year = v;
            }
            ui.KYearEdit->addItem(v);
        }

        ui.KYearEdit->setText(year);
        connect(ui.KYearEdit,
                static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged),
                this,
                &SKGCalculatorPluginWidget::onFilterChanged);
    }
    // Refresh computation
    if (db != nullptr && (iTableName == QStringLiteral("interest") || iTableName == QStringLiteral("account") || iTableName.isEmpty())) {
        computeInterest();
    }
    QApplication::restoreOverrideCursor();
}

void SKGCalculatorPluginWidget::computeInterest()
{
    // Compute
    SKGAccountObject account(getDocument());
    SKGError err = account.setName(ui.kDisplayAccountCombo->currentText());
    IFOKDO(err, account.load())
    SKGAccountObject::SKGInterestItemList oInterestList;
    double oInterests = 0;
    IFOKDO(err, account.getInterestItems(oInterestList, oInterests, SKGServices::stringToInt(ui.KYearEdit->text())))
    IFOK(err)
    {
        // Refresh table
        ui.kInterestResultTable->setState(ui.kInterestResultTable->getState());

        // Set text
        auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
        if (doc != nullptr) {
            SKGServices::SKGUnitInfo unit1 = doc->getPrimaryUnit();
            SKGServices::SKGUnitInfo unit2 = doc->getSecondaryUnit();

            QString s1 = doc->formatMoney(oInterests, unit1);
            ui.kInterestLbl->setText(i18nc("The Annual interest is the amount of money gained in one year on a remunerated account", "Annual interest=%1", s1));
            if (!unit2.Symbol.isEmpty() && (unit2.Value != 0.0)) {
                s1 = doc->formatMoney(oInterests, unit2);
                ui.kInterestLbl->setToolTip(
                    i18nc("The Annual interest is the amount of money gained in one year on a remunerated account", "Annual interest=%1", s1));
            }
        }
    }
}

void SKGCalculatorPluginWidget::onFilterChanged()
{
    SKGTRACEINFUNC(1)
    if (!isEnabled()) {
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Compute where clause
    QString account = ui.kDisplayAccountCombo->currentText();
    QString filter2 = "t_ACCOUNT='" % SKGServices::stringToSqlString(account) % "' ORDER BY d_date";

    // Update model
    if (m_objectModel != nullptr) {
        if (m_objectModel->setFilter(filter2)) {
            ui.kInterestView->setState(ui.kInterestView->getState());
        }

        // Compute result
        computeInterest();
    }
    QApplication::restoreOverrideCursor();
}

SKGObjectBase::SKGListSKGObjectBase SKGCalculatorPluginWidget::getSelectedObjects()
{
    SKGObjectBase::SKGListSKGObjectBase output;
    if (ui.kWidgetSelector->getSelectedMode() == 0) {
        output = ui.kInterestView->getSelectedObjects();
    }
    return output;
}

int SKGCalculatorPluginWidget::getNbSelectedObjects()
{
    int output = 0;
    if (ui.kWidgetSelector->getSelectedMode() == 0) {
        output = ui.kInterestView->getNbSelectedObjects();
    }
    return output;
}

// void SKGCalculatorPluginWidget::onCalculatorReturnPressed(const QString& iText)
// {
// //         ui.kCalculatorList->addItem(iText);
// }

// /*void SKGCalculatorPluginWidget::onCalculatorListClicked(const QString& iText)
// {
// //         ui.kCalculatorEdit->setText(iText);
// //         ui.kCalculatorEdit->setFocus();
// }*/

void SKGCalculatorPluginWidget::onSelectedInterestChanged()
{
    int nbSel = getNbSelectedObjects();
    ui.kDateEdit->setEnabled(nbSel <= 1);
    ui.kRateEdit->setEnabled(nbSel <= 1);
    ui.kAdd->setEnabled(nbSel <= 1);
}

void SKGCalculatorPluginWidget::onAdd()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        SKGInterestObject interestObj;
        {
            QString accountname = ui.kDisplayAccountCombo->currentText();
            SKGBEGINTRANSACTION(
                *getDocument(),
                i18nc("Lets the user create parameters for computing interests on an account", "Interest parameter creation for account '%1'", accountname),
                err)

            // Get Parent account
            SKGAccountObject accountObj(getDocument());
            IFOKDO(err, accountObj.setName(accountname))
            IFOKDO(err, accountObj.load())

            // Create interest object
            IFOKDO(err, accountObj.addInterest(interestObj))
            IFOKDO(err, interestObj.setDate(ui.kDateEdit->date()))
            IFOKDO(err, interestObj.setRate(ui.kRateEdit->value()))
            IFOKDO(err, interestObj.setIncomeValueDateMode(static_cast<SKGInterestObject::ValueDateMode>(ui.kCreditValueDate->currentIndex())))
            IFOKDO(err, interestObj.setExpenditueValueDateMode(static_cast<SKGInterestObject::ValueDateMode>(ui.kDebitValueDate->currentIndex())))
            IFOKDO(err, interestObj.setInterestComputationMode(static_cast<SKGInterestObject::InterestMode>(ui.kMode->currentIndex())))
            IFOKDO(err, interestObj.save())

            // Send message
            IFOKDO(err,
                   interestObj.getDocument()->sendMessage(
                       i18nc("An information to the user", "The interest parameter '%1' has been added", interestObj.getDisplayName()),
                       SKGDocument::Hidden))
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("User defined parameters for computing interests were successfully created", "Interest parameter created"));
            ui.kInterestView->selectObject(interestObj.getUniqueID());
        }
        else
        {
            err.addError(ERR_FAIL,
                         i18nc("Error message: User defined parameters for computing interests could not be created", "Interest parameter creation failed"));
        }
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGCalculatorPluginWidget::onUpdate()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Get Parent account
    QString accountname = ui.kDisplayAccountCombo->currentText();
    SKGAccountObject accountObj(getDocument());
    IFOKDO(err, accountObj.setName(accountname))
    IFOKDO(err, accountObj.load())

    SKGObjectBase::SKGListSKGObjectBase updatedInterest = getSelectedObjects();
    int nb = updatedInterest.count();
    {
        SKGInterestObject interestObj;
        SKGBEGINTRANSACTION(
            *getDocument(),
            i18nc("Lets the user update parameters for computing interests on an account", "Interest parameter update for account '%1'", accountname),
            err)

        for (int i = 0; !err && i < nb; ++i) {
            interestObj = updatedInterest.at(i);
            if (nb == 1) {
                IFOKDO(err, interestObj.setDate(ui.kDateEdit->date()))
                IFOKDO(err, interestObj.setRate(ui.kRateEdit->value()))
            }
            IFOKDO(err, interestObj.setIncomeValueDateMode(static_cast<SKGInterestObject::ValueDateMode>(ui.kCreditValueDate->currentIndex())))
            IFOKDO(err, interestObj.setExpenditueValueDateMode(static_cast<SKGInterestObject::ValueDateMode>(ui.kDebitValueDate->currentIndex())))
            IFOKDO(err, interestObj.setInterestComputationMode(static_cast<SKGInterestObject::InterestMode>(ui.kMode->currentIndex())))
            IFOKDO(err, interestObj.save())

            // Send message
            IFOKDO(err,
                   interestObj.getDocument()->sendMessage(
                       i18nc("An information to the user", "The interest parameter '%1' has been updated", interestObj.getDisplayName()),
                       SKGDocument::Hidden))
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("User defined parameters for computing interests were successfully updated", "Interest parameter updated")))
    else
    {
        err.addError(ERR_FAIL,
                     i18nc("Error message: User defined parameters for computing interests could not be updated", "Interest parameter update failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGCalculatorPluginWidget::onAmortizationComputationDelayed()
{
    m_timer.start(300);
}

void SKGCalculatorPluginWidget::onAmortizationComputation()
{
    SKGTRACEINFUNC(10)
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Get unit
    SKGServices::SKGUnitInfo unitInfo;
    unitInfo.Symbol = ui.kUnitEdit->text();

    SKGUnitObject unit(getDocument());
    unit.setSymbol(unitInfo.Symbol);
    unit.load();
    unitInfo.NbDecimal = unit.getNumberDecimal();

    int p = qPow(10, unitInfo.NbDecimal);

    // Computation
    int periodInMonths = 12;
    int numberPayments = ui.kLenghtEdit->value() * periodInMonths;
    double periodicRate = ui.kAnnualRateEdit->value() / 100 / periodInMonths;
    double insurance = qRound(ui.kLoanEdit->value() * ui.kInsuranceRateEdit->value() / 100.0 / static_cast<double>(periodInMonths) * static_cast<double>(p))
        / static_cast<double>(p);
    double periodicAmount =
        qRound(ui.kLoanEdit->value() * periodicRate / (1 - pow(1 + periodicRate, -numberPayments)) * static_cast<double>(p)) / static_cast<double>(p);

    // Fill table
    double dueAmount = ui.kLoanEdit->value();
    double sumInterest = 0;
    ui.kAmortizationTable->setRowCount(numberPayments);
    for (int i = 0; i < numberPayments; ++i) {
        // Compute
        double roundedInterest = qRound(dueAmount * periodicRate * static_cast<double>(p)) / static_cast<double>(p);
        double principal = qRound((i == numberPayments - 1 ? dueAmount : periodicAmount - roundedInterest) * p) / static_cast<double>(p);
        double amount = roundedInterest + principal;
        dueAmount -= principal;
        sumInterest += roundedInterest;

        // Add items
        ui.kAmortizationTable->setItem(i, 0, new QTableWidgetItem(SKGServices::toCurrencyString(amount, unitInfo.Symbol, unitInfo.NbDecimal)));
        ui.kAmortizationTable->setItem(i, 1, new QTableWidgetItem(SKGServices::toCurrencyString(principal, unitInfo.Symbol, unitInfo.NbDecimal)));
        ui.kAmortizationTable->setItem(i, 2, new QTableWidgetItem(SKGServices::toCurrencyString(roundedInterest, unitInfo.Symbol, unitInfo.NbDecimal)));
        ui.kAmortizationTable->setItem(i, 3, new QTableWidgetItem(SKGServices::toCurrencyString(insurance, unitInfo.Symbol, unitInfo.NbDecimal)));
        ui.kAmortizationTable->setItem(i, 4, new QTableWidgetItem(SKGServices::toCurrencyString(dueAmount, unitInfo.Symbol, unitInfo.NbDecimal)));
    }

    ui.kAmortizationResult->setText(
        i18n("Number of payments:	%1\n" // NOLINT(whitespace/tab)
             "Monthly payment: 	%2\n" // NOLINT(whitespace/tab)
             "Monthly insurance: 	%3\n" // NOLINT(whitespace/tab)
             "Total principal paid: 	%4\n" // NOLINT(whitespace/tab)
             "Total interest paid: 	%5\n" // NOLINT(whitespace/tab)
             "Total insurance paid: 	%6\n" // NOLINT(whitespace/tab)
             "Total paid: 		%7", // NOLINT(whitespace/tab)
             SKGServices::intToString(numberPayments),
             SKGServices::toCurrencyString(periodicAmount, unitInfo.Symbol, unitInfo.NbDecimal),
             SKGServices::toCurrencyString(insurance, unitInfo.Symbol, unitInfo.NbDecimal),
             SKGServices::toCurrencyString(ui.kLoanEdit->value(), unitInfo.Symbol, unitInfo.NbDecimal),
             SKGServices::toCurrencyString(sumInterest, unitInfo.Symbol, unitInfo.NbDecimal),
             SKGServices::toCurrencyString(numberPayments * insurance, unitInfo.Symbol, unitInfo.NbDecimal),
             SKGServices::toCurrencyString(ui.kLoanEdit->value() + sumInterest + numberPayments * insurance, unitInfo.Symbol, unitInfo.NbDecimal)));

    QApplication::restoreOverrideCursor();
}

QWidget *SKGCalculatorPluginWidget::zoomableWidget()
{
    if (ui.kWidgetSelector->getSelectedMode() == 0) {
        return ui.kInterestView;
    }
    return ui.kAmortizationTable;
}

QWidget *SKGCalculatorPluginWidget::mainWidget()
{
    if (ui.kWidgetSelector->getSelectedMode() == 0) {
        return ui.kInterestView;
    }
    return ui.kAmortizationTable;
}
