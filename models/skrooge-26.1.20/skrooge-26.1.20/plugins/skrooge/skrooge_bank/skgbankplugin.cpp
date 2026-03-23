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
#include "skgbankplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>

#include <qstandardpaths.h>

#include "skgaccountboardwidget.h"
#include "skgaccountobject.h"
#include "skgbankobject.h"
#include "skgbankpluginwidget.h"
#include "skgdocumentbank.h"
#include "skghtmlboardwidget.h"
#include "skgmainpanel.h"
#include "skgoperationobject.h"
#include "skgsuboperationobject.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGBankPlugin, "metadata.json")

SKGBankPlugin::SKGBankPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
    , m_reconcileAction(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGBankPlugin::~SKGBankPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGBankPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_bank"), title());
    setXMLFile(QStringLiteral("skrooge_bank.rc"));

    // Menu
    auto actReconciliate = new QAction(SKGServices::fromTheme(QStringLiteral("window-duplicate")),
                                       i18nc("Verb: Reconciliation is process through which you ensure compliance with your bank's statement", "Reconcile…"),
                                       this);
    connect(actReconciliate, &QAction::triggered, this, &SKGBankPlugin::onReconciliate);
    actionCollection()->setDefaultShortcut(actReconciliate, Qt::ALT | Qt::Key_R);
    registerGlobalAction(QStringLiteral("edit_reconcile"), actReconciliate, QStringList() << QStringLiteral("account"), 1, -1, 320);
    return true;
}

int SKGBankPlugin::getNbDashboardWidgets()
{
    return 5;
}

QString SKGBankPlugin::getDashboardWidgetTitle(int iIndex)
{
    if (iIndex == 0) {
        return i18nc("Noun, a list of bank accounts", "Accounts (Medium)");
    }
    if (iIndex == 1) {
        return i18nc("Noun, a list of bank accounts", "Accounts (Full)");
    }
    if (iIndex == 2) {
        return i18nc("Noun, a list of banks", "Banks (Light)");
    }
    if (iIndex == 3) {
        return i18nc("Noun, a list of banks", "Banks (Full)");
    }
    return i18nc("Noun, a list of bank accounts", "Accounts (Light)");
}

SKGBoardWidget *SKGBankPlugin::getDashboardWidget(int iIndex)
{
    auto listForFilter = std::move(QStringList() << QStringLiteral("t_name") << QStringLiteral("t_number") << QStringLiteral("t_agency_number")
                                                 << QStringLiteral("t_agency_address") << QStringLiteral("t_comment") << QStringLiteral("t_bookmarked")
                                                 << QStringLiteral("t_TYPENLS") << QStringLiteral("t_BANK") << QStringLiteral("t_BANK_NUMBER"));
    if (iIndex == 0) {
        return new SKGAccountBoardWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
    }
    if (iIndex == 1) {
        return new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                      m_currentBankDocument,
                                      getDashboardWidgetTitle(iIndex),
                                      QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/account_table.qml")),
                                      QStringList() << QStringLiteral("v_account_display"),
                                      SKGSimplePeriodEdit::PREVIOUS_AND_CURRENT_PERIODS,
                                      listForFilter);
    }
    if (iIndex == 2) {
        auto w =
            new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                   m_currentBankDocument,
                                   getDashboardWidgetTitle(iIndex),
                                   QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/bank_table_light.qml")),
                                   QStringList() << QStringLiteral("v_account_display"),
                                   SKGSimplePeriodEdit::NONE,
                                   listForFilter);

        QStringList overlayopen;
        overlayopen.push_back(QStringLiteral("quickopen"));
        auto open = new QAction(SKGServices::fromTheme(QStringLiteral("view-statistics"), overlayopen), i18nc("Verb", "Open report…"), w);
        connect(open, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
            SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
        });
        QString u =
            QStringLiteral(
                "skg://skrooge_report_plugin/?grouped=Y&transfers=Y&tracked=Y&expenses=Y&incomes=Y&lines2=t_BANK&currentPage=-1&mode=0&interval=3&period=0")
            % "&tableAndGraphState.graphMode=2&tableAndGraphState.allPositive=N&tableAndGraphState.show=graph&columns="
            % SKGServices::encodeForUrl(QStringLiteral("#NOTHING#"));
        open->setData(u);

        w->addAction(open);
        return w;
    }
    if (iIndex == 3) {
        return new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                      m_currentBankDocument,
                                      getDashboardWidgetTitle(iIndex),
                                      QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/bank_table.qml")),
                                      QStringList() << QStringLiteral("v_account_display"),
                                      SKGSimplePeriodEdit::PREVIOUS_AND_CURRENT_PERIODS,
                                      listForFilter);
    }

    listForFilter = QStringList() << QStringLiteral("t_name") << QStringLiteral("t_number") << QStringLiteral("t_agency_number")
                                  << QStringLiteral("t_agency_address") << QStringLiteral("t_comment") << QStringLiteral("t_TYPENLS")
                                  << QStringLiteral("t_BANK") << QStringLiteral("t_BANK_NUMBER") << QStringLiteral("t_close") << QStringLiteral("t_type")
                                  << QStringLiteral("t_bookmarked");
    return new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                  m_currentBankDocument,
                                  getDashboardWidgetTitle(iIndex),
                                  QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/account_table_light.qml")),
                                  QStringList() << QStringLiteral("v_account_display"),
                                  SKGSimplePeriodEdit::PREVIOUS_AND_CURRENT_PERIODS,
                                  listForFilter);
}

SKGTabPage *SKGBankPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGBankPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QString SKGBankPlugin::title() const
{
    return i18nc("Display a list of Accounts", "Accounts");
}

QString SKGBankPlugin::icon() const
{
    return QStringLiteral("view-bank");
}

