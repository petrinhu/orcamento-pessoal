/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is Skrooge plugin to generate report.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgreportplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <qaction.h>

#include <qdom.h>

#include "skgaccountobject.h"
#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skghtmlboardwidget.h"
#include "skgmainpanel.h"
#include "skgnodeobject.h"
#include "skgpayeeobject.h"
#include "skgreport_settings.h"
#include "skgreportboardwidget.h"
#include "skgreportpluginwidget.h"
#include "skgruleobject.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtrackerobject.h"
#include "skgunitobject.h"

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGReportPlugin, "metadata.json")

SKGReportPlugin::SKGReportPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGReportPlugin::~SKGReportPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
}

bool SKGReportPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    setComponentName(QStringLiteral("skrooge_report"), title());
    setXMLFile(QStringLiteral("skrooge_report.rc"));

    // Menu
    QStringList overlayopen;
    overlayopen.push_back(QStringLiteral("quickopen"));
    auto actOpenReport = new QAction(SKGServices::fromTheme(QStringLiteral("view-statistics"), overlayopen), i18nc("Verb", "Open report…"), this);
    connect(actOpenReport, &QAction::triggered, this, &SKGReportPlugin::onOpenReport);
    actionCollection()->setDefaultShortcut(actOpenReport, Qt::META | Qt::Key_R);
    registerGlobalAction(QStringLiteral("open_report"),
                         actOpenReport,
                         QStringList() << QStringLiteral("operation") << QStringLiteral("suboperation") << QStringLiteral("account") << QStringLiteral("unit")
                                       << QStringLiteral("category") << QStringLiteral("refund") << QStringLiteral("payee") << QStringLiteral("rule"),
                         1,
                         -1,
                         120);

    // ---------------
    {
        auto act = new QAction(SKGServices::fromTheme(QStringLiteral("security-low"), overlayopen), i18nc("Verb", "Open very old transactions…"), this);
        act->setData(QString("skg://skrooge_operation_plugin/?title_icon=security-low&title="
                             % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Very old transactions")) % "&operationWhereClause="
                             % SKGServices::encodeForUrl(QStringLiteral("d_date<=(date('now', 'localtime', '-50 year')) AND d_date<>'0000-00-00'"))));
        connect(act, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
            SKGMainPanel::getMainPanel()->openPage();
        });
        registerGlobalAction(QStringLiteral("view_open_very_old_operations"), act);
    }

    // ---------------
    {
        auto act =
            new QAction(SKGServices::fromTheme(QStringLiteral("security-low"), overlayopen), i18nc("Verb", "Open very far transactions in the future…"), this);
        act->setData(QString("skg://skrooge_operation_plugin/?title_icon=security-low&title="
                             % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Very far transactions in the future")) % "&operationWhereClause="
                             % SKGServices::encodeForUrl(QStringLiteral("d_date>=(date('now', 'localtime', '+50 year'))"))));
        connect(act, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
            SKGMainPanel::getMainPanel()->openPage();
        });
        registerGlobalAction(QStringLiteral("view_open_very_far_operations"), act);
    }
    return true;
}

int SKGReportPlugin::getNbDashboardWidgets()
{
    // Count the number of bookmark on reports
    int nb = 0;
    m_currentBankDocument->getNbObjects(QStringLiteral("node"), QStringLiteral("t_data like '\"Skrooge report plugin\";%' "), nb);
    return 2 + nb;
}

QString SKGReportPlugin::getDashboardWidgetTitle(int iIndex)
{
    Q_UNUSED(iIndex)
    if (iIndex == 0) {
        return i18nc("Noun, the title of a section", "Report");
    }
    if (iIndex == 1) {
        return i18nc("Noun, the title of a section", "Personal Financial Score");
    }
    SKGObjectBase::SKGListSKGObjectBase objs;
    m_currentBankDocument->getObjects(QStringLiteral("node"), QStringLiteral("t_data like '\"Skrooge report plugin\";%' ORDER BY t_fullname"), objs);
    if (iIndex - 2 < objs.count()) {
        return i18nc("Noun, the title of a section", "Report bookmarked named \"%1\"", objs[iIndex - 2].getAttribute(QStringLiteral("t_fullname")));
    }

    return QString();
}

