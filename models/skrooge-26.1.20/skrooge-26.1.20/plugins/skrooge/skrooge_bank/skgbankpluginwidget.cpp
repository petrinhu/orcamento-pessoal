/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbankpluginwidget.h"

#include <kmessagebox.h>

#include <qdir.h>
#include <qdom.h>
#include <qevent.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qregularexpression.h>
#include <qstandardpaths.h>

#include "skgbankobject.h"
#include "skgboardwidget.h"
#include "skgdocumentbank.h"
#include "skginterfaceplugin.h"
#include "skgmainpanel.h"
#include "skgobjectmodel.h"
#include "skgtablewithgraph.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgunitobject.h"
#include "skgunitvalueobject.h"

SKGBankPluginWidget::SKGBankPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument)
    : SKGTabPage(iParent, iDocument)
    , m_graph(nullptr)
{
    SKGTRACEINFUNC(10)
    if (iDocument == nullptr) {
        return;
    }

    ui.setupUi(this);

    // Add report
    SKGInterfacePlugin *reportPlugin = SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge report plugin"));
    if (reportPlugin != nullptr) {
        m_graph = reportPlugin->getDashboardWidget(0);
        if (m_graph != nullptr) {
            // To avoid hidden page when no account exist
            auto w = new QWidget(this);
            auto verticalLayout = new QVBoxLayout(w);
            verticalLayout->addWidget(m_graph);

            m_graph->hideTitle();

            ui.verticalLayout->insertWidget(2, w);
            // Set initial state
            m_graph->hide();

            QDomDocument doc(QStringLiteral("SKGML"));
            QDomElement root = doc.createElement(QStringLiteral("parameters"));
            doc.appendChild(root);
            {
                QDomDocument doc1(QStringLiteral("SKGML"));
                QDomElement root1 = doc1.createElement(QStringLiteral("parameters"));
                doc1.appendChild(root1);

                root1.setAttribute(QStringLiteral("period"), QStringLiteral("0"));
                root1.setAttribute(QStringLiteral("columns"), QStringLiteral("d_DATEMONTH"));
                root1.setAttribute(QStringLiteral("mode"), QStringLiteral("1"));
                root1.setAttribute(QStringLiteral("currentPage"), QStringLiteral("-2"));
                root1.setAttribute(QStringLiteral("operationWhereClause"), QStringLiteral("1=0"));
                root1.setAttribute(QStringLiteral("title"), QStringLiteral("none"));

                {
                    QDomDocument doc2(QStringLiteral("SKGML"));
                    QDomElement root2 = doc2.createElement(QStringLiteral("parameters"));
                    doc2.appendChild(root2);
                    root2.setAttribute(QStringLiteral("graphMode"), SKGServices::intToString(static_cast<int>(SKGTableWithGraph::LINE)));

                    {
                        QDomDocument doc3(QStringLiteral("SKGML"));
                        QDomElement root3 = doc3.createElement(QStringLiteral("parameters"));
                        doc3.appendChild(root3);
                        root3.setAttribute(QStringLiteral("isToolBarVisible"), QStringLiteral("N"));

                        root2.setAttribute(QStringLiteral("graphicViewState"), doc3.toString());
                    }

                    root1.setAttribute(QStringLiteral("tableAndGraphState"), doc2.toString());
                }
                root.setAttribute(QStringLiteral("graph"), doc1.toString());
            }

            m_graph->setState(doc.toString());

            m_timer2.setSingleShot(true);
            connect(&m_timer2, &QTimer::timeout, this, &SKGBankPluginWidget::onRefreshGraph, Qt::QueuedConnection);
            onRefreshGraphDelayed();

            connect(ui.kView->getShowWidget(), &SKGShow::stateChanged, this, &SKGBankPluginWidget::onRefreshGraphDelayed, Qt::QueuedConnection);
        }
    }

    // Set label titles
    ui.kBankLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_bank"))));
    ui.kAccountLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_ACCOUNT"))));
    ui.kTypeLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_type"))));
    ui.kBankNumberLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_bank_number"))));
    ui.kAgencyNumberLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_agency_number"))));
    ui.kNumberLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_number"))));
    ui.kAgencyAddressLbl->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_agency_address"))));
    ui.kMaxLimit->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("f_maxamount"))));
    ui.kMinLimit->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("f_minamount"))));
    ui.kCommentLabel->setText(i18n("%1:", iDocument->getDisplay(QStringLiteral("t_comment"))));

    // Set show widget
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("all"), i18n("All"), QString(), QString(), QString(), Qt::META | Qt::Key_A);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("opened"),
                                              i18n("Opened"),
                                              QStringLiteral("vcs-normal"),
                                              QStringLiteral("t_close='N'"),
                                              QString(),
                                              Qt::META | Qt::Key_O);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("closed"),
                                              i18n("Closed"),
                                              QStringLiteral("vcs-conflicting"),
                                              QStringLiteral("t_close='Y'"),
                                              QString(),
                                              Qt::META | Qt::Key_C);
    ui.kView->getShowWidget()->addGroupedItem(QStringLiteral("highlighted"),
                                              i18n("Highlighted only"),
                                              QStringLiteral("bookmarks"),
                                              QStringLiteral("t_bookmarked='Y'"),
                                              QString(),
                                              Qt::META | Qt::Key_H);
    if (m_graph != nullptr) {
        ui.kView->getShowWidget()->addSeparator();
        ui.kView->getShowWidget()->addItem(QStringLiteral("graph"),
                                           i18n("Graph"),
                                           QStringLiteral("view-statistics"),
                                           QStringLiteral("1=0"),
                                           QString(),
                                           QString(),
                                           QString(),
                                           QString(),
                                           Qt::META | Qt::Key_G);
    }

    ui.kView->getShowWidget()->setDefaultState(QStringLiteral("opened"));

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGBankPluginWidget::refreshInfoZone, Qt::QueuedConnection);

    // Add Standard KDE Icons to buttons to Accounts
    ui.kAccountCreatorUpdate->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-ok")));
    ui.kAccountCreatorAdd->setIcon(SKGServices::fromTheme(QStringLiteral("list-add")));

    SKGWidgetSelector::SKGListQWidget list;
    list.push_back(ui.SKGEditionWidget);
    list.push_back(ui.SKGEditionButtonswidget);
    ui.kWidgetSelector->addButton(SKGServices::fromTheme(QStringLiteral("configure")), i18n("Edit"), i18n("Display the account edit panel"), list);

    // Add NLS values for type of account
    // C=current D=credit card P=passif (for objects) I=Investment O=other
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("view-financial-account-checking")),
                                    i18nc("Noun, a type of account", "Current"),
                                    static_cast<int>(SKGAccountObject::CURRENT));
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("skrooge_credit_card")),
                                    i18nc("Noun, a type of account", "Credit card"),
                                    static_cast<int>(SKGAccountObject::CREDITCARD));
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("view-financial-account-savings")),
                                    i18nc("Noun, a type of account", "Saving"),
                                    static_cast<int>(SKGAccountObject::SAVING));
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("view-financial-account-investment")),
                                    i18nc("Noun, a type of account", "Investment"),
                                    static_cast<int>(SKGAccountObject::INVESTMENT));
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("view-financial-account-asset")),
                                    i18nc("Noun, a type of account", "Assets"),
                                    static_cast<int>(SKGAccountObject::ASSETS));
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("view-financial-account-savings")),
                                    i18nc("Noun, a type of account", "Loan"),
                                    static_cast<int>(SKGAccountObject::LOAN));
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("view-financial-account-loan")),
                                    i18nc("Noun, a type of account", "Pension"),
                                    static_cast<int>(SKGAccountObject::PENSION));
    ui.kAccountCreatorType->addItem(SKGServices::fromTheme(QStringLiteral("wallet-closed")),
                                    i18nc("Noun, a type of account", "Wallet"),
                                    static_cast<int>(SKGAccountObject::WALLET));
    ui.kAccountCreatorType->addItem(i18nc("Noun, a type of account", "Other"), static_cast<int>(SKGAccountObject::OTHER));

    // Bind account creation view
    ui.kView->setModel(
        new SKGObjectModel(qobject_cast<SKGDocumentBank *>(getDocument()), QStringLiteral("v_account_display"), QStringLiteral("1=0"), this, QString(), false));

    connect(ui.kView->getView(), &SKGTreeView::clickEmptyArea, this, &SKGBankPluginWidget::cleanEditor);
    connect(ui.kView->getView(), &SKGTreeView::doubleClicked, SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open")).data(), &QAction::trigger);
    connect(ui.kView->getView(), &SKGTreeView::selectionChangedDelayed, this, [this] {
        this->onSelectionChanged();
    });

    // Search file list
    QString listIconFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation,
                                                  "skrooge/images/logo/l10n/" % QLocale().name().split(QStringLiteral("_")).at(0) % "/list_bank.txt");
    if (listIconFile.isEmpty()) {
        listIconFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/images/logo/list_bank.txt"));
    }

    // Logo for banks
    ui.kAccountCreatorIcon->addItem(QString());
    QFile file(listIconFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (!line.isEmpty()) {
                QString fileName = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "skrooge/images/logo/" % line);
                QString bankName = std::move(line);
                bankName.remove(QStringLiteral(".png"));
                bankName.replace('_', ' ');

                static QRegularExpression reg(QStringLiteral("(.+) {2,}(.+)"));
                auto rx = reg.match(bankName);
                if (rx.hasMatch()) {
                    // Icon is something like <bank name>__<banknumber>.png
                    ui.kAccountCreatorIcon->addItem(QIcon(fileName), rx.captured(1), rx.captured(2));
                } else {
                    ui.kAccountCreatorIcon->addItem(QIcon(fileName), bankName, "");
                }
            }
        }
        ui.kAccountCreatorIcon->addItem(SKGServices::fromTheme(QStringLiteral("document-open-folder")), i18nc("Other type of bank account", "Other…"));

        file.close();
    }
    if (ui.kAccountCreatorIcon->count() < 11) {
        ui.kAccountCreatorIcon->setMaxVisibleItems(ui.kAccountCreatorIcon->count() - 1);
    }

    // Set Event filters to catch CTRL+ENTER or SHIFT+ENTER
    this->installEventFilter(this);

    // Activate the edit panel per default
    ui.kWidgetSelector->setSelectedMode(0);

    // Connects
    connect(ui.kAccountCreatorAdd, &QPushButton::clicked, this, &SKGBankPluginWidget::onAddAccountClicked);
    connect(ui.kAccountCreatorUpdate, &QPushButton::clicked, this, &SKGBankPluginWidget::onModifyAccountClicked);
    connect(ui.kAccountCreatorAccount, &QLineEdit::textChanged, this, &SKGBankPluginWidget::onAccountCreatorModified);
    connect(ui.kUnitEdit, &SKGUnitComboBox::editTextChanged, this, &SKGBankPluginWidget::onAccountCreatorModified);
    connect(ui.kAccountCreatorType,
            static_cast<void (SKGComboBox::*)(int)>(&SKGComboBox::currentIndexChanged),
            this,
            &SKGBankPluginWidget::onAccountCreatorModified);
    connect(ui.kWidgetSelector, &SKGWidgetSelector::selectedModeChanged, this, &SKGBankPluginWidget::onAccountCreatorModified);
    connect(ui.kAmountEdit, &SKGCalculatorEdit::textChanged, this, &SKGBankPluginWidget::onAccountCreatorModified);
    connect(ui.kAccountCreatorIcon, static_cast<void (SKGComboBox::*)(int)>(&SKGComboBox::currentIndexChanged), this, &SKGBankPluginWidget::onIconChanged);
    connect(ui.kAccountCreatorBank, &SKGComboBox::editTextChanged, this, &SKGBankPluginWidget::onAccountCreatorModified);
    connect(ui.kMinLimit, &QCheckBox::toggled, ui.kMinLimitAmout, &SKGCalculatorEdit::setEnabled);
    connect(ui.kMaxLimit, &QCheckBox::toggled, ui.kMaxLimitAmout, &SKGCalculatorEdit::setEnabled);
    connect(ui.kUnitEdit, &SKGUnitComboBox::editTextChanged, ui.kMinLimitunit, &QLabel::setText);
    connect(ui.kUnitEdit, &SKGUnitComboBox::editTextChanged, ui.kMaxLimitunit, &QLabel::setText);

    // Must be done after the connects
    ui.kUnitEdit->setDocument(iDocument);

    // Refresh
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGBankPluginWidget::dataModified, Qt::QueuedConnection);
    dataModified(QString(), 0);
}

