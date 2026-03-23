/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to manage budgets
 *
 * @author Stephane MANKOWSKI
 */
#include "skgbudgetplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <klocalizedstring.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgbudgetobject.h"
#include "skgbudgetpluginwidget.h"
#include "skgbudgetruleobject.h"
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGBudgetPlugin, "metadata.json")

SKGBudgetPlugin::SKGBudgetPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGBudgetPlugin::~SKGBudgetPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGBudgetPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_budget"), title());
    setXMLFile(QStringLiteral("skrooge_budget.rc"));

    // Create yours actions here
    // -----------
    QStringList overlayrun;
    overlayrun.push_back(QStringLiteral("system-run"));
    auto act = new QAction(SKGServices::fromTheme(icon(), overlayrun), i18nc("Verb", "Process budget rules"), this);
    connect(act, &QAction::triggered, this, &SKGBudgetPlugin::onProcessRules);
    registerGlobalAction(QStringLiteral("tool_process_budget_rules"), act);
    return true;
}

SKGTabPage *SKGBudgetPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGBudgetPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QString SKGBudgetPlugin::title() const
{
    return i18nc("The title", "Budget");
}

QString SKGBudgetPlugin::icon() const
{
    return QStringLiteral("view-calendar-whatsnext");
}

QString SKGBudgetPlugin::toolTip() const
{
    return i18nc("The tool tip", "Budget");
}

int SKGBudgetPlugin::getOrder() const
{
    return 32;
}

QStringList SKGBudgetPlugin::tips() const
{
    QStringList output;
    return output;
}

bool SKGBudgetPlugin::isInPagesChooser() const
{
    return true;
}

SKGAdviceList SKGBudgetPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    QString month = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
    QString year = QDate::currentDate().toString(QStringLiteral("yyyy"));

    // Alarms
    if (!iIgnoredAdvice.contains(QStringLiteral("skgbudgetplugin_alarm"))) {
        SKGObjectBase::SKGListSKGObjectBase budgets;
        // Query is done on v_budget_display because v_budget_display is faster than v_budget
        SKGError err = m_currentBankDocument->getObjects(
            QStringLiteral("v_budget_display"),
            "(f_CURRENTAMOUNT/f_budgeted_modified>0.8 OR (f_CURRENTAMOUNT<0 AND f_budgeted_modified>0)) AND ABS(f_CURRENTAMOUNT-f_budgeted_modified)>"
                % SKGServices::doubleToString(EPSILON) % " AND f_budgeted<0 AND (t_PERIOD='" % month % "' OR t_PERIOD='" % year % "');",
            budgets);
        int nb = budgets.count();
        if (nb != 0) {
            SKGServices::SKGUnitInfo primary = m_currentBankDocument->getPrimaryUnit();
            SKGAdvice::SKGAdviceActionList autoCorrections;
            for (int i = 0; !err && i < nb; ++i) {
                SKGBudgetObject budget(budgets.at(i));
                double f_CURRENTAMOUNT = SKGServices::stringToDouble(budget.getAttribute(QStringLiteral("f_CURRENTAMOUNT")));
                double f_budgeted_modified = SKGServices::stringToDouble(budget.getAttribute(QStringLiteral("f_budgeted_modified")));

                SKGAdvice ad;
                ad.setUUID("skgbudgetplugin_alarm|" % SKGServices::intToString(budget.getID()));
                ad.setPriority((f_CURRENTAMOUNT < f_budgeted_modified ? 9 : 6));
                ad.setShortMessage(i18nc("Advice on making the best (short)", "Budget alarm for '%1'", budget.getAttribute(QStringLiteral("t_CATEGORY"))));
                ad.setLongMessage(i18nc("Advice on making the best (long)",
                                        "Take care to your budget.<br> %1.<br>%2 / %3",
                                        budget.getAttribute(QStringLiteral("t_CATEGORY")),
                                        m_currentBankDocument->formatMoney(f_CURRENTAMOUNT, primary),
                                        m_currentBankDocument->formatMoney(f_budgeted_modified, primary)));
                autoCorrections.resize(0);
                {
                    SKGAdvice::SKGAdviceAction a;
                    a.Title = i18nc("Advice on making the best (action)", "Open transactions corresponding to this budget");
                    a.IconName = QStringLiteral("quickopen");
                    a.IsRecommended = false;
                    autoCorrections.push_back(a);
                }
                ad.setAutoCorrections(autoCorrections);
                output.push_back(ad);
            }
        }
    }

    if (!iIgnoredAdvice.contains(QStringLiteral("skgbudgetplugin_oldprocessing"))) {
        QString lastBudgetProcessingDate = m_currentBankDocument->getParameter(QStringLiteral("SKG_LAST_BUDGET_PROCESSING"));
        if (lastBudgetProcessingDate.isEmpty() || SKGServices::stringToDate(lastBudgetProcessingDate) < QDate::currentDate().addMonths(-1)) { // Ignore header
            bool exist = false;
            m_currentBankDocument->existObjects(QStringLiteral("budgetrule"), QString(), exist);
            if (exist) {
                SKGAdvice ad;
                ad.setUUID(QStringLiteral("skgbudgetplugin_oldprocessing"));
                ad.setPriority(3);
                ad.setShortMessage(i18nc("Advice on making the best (short)", "Old budget treatment"));
                ad.setLongMessage(i18nc("Advice on making the best (long)", "The budget has not been treated for at least one month."));
                SKGAdvice::SKGAdviceActionList autoCorrections;
                {
                    SKGAdvice::SKGAdviceAction a;
                    a.Title = QStringLiteral("skg://tool_process_budget_rules");
                    a.IsRecommended = true;
                    autoCorrections.push_back(a);
                }
                ad.setAutoCorrections(autoCorrections);
                output.push_back(ad);
            }
        }
    }

    return output;
}
SKGError SKGBudgetPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgbudgetplugin_alarm|"))) {
        // Get parameters
        QString id = iAdviceIdentifier.right(iAdviceIdentifier.length() - 22);
        SKGBudgetObject budget(m_currentBankDocument, SKGServices::stringToInt(id));
        budget.load();

        QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("open"));
        if (act != nullptr) {
            act->setData(budget.getUniqueID());
            act->trigger();
        }

        return SKGError();
    }
    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

void SKGBudgetPlugin::onProcessRules()
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)

    {
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Process budget rules"), err)
        err = SKGBudgetRuleObject::processAllRules(m_currentBankDocument);
    }
    // status bar
    IFOK(err)
    {
        err = SKGError(0, i18nc("Successful message after an user action", "Budget rules processed"));
    }
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Budget rules failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

#include <skgbudgetplugin.moc>
