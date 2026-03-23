/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to calculate
 *
 * @author Stephane MANKOWSKI
 */
#include "skgcalculatorplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgaccountobject.h"
#include "skgcalculatorpluginwidget.h"
#include "skgdocumentbank.h"
#include "skghtmlboardwidget.h"
#include "skgtraces.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGCalculatorPlugin, "metadata.json")

SKGCalculatorPlugin::SKGCalculatorPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGCalculatorPlugin::~SKGCalculatorPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGCalculatorPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_calculator"), title());
    setXMLFile(QStringLiteral("skrooge_calculator.rc"));

    // Create yours actions here
    return true;
}

int SKGCalculatorPlugin::getNbDashboardWidgets()
{
    return 1;
}

QString SKGCalculatorPlugin::getDashboardWidgetTitle(int iIndex)
{
    Q_UNUSED(iIndex)
    return i18nc("The estimated amount of money earned through interests on a remunerated account", "Estimated interest");
}

SKGBoardWidget *SKGCalculatorPlugin::getDashboardWidget(int iIndex)
{
    Q_UNUSED(iIndex)
    auto listForFilter = std::move(QStringList() << QStringLiteral("t_name") << QStringLiteral("t_number") << QStringLiteral("t_agency_number")
                                                 << QStringLiteral("t_agency_address") << QStringLiteral("t_comment") << QStringLiteral("t_bookmarked")
                                                 << QStringLiteral("t_TYPENLS") << QStringLiteral("t_BANK") << QStringLiteral("t_BANK_NUMBER"));

    return new SKGHtmlBoardWidget(SKGMainPanel::getMainPanel(),
                                  m_currentBankDocument,
                                  getDashboardWidgetTitle(iIndex),
                                  QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/interests.qml")),
                                  QStringList() << QStringLiteral("v_account_display") << QStringLiteral("interest"),
                                  SKGSimplePeriodEdit::PREVIOUS_AND_CURRENT_YEARS,
                                  listForFilter);
}

SKGTabPage *SKGCalculatorPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGCalculatorPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QString SKGCalculatorPlugin::title() const
{
    return toolTip();
}

QString SKGCalculatorPlugin::icon() const
{
    return QStringLiteral("dialog-scripts");
}

QString SKGCalculatorPlugin::toolTip() const
{
    return i18nc("Compute financial simulations for various situations (interests…)", "Simulations");
}

int SKGCalculatorPlugin::getOrder() const
{
    return 100;
}

QStringList SKGCalculatorPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… you can use the <a href=\"skg://skrooge_calculator_plugin\">calculator</a> for many things</p>"));
    return output;
}

bool SKGCalculatorPlugin::isInPagesChooser() const
{
    return true;
}
SKGAdviceList SKGCalculatorPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;

    // Search investment accounts without interest rate
    if (!iIgnoredAdvice.contains(QStringLiteral("skgcalculatorplugin_nointerest"))) {
        SKGObjectBase::SKGListSKGObjectBase accounts;
        m_currentBankDocument->getObjects(
            QStringLiteral("account"),
            QStringLiteral("t_type='I' AND t_close='N' AND NOT EXISTS (SELECT 1 FROM interest WHERE interest.rd_account_id=account.id)"),
            accounts);
        int nb = accounts.count();
        SKGAdvice::SKGAdviceActionList autoCorrections;
        for (int i = 0; i < nb; ++i) {
            SKGAccountObject account(accounts.at(i));
            SKGAdvice ad;
            ad.setUUID("skgcalculatorplugin_nointerest|" % account.getName());
            ad.setPriority(3);
            ad.setShortMessage(
                i18nc("User did not define an interest rate on an investment account", "No interest rate defined for account '%1'", account.getName()));
            ad.setLongMessage(i18nc("User did not define an interest rate on an investment account",
                                    "Your investment account '%1' doesn't have interest rate defined",
                                    account.getName()));
            autoCorrections.resize(0);
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = i18nc("Link allowing user to open a new tab for defining interests parameters", "Open interest page");
                a.IconName = QStringLiteral("quickopen");
                a.IsRecommended = false;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }
    return output;
}

SKGError SKGCalculatorPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgcalculatorplugin_nointerest|"))) {
        // Get parameters
        QString account = iAdviceIdentifier.right(iAdviceIdentifier.length() - 31);
        SKGMainPanel::getMainPanel()->openPage("skg://skrooge_calculator_plugin/?currentPage=0&account=" % SKGServices::encodeForUrl(account));
        return SKGError();
    }

    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

#include <skgcalculatorplugin.moc>