SKGBankPluginWidget::~SKGBankPluginWidget()
{
    SKGTRACEINFUNC(10)
}

bool SKGBankPluginWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if ((iEvent != nullptr) && iEvent->type() == QEvent::KeyPress) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(iEvent);
        if (keyEvent && (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && iObject == this) {
            if ((QApplication::keyboardModifiers() & Qt::ControlModifier) != 0u && ui.kAccountCreatorAdd->isEnabled()) {
                ui.kAccountCreatorAdd->click();
            } else if ((QApplication::keyboardModifiers() & Qt::ShiftModifier) != 0u && ui.kAccountCreatorUpdate->isEnabled()) {
                ui.kAccountCreatorUpdate->click();
            }
        }
    }

    return SKGTabPage::eventFilter(iObject, iEvent);
}

void SKGBankPluginWidget::onSelectionChanged()
{
    SKGTRACEINFUNC(10)
    // Mapping
    int nbSelect = ui.kView->getView()->getNbSelectedObjects();
    if (nbSelect == 1) {
        // set the icon
        SKGAccountObject account(ui.kView->getView()->getFirstSelectedObject());
        SKGBankObject bank;
        account.getBank(bank);
        QString fileName = bank.getIcon();
        QString iconName = fileName;
        if (!iconName.isEmpty()) {
            iconName.remove(QStringLiteral(".png"));
            iconName.replace('_', ' ');

            static QRegularExpression reg(QStringLiteral("(.+) {2,}(.+)"));
            auto rx = reg.match(iconName);
            if (rx.hasMatch()) {
                iconName = rx.captured(1);
            }
            if (ui.kAccountCreatorIcon->contains(iconName)) {
                ui.kAccountCreatorIcon->setText(iconName);
            } else if (ui.kAccountCreatorIcon->contains(fileName)) {
                ui.kAccountCreatorIcon->setText(fileName);
            } else {
                int c = ui.kAccountCreatorIcon->count() - 1;
                bool b = ui.kAccountCreatorIcon->blockSignals(true);
                ui.kAccountCreatorIcon->insertItem(c, QIcon(fileName), fileName);
                ui.kAccountCreatorIcon->setCurrentIndex(c);
                ui.kAccountCreatorIcon->blockSignals(b);
            }
        } else {
            ui.kAccountCreatorIcon->setText(QString());
        }

        ui.kAccountCreatorBank->setText(account.getAttribute(QStringLiteral("t_BANK")));
        ui.kAccountCreatorAccount->setText(account.getAttribute(QStringLiteral("t_name")));
        ui.kAccountCreatorBankNumber->setText(account.getAttribute(QStringLiteral("t_BANK_NUMBER")));
        ui.kAccountCreatorAgencyNumber->setText(account.getAttribute(QStringLiteral("t_agency_number")));
        ui.kAccountCreatorNumber->setText(account.getAttribute(QStringLiteral("t_number")));
        ui.kAccountCreatorType->setText(account.getAttribute(QStringLiteral("t_TYPENLS")));
        ui.kAccountCreatorAddress->setText(account.getAttribute(QStringLiteral("t_agency_address")));
        ui.kAccountCreatorComment->setText(account.getAttribute(QStringLiteral("t_comment")));
        ui.kMinLimitAmout->setText(account.getAttribute(QStringLiteral("f_minamount")));
        ui.kMaxLimitAmout->setText(account.getAttribute(QStringLiteral("f_maxamount")));
        ui.kMaxLimit->setTristate(false);
        ui.kMaxLimit->setCheckState(account.isMaxLimitAmountEnabled() ? Qt::Checked : Qt::Unchecked);
        ui.kMinLimit->setTristate(false);
        ui.kMinLimit->setCheckState(account.isMinLimitAmountEnabled() ? Qt::Checked : Qt::Unchecked);

        double oBalance = 0;
        SKGUnitObject oUnit;
        account.getInitialBalance(oBalance, oUnit);

        int nbDec = oUnit.getNumberDecimal();
        if (nbDec == 0) {
            nbDec = 2;
        }

        ui.kAmountEdit->setText(SKGServices::toCurrencyString(oBalance, QString(), nbDec));
        if (oUnit.exist()) {
            ui.kUnitEdit->setUnit(oUnit);
        } else {
            auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
            if (doc != nullptr) {
                SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();
                ui.kUnitEdit->setText(primary.Symbol);
            }
        }
    } else if (nbSelect > 1) {
        ui.kAccountCreatorIcon->setText(NOUPDATE);
        ui.kAccountCreatorBank->setText(NOUPDATE);
        ui.kAccountCreatorAccount->setText(NOUPDATE);
        ui.kAccountCreatorBankNumber->setText(NOUPDATE);
        ui.kAccountCreatorAgencyNumber->setText(NOUPDATE);
        ui.kAccountCreatorNumber->setText(NOUPDATE);
        ui.kAccountCreatorType->setText(NOUPDATE);
        ui.kAccountCreatorAddress->setText(NOUPDATE);
        ui.kAccountCreatorComment->setText(NOUPDATE);
        ui.kAmountEdit->setText(NOUPDATE);
        ui.kUnitEdit->setText(NOUPDATE);
        ui.kMinLimitAmout->setText(NOUPDATE);
        ui.kMaxLimitAmout->setText(NOUPDATE);
        ui.kMaxLimit->setTristate(true);
        ui.kMaxLimit->setCheckState(Qt::PartiallyChecked);
        ui.kMinLimit->setTristate(true);
        ui.kMinLimit->setCheckState(Qt::PartiallyChecked);
    }

    // Refresh graph
    m_timer2.start(300);

    onAccountCreatorModified();
    Q_EMIT selectionChanged();
}