SKGBoardWidget *SKGReportPlugin::getDashboardWidget(int iIndex)
{
    Q_UNUSED(iIndex)
    if (iIndex == 0) {
        return new SKGReportBoardWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
    }
    if (iIndex == 1) {
        return new SKGHtmlBoardWidget(
            SKGMainPanel::getMainPanel(),
            m_currentBankDocument,
            getDashboardWidgetTitle(iIndex) % " - %1",
            QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/personal_finance_score.qml")),
            QStringList() << QStringLiteral("v_suboperation_consolidated"),
            SKGSimplePeriodEdit::PREVIOUS_MONTHS);
    }
    SKGObjectBase::SKGListSKGObjectBase objs;
    m_currentBankDocument->getObjects(QStringLiteral("node"), QStringLiteral("t_data like '\"Skrooge report plugin\";%' ORDER BY t_fullname"), objs);
    if (iIndex - 2 < objs.count()) {
        auto *report = new SKGReportBoardWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
        QString state = SKGServices::splitCSVLine(objs.at(iIndex - 2).getAttribute(QStringLiteral("t_data"))).at(2);
        state = state.replace(QStringLiteral("isToolBarVisible=&amp;quot;Y&amp;quot;"), QStringLiteral("isToolBarVisible=&amp;quot;N&amp;quot;"));
        state = state.replace(QStringLiteral("show=&quot;&amp;quot;table&amp;quot;;&amp;quot;graph&amp;quot;&quot;"),
                              QStringLiteral("show=&quot;&amp;quot;graph&amp;quot;&quot;"));
        state = state.replace(QStringLiteral("currentPage=\"0\""), QStringLiteral("currentPage=\"-1\""));
        report->setState(state);
        return report;
    }

    return nullptr;
}

SKGTabPage *SKGReportPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGReportPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QWidget *SKGReportPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    auto w = new QWidget();
    ui.setupUi(w);
    return w;
}

KConfigSkeleton *SKGReportPlugin::getPreferenceSkeleton()
{
    return skgreport_settings::self();
}

QString SKGReportPlugin::title() const
{
    return i18nc("Noun", "Report");
}

QString SKGReportPlugin::icon() const
{
    return QStringLiteral("view-statistics");
}

QString SKGReportPlugin::toolTip() const
{
    return i18nc("Noun", "Generate report");
}

int SKGReportPlugin::getOrder() const
{
    return 40;
}

QStringList SKGReportPlugin::tips() const
{
    QStringList output;
    output.push_back(
        i18nc("Description of a tips",
              "<p>… you can double click on a value in <a href=\"skg://skrooge_report_plugin\">reports</a> to show corresponding transactions.</p>"));
    output.push_back(
        i18nc("Description of a tips", "<p>… you can open <a href=\"skg://skrooge_report_plugin\">reports</a> for selections made in other pages.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can export <a href=\"skg://skrooge_report_plugin\">reports</a> in many formats.</p>"));
    return output;
}

bool SKGReportPlugin::isInPagesChooser() const
{
    return true;
}