QString SKGBankPlugin::toolTip() const
{
    return i18nc("This allows the user to manage his list of accounts", "Manage your accounts");
}

QStringList SKGBankPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tip", "<p>… you can associate a logo with your <a href=\"skg://skrooge_bank_plugin\">banks</a>.</p>"));
    output.push_back(i18nc("Description of a tip", "<p>… <a href=\"skg://skrooge_bank_plugin\">accounts</a> can be merged by drag & drop.</p>"));
    output.push_back(
        i18nc("Description of a tip",
              "<p>… you can set a minimum and a maximum limit on your <a href=\"skg://skrooge_bank_plugin\">accounts</a>. This will trigger an alarm.</p>"));
    return output;
}

int SKGBankPlugin::getOrder() const
{
    // Must be one of the first
    return 10;
}

bool SKGBankPlugin::isInPagesChooser() const
{
    return true;
}

void SKGBankPlugin::onReconciliate()
{
    if ((m_currentBankDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        // Open in transaction plugin
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        for (int i = 0; i < nb; ++i) {
            SKGAccountObject accountObj(selection.at(i));
            SKGMainPanel::getMainPanel()->openPage("skg://skrooge_operation_plugin/?modeInfoZone=1&currentPage=-1&account="
                                                   % SKGServices::encodeForUrl(accountObj.getName()));
        }
    }
}

SKGAdviceList SKGBankPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    output.reserve(40);

    // Get bank without accounts
    if (!iIgnoredAdvice.contains(QStringLiteral("skgbankplugin_withoutaccount"))) {
        SKGStringListList result;
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT bank.t_name FROM bank WHERE NOT EXISTS (SELECT 1 FROM account WHERE account.rd_bank_id=bank.id)"),
            result);
        int nb = result.count();
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &bank = line.at(0);

            SKGAdvice ad;
            ad.setUUID("skgbankplugin_withoutaccount|" % bank);
            ad.setPriority(3);
            ad.setShortMessage(i18nc("A bank is in the list of used banks, but it doesn't have any account attached", "Bank '%1' has no account", bank));
            ad.setLongMessage(i18nc("User can delete banks with no accounts", "Do not forget to remove useless banks"));
            QStringList autoCorrections;
            autoCorrections.push_back(i18nc("Action to delete a bank", "Delete '%1'", bank));
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Get accounts closed with money
    if (!iIgnoredAdvice.contains(QStringLiteral("skgbankplugin_closedaccount"))) {
        SKGStringListList result;
        m_currentBankDocument->executeSelectSqliteOrder(QStringLiteral("SELECT t_name FROM v_account_amount WHERE f_CURRENTAMOUNT>0.1 AND t_close='Y'"),
                                                        result);
        int nb = result.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        for (int i = 1; i < nb; ++i) { // Ignore header
            // Get parameters
            const QStringList &line = result.at(i);
            const QString &account = line.at(0);

            SKGAdvice ad;
            ad.setUUID("skgbankplugin_closedaccount|" % account);
            ad.setPriority(3);
            ad.setShortMessage(i18nc("A account is closed but the amount is not equal to 0", "Closed account '%1' still has money", account));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "This is may be not normal"));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Action to reopen the account", "Reopen '%1'", account);
                a.IconName = QStringLiteral("edit_undo");
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Action to create a fake transaction to set the amount of the account to 0", "Create fake transaction");
                a.IconName = QStringLiteral("edit-delete");
                a.IsRecommended = true;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    return output;
}

SKGError SKGBankPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgbankplugin_withoutaccount|"))) {
        // Get parameters
        QString bank = iAdviceIdentifier.right(iAdviceIdentifier.length() - 29);

        SKGError err;
        {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Delete banks with no account", "Delete unused banks"), err)

            SKGBankObject bankObj(m_currentBankDocument);
            err = bankObj.setName(bank);
            IFOKDO(err, bankObj.load())
            IFOKDO(err, bankObj.remove())
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successfully deleted a bank with no account", "Unused bank deleted")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Could not delete a bank with no account", "Unused bank deletion failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);

        return SKGError();
    }
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgbankplugin_closedaccount|"))) {
        // Get parameters
        QString account = iAdviceIdentifier.right(iAdviceIdentifier.length() - 28);

        SKGAccountObject accountObj(m_currentBankDocument);
        SKGError err = accountObj.setName(account);
        IFOKDO(err, accountObj.load())

        if (iSolution == 0) {
            {SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Reopen a closed account", "Reopen account '%1'", account), err)
                 IFOKDO(err, accountObj.setClosed(false)) IFOKDO(err, accountObj.save())}

            // status bar
            IFOKDO(err, SKGError(0, i18nc("Successfully reopen account", "Account reopened"))) else
            {
                err.addError(ERR_FAIL, i18nc("Failure", "reopening of the account failed"));
            }
        } else if (iSolution == 1) {
            {
                SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Create fake transaction"), err)

                SKGOperationObject op;
                IFOKDO(err, accountObj.setClosed(false))
                IFOKDO(err, accountObj.addOperation(op))
                IFOKDO(err, op.setDate(QDate::currentDate()))
                IFOKDO(err, op.setComment(i18nc("Noun, default comment for a fake transaction", "fake transaction")))
                SKGUnitObject unit;
                IFOKDO(err, accountObj.getUnit(unit))
                IFOKDO(err, op.setUnit(unit))
                IFOKDO(err, op.save())

                SKGSubOperationObject sop;
                IFOKDO(err, op.addSubOperation(sop))
                IFOKDO(err, sop.setQuantity(-accountObj.getAmount(QDate::currentDate())))
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
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);

        return SKGError();
    }
    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

#include <skgbankplugin.moc>