void SKGBankPluginWidget::onRefreshGraphDelayed()
{
    m_timer2.start(300);
}

void SKGBankPluginWidget::onRefreshGraph()
{
    SKGTRACEINFUNC(10)
    if (m_graph != nullptr) {
        bool visible = ui.kView->getShowWidget()->getState().contains(QStringLiteral("graph"));

        QDomDocument doc(QStringLiteral("SKGML"));
        if (doc.setContent(m_graph->getState())) {
            QDomElement root = doc.documentElement();

            QString graphS = root.attribute(QStringLiteral("graph"));

            QDomDocument doc2(QStringLiteral("SKGML"));
            if (doc2.setContent(graphS)) {
                QDomElement root2 = doc2.documentElement();
                QString wc;
                QString title;
                int nbSelect = 0;
                if (visible) {
                    SKGObjectBase::SKGListSKGObjectBase objs = ui.kView->getView()->getSelectedObjects();
                    nbSelect = objs.count();
                    if (nbSelect != 0) {
                        wc = QStringLiteral("t_ACCOUNT IN (");
                        title = i18nc("Noun, a list of items", "Transactions of account ");
                        for (int i = 0; i < nbSelect; ++i) {
                            if (i != 0) {
                                wc += ',';
                                title += ',';
                            }
                            SKGAccountObject act(objs.at(i));
                            wc += '\'' % SKGServices::stringToSqlString(act.getName()) % u'\'';
                            title += i18n("'%1'", act.getName());
                        }
                        wc += ')';
                    }
                } else {
                    wc = QStringLiteral("1=0");
                    title = QStringLiteral("none");
                }
                root2.setAttribute(QStringLiteral("operationWhereClause"), wc);
                root2.setAttribute(QStringLiteral("title"), title);
                root2.setAttribute(QStringLiteral("lines"), nbSelect != 0 ? QStringLiteral("t_ACCOUNT") : QStringLiteral("#NOTHING#"));
            }
            root.setAttribute(QStringLiteral("graph"), doc2.toString());
        }
        QString newGraphState = doc.toString();
        if (newGraphState != m_graphState) {
            m_graphState = std::move(newGraphState);
            m_graph->setState(m_graphState);
        }

        m_graph->setVisible(visible);
    }
}

