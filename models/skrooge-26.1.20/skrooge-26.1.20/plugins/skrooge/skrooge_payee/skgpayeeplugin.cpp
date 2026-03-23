/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A skrooge plugin to payee transactions
 *
 * @author Stephane MANKOWSKI
 */
#include "skgpayeeplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include "skgdocumentbank.h"
#include "skgpayee_settings.h"
#include "skgpayeepluginwidget.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGPayeePlugin, "metadata.json")

SKGPayeePlugin::SKGPayeePlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGPayeePlugin::~SKGPayeePlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGPayeePlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    if (qobject_cast<SKGDocumentBank *>(iDocument) == nullptr) {
        return false;
    }

    m_currentBankDocument = iDocument;

    setComponentName(QStringLiteral("skrooge_payee"), title());
    setXMLFile(QStringLiteral("skrooge_payee.rc"));

    // Actions
    QStringList overlaydelete;
    overlaydelete.push_back(QStringLiteral("edit-delete"));

    auto deleteUnusedPayeesAction = new QAction(SKGServices::fromTheme(icon(), overlaydelete), i18nc("Verb", "Delete unused payees"), this);
    connect(deleteUnusedPayeesAction, &QAction::triggered, this, &SKGPayeePlugin::deleteUnusedPayees);
    registerGlobalAction(QStringLiteral("clean_delete_unused_payees"), deleteUnusedPayeesAction);

    // ------------
    auto actTmp = new QAction(SKGServices::fromTheme(icon()), i18nc("Verb", "Open similar payees…"), this);
    actTmp->setData(
        QString("skg://skrooge_payee_plugin/?title_icon=" % icon() % "&title=" % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Similar payees"))
                % "&whereClause="
                % SKGServices::encodeForUrl(QStringLiteral("EXISTS (SELECT 1 FROM payee p2 WHERE p2.id<>v_payee_display.id AND "
                                                           "upper(p2.t_name)=upper(v_payee_display.t_name) AND p2.t_name<>v_payee_display.t_name)"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_similar_payees"), actTmp);

    return true;
}

SKGTabPage *SKGPayeePlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGPayeePluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

KConfigSkeleton *SKGPayeePlugin::getPreferenceSkeleton()
{
    return skgpayee_settings::self();
}

QString SKGPayeePlugin::title() const
{
    return i18nc("Noun, something that is used to track items", "Payees");
}

QString SKGPayeePlugin::icon() const
{
    return QStringLiteral("user-group-properties");
}

QString SKGPayeePlugin::toolTip() const
{
    return i18nc("A tool tip", "Payees management");
}

int SKGPayeePlugin::getOrder() const
{
    return 28;
}

QStringList SKGPayeePlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… <a href=\"skg://skrooge_payee_plugin\">payees</a> can be merged by drag & drop.</p>"));
    return output;
}

bool SKGPayeePlugin::isInPagesChooser() const
{
    return true;
}

SKGAdviceList SKGPayeePlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    // Check unused payee
    if (!iIgnoredAdvice.contains(QStringLiteral("skgpayeeplugin_unused"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(QStringLiteral("payee LEFT JOIN operation ON operation.r_payee_id=payee.id"),
                                            QStringLiteral("operation.id IS NULL"),
                                            exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgpayeeplugin_unused"));
            ad.setPriority(5);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Many unused payees"));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)", "You can improve performances by removing payees for which no transaction is registered."));
            SKGAdvice::SKGAdviceActionList autoCorrections;
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = QStringLiteral("skg://clean_delete_unused_payees");
                a.IsRecommended = true;
                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Check payee with different case
    if (!iIgnoredAdvice.contains(QStringLiteral("skgpayeeplugin_case"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(
            QStringLiteral("payee"),
            QStringLiteral("EXISTS (SELECT 1 FROM payee p2 WHERE p2.id>payee.id AND p2.t_name=payee.t_name COLLATE NOCASE AND p2.t_name<>payee.t_name)"),
            exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgpayeeplugin_case"));
            ad.setPriority(3);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Some payees seem to be identical"));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "Some payee seem to be identical but with different syntax. They could be merged."));
            QStringList autoCorrections;
            autoCorrections.push_back(QStringLiteral("skg://view_open_similar_payees"));
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }
    return output;
}

void SKGPayeePlugin::deleteUnusedPayees() const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (m_currentBankDocument != nullptr) {
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Delete unused payees"), err)

        // Modification of payee object
        QString sql = QStringLiteral(
            "DELETE FROM payee  WHERE id IN (SELECT payee.id FROM payee LEFT JOIN operation ON operation.r_payee_id=payee.id WHERE operation.id IS NULL)");
        err = m_currentBankDocument->executeSqliteOrder(sql);
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Unused payees deleted")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Unused payees deletion failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

#include <skgpayeeplugin.moc>