void SKGReportPlugin::getTitleAndWhereClause(const SKGObjectBase::SKGListSKGObjectBase &iSelection, QString &oTitle, QString &oWhereClause) const
{
    int nb = iSelection.count();
    if (nb > 0) {
        QString table = iSelection.at(0).getRealTable();
        if (table == QStringLiteral("account")) {
            oWhereClause = QStringLiteral("rd_account_id in (");
            oTitle = i18nc("Noun, a list of items", "Transactions of account ");

            for (int i = 0; i < nb; ++i) {
                SKGAccountObject tmp(iSelection.at(i));
                if (i != 0) {
                    oWhereClause += ',';
                    oTitle += ',';
                }
                oWhereClause += SKGServices::intToString(tmp.getID());
                oTitle += i18n("'%1'", tmp.getName());
            }
            oWhereClause += ')';
        } else if (table == QStringLiteral("unit")) {
            oWhereClause = QStringLiteral("rc_unit_id in (");
            oTitle = i18nc("Noun, a list of items", "Transactions with Unit equal to ");

            for (int i = 0; i < nb; ++i) {
                SKGUnitObject tmp(iSelection.at(i));
                if (i != 0) {
                    oWhereClause += ',';
                    oTitle += ',';
                }
                oWhereClause += SKGServices::intToString(tmp.getID());
                oTitle += i18n("'%1'", tmp.getName());
            }
            oWhereClause += ')';
        } else if (table == QStringLiteral("category")) {
            oWhereClause = QStringLiteral("t_REALCATEGORY in (");
            QString wc2;
            oTitle = i18nc("Noun, a list of items", "Transactions with Category equal to ");

            for (int i = 0; i < nb; ++i) {
                SKGCategoryObject tmp(iSelection.at(i));
                if (i != 0) {
                    oWhereClause += ',';
                    wc2 += QStringLiteral(" OR ");
                    oTitle += ',';
                }
                oWhereClause += '\'' % SKGServices::stringToSqlString(tmp.getFullName()) % u'\'';
                wc2 += "t_REALCATEGORY like '" % SKGServices::stringToSqlString(tmp.getFullName()) % "%'";
                oTitle += i18n("'%1'", tmp.getFullName());
            }
            oWhereClause += ") OR " % wc2;
        } else if (table == QStringLiteral("refund")) {
            oWhereClause = QStringLiteral("r_refund_id in (");
            oTitle = i18nc("Noun, a list of items", "Transactions followed by ");

            for (int i = 0; i < nb; ++i) {
                SKGTrackerObject tmp(iSelection.at(i));
                if (i != 0) {
                    oWhereClause += ',';
                    oTitle += ',';
                }
                oWhereClause += SKGServices::intToString(tmp.getID());
                oTitle += i18n("'%1'", tmp.getName());
            }
            oWhereClause += ')';
        } else if (table == QStringLiteral("payee")) {
            oWhereClause = QStringLiteral("r_payee_id in (");
            oTitle = i18nc("Noun, a list of items", "Transactions assigned to ");

            for (int i = 0; i < nb; ++i) {
                SKGPayeeObject tmp(iSelection.at(i));
                if (i != 0) {
                    oWhereClause += ',';
                    oTitle += ',';
                }
                oWhereClause += SKGServices::intToString(tmp.getID());
                oTitle += i18n("'%1'", tmp.getName());
            }
            oWhereClause += ')';
        } else if (table == QStringLiteral("operation")) {
            oWhereClause = QStringLiteral("i_OPID in (");
            oTitle = i18nc("Noun, a list of items", "Selected transactions");

            for (int i = 0; i < nb; ++i) {
                if (i != 0) {
                    oWhereClause += ',';
                }
                oWhereClause += SKGServices::intToString(iSelection.at(i).getID());
            }
            oWhereClause += ')';
        } else if (table == QStringLiteral("suboperation")) {
            oWhereClause = QStringLiteral("i_SUBOPID in (");
            oTitle = i18nc("Noun, a list of items", "Selected sub transactions");

            for (int i = 0; i < nb; ++i) {
                if (i != 0) {
                    oWhereClause += ',';
                }
                oWhereClause += SKGServices::intToString(iSelection.at(i).getID());
            }
            oWhereClause += ')';
        } else if (table == QStringLiteral("rule")) {
            oTitle = i18nc("Noun, a list of items", "Transactions corresponding to rule ");

            for (int i = 0; i < nb; ++i) {
                SKGRuleObject tmp(iSelection.at(i));
                QString ruleWc = tmp.getSelectSqlOrder();
                if (!ruleWc.isEmpty()) {
                    if (!oWhereClause.isEmpty()) {
                        oWhereClause += QStringLiteral(" OR ");
                        oTitle += ',';
                    }
                    oWhereClause += u'(' % ruleWc % u')';
                    oTitle += i18n("'%1'", tmp.getSearchDescription());
                }
            }
        }
    }
}

void SKGReportPlugin::onOpenReport()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();

        int nb = selection.count();
        if (nb > 0) {
            QString titleTable;
            QString wc;
            getTitleAndWhereClause(selection, titleTable, wc);

            // Call report plugin
            SKGMainPanel::getMainPanel()->openPage("skg://skrooge_report_plugin/?period=0&title_icon=" % icon() % "&title="
                                                   % SKGServices::encodeForUrl(titleTable) % "&operationWhereClause=" % SKGServices::encodeForUrl(wc));
        }
    }
}

SKGAdviceList SKGReportPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;

    // Very old operation
    if (!iIgnoredAdvice.contains(QStringLiteral("skgreportplugin_veryold"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(QStringLiteral("operation"),
                                            QStringLiteral("d_date<=(date('now', 'localtime', '-50 year')) AND d_date<>'0000-00-00'"),
                                            exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgreportplugin_veryold"));
            ad.setPriority(3);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Some transactions are very old"));
            ad.setLongMessage(
                i18nc("Advice on making the best (long)",
                      "For performances reasons, very old transactions are not taken into account in graph report. Check if these transactions are normal."));
            QStringList autoCorrections;
            autoCorrections.push_back(QStringLiteral("skg://view_open_very_old_operations"));
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    // Very far operation
    if (!iIgnoredAdvice.contains(QStringLiteral("skgreportplugin_veryfar"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(QStringLiteral("operation"), QStringLiteral("d_date>=(date('now', 'localtime', '+50 year'))"), exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgreportplugin_veryfar"));
            ad.setPriority(3);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Some transactions are very far in the future"));
            ad.setLongMessage(i18nc("Advice on making the best (long)",
                                    "For performances reasons, transactions very far in the future are not taken into account in graph report. Check if these "
                                    "transactions are normal."));
            QStringList autoCorrections;
            autoCorrections.push_back(QStringLiteral("skg://view_open_very_far_operations"));
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }
    return output;
}

#include <skgreportplugin.moc>