void SKGBankPluginWidget::onIconChanged()
{
    SKGTRACEINFUNC(10)

    int c = ui.kAccountCreatorIcon->currentIndex();
    if ((c != 0) && c == ui.kAccountCreatorIcon->count() - 1) {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            i18nc("Title of panel", "Select a bank icon"),
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/images/logo/"), QStandardPaths::LocateDirectory),
            i18nc("File format for open dialog panel", "Image files") % " (*.png *.jpeg *.jpg *.gif *.tiff)");
        if (!fileName.isEmpty()) {
            if (ui.kAccountCreatorIcon->contains(fileName)) {
                ui.kAccountCreatorIcon->setText(fileName);
            } else {
                bool b = ui.kAccountCreatorIcon->blockSignals(true);
                ui.kAccountCreatorIcon->insertItem(c, QIcon(fileName), fileName);
                ui.kAccountCreatorIcon->setCurrentIndex(c);
                ui.kAccountCreatorIcon->blockSignals(b);
            }
        } else {
            ui.kAccountCreatorIcon->setCurrentIndex(c - 1);
        }
    }

    if (ui.kAccountCreatorIcon->currentIndex() != 0) {
        // Set the bank name
        QString name = ui.kAccountCreatorIcon->currentText();
        QFileInfo f(name);
        if (f.exists()) {
            name = f.baseName();
            name = name.replace('_', ' ');
        }
        ui.kAccountCreatorBank->setText(name);
    } else {
        ui.kAccountCreatorBank->setText(QString());
    }

    // Facilitate bank number
    QString code = ui.kAccountCreatorIcon->itemData(ui.kAccountCreatorIcon->currentIndex()).toString();
    ui.kAccountCreatorBankNumber->setText(code);
}

void SKGBankPluginWidget::onAccountCreatorModified()
{
    SKGTRACEINFUNC(10)

    bool activatedForWallet = ui.kWidgetSelector->getSelectedMode() != -1 && !ui.kAccountCreatorAccount->text().isEmpty()
        && !ui.kAccountCreatorAccount->text().startsWith(u'=') && !ui.kUnitEdit->currentText().isEmpty()
        && (ui.kAmountEdit->valid() || ui.kAmountEdit->text().isEmpty() || ui.kAmountEdit->text() == NOUPDATE);
    bool activated = activatedForWallet && !ui.kAccountCreatorBank->text().isEmpty() && !ui.kAccountCreatorBank->text().startsWith(u'=');
    bool wallet = (static_cast<SKGAccountObject::AccountType>(ui.kAccountCreatorType->itemData(ui.kAccountCreatorType->currentIndex()).toInt())
                   == SKGAccountObject::WALLET);

    int nbSelect = getNbSelectedObjects();
    ui.kAccountCreatorAdd->setEnabled(activated || (wallet && activatedForWallet));
    ui.kAccountCreatorUpdate->setEnabled((activated || (wallet && activatedForWallet)) && nbSelect > 0);

    // Wallet specific mode
    ui.kBankNumberLbl->setVisible(!wallet);
    ui.kAccountCreatorBankNumber->setVisible(!wallet);
    ui.kAgencyNumberLbl->setVisible(!wallet);
    ui.kAccountCreatorAgencyNumber->setVisible(!wallet);
    ui.kNumberLbl->setVisible(!wallet);
    ui.kAccountCreatorNumber->setVisible(!wallet);
    ui.kBankLbl->setVisible(!wallet);
    ui.kAccountCreatorIcon->setVisible(!wallet);
    ui.kAccountCreatorBank->setVisible(!wallet);
    ui.kAccountCreatorAddress->setVisible(!wallet);
    ui.kAgencyAddressLbl->setVisible(!wallet);
}

SKGError SKGBankPluginWidget::setInitialBalanceFromEditor(SKGAccountObject &iAccount)
{
    return (ui.kAmountEdit->text() != NOUPDATE && ui.kUnitEdit->text() != NOUPDATE
                ? iAccount.setInitialBalance(ui.kAmountEdit->value(), ui.kUnitEdit->getUnit())
                : SKGError());
}

void SKGBankPluginWidget::onAddAccountClicked()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        SKGAccountObject accountObj;

        QString bankName = ui.kAccountCreatorBank->text();
        QString accountName = ui.kAccountCreatorAccount->text();
        QString name = bankName % u'-' % accountName;
        SKGAccountObject::AccountType accountType =
            static_cast<SKGAccountObject::AccountType>(ui.kAccountCreatorType->itemData(ui.kAccountCreatorType->currentIndex()).toInt());
        const bool wallet = (accountType == SKGAccountObject::WALLET);
        if (wallet) {
            bankName = QString();
        }
        {
            SKGBEGINTRANSACTION(*getDocument(), i18nc("Creating an account", "Account creation '%1'", name), err)

            // Create bank object in case of missing
            SKGBankObject bankObj(getDocument());
            IFOKDO(err, bankObj.setName(bankName))
            IFOK(err)
            {
                // Build icon name
                QString icon = wallet ? QString() : ui.kAccountCreatorIcon->currentText();
                if (!QFile(icon).exists() && !icon.isEmpty()) {
                    QString code = ui.kAccountCreatorIcon->itemData(ui.kAccountCreatorIcon->currentIndex()).toString();
                    if (!code.isEmpty()) {
                        icon += "  " % code;
                    }

                    icon.replace(' ', '_');
                    icon += QStringLiteral(".png");
                }
                err = bankObj.setIcon(icon);
            }
            IFOKDO(err, bankObj.setNumber(ui.kAccountCreatorBankNumber->text()))

            IFOKDO(err, bankObj.save())
            IFOKDO(err, bankObj.load())

            // Create account object in case of missing
            IFOKDO(err, bankObj.addAccount(accountObj))
            IFOKDO(err, accountObj.setName(accountName))
            IFOKDO(err, accountObj.setAgencyNumber(ui.kAccountCreatorAgencyNumber->text()))
            IFOKDO(err, accountObj.setAgencyAddress(ui.kAccountCreatorAddress->text()))
            IFOKDO(err, accountObj.setComment(ui.kAccountCreatorComment->text()))
            IFOKDO(err, accountObj.setNumber(ui.kAccountCreatorNumber->text()))
            IFOKDO(err, accountObj.setType(accountType))

            IFOKDO(err, accountObj.maxLimitAmountEnabled(ui.kMaxLimit->isChecked()))
            IFOKDO(err, accountObj.setMaxLimitAmount(ui.kMaxLimitAmout->value()))

            IFOKDO(err, accountObj.minLimitAmountEnabled(ui.kMinLimit->isChecked()))
            IFOKDO(err, accountObj.setMinLimitAmount(ui.kMinLimitAmout->value()))

            IFOKDO(err, accountObj.save(false))
            IFOKDO(err, setInitialBalanceFromEditor(accountObj))
            IFOKDO(err, accountObj.save())

            // Send message
            IFOKDO(err,
                   accountObj.getDocument()->sendMessage(
                       i18nc("An information to the user that something was added", "The account '%1' has been added", accountObj.getDisplayName()),
                       SKGDocument::Hidden))
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successfully created an account", "Account '%1' created", name));
            ui.kView->getView()->selectObject(accountObj.getUniqueID());
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message : Could not create an account", "Account creation failed"));
        }
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err, true);
}

void SKGBankPluginWidget::onModifyAccountClicked()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        // Get Selection
        SKGObjectBase::SKGListSKGObjectBase selection = getSelectedObjects();

        int nb = selection.count();

        // Is it a massive modification of accounts to merge them ?
        SKGBEGINPROGRESSTRANSACTION(*getDocument(), i18nc("Updating an account", "Account update"), err, nb)
        auto name = ui.kAccountCreatorAccount->text();
        if (name != NOUPDATE && !name.startsWith(QStringLiteral("="))) {
            // Is this name already existing?
            bool messageSent = false;
            SKGAccountObject p(getDocument());
            p.setName(name);
            IFOK(p.load())
            {
                if (selection.indexOf(p) == -1) {
                    // We will have to merge with the existing account
                    selection.insert(0, p);
                    nb++;

                    getDocument()->sendMessage(
                        i18nc("Information message", "You tried to modify names of selected accounts to an existing account. Accounts have been merged."));
                    messageSent = true;
                }
            }

            // Is it a massive modification of account to merge them ?
            if (nb > 1) {
                if (!messageSent) {
                    getDocument()->sendMessage(i18nc("Information message", "You tried to modify all names of selected accounts. Accounts have been merged."));
                }

                // Do the merge
                SKGAccountObject accountObj1(selection[0]);
                for (int i = 1; !err && i < nb; ++i) {
                    SKGAccountObject accountObj(selection.at(i));
                    err = accountObj1.merge(accountObj, true);
                }

                // Change selection for the rest of the transaction
                selection.clear();
                selection.push_back(accountObj1);
                nb = 1;
            }
        }

        QString bankName = ui.kAccountCreatorBank->text();
        SKGAccountObject::AccountType accountType =
            static_cast<SKGAccountObject::AccountType>(ui.kAccountCreatorType->itemData(ui.kAccountCreatorType->currentIndex()).toInt());
        const bool wallet = (accountType == SKGAccountObject::WALLET);
        if (wallet) {
            bankName = QString();
        }

        for (int i = 0; !err && i < nb; ++i) {
            SKGAccountObject accountObj(selection.at(i));
            err = accountObj.setName(name);
            IFOKDO(err, accountObj.setNumber(ui.kAccountCreatorNumber->text()))
            IFOKDO(err, setInitialBalanceFromEditor(accountObj))
            if (!err && ui.kAccountCreatorType->text() != NOUPDATE) {
                err = accountObj.setType(
                    static_cast<SKGAccountObject::AccountType>(ui.kAccountCreatorType->itemData(ui.kAccountCreatorType->currentIndex()).toInt()));
            }
            IFOKDO(err, accountObj.setAgencyNumber(ui.kAccountCreatorAgencyNumber->text()))
            IFOKDO(err, accountObj.setAgencyAddress(ui.kAccountCreatorAddress->text()))
            IFOKDO(err, accountObj.setComment(ui.kAccountCreatorComment->text()))

            if (!err && ui.kMaxLimit->checkState() != Qt::PartiallyChecked) {
                err = accountObj.maxLimitAmountEnabled(ui.kMaxLimit->isChecked());
            }
            if (!err && ui.kMaxLimitAmout->text() != NOUPDATE) {
                err = accountObj.setMaxLimitAmount(ui.kMaxLimitAmout->value());
            }

            if (!err && ui.kMinLimit->checkState() != Qt::PartiallyChecked) {
                err = accountObj.minLimitAmountEnabled(ui.kMinLimit->isChecked());
            }
            if (!err && ui.kMaxLimitAmout->text() != NOUPDATE) {
                err = accountObj.setMinLimitAmount(ui.kMinLimitAmout->value());
            }

            IFOKDO(err, accountObj.save())

            // Send message
            IFOKDO(err,
                   accountObj.getDocument()->sendMessage(i18nc("An information message", "The account '%1' has been updated", accountObj.getDisplayName()),
                                                         SKGDocument::Hidden))

            // Update bank
            SKGBankObject bankObj;
            if (bankName == NOUPDATE) {
                accountObj.getBank(bankObj);
            } else {
                if (SKGNamedObject::getObjectByName(getDocument(), QStringLiteral("bank"), bankName, bankObj).isSucceeded()) {
                    // The created bank already exist ==> update parent bank
                    IFOKDO(err, accountObj.setBank(bankObj))
                    IFOKDO(err, accountObj.save())
                } else {
                    // The bank does not exist
                    int code = KMessageBox::PrimaryAction;
                    SKGBankObject currentBank;
                    err = accountObj.getBank(currentBank);
                    IFOK(err)
                    {
                        SKGObjectBase::SKGListSKGObjectBase accounts;
                        err = currentBank.getAccounts(accounts);
                        if (!err && accounts.count() > 1) {
                            QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                            code = KMessageBox::questionTwoActions(this,
                                                                   i18nc("Question",
                                                                         "You are trying to modify bank name of account named '%1'.\nDo you want to do this "
                                                                         "modification for all accounts of this bank ? ",
                                                                         accountObj.getName()),
                                                                   i18nc("Question", "Do you want to modify the account ?"),
                                                                   KStandardGuiItem::apply(),
                                                                   KStandardGuiItem::cancel());
                            QApplication::restoreOverrideCursor();
                        }
                    }

                    IFOKDO(err, accountObj.getBank(bankObj))
                    if (code == KMessageBox::PrimaryAction) {
                        // The bank does not exist ==> update this one
                        IFOKDO(err, bankObj.setName(bankName))
                    } else {
                        // The bank does not exist ==> create a new one
                        IFOKDO(err, bankObj.resetID())
                        IFOKDO(err, bankObj.setName(bankName))
                        IFOKDO(err, bankObj.save())
                        IFOKDO(err, accountObj.setBank(bankObj))
                        IFOKDO(err, accountObj.save())
                    }
                }
            }
            if (ui.kAccountCreatorIcon->text() != NOUPDATE) {
                IFOK(err)
                {
                    // Build icon name
                    QString icon = wallet ? QString() : ui.kAccountCreatorIcon->currentText();
                    if (!QFile(icon).exists() && !icon.isEmpty()) {
                        QString code = ui.kAccountCreatorIcon->itemData(ui.kAccountCreatorIcon->currentIndex()).toString();
                        if (!code.isEmpty()) {
                            icon += "  " % code;
                        }
                        icon.replace(' ', '_');
                        icon += QStringLiteral(".png");
                    }
                    err = bankObj.setIcon(icon);
                }
            }
            IFOKDO(err, bankObj.setNumber(ui.kAccountCreatorBankNumber->text()))
            IFOKDO(err, bankObj.save())

            IFOKDO(err, getDocument()->stepForward(i + 1))
        }

        // Remove bank without account
        IFOK(err)
        {
            err = getDocument()->executeSqliteOrder(
                QStringLiteral("DELETE FROM bank WHERE NOT EXISTS(SELECT 1 FROM account WHERE account.rd_bank_id = bank.id)"));
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successfully updated an account", "Account updated.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message : Could not update an account", "Update failed"));
    }
    // Display error
    SKGMainPanel::displayErrorMessage(err, true);

    // Set focus on table
    ui.kView->getView()->setFocus();
}

void SKGBankPluginWidget::cleanEditor()
{
    if (getNbSelectedObjects() == 0) {
        ui.kAccountCreatorIcon->setText(QString());
        ui.kAccountCreatorBank->setText(QString());
        ui.kAccountCreatorAccount->setText(QString());
        ui.kAccountCreatorBankNumber->setText(QString());
        ui.kAccountCreatorAgencyNumber->setText(QString());
        ui.kAccountCreatorNumber->setText(QString());
        ui.kAccountCreatorType->setText(i18nc("Noun, a type of account", "Current"));
        ui.kAccountCreatorAddress->setText(QString());
        ui.kAccountCreatorComment->setText(QString());
        ui.kAmountEdit->setText(QStringLiteral("0"));
        ui.kUnitEdit->refershList();
    }
}

QString SKGBankPluginWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);
    root.setAttribute(QStringLiteral("currentPage"), SKGServices::intToString(ui.kWidgetSelector->getSelectedMode()));
    root.setAttribute(QStringLiteral("view"), ui.kView->getState());
    root.setAttribute(QStringLiteral("graph"), m_graph->getState());
    return doc.toString();
}

void SKGBankPluginWidget::setState(const QString &iState)
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

    ui.kView->setState(root.attribute(QStringLiteral("view")));
    m_graph->setState(root.attribute(QStringLiteral("graph")));

    onRefreshGraph();
}

QString SKGBankPluginWidget::getDefaultStateAttribute()
{
    return QStringLiteral("SKGBANK_DEFAULT_PARAMETERS");
}

void SKGBankPluginWidget::dataModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    // Refresh widgets
    if (iTableName == QStringLiteral("v_account_display") || iTableName.isEmpty()) {
        // Refresh info area
        m_timer.start(300);
    }

    if (!iLightTransaction) {
        if (iTableName == QStringLiteral("bank") || iTableName.isEmpty()) {
            // Set completions
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAccountCreatorBank,
                                                getDocument(),
                                                QStringLiteral("bank"),
                                                QStringLiteral("t_name"),
                                                QString(),
                                                true);
        }

        if (iTableName == QStringLiteral("account") || iTableName.isEmpty()) {
            // Set completions
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAccountCreatorAccount,
                                                getDocument(),
                                                QStringLiteral("account"),
                                                QStringLiteral("t_name"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAccountCreatorBankNumber,
                                                getDocument(),
                                                QStringLiteral("bank"),
                                                QStringLiteral("t_bank_number"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAccountCreatorAgencyNumber,
                                                getDocument(),
                                                QStringLiteral("account"),
                                                QStringLiteral("t_agency_number"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAccountCreatorNumber,
                                                getDocument(),
                                                QStringLiteral("account"),
                                                QStringLiteral("t_number"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAccountCreatorAddress,
                                                getDocument(),
                                                QStringLiteral("account"),
                                                QStringLiteral("t_agency_address"),
                                                QString(),
                                                true);
            SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kAccountCreatorComment,
                                                getDocument(),
                                                QStringLiteral("account"),
                                                QStringLiteral("t_comment"),
                                                QString(),
                                                true);
        }
    }
}

void SKGBankPluginWidget::refreshInfoZone()
{
    SKGTRACEINFUNC(10)
    auto *doc = qobject_cast<SKGDocumentBank *>(getDocument());
    if (doc != nullptr) {
        ui.kInfo->setText(i18nc("Message", "Computing…"));
        doc->concurrentExecuteSelectSqliteOrder(
            QStringLiteral("SELECT TOTAL(f_TODAYAMOUNT), TOTAL(f_CURRENTAMOUNT), TOTAL(f_CHECKED), TOTAL(f_COMING_SOON) from v_account_display"),
            [this, doc](const SKGStringListList &iResult) {
                if (iResult.count() == 2 && SKGMainPanel::getMainPanel()->pageIndex(this) != -1) {
                    SKGServices::SKGUnitInfo primary = doc->getPrimaryUnit();
                    double v1 = SKGServices::stringToDouble(iResult.at(1).at(0));
                    double v2 = SKGServices::stringToDouble(iResult.at(1).at(1));
                    double v3 = SKGServices::stringToDouble(iResult.at(1).at(2));
                    double v4 = SKGServices::stringToDouble(iResult.at(1).at(3));
                    QString s1 = doc->formatMoney(v1, primary);
                    QString s2 = doc->formatMoney(v2, primary);
                    QString s3 = doc->formatMoney(v3, primary);
                    QString s4 = doc->formatMoney(v4, primary);
                    ui.kInfo->setText(
                        i18nc("Information on an account's status : Balance is the current amount of money on the account, Checked is the amount of money on "
                              "your last bank's statement, To be Checked is the differences between these two values",
                              "Today balance : %1     Balance : %2     Checked : %3     To be Checked : %4",
                              s1,
                              s2,
                              s3,
                              s4));

                    SKGServices::SKGUnitInfo secondaryUnit = doc->getSecondaryUnit();
                    if (!secondaryUnit.Symbol.isEmpty() && (secondaryUnit.Value != 0.0)) {
                        s1 = doc->formatMoney(v1, secondaryUnit);
                        s2 = doc->formatMoney(v2, secondaryUnit);
                        s3 = doc->formatMoney(v3, secondaryUnit);
                        s4 = doc->formatMoney(v4, secondaryUnit);
                    }
                    ui.kInfo->setToolTip(
                        i18nc("Information on an account's status : Balance is the current amount of money on the account, Checked is the amount of money on "
                              "your last bank's statement, To be Checked is the differences between these two values",
                              "<p>Today balance : %1 < / p > <p>Balance : %2 < / p > <p>Checked : %3 < / p > <p>To be Checked : %4 < / p > ",
                              s1,
                              s2,
                              s3,
                              s4));
                }
            });
    }
}

QWidget *SKGBankPluginWidget::mainWidget()
{
    return ui.kView->getView();
}

QList<QWidget *> SKGBankPluginWidget::printableWidgets()
{
    QList<QWidget *> output;
    output.push_back(mainWidget());
    if ((m_graph != nullptr) && m_graph->isVisible()) {
        output.push_back(m_graph);
    }
    return output;
}

void SKGBankPluginWidget::activateEditor()
{
    if (ui.kWidgetSelector->getSelectedMode() == -1) {
        ui.kWidgetSelector->setSelectedMode(0);
    }
    ui.kAccountCreatorBank->setFocus();
}

bool SKGBankPluginWidget::isEditor()
{
    return true;
}
