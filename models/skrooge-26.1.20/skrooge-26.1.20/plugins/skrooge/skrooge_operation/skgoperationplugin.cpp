
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
#include "skgoperationplugin.h"

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include <qdom.h>
#include <qstandardpaths.h>
#include <qthread.h>

#include "skgaccountobject.h"
#include "skgbudgetobject.h"
#include "skgcategoryobject.h"
#include "skgdocumentbank.h"
#include "skghtmlboardwidget.h"
#include "skgmainpanel.h"
#include "skgoperation_settings.h"
#include "skgoperationboardwidgetqml.h"
#include "skgoperationobject.h"
#include "skgoperationpluginwidget.h"
#include "skgpayeeobject.h"
#include "skgrecurrentoperationobject.h"
#include "skgruleobject.h"
#include "skgsuboperationobject.h"
#include "skgtableview.h"
#include "skgtraces.h"
#include "skgtrackerobject.h"
#include "skgtransactionmng.h"
#include <utility>

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGOperationPlugin, "metadata.json")

SKGOperationPlugin::SKGOperationPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_applyTemplateMenu(nullptr)
    , m_openOperationsWithMenu(nullptr)
    , m_openSubOperationsWithMenu(nullptr)
    , m_currentBankDocument(nullptr)
{
    Q_UNUSED(iWidget)
    SKGTRACEINFUNC(10)
}

SKGOperationPlugin::~SKGOperationPlugin()
{
    SKGTRACEINFUNC(10)
    m_currentBankDocument = nullptr;
    m_applyTemplateMenu = nullptr;
    m_openOperationsWithMenu = nullptr;
    m_openSubOperationsWithMenu = nullptr;
}

bool SKGOperationPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentBankDocument = qobject_cast<SKGDocumentBank *>(iDocument);
    if (m_currentBankDocument == nullptr) {
        return false;
    }

    m_currentBankDocument->setComputeBalances(skgoperation_settings::computeBalances());
    m_currentBankDocument->addEndOfTransactionCheck(SKGOperationPlugin::checkReconciliation);
    m_currentBankDocument->addEndOfTransactionCheck(SKGOperationPlugin::checkImport);

    setComponentName(QStringLiteral("skrooge_operation"), title());
    setXMLFile(QStringLiteral("skrooge_operation.rc"));

    QStringList listOperation;
    listOperation << QStringLiteral("operation");

    // Menu
    // ------------
    auto actDuplicateAction = new QAction(SKGServices::fromTheme(QStringLiteral("window-duplicate")), i18nc("Verb, duplicate an object", "Duplicate"), this);
    connect(actDuplicateAction, &QAction::triggered, this, &SKGOperationPlugin::onDuplicate);
    actionCollection()->setDefaultShortcut(actDuplicateAction, Qt::CTRL | Qt::Key_D);
    registerGlobalAction(QStringLiteral("edit_duplicate_operation"), actDuplicateAction, listOperation, 1, -1, 400);

    // ------------
    auto actCreateTemplateAction = new QAction(SKGServices::fromTheme(QStringLiteral("edit-guides")), i18nc("Verb", "Create template"), this);
    connect(actCreateTemplateAction, &QAction::triggered, this, &SKGOperationPlugin::onCreateTemplate);
    actionCollection()->setDefaultShortcut(actCreateTemplateAction, Qt::CTRL | Qt::SHIFT | Qt::Key_T);
    registerGlobalAction(QStringLiteral("edit_template_operation"), actCreateTemplateAction, listOperation, 1, -1, 401);

    // ------------
    auto actSwitchToMarkedAction = new QAction(SKGServices::fromTheme(QStringLiteral("dialog-ok")), i18nc("Verb, mark an object", "Mark"), this);
    connect(actSwitchToMarkedAction, &QAction::triggered, this, &SKGOperationPlugin::onSwitchToMarked);
    actionCollection()->setDefaultShortcut(actSwitchToMarkedAction, Qt::CTRL | Qt::Key_R);
    registerGlobalAction(QStringLiteral("edit_mark_selected_operation"), actSwitchToMarkedAction, listOperation, 1, -1, 310);

    // ------------
    auto actSwitchToCheckedAction = new QAction(SKGServices::fromTheme(QStringLiteral("dialog-ok")), i18nc("Verb, mark an object", "Check"), this);
    connect(actSwitchToCheckedAction, &QAction::triggered, this, &SKGOperationPlugin::onSwitchToChecked);
    actionCollection()->setDefaultShortcut(actSwitchToCheckedAction, Qt::CTRL | Qt::SHIFT | Qt::Key_R);
    registerGlobalAction(QStringLiteral("edit_check_selected_operation"), actSwitchToCheckedAction, listOperation, 1, -1, 311);

    // ------------
    auto actFastEdition = new QAction(SKGServices::fromTheme(QStringLiteral("games-solve")), i18nc("Verb", "Fast edit"), this);
    actFastEdition->setEnabled(false);
    actionCollection()->setDefaultShortcut(actFastEdition, Qt::Key_F10);
    registerGlobalAction(QStringLiteral("fast_edition"), actFastEdition, listOperation);

    // ------------
    QStringList overlayopen;
    overlayopen.push_back(QStringLiteral("quickopen"));

    QStringList overlayrun;
    overlayrun.push_back(QStringLiteral("system-run"));
    auto actOpen = new QAction(SKGServices::fromTheme(icon(), overlayopen), i18nc("Verb", "Open transactions…"), this);
    connect(actOpen, &QAction::triggered, this, &SKGOperationPlugin::onOpenOperations);
    registerGlobalAction(QStringLiteral("open"),
                         actOpen,
                         QStringList() << QStringLiteral("account") << QStringLiteral("unit") << QStringLiteral("category") << QStringLiteral("refund")
                                       << QStringLiteral("payee") << QStringLiteral("budget") << QStringLiteral("recurrentoperation")
                                       << QStringLiteral("operation") << QStringLiteral("suboperation") << QStringLiteral("rule"),
                         1,
                         -1,
                         110);

    auto actOpen2 = new QAction(SKGServices::fromTheme(icon(), overlayopen), i18nc("Verb", "Open transactions with …"), this);
    m_openOperationsWithMenu = new QMenu();
    actOpen2->setMenu(m_openOperationsWithMenu);
    connect(m_openOperationsWithMenu, &QMenu::aboutToShow, this, &SKGOperationPlugin::onShowOpenWithMenu);
    registerGlobalAction(QStringLiteral("open_operations_with"),
                         actOpen2,
                         QStringList() << QStringLiteral("operation") << QStringLiteral("suboperation"),
                         1,
                         1,
                         111);

    auto actOpen3 = new QAction(SKGServices::fromTheme(icon(), overlayopen), i18nc("Verb", "Open sub transactions with …"), this);
    m_openSubOperationsWithMenu = new QMenu();
    actOpen3->setMenu(m_openSubOperationsWithMenu);
    connect(m_openSubOperationsWithMenu, &QMenu::aboutToShow, this, &SKGOperationPlugin::onShowOpenWithMenu);
    registerGlobalAction(QStringLiteral("open_suboperations_with"),
                         actOpen3,
                         QStringList() << QStringLiteral("operation") << QStringLiteral("suboperation"),
                         1,
                         1,
                         112);

    auto actOpenHighLights = new QAction(SKGServices::fromTheme(QStringLiteral("bookmarks"), overlayopen), i18nc("Verb", "Open highlights…"), this);
    actOpenHighLights->setData(QString("skg://skrooge_operation_plugin/?title_icon=bookmarks&title="
                                       % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Highlighted transactions"))
                                       % "&operationWhereClause=t_bookmarked='Y'"));
    connect(actOpenHighLights, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    actionCollection()->setDefaultShortcut(actOpenHighLights, Qt::CTRL | Qt::META | Qt::Key_H);
    registerGlobalAction(QStringLiteral("view_open_highlight"), actOpenHighLights);

    // ------------
    auto actOpenLastModified = new QAction(SKGServices::fromTheme(QStringLiteral("view-refresh"), overlayopen), i18nc("Verb", "Open last modified…"), this);
    actOpenLastModified->setData(
        QString("skg://skrooge_operation_plugin/?title_icon=view-refresh&title="
                % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions modified or created during last action")) % "&operationWhereClause="
                % SKGServices::encodeForUrl(QStringLiteral(
                    "id in (SELECT i_object_id FROM doctransactionitem di, doctransaction dt WHERE dt.t_mode='U' AND +di.rd_doctransaction_id=dt.id AND "
                    "di.t_object_table='operation'AND NOT EXISTS(select 1 from doctransaction B where B.i_parent=dt.id))"))));
    connect(actOpenLastModified, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    actionCollection()->setDefaultShortcut(actOpenLastModified, Qt::META | Qt::Key_L);
    registerGlobalAction(QStringLiteral("view_open_last_modified"), actOpenLastModified);

    // ------------
    auto actOpenModifiedByTransaction =
        new QAction(SKGServices::fromTheme(QStringLiteral("view-refresh"), overlayopen), i18nc("Verb", "Open transactions modified by this action…"), this);
    connect(actOpenModifiedByTransaction, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        if (!selection.isEmpty()) {
            SKGObjectBase obj = selection[0];
            QString name = obj.getAttribute(QStringLiteral("t_name"));
            QString url = QString("skg://skrooge_operation_plugin/?title_icon=view-refresh&title="
                                  % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions modified or created during the action '%1'", name))
                                  % "&operationWhereClause="
                                  % SKGServices::encodeForUrl("id in (SELECT i_object_id FROM doctransactionitem WHERE rd_doctransaction_id="
                                                              % SKGServices::intToString(obj.getID()) % " AND t_object_table='operation')"));
            SKGMainPanel::getMainPanel()->SKGMainPanel::openPage(url);
        }
    });
    registerGlobalAction(QStringLiteral("view_open_modified_by_transaction"),
                         actOpenModifiedByTransaction,
                         QStringList() << QStringLiteral("doctransaction"),
                         1,
                         1,
                         100);

    // ------------
    auto actOpenSuboperations = new QAction(SKGServices::fromTheme(QStringLiteral("split"), overlayopen), i18nc("Verb", "Open sub transactions…"), this);
    actOpenSuboperations->setData(
        QString("skg://skrooge_operation_plugin/SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/"
                "?title_icon=split&operationTable=v_suboperation_consolidated&operationWhereClause=&title="
                % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Sub transactions")) % "#"
                % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Sub transactions"))));
    connect(actOpenSuboperations, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    actionCollection()->setDefaultShortcut(actOpenSuboperations, Qt::META | Qt::Key_S);
    registerGlobalAction(QStringLiteral("view_open_suboperations"), actOpenSuboperations);

    // ------------
    auto actOpenDuplicate =
        new QAction(SKGServices::fromTheme(QStringLiteral("window-duplicate"), overlayopen), i18nc("Verb", "Open potential duplicates…"), this);
    actOpenDuplicate->setData(QString(
        "skg://skrooge_operation_plugin/?title_icon=window-duplicate&title="
        % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions potentially duplicated")) % "&operationWhereClause="
        % SKGServices::encodeForUrl("id in (SELECT o1.id FROM v_operation o1 WHERE EXISTS (SELECT 1 FROM v_operation o2 WHERE o1.id<>o2.id AND "
                                    "o1.t_template='N' AND o2.t_template='N' AND o1.d_date=o2.d_date  AND ABS(o1.f_CURRENTAMOUNT-o2.f_CURRENTAMOUNT)<"
                                    % SKGServices::doubleToString(EPSILON)
                                    % " AND o1.rd_account_id=o2.rd_account_id AND o1.rc_unit_id=o2.rc_unit_id AND (o1.t_status<>'Y' OR o2.t_status<>'Y')))")));
    connect(actOpenDuplicate, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    actionCollection()->setDefaultShortcut(actOpenDuplicate, Qt::META | Qt::Key_D);
    registerGlobalAction(QStringLiteral("view_open_duplicates"), actOpenDuplicate);

    // ------------
    auto actTmp = new QAction(SKGServices::fromTheme(QStringLiteral("exchange-positions"), overlayopen), i18nc("Verb", "Open transfers without payee…"), this);
    actTmp->setData(QString("skg://skrooge_operation_plugin/?title_icon=user-group-properties&title="
                            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transfers without payee")) % "&operationWhereClause="
                            % SKGServices::encodeForUrl(QStringLiteral("t_TRANSFER='Y' AND r_payee_id=0"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_transfers_without_payee"), actTmp);

    // ------------
    actTmp = new QAction(SKGServices::fromTheme(QStringLiteral("user-group-properties"), overlayopen), i18nc("Verb", "Open transactions without payee…"), this);
    actTmp->setData(QString("skg://skrooge_operation_plugin/?title_icon=user-group-properties&title="
                            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions without payee")) % "&operationWhereClause="
                            % SKGServices::encodeForUrl(QStringLiteral("t_TRANSFER='N' AND r_payee_id=0"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_operation_without_payee"), actTmp);

    // ------------
    actTmp = new QAction(SKGServices::fromTheme(QStringLiteral("exchange-positions"), overlayopen), i18nc("Verb", "Open transfers without category…"), this);
    actTmp->setData(QString("skg://skrooge_operation_plugin/?title_icon=applications-all-symbolic&title="
                            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transfers without category")) % "&operationWhereClause="
                            % SKGServices::encodeForUrl(QStringLiteral(
                                "t_TRANSFER='Y' AND EXISTS (SELECT 1 FROM suboperation WHERE rd_operation_id=v_operation_display.id AND r_category_id=0)"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_transfers_without_category"), actTmp);

    // ------------
    actTmp = new QAction(SKGServices::fromTheme(QStringLiteral("applications-all-symbolic"), overlayopen),
                         i18nc("Verb", "Open transactions without category…"),
                         this);
    actTmp->setData(QString("skg://skrooge_operation_plugin/?title_icon=applications-all-symbolic&title="
                            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions without category")) % "&operationWhereClause="
                            % SKGServices::encodeForUrl(QStringLiteral(
                                "t_TRANSFER='N' AND EXISTS (SELECT 1 FROM suboperation WHERE rd_operation_id=v_operation_display.id AND r_category_id=0)"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_operation_without_category"), actTmp);

    // ------------
    actTmp = new QAction(SKGServices::fromTheme(QStringLiteral("skrooge_credit_card"), overlayopen), i18nc("Verb", "Open transactions without mode…"), this);
    actTmp->setData(QString("skg://skrooge_operation_plugin/?title_icon=skrooge_credit_card&title="
                            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions without mode")) % "&operationWhereClause="
                            % SKGServices::encodeForUrl(QStringLiteral("t_template='N' AND t_mode='' AND d_date<>'0000-00-00'"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_operation_without_mode"), actTmp);

    // ------------
    actTmp =
        new QAction(SKGServices::fromTheme(QStringLiteral("draw-freehand"), overlayopen), i18nc("Verb", "Open transactions with comments not aligned…"), this);
    actTmp->setData(QString("skg://skrooge_operation_plugin/?title_icon=draw-freehand&title="
                            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions with comments not aligned")) % "&operationWhereClause="
                            % SKGServices::encodeForUrl(
                                QStringLiteral("id IN (SELECT op.id FROM operation op, suboperation so WHERE so.rd_operation_id=op.id AND "
                                               "so.t_comment<>op.t_comment AND (SELECT COUNT(1) FROM suboperation so2 WHERE so2.rd_operation_id=op.id)=1)"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_operation_with_comment_not_aligned"), actTmp);

    // ------------
    actTmp =
        new QAction(SKGServices::fromTheme(QStringLiteral("view-pim-calendar"), overlayopen), i18nc("Verb", "Open transactions with dates not aligned…"), this);
    actTmp->setData(QString("skg://skrooge_operation_plugin/?title_icon=draw-freehand&title="
                            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions with dates not aligned")) % "&operationWhereClause="
                            % SKGServices::encodeForUrl(
                                QStringLiteral("id IN (SELECT op.id FROM operation op, suboperation so WHERE so.rd_operation_id=op.id AND (so.d_date<op.d_date "
                                               "OR (so.d_date<>op.d_date AND (SELECT COUNT(1) FROM suboperation so2 WHERE so2.rd_operation_id=op.id)=1)))"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_operation_with_date_not_aligned"), actTmp);

    // ------------
    actTmp = new QAction(SKGServices::fromTheme(QStringLiteral("exchange-positions"), overlayopen),
                         i18nc("Verb", "Open transactions without payees category…"),
                         this);
    actTmp->setData(
        QString("skg://skrooge_operation_plugin/?title_icon=applications-all-symbolic&title="
                % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Sub-transactions without payees category")) % "&operationWhereClause="
                % SKGServices::encodeForUrl(QStringLiteral("id IN (SELECT v_suboperation_consolidated.i_OPID FROM v_suboperation_consolidated INNER JOIN payee "
                                                           "ON v_suboperation_consolidated.r_payee_id=payee.id WHERE payee.r_category_id!=0 AND "
                                                           "v_suboperation_consolidated.r_category_id!=payee.r_category_id AND i_NBSUBOPERATIONS=1)"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_suboperations_without_payee_category"), actTmp);

    // ------------
    auto actAlignComment = new QAction(SKGServices::fromTheme(QStringLiteral("draw-freehand"), overlayrun),
                                       i18nc("Verb", "Align comment of subtransactions of all single transactions"),
                                       this);
    connect(actAlignComment, &QAction::triggered, this, &SKGOperationPlugin::onAlignComment);
    registerGlobalAction(QStringLiteral("align_comment"), actAlignComment);

    auto actAlignComment2 = new QAction(SKGServices::fromTheme(QStringLiteral("draw-freehand"), overlayrun),
                                        i18nc("Verb", "Align comment of transactions of all single transactions"),
                                        this);
    connect(actAlignComment2, &QAction::triggered, this, &SKGOperationPlugin::onAlignComment2);
    registerGlobalAction(QStringLiteral("align_comment2"), actAlignComment2);

    // ------------
    auto actAlignDate = new QAction(SKGServices::fromTheme(QStringLiteral("view-pim-calendar"), overlayrun),
                                    i18nc("Verb", "Align date of subtransactions of all transactions"),
                                    this);
    connect(actAlignDate, &QAction::triggered, this, &SKGOperationPlugin::onAlignDate);
    registerGlobalAction(QStringLiteral("align_date"), actAlignDate);

    auto actAlignCategory = new QAction(SKGServices::fromTheme(QStringLiteral("view-pim-calendar"), overlayrun),
                                        i18nc("Verb", "Align the category of all single transactions with the category of their payee"),
                                        this);
    connect(actAlignCategory, &QAction::triggered, this, &SKGOperationPlugin::onAlignWithCategoryOfPayee);
    registerGlobalAction(QStringLiteral("align_category"), actAlignCategory);

    // ------------
    actTmp = new QAction(SKGServices::fromTheme(QStringLiteral("exchange-positions"), overlayopen),
                         i18nc("Verb", "Open transactions in groups with only one transaction…"),
                         this);
    actTmp->setData(
        QString("skg://skrooge_operation_plugin/?title_icon=exchange-positions&title="
                % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions in groups with only one transaction")) % "&operationWhereClause="
                % SKGServices::encodeForUrl(QStringLiteral(
                    "v_operation_display.i_group_id<>0 AND (SELECT COUNT(1) FROM operation o WHERE o.i_group_id=v_operation_display.i_group_id)<2"))));
    connect(actTmp, &QAction::triggered, SKGMainPanel::getMainPanel(), []() {
        SKGMainPanel::getMainPanel()->SKGMainPanel::openPage();
    });
    registerGlobalAction(QStringLiteral("view_open_operation_in_group_of_one"), actTmp);

    // ------------
    auto actCleanRemoveGroupWithOneOperation = new QAction(SKGServices::fromTheme(QStringLiteral("exchange-positions"), overlayrun),
                                                           i18nc("Verb", "Remove groups with only one transaction of all transactions"),
                                                           this);
    connect(actCleanRemoveGroupWithOneOperation, &QAction::triggered, this, &SKGOperationPlugin::onRemoveGroupWithOneOperation);
    registerGlobalAction(QStringLiteral("clean_remove_group_with_one_operation"), actCleanRemoveGroupWithOneOperation);

    // ------------
    auto actGroupOperation = new QAction(SKGServices::fromTheme(QStringLiteral("exchange-positions")), i18nc("Verb", "Group transactions"), this);
    connect(actGroupOperation, &QAction::triggered, this, &SKGOperationPlugin::onGroupOperation);
    actionCollection()->setDefaultShortcut(actGroupOperation, Qt::CTRL | Qt::Key_G);
    registerGlobalAction(QStringLiteral("edit_group_operation"), actGroupOperation, listOperation, 2, -1, 311);

    // ------------
    QStringList overlay;
    overlay.push_back(QStringLiteral("edit-delete"));
    auto actUngroupOperation = new QAction(SKGServices::fromTheme(QStringLiteral("exchange-positions"), overlay), i18nc("Verb", "Ungroup transactions"), this);
    connect(actUngroupOperation, &QAction::triggered, this, &SKGOperationPlugin::onUngroupOperation);
    actionCollection()->setDefaultShortcut(actUngroupOperation, Qt::CTRL | Qt::SHIFT | Qt::Key_G);
    registerGlobalAction(QStringLiteral("edit_ungroup_operation"), actUngroupOperation, listOperation, 1, -1, 312);

    // ------------
    auto actMergeOperationAction = new QAction(SKGServices::fromTheme(QStringLiteral("split")), i18nc("Verb, action to merge", "Merge sub transactions"), this);
    connect(actMergeOperationAction, &QAction::triggered, this, &SKGOperationPlugin::onMergeSubOperations);
    actionCollection()->setDefaultShortcut(actMergeOperationAction, Qt::CTRL | Qt::SHIFT | Qt::Key_M);
    registerGlobalAction(QStringLiteral("merge_sub_operations"), actMergeOperationAction, listOperation, 1, -1, 320);

    auto actApplyTemplateAction =
        new QAction(SKGServices::fromTheme(QStringLiteral("edit-guides")), i18nc("Verb, action to apply a template", "Apply template"), this);
    m_applyTemplateMenu = new QMenu();
    actApplyTemplateAction->setMenu(m_applyTemplateMenu);
    connect(m_applyTemplateMenu, &QMenu::aboutToShow, this, &SKGOperationPlugin::onShowApplyTemplateMenu);
    actApplyTemplateAction->setData(1);
    registerGlobalAction(QStringLiteral("edit_apply_template"), actApplyTemplateAction, listOperation, 1, -1, 402);

    return true;
}

void SKGOperationPlugin::onShowApplyTemplateMenu()
{
    if ((m_applyTemplateMenu != nullptr) && (m_currentBankDocument != nullptr)) {
        // Clean Menu
        QMenu *m = m_applyTemplateMenu;
        m->clear();

        // Search templates
        SKGStringListList listTmp;
        m_currentBankDocument->executeSelectSqliteOrder(
            QStringLiteral("SELECT t_displayname, id, t_bookmarked FROM v_operation_displayname WHERE t_template='Y' ORDER BY t_bookmarked DESC, t_PAYEE ASC"),
            listTmp);

        // Build menus
        int count = 0;
        bool fav = true;
        int nb = listTmp.count();
        for (int i = 1; i < nb; ++i) {
            // Add more sub menu
            if (count == 8) {
                m = m->addMenu(i18nc("More items in a menu", "More"));
                count = 0;
            }
            count++;

            // Add separator for bookmarked templates
            if (fav && listTmp.at(i).at(2) == QStringLiteral("N") && i > 1) {
                m->addSeparator();
            }
            fav = (listTmp.at(i).at(2) == QStringLiteral("Y"));

            // Add actions
            QAction *act = m->addAction(SKGServices::fromTheme(QStringLiteral("edit-guides")), listTmp.at(i).at(0));
            if (act != nullptr) {
                act->setData(listTmp.at(i).at(1));
                connect(act, &QAction::triggered, this, &SKGOperationPlugin::onApplyTemplate);
            }
        }
    }
}

void SKGOperationPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    if ((m_currentBankDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        bool onOperation = (nb > 0 && selection.at(0).getRealTable() == QStringLiteral("operation"));

        QAction *act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("align_date"));
        act->setText(onOperation ? i18nc("Verb", "Align date of subtransactions of selected transactions (single or split)")
                                 : i18nc("Verb", "Align date of subtransactions of all single transactions"));
        act->setData(onOperation);

        act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("align_comment2"));
        act->setText(onOperation ? i18nc("Verb", "Align comment of transactions of selected single transactions")
                                 : i18nc("Verb", "Align comment of transactions of all single transactions"));
        act->setData(onOperation);

        act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("align_category"));
        act->setText(onOperation ? i18nc("Verb", "Align the category of all selected single transactions with the category of their payee")
                                 : i18nc("Verb", "Align the category of all single transactions with the category of their payee"));
        act->setData(onOperation);

        act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("align_comment"));
        act->setText(onOperation ? i18nc("Verb", "Align comment of subtransactions of selected single transactions")
                                 : i18nc("Verb", "Align comment of subtransactions of all single transactions"));
        act->setData(onOperation);

        act = SKGMainPanel::getMainPanel()->getGlobalAction(QStringLiteral("clean_remove_group_with_one_operation"));
        act->setText(onOperation ? i18nc("Verb", "Remove groups with only one transaction of selected transactions")
                                 : i18nc("Verb", "Remove groups with only one transaction of all transactions"));
        act->setData(onOperation);
    }
}

SKGError SKGOperationPlugin::checkReconciliation(SKGDocument *iDocument)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    if ((iDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr) && skgoperation_settings::broken_reconciliation() > QStringLiteral("0")) {
        // Check the reconciliation for all opened account
        SKGObjectBase::SKGListSKGObjectBase accounts;
        iDocument->getObjects(QStringLiteral("v_account"),
                              QStringLiteral("t_close='N' AND f_reconciliationbalance!='' AND (SELECT COUNT(1) FROM (SELECT DISTINCT(operation.rc_unit_id) "
                                             "FROM operation WHERE operation.rd_account_id=v_account.id GROUP BY operation.rc_unit_id))=1"),
                              accounts);
        for (const auto &account : std::as_const(accounts)) {
            SKGAccountObject a(account);
            auto soluces = a.getPossibleReconciliations(SKGServices::stringToDouble(account.getAttribute(QStringLiteral("f_reconciliationbalance"))), false);
            if (soluces.isEmpty()) {
                if (skgoperation_settings::broken_reconciliation() == QStringLiteral("1")) {
                    iDocument->sendMessage(
                        i18nc("Warning message", "The previous reconciliation of '%1' has been broken by this action or a previous one.", a.getDisplayName()),
                        SKGDocument::Warning,
                        QStringLiteral("skg://edit_undo"));
                } else {
                    auto msg = i18nc("Warning message", "This action would break the previous reconciliation of '%1', so it is cancelled.", a.getDisplayName());
                    iDocument->sendMessage(msg, SKGDocument::Error);
                    return err = SKGError(ERR_ABORT, std::move(msg));
                }
            }
        }
    }
    return err;
}

SKGError SKGOperationPlugin::checkImport(SKGDocument *iDocument)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    if ((iDocument != nullptr) && (SKGMainPanel::getMainPanel() != nullptr) && skgoperation_settings::broken_import() > QStringLiteral("0")) {
        // Check the reconciliation for all opened account
        SKGObjectBase::SKGListSKGObjectBase accounts;
        iDocument->getObjects(QStringLiteral("v_account"), QStringLiteral("t_close='N' AND f_importbalance!=''"), accounts);
        for (const auto &account : std::as_const(accounts)) {
            SKGAccountObject a(account);
            auto soluces = a.getPossibleReconciliations(SKGServices::stringToDouble(account.getAttribute(QStringLiteral("f_importbalance"))), false);
            if (soluces.isEmpty()) {
                if (skgoperation_settings::broken_import() == QStringLiteral("1")) {
                    iDocument->sendMessage(
                        i18nc("Warning message", "The previous import in '%1' has been broken by this action or a previous one.", a.getDisplayName()),
                        SKGDocument::Warning,
                        QStringLiteral("skg://edit_undo"));
                } else {
                    auto msg = i18nc("Warning message", "This action would break the previous import in '%1', so it is cancelled.", a.getDisplayName());
                    iDocument->sendMessage(msg, SKGDocument::Error);
                    return err = SKGError(ERR_ABORT, std::move(msg));
                }
            }
        }
    }
    return err;
}

int SKGOperationPlugin::getNbDashboardWidgets()
{
    return 2;
}

QString SKGOperationPlugin::getDashboardWidgetTitle(int iIndex)
{
    if (iIndex == 0) {
        return i18nc("Noun, the title of a section", "Income && Expenditure");
    }
    return i18nc("Noun, the title of a section", "Highlighted transactions");
}

SKGBoardWidget *SKGOperationPlugin::getDashboardWidget(int iIndex)
{
    if (iIndex == 0) {
        return new SKGOperationBoardWidgetQml(SKGMainPanel::getMainPanel(), m_currentBankDocument);
    }
    return new SKGHtmlBoardWidget(
        SKGMainPanel::getMainPanel(),
        m_currentBankDocument,
        getDashboardWidgetTitle(iIndex),
        QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("skrooge/html/default/highlighted_operations.html")),
        QStringList() << QStringLiteral("operation"));
}

SKGTabPage *SKGOperationPlugin::getWidget()
{
    SKGTRACEINFUNC(10)
    return new SKGOperationPluginWidget(SKGMainPanel::getMainPanel(), m_currentBankDocument);
}

QWidget *SKGOperationPlugin::getPreferenceWidget()
{
    SKGTRACEINFUNC(10)
    auto w = new QWidget();
    ui.setupUi(w);

    // Set labels
    ui.kPayeeFakeLbl->setText(i18n("%1:", m_currentBankDocument->getDisplay(QStringLiteral("t_payee"))));
    ui.kCategoryFakeLbl->setText(i18n("%1:", m_currentBankDocument->getDisplay(QStringLiteral("t_CATEGORY"))));
    ui.kCommentFakeLbl->setText(i18n("%1:", m_currentBankDocument->getDisplay(QStringLiteral("t_comment"))));

    ui.kCategoryCommissionLbl->setText(ui.kCategoryFakeLbl->text());
    ui.kCommentCommissionLbl->setText(ui.kCommentFakeLbl->text());

    ui.kCategoryTaxLbl->setText(ui.kCategoryFakeLbl->text());
    ui.kCommentTaxLbl->setText(ui.kCommentFakeLbl->text());

    // Fill combo boxes and auto completion
    SKGMainPanel::fillWithDistinctValue(QList<QWidget *>()
                                            << ui.kcfg_categoryFakeOperation << ui.kcfg_categoryCommissionOperation << ui.kcfg_categoryTaxOperation,
                                        m_currentBankDocument,
                                        QStringLiteral("category"),
                                        QStringLiteral("t_fullname"),
                                        QString());
    SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kcfg_payeeFakeOperation,
                                        m_currentBankDocument,
                                        QStringLiteral("payee"),
                                        QStringLiteral("t_name"),
                                        QString());
    SKGMainPanel::fillWithDistinctValue(QList<QWidget *>() << ui.kcfg_commentFakeOperation << ui.kcfg_commentCommissionOperation << ui.kcfg_commentTaxOperation,
                                        m_currentBankDocument,
                                        QStringLiteral("v_operation_all_comment"),
                                        QStringLiteral("t_comment"),
                                        QString(),
                                        true);

    return w;
}

KConfigSkeleton *SKGOperationPlugin::getPreferenceSkeleton()
{
    return skgoperation_settings::self();
}

SKGError SKGOperationPlugin::savePreferences() const
{
    m_currentBankDocument->setComputeBalances(skgoperation_settings::computeBalances());
    return SKGInterfacePlugin::savePreferences();
}

QString SKGOperationPlugin::title() const
{
    return i18nc("Noun", "Transactions");
}

QString SKGOperationPlugin::icon() const
{
    return QStringLiteral("view-institution-edit");
}

QString SKGOperationPlugin::toolTip() const
{
    return i18nc("Noun", "Transaction management");
}

int SKGOperationPlugin::getOrder() const
{
    return 15;
}

QStringList SKGOperationPlugin::tips() const
{
    QStringList output;
    output.push_back(i18nc("Description of a tips", "<p>… you can press +, -, CTRL + or CTRL - to quickly change dates.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can update many <a href=\"skg://skrooge_operation_plugin\">transactions</a> in one shot.</p>"));
    output.push_back(i18nc("Description of a tips",
                           "<p>… you can double click on an <a href=\"skg://skrooge_operation_plugin\">operation</a> to show or edit sub transactions.</p>"));
    output.push_back(
        i18nc("Description of a tips",
              "<p>… you can duplicate an <a href=\"skg://skrooge_operation_plugin\">operation</a> including complex transactions (split, grouped, …).</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can create template of <a href=\"skg://skrooge_operation_plugin\">transactions</a>.</p>"));
    output.push_back(i18nc("Description of a tips", "<p>… you can group and ungroup <a href=\"skg://skrooge_operation_plugin\">transactions</a>.</p>"));
    output.push_back(i18nc("Description of a tips",
                           "<p>… you have to indicate the sign of an <a href=\"skg://skrooge_operation_plugin\">operation</a> only if you want to force it, "
                           "else it will be determined automatically with the <a href=\"skg://skrooge_category_plugin\">category</a>.</p>"));
    return output;
}

bool SKGOperationPlugin::isInPagesChooser() const
{
    return true;
}

void SKGOperationPlugin::onApplyTemplate()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        // Get template
        SKGOperationObject temp(m_currentBankDocument, SKGServices::stringToInt(act->data().toString()));

        // Get Selection
        if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
            QStringList listUUID;

            SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
            int nb = selection.count();
            {
                SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Apply template"), err, nb)
                for (int i = 0; !err && i < nb; ++i) {
                    SKGOperationObject operationObj(selection.at(i));

                    SKGOperationObject op;
                    IFOKDO(err, temp.duplicate(op))
                    IFOKDO(err, op.mergeAttribute(operationObj, SKGOperationObject::PROPORTIONAL, false))
                    listUUID.push_back(op.getUniqueID());

                    IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
                }
            }

            // status bar
            IFOK(err)
            {
                err = SKGError(0, i18nc("Successful message after an user action", "Template applied."));
                auto *w = qobject_cast<SKGOperationPluginWidget *>(SKGMainPanel::getMainPanel()->currentPage());
                if (w != nullptr) {
                    w->getTableView()->selectObjects(listUUID, true);
                }
            }
            else
            {
                err.addError(ERR_FAIL, i18nc("Error message", "Apply of template failed"));
            }
        }
        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGOperationPlugin::onGroupOperation()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        if (nb >= 2) {
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Group transactions"), err, nb)
            SKGOperationObject main(selection.at(0));
            IFOKDO(err, m_currentBankDocument->stepForward(1))
            for (int i = 1; !err && i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                IFOKDO(err, operationObj.setGroupOperation(main))
                IFOKDO(err, operationObj.save())
                IFOKDO(err, main.load())

                // Send message
                IFOKDO(err,
                       m_currentBankDocument->sendMessage(i18nc("An information to the user",
                                                                "The transaction '%1' has been grouped with '%2'",
                                                                operationObj.getDisplayName(),
                                                                main.getDisplayName()),
                                                          SKGDocument::Hidden))

                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transactions grouped.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Group creation failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGOperationPlugin::onUngroupOperation()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Ungroup transaction"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                IFOKDO(err, operationObj.setGroupOperation(operationObj))
                IFOKDO(err, operationObj.save())

                // Send message
                IFOKDO(err,
                       m_currentBankDocument->sendMessage(
                           i18nc("An information to the user", "The transaction '%1' has been ungrouped", operationObj.getDisplayName()),
                           SKGDocument::Hidden))

                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transaction ungrouped.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Group deletion failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGOperationPlugin::onSwitchToMarked()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Switch to marked"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                IFOKDO(err,
                       operationObj.setStatus(operationObj.getStatus() != SKGOperationObject::MARKED ? SKGOperationObject::MARKED : SKGOperationObject::NONE))
                IFOKDO(err, operationObj.save())

                // Send message
                IFOKDO(err,
                       m_currentBankDocument->sendMessage(
                           i18nc("An information to the user", "The status of the transaction '%1' has been changed", operationObj.getDisplayName()),
                           SKGDocument::Hidden))

                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
            }
        }

        // status bar
        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transaction marked.")))
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Switch failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGOperationPlugin::onSwitchToChecked()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Switch to checked"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                IFOKDO(err,
                       operationObj.setStatus(operationObj.getStatus() != SKGOperationObject::CHECKED ? SKGOperationObject::CHECKED : SKGOperationObject::NONE))
                IFOKDO(err, operationObj.save())

                // Send message
                IFOKDO(err,
                       m_currentBankDocument->sendMessage(
                           i18nc("An information to the user", "The status of the transaction '%1' has been changed", operationObj.getDisplayName()),
                           SKGDocument::Hidden))

                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))
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
}

void SKGOperationPlugin::onDuplicate()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        QStringList listUUID;
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Duplicate transaction"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                SKGOperationObject dup;
                IFOKDO(err, operationObj.duplicate(dup))
                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))

                // Send message
                IFOKDO(err,
                       m_currentBankDocument->sendMessage(
                           i18nc("An information to the user", "The duplicate transaction '%1' has been added", dup.getDisplayName()),
                           SKGDocument::Hidden))

                listUUID.push_back(dup.getUniqueID());
            }
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Transaction duplicated."));
            auto *w = qobject_cast<SKGOperationPluginWidget *>(SKGMainPanel::getMainPanel()->currentPage());
            if (w != nullptr) {
                w->getTableView()->selectObjects(listUUID, true);
            }
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Duplicate transaction failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGOperationPlugin::onCreateTemplate()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Get Selection
    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        QStringList listUUID;
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        {
            SKGBEGINPROGRESSTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Create template"), err, nb)
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject operationObj(selection.at(i));
                SKGOperationObject dup;
                IFOKDO(err, operationObj.duplicate(dup, QDate::currentDate(), true))
                IFOKDO(err, m_currentBankDocument->stepForward(i + 1))

                // Send message
                IFOKDO(err,
                       m_currentBankDocument->sendMessage(i18nc("An information to the user", "The template '%1' has been added", dup.getDisplayName()),
                                                          SKGDocument::Hidden))

                listUUID.push_back(dup.getUniqueID());
            }
        }

        // status bar
        IFOK(err)
        {
            err = SKGError(0, i18nc("Successful message after an user action", "Template created."));
            auto *w = qobject_cast<SKGOperationPluginWidget *>(SKGMainPanel::getMainPanel()->currentPage());
            if (w != nullptr) {
                w->setTemplateMode(true);
                w->getTableView()->selectObjects(listUUID, true);
            }
        }
        else
        {
            err.addError(ERR_FAIL, i18nc("Error message", "Creation template failed"));
        }

        // Display error
        SKGMainPanel::displayErrorMessage(err);
    }
}

void SKGOperationPlugin::onOpenOperations()
{
    SKGTRACEINFUNC(10)
    if (SKGMainPanel::getMainPanel() != nullptr) {
        SKGObjectBase::SKGListSKGObjectBase selection;
        auto *act = qobject_cast<QAction *>(sender());
        if (act != nullptr) {
            QStringList data = SKGServices::splitCSVLine(act->data().toString(), '-');
            if (data.count() == 2) {
                selection.push_back(SKGObjectBase(m_currentBankDocument, data[1], SKGServices::stringToInt(data[0])));
            }
        }
        if (selection.isEmpty()) {
            selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        }

        int nb = selection.count();
        if (nb > 0) {
            QString wc;
            QString titleOpen;
            QString iconOpen;
            QString table = selection.at(0).getRealTable();
            QString view;
            QString account;
            if (table == QStringLiteral("account")) {
                if (nb == 1) {
                    SKGAccountObject tmp(selection.at(0));
                    account = tmp.getName();
                } else {
                    // Build whereclause and title
                    wc = QStringLiteral("rd_account_id in (");
                    titleOpen = i18nc("Noun, a list of items", "Transactions of account:");

                    for (int i = 0; i < nb; ++i) {
                        SKGAccountObject tmp(selection.at(i));
                        if (i != 0) {
                            wc += ',';
                            titleOpen += ',';
                        }
                        wc += SKGServices::intToString(tmp.getID());
                        titleOpen += i18n("'%1'", tmp.getDisplayName());
                    }
                    wc += ')';
                    // Set icon
                    iconOpen = QStringLiteral("view-institution-edit");
                }
            } else if (table == QStringLiteral("unit")) {
                // Build whereclause and title
                wc = QStringLiteral("rc_unit_id in (");
                titleOpen = i18nc("Noun, a list of items", "Transactions with unit:");

                for (int i = 0; i < nb; ++i) {
                    SKGUnitObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += ',';
                        titleOpen += ',';
                    }
                    wc += SKGServices::intToString(tmp.getID());
                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }
                wc += ')';

                // Set icon
                iconOpen = QStringLiteral("taxes-finances");
            } else if (table == QStringLiteral("category")) {
                titleOpen = i18nc("Noun, a list of items", "Sub transactions with category:");

                for (int i = 0; i < nb; ++i) {
                    SKGCategoryObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += QStringLiteral(" OR ");
                        titleOpen += ',';
                    }

                    QString name = tmp.getFullName();

                    wc += QStringLiteral("(t_REALCATEGORY");
                    if (name.isEmpty()) {
                        wc += QStringLiteral(" IS NULL OR t_REALCATEGORY='')");
                    } else {
                        wc += " = '" % SKGServices::stringToSqlString(name) % "' OR t_REALCATEGORY like '" % SKGServices::stringToSqlString(name)
                            % OBJECTSEPARATOR % "%')";
                    }

                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }

                // Set icon
                iconOpen = QStringLiteral("applications-all-symbolic");
                view = QStringLiteral("v_suboperation_consolidated");
            } else if (table == QStringLiteral("refund")) {
                // Build whereclause and title
                wc = QStringLiteral("t_REALREFUND in (");
                titleOpen = i18nc("Noun, a list of items", "Sub transactions followed by tracker:");

                for (int i = 0; i < nb; ++i) {
                    SKGTrackerObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += ',';
                        titleOpen += ',';
                    }
                    wc += '\'' % SKGServices::stringToSqlString(tmp.getName()) % u'\'';
                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }
                wc += ')';

                // Set icon
                iconOpen = QStringLiteral("crosshairs");
                view = QStringLiteral("v_suboperation_consolidated");
            } else if (table == QStringLiteral("payee")) {
                // Build whereclause and title
                wc = QStringLiteral("r_payee_id in (");
                titleOpen = i18nc("Noun, a list of items", "Transactions assigned to payee:");

                for (int i = 0; i < nb; ++i) {
                    SKGPayeeObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += ',';
                        titleOpen += ',';
                    }
                    wc += SKGServices::intToString(tmp.getID());
                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }
                wc += ')';

                // Set icon
                iconOpen = QStringLiteral("user-group-properties");
            } else if (table == QStringLiteral("budget")) {
                titleOpen = i18nc("Noun, a list of items", "Transactions assigned to budget:");

                for (int i = 0; i < nb; ++i) {
                    SKGBudgetObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += QStringLiteral(" OR ");
                        titleOpen += ',';
                    }

                    wc += "(t_TYPEACCOUNT<>'L' AND i_SUBOPID IN (SELECT b2.id_suboperation FROM budgetsuboperation b2 WHERE b2.id="
                        % SKGServices::intToString(tmp.getID()) % "))";
                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }

                // Set icon
                iconOpen = QStringLiteral("view-calendar-whatsnext");
                view = QStringLiteral("v_suboperation_consolidated");
            } else if (table == QStringLiteral("recurrentoperation")) {
                titleOpen = i18nc("Noun, a list of items", "Scheduled transactions:");

                for (int i = 0; i < nb; ++i) {
                    SKGRecurrentOperationObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += QStringLiteral(" OR ");
                        titleOpen += ',';
                    }

                    wc += "(EXISTS(SELECT 1 FROM recurrentoperation s WHERE s.rd_operation_id=v_operation_display.id and s.id="
                            % SKGServices::intToString(tmp.getID()) % ") OR r_recurrentoperation_id=" % SKGServices::intToString(tmp.getID())
                        + ')';
                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }

                // Set icon
                iconOpen = QStringLiteral("view-calendar-day");
                view = QStringLiteral("v_operation_display");
            } else if (table == QStringLiteral("rule")) {
                titleOpen = i18nc("Noun, a list of items", "Sub transactions corresponding to rule:");

                for (int i = 0; i < nb; ++i) {
                    SKGRuleObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += QStringLiteral(" OR ");
                        titleOpen += ',';
                    }

                    wc += "i_SUBOPID in (SELECT i_SUBOPID FROM v_operation_prop WHERE " % tmp.getSelectSqlOrder() % u')';
                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }

                // Set icon
                iconOpen = QStringLiteral("system-search");
                view = QStringLiteral("v_suboperation_consolidated");
            } else if (table == QStringLiteral("operation")) {
                // Build whereclause and title
                titleOpen = i18nc("Noun, a list of items", "Sub transactions grouped or split of:");
                view = QStringLiteral("v_suboperation_consolidated");

                for (int i = 0; i < nb; ++i) {
                    SKGOperationObject tmp(selection.at(i));
                    if (i != 0) {
                        wc += QStringLiteral(" OR ");
                        titleOpen += ',';
                    }

                    int opid = tmp.getID();
                    wc += QStringLiteral("(i_OPID=") % SKGServices::intToString(opid);

                    opid = SKGServices::stringToInt(tmp.getAttribute(QStringLiteral("i_group_id")));
                    if (opid != 0) {
                        wc += " or i_group_id=" % SKGServices::intToString(opid);
                    }
                    wc += ')';

                    titleOpen += i18n("'%1'", tmp.getDisplayName());
                }

                // Set icon
                iconOpen = icon();
            } else if (table == QStringLiteral("suboperation")) {
                // Build whereclause and title
                titleOpen = i18nc("Noun, a list of items", "Transactions grouped with:");
                view = QStringLiteral("v_operation_display_all");

                for (int i = 0; i < nb; ++i) {
                    SKGSubOperationObject tmp(selection.at(i).getDocument(), selection.at(i).getID());
                    SKGOperationObject op;
                    tmp.getParentOperation(op);
                    if (i != 0) {
                        wc += QStringLiteral(" OR ");
                        titleOpen += ',';
                    }

                    int opid = op.getID();
                    wc += QStringLiteral("(id=") % SKGServices::intToString(opid);

                    opid = SKGServices::stringToInt(op.getAttribute(QStringLiteral("i_group_id")));
                    if (opid != 0) {
                        wc += " or i_group_id=" % SKGServices::intToString(opid);
                    }
                    wc += ')';

                    titleOpen += i18n("'%1'", op.getDisplayName());
                }

                // Set icon
                iconOpen = icon();
            }

            // Open
            QDomDocument doc(QStringLiteral("SKGML"));
            doc.setContent(SKGMainPanel::getMainPanel()->getDocument()->getParameter(view != QStringLiteral("v_suboperation_consolidated")
                                                                                         ? QStringLiteral("SKGOPERATION_DEFAULT_PARAMETERS")
                                                                                         : QStringLiteral("SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS")));
            QDomElement root = doc.documentElement();
            if (root.isNull()) {
                root = doc.createElement(QStringLiteral("parameters"));
                doc.appendChild(root);
            }

            if (!view.isEmpty()) {
                root.setAttribute(QStringLiteral("operationTable"), view);
            }
            if (!wc.isEmpty()) {
                root.setAttribute(QStringLiteral("operationWhereClause"), wc);
            }
            if (!titleOpen.isEmpty()) {
                root.setAttribute(QStringLiteral("title"), titleOpen);
            }
            if (!iconOpen.isEmpty()) {
                root.setAttribute(QStringLiteral("title_icon"), iconOpen);
            }
            if (!account.isEmpty()) {
                root.setAttribute(QStringLiteral("account"), account);
            } else {
                root.setAttribute(QStringLiteral("currentPage"), QStringLiteral("-1"));
            }

            SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge operation plugin")),
                                                   -1,
                                                   doc.toString(),
                                                   view == QStringLiteral("v_suboperation_consolidated") ? i18nc("Noun, a list of items", "Sub transactions")
                                                                                                         : QString());
        }
    }
}

SKGAdviceList SKGOperationPlugin::advice(const QStringList &iIgnoredAdvice)
{
    SKGTRACEINFUNC(10)
    SKGAdviceList output;
    output.reserve(20);
    int nbConcurrentCheckExecuted = 0;
    int nbConcurrentCheckTargeted = 0;
    QMutex mutex;

    // Search duplicate number on operation
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_duplicate"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExecuteSelectSqliteOrder(
            QStringLiteral("SELECT count(1), account.t_name, operation.t_number FROM operation, account  WHERE +operation.rd_account_id=account.id AND "
                           "operation.t_number!='' GROUP BY account.t_name, operation.t_number HAVING count(1)>1 ORDER BY count(1) DESC"),
            [&](const SKGStringListList &iResult) {
                int nb = iResult.count();
                SKGAdvice::SKGAdviceActionList autoCorrections;
                for (int i = 1; i < nb; ++i) { // Ignore header
                    // Get parameters
                    const QStringList &line = iResult.at(i);
                    const QString &account = line.at(1);
                    const QString &number = line.at(2);

                    SKGAdvice ad;
                    ad.setUUID("skgoperationplugin_duplicate|" % number % u';' % account);
                    ad.setPriority(7);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Duplicate number %1 in account '%2'", number, account));
                    ad.setLongMessage(i18nc("Advice on making the best (long)",
                                            "Your account '%1' contains more than one transaction with number %2.The transaction number should be unique "
                                            "(check number, transaction reference…)",
                                            account,
                                            number));
                    autoCorrections.resize(0);
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = i18nc("Advice on making the best (action)", "Edit transactions with duplicate number");
                        a.IconName = QStringLiteral("quickopen");
                        a.IsRecommended = false;
                        autoCorrections.push_back(a);
                    }
                    ad.setAutoCorrections(autoCorrections);
                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check transactions not reconciled
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_notreconciled"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExecuteSelectSqliteOrder(
            QStringLiteral("SELECT count(1), account.t_name FROM operation, account  WHERE +operation.rd_account_id=account.id AND t_status='N' GROUP BY "
                           "account.t_name HAVING count(1)>100 ORDER BY count(1) DESC"),
            [&](const SKGStringListList &iResult) {
                int nb = iResult.count();
                SKGAdvice::SKGAdviceActionList autoCorrections;
                for (int i = 1; i < nb; ++i) { // Ignore header
                    // Get parameters
                    const QStringList &line = iResult.at(i);
                    const QString &account = line.at(1);

                    SKGAdvice ad;
                    ad.setUUID("skgoperationplugin_notreconciled|" % account);
                    ad.setPriority(9);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Many transactions of '%1' not reconciled", account));
                    ad.setLongMessage(
                        i18nc("Advice on making the best (long)",
                              "Do not forget to reconcile your accounts. By doing so, you acknowledge that your bank has indeed processed these transactions "
                              "on your account. This is how you enforce compliance with your bank's statements. See online help for more details"));
                    autoCorrections.resize(0);
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = i18nc("Advice on making the best (action)", "Open account '%1' for reconciliation", account);
                        a.IconName = QStringLiteral("quickopen");
                        a.IsRecommended = false;
                        autoCorrections.push_back(a);
                    }
                    ad.setAutoCorrections(autoCorrections);
                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Not enough money in your account
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_minimum_limit"))) {
        nbConcurrentCheckTargeted++;
        // Get accounts with amount close or below the minimum limit
        m_currentBankDocument->concurrentExecuteSelectSqliteOrder(
            QStringLiteral(
                "SELECT t_name FROM v_account_amount WHERE t_close='N' AND t_minamount_enabled='Y' AND f_CURRENTAMOUNT<f_CURRENTAMOUNTUNIT*f_minamount"),
            [&](const SKGStringListList &iResult) {
                int nb = iResult.count();
                mutex.lock();
                output.reserve(output.count() + nb);
                mutex.unlock();
                for (int i = 1; i < nb; ++i) { // Ignore header
                    // Get parameters
                    QString account = iResult.at(i).at(0);

                    SKGAdvice ad;
                    ad.setUUID("skgoperationplugin_minimum_limit|" % account);
                    ad.setPriority(9);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Not enough money in your account '%1'", account));
                    ad.setLongMessage(
                        i18nc("Advice on making the best (long)", "The amount of this account is below the minimum limit. You should replenish it."));
                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Maximum limit reached
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_maximum_limit"))) {
        nbConcurrentCheckTargeted++;
        // Get accounts with amount over the maximum limit
        m_currentBankDocument->concurrentExecuteSelectSqliteOrder(
            QStringLiteral(
                "SELECT t_name FROM v_account_amount WHERE t_close='N' AND t_maxamount_enabled='Y' AND f_CURRENTAMOUNT>f_CURRENTAMOUNTUNIT*f_maxamount"),
            [&](const SKGStringListList &iResult) {
                int nb = iResult.count();
                mutex.lock();
                output.reserve(output.count() + nb);
                mutex.unlock();
                for (int i = 1; i < nb; ++i) { // Ignore header
                    // Get parameters
                    QString account = iResult.at(i).at(0);

                    SKGAdvice ad;
                    ad.setUUID("skgoperationplugin_maximum_limit|" % account);
                    ad.setPriority(6);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Balance in account '%1' exceeds the maximum limit", account));
                    ad.setLongMessage(i18nc("Advice on making the best (long)", "The balance of this account exceeds the maximum limit."));
                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Minimum limit reached
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_close_minimum_limit"))) {
        nbConcurrentCheckTargeted++;
        // Get accounts with amount close or below the minimum limit
        m_currentBankDocument->concurrentExecuteSelectSqliteOrder(
            QStringLiteral("SELECT t_name FROM v_account_amount WHERE t_close='N' AND t_minamount_enabled='Y' AND t_maxamount_enabled='Y'"
                           " AND f_CURRENTAMOUNT>=f_CURRENTAMOUNTUNIT*f_minamount AND "
                           "f_CURRENTAMOUNT<=f_CURRENTAMOUNTUNIT*f_minamount+0.1*(f_CURRENTAMOUNTUNIT*f_maxamount-f_CURRENTAMOUNTUNIT*f_minamount)"),
            [&](const SKGStringListList &iResult) {
                int nb = iResult.count();
                mutex.lock();
                output.reserve(output.count() + nb);
                mutex.unlock();
                for (int i = 1; i < nb; ++i) { // Ignore header
                    // Get parameters
                    QString account = iResult.at(i).at(0);

                    SKGAdvice ad;
                    ad.setUUID("skgoperationplugin_close_minimum_limit|" % account);
                    ad.setPriority(5);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Your account '%1' is close to the minimum limit", account));
                    ad.setLongMessage(
                        i18nc("Advice on making the best (long)", "The amount of this account is close to the minimum limit. You should take care of it."));
                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Too much money in your account
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_too_much_money"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExecuteSelectSqliteOrder(
            QStringLiteral("SELECT t_name, f_RATE FROM v_account_display WHERE t_close='N' AND f_RATE>0 AND (t_maxamount_enabled='N' OR "
                           "f_CURRENTAMOUNT<f_CURRENTAMOUNTUNIT*f_maxamount*0.9) ORDER BY f_RATE DESC"),
            [&](const SKGStringListList &iResult) {
                int nb = iResult.count();
                if (nb > 1) {
                    // Get better interest account
                    QString target = iResult.at(1).at(0);
                    QString rate = iResult.at(1).at(1);

                    // Get accounts with too much money
                    m_currentBankDocument->concurrentExecuteSelectSqliteOrder("SELECT t_name FROM v_account_display WHERE t_close='N' AND ("
                        // Interest are computed on amount over the limit too, not need to transfer them "(t_maxamount_enabled='Y' AND f_CURRENTAMOUNT>f_CURRENTAMOUNTUNIT*f_maxamount) OR "
                        "(f_RATE<" % rate % " AND t_type='C' AND f_CURRENTAMOUNT>-2*(SELECT TOTAL(s.f_CURRENTAMOUNT) FROM v_operation_display s WHERE s.rd_account_id=v_account_display.id AND s.t_TYPEEXPENSE='-' AND s.d_DATEMONTH = (SELECT strftime('%Y-%m',date('now', 'localtime','start of month', '-1 MONTH')))))"
                ")", [ &output, target, rate ](const SKGStringListList & iResult2) {
                    int nb = iResult2.count();
                    QMutex mutex;
                    mutex.lock();
                    output.reserve(output.count() + nb);
                    mutex.unlock();
                    for (int i = 1; i < nb; ++i) {  // Ignore header
                        // Get parameters
                        QString account = iResult2.at(i).at(0);

                        SKGAdvice ad;
                        ad.setUUID("skgoperationplugin_too_much_money|" % account);
                        ad.setPriority(6);
                        ad.setShortMessage(i18nc("Advice on making the best (short)", "Too much money in your account '%1'", account));
                        ad.setLongMessage(i18nc("Advice on making the best (long)", "You could save money on an account with a better rate. Example: '%1' (%2%)", target, rate));
                        mutex.lock();
                        output.push_back(ad);
                        mutex.unlock();
                    }
                }, false);
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check transactions without mode
    if (!iIgnoredAdvice.contains(QStringLiteral("skgimportexportplugin_notvalidated"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("operation"),
            QStringLiteral("t_template='N' AND t_mode='' AND d_date<>'0000-00-00'"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgimportexportplugin_notvalidated"));
                    ad.setPriority(5);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Many transactions do not have mode"));
                    ad.setLongMessage(i18nc("Advice on making the best (long)",
                                            "Do not forget to set a mode for each transaction. This will allow you to generate better reports."));
                    QStringList autoCorrections;
                    autoCorrections.push_back(QStringLiteral("skg://view_open_operation_without_mode"));
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check transactions without category
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_nocategory"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("v_operation_display"),
            QStringLiteral("t_TRANSFER='N' AND EXISTS (SELECT 1 FROM suboperation WHERE rd_operation_id=v_operation_display.id AND r_category_id=0)"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgoperationplugin_nocategory"));
                    ad.setPriority(5);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Many transactions do not have category"));
                    ad.setLongMessage(i18nc("Advice on making the best (long)",
                                            "Do not forget to associate a category for each transaction. This will allow you to generate better reports."));
                    QStringList autoCorrections;
                    autoCorrections.push_back(QStringLiteral("skg://view_open_operation_without_category"));
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_transfer_nocategory"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("v_operation_display"),
            QStringLiteral("t_TRANSFER='Y' AND EXISTS (SELECT 1 FROM suboperation WHERE rd_operation_id=v_operation_display.id AND r_category_id=0)"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgoperationplugin_transfer_nocategory"));
                    ad.setPriority(3);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Many transfers do not have category"));
                    ad.setLongMessage(i18nc("Advice on making the best (long)", "Do not forget to associate a category for each transfer."));
                    QStringList autoCorrections;
                    autoCorrections.push_back(QStringLiteral("skg://view_open_transfers_without_category"));
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check transactions without payee
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_nopayee"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("v_operation_display"),
            QStringLiteral("t_TRANSFER='N' AND r_payee_id=0"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgoperationplugin_nopayee"));
                    ad.setPriority(5);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Many transactions do not have payee"));
                    ad.setLongMessage(i18nc("Advice on making the best (long)",
                                            "Do not forget to associate a payee for each transaction. This will allow you to generate better reports."));
                    QStringList autoCorrections;
                    autoCorrections.push_back(QStringLiteral("skg://view_open_operation_without_payee"));
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_transfer_nopayee"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("v_operation_display"),
            QStringLiteral("t_TRANSFER='Y' AND r_payee_id=0"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgoperationplugin_transfer_nopayee"));
                    ad.setPriority(3);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Many transfers do not have payee"));
                    ad.setLongMessage(i18nc("Advice on making the best (long)", "Do not forget to associate a payee for each transfer."));
                    QStringList autoCorrections;
                    autoCorrections.push_back(QStringLiteral("skg://view_open_transfers_without_payee"));
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check transactions in group of only one transaction
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_groupofone"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("operation"),
            QStringLiteral("i_group_id<>0 AND (SELECT COUNT(1) FROM operation o WHERE o.i_group_id=operation.i_group_id)<2"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgoperationplugin_groupofone"));
                    ad.setPriority(4);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Some transactions are in groups with only one transaction"));
                    ad.setLongMessage(i18nc("Advice on making the best (long)",
                                            "When a transfer is created and when only one part of this transfer is removed, the second part is in a group of "
                                            "only one transaction. This makes no sense."));
                    SKGAdvice::SKGAdviceActionList autoCorrections;
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = QStringLiteral("skg://view_open_operation_in_group_of_one");
                        a.IsRecommended = false;
                        autoCorrections.push_back(a);
                    }
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = QStringLiteral("skg://clean_remove_group_with_one_operation");
                        a.IsRecommended = true;
                        autoCorrections.push_back(a);
                    }
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check simple transactions with comment not aligned with the comment of the subtransaction
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_commentsnotaligned"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("operation op, suboperation so"),
            QStringLiteral(
                "so.rd_operation_id=op.id AND so.t_comment<>op.t_comment AND (SELECT COUNT(1) FROM suboperation so2 WHERE so2.rd_operation_id=op.id)=1"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgoperationplugin_commentsnotaligned"));
                    ad.setPriority(5);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Some simple transactions do not have their comments aligned"));
                    ad.setLongMessage(
                        i18nc("Advice on making the best (long)", "The comment of the transaction is not aligned with the comment of the subtransaction."));
                    SKGAdvice::SKGAdviceActionList autoCorrections;
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = QStringLiteral("skg://view_open_operation_with_comment_not_aligned");
                        a.IsRecommended = false;
                        autoCorrections.push_back(a);
                    }
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = QStringLiteral("skg://align_comment");
                        a.IsRecommended = true;
                        autoCorrections.push_back(a);
                    }
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = QStringLiteral("skg://align_comment2");
                        a.IsRecommended = false;
                        autoCorrections.push_back(a);
                    }
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check simple transactions with date not aligned with the date of the subtransaction
    if (!iIgnoredAdvice.contains(QStringLiteral("skgoperationplugin_datesnotaligned"))) {
        nbConcurrentCheckTargeted++;
        m_currentBankDocument->concurrentExistObjects(
            QStringLiteral("operation op, suboperation so"),
            QStringLiteral("so.rd_operation_id=op.id AND (so.d_date<op.d_date OR (so.d_date<>op.d_date AND (SELECT COUNT(1) FROM suboperation so2 WHERE "
                           "so2.rd_operation_id=op.id)=1))"),
            [&](bool iFound) {
                if (iFound) {
                    SKGAdvice ad;
                    ad.setUUID(QStringLiteral("skgoperationplugin_datesnotaligned"));
                    ad.setPriority(5);
                    ad.setShortMessage(i18nc("Advice on making the best (short)", "Some transactions do not have their dates aligned"));
                    ad.setLongMessage(i18nc("Advice on making the best (long)",
                                            "The date of the transaction is not aligned with the date of the subtransaction. This case seems to be abnormal."));
                    SKGAdvice::SKGAdviceActionList autoCorrections;
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = QStringLiteral("skg://view_open_operation_with_date_not_aligned");
                        a.IsRecommended = false;
                        autoCorrections.push_back(a);
                    }
                    {
                        SKGAdvice::SKGAdviceAction a;
                        a.Title = QStringLiteral("skg://align_date");
                        a.IsRecommended = true;
                        autoCorrections.push_back(a);
                    }
                    ad.setAutoCorrections(autoCorrections);

                    mutex.lock();
                    output.push_back(ad);
                    mutex.unlock();
                }
                mutex.lock();
                nbConcurrentCheckExecuted++;
                mutex.unlock();
            },
            false);
    }

    // Check transactions having category not aligned with the category of the payee
    if (!iIgnoredAdvice.contains(QStringLiteral("skgpayeeplugin_subop_with_wrong_wategory"))) {
        bool exist = false;
        m_currentBankDocument->existObjects(QStringLiteral("v_suboperation_consolidated INNER JOIN payee ON v_suboperation_consolidated.r_payee_id=payee.id"),
                                            QStringLiteral("payee.r_category_id!=0 AND v_suboperation_consolidated.r_category_id!=payee.r_category_id"),
                                            exist);
        if (exist) {
            SKGAdvice ad;
            ad.setUUID(QStringLiteral("skgpayeeplugin_subop_with_wrong_wategory"));
            ad.setPriority(3);
            ad.setShortMessage(i18nc("Advice on making the best (short)", "Category not aligned with payee's category"));
            ad.setLongMessage(i18nc("Advice on making the best (long)", "Some transactions don't have the category aligned with the category of the payee."));
            SKGAdvice::SKGAdviceActionList autoCorrections;
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = QStringLiteral("skg://view_open_suboperations_without_payee_category");
                autoCorrections.push_back(a);
            }
            {
                SKGAdvice::SKGAdviceAction a;
                a.Title = QStringLiteral("skg://align_category");

                autoCorrections.push_back(a);
            }
            ad.setAutoCorrections(autoCorrections);
            output.push_back(ad);
        }
    }

    do {
        QThread::yieldCurrentThread();
        if (nbConcurrentCheckExecuted == nbConcurrentCheckTargeted) {
            break;
        }
    } while (true);

    return output;
}

SKGError SKGOperationPlugin::executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution)
{
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgoperationplugin_duplicate|"))) {
        // Get parameters
        QString parameters = iAdviceIdentifier.right(iAdviceIdentifier.length() - 29);
        int pos = parameters.indexOf(';');
        QString num = parameters.left(pos);
        QString account = parameters.right(parameters.length() - 1 - pos);

        // Call transaction plugin
        SKGMainPanel::getMainPanel()->openPage(
            "skg://skrooge_operation_plugin/?title_icon=security-low&title="
            % SKGServices::encodeForUrl(i18nc("Noun, a list of items", "Transactions of '%1' with duplicate number %2", account, num))
            % "&operationWhereClause="
            % SKGServices::encodeForUrl("t_number='" % SKGServices::stringToSqlString(num) % "' AND t_ACCOUNT='" % SKGServices::stringToSqlString(account)
                                        % u'\''));
        return SKGError();
    }
    if ((m_currentBankDocument != nullptr) && iAdviceIdentifier.startsWith(QStringLiteral("skgoperationplugin_notreconciled|"))) {
        // Get parameters
        QString account = iAdviceIdentifier.right(iAdviceIdentifier.length() - 36);
        SKGMainPanel::getMainPanel()->openPage("skg://skrooge_operation_plugin/?currentPage=-1&modeInfoZone=1&account=" % SKGServices::encodeForUrl(account));
        return SKGError();
    }

    return SKGInterfacePlugin::executeAdviceCorrection(iAdviceIdentifier, iSolution);
}

void SKGOperationPlugin::onRemoveGroupWithOneOperation()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        auto *act = qobject_cast<QAction *>(sender());
        if ((act == nullptr) || !act->data().toBool()) {
            // Clear selection to enable the model "All"
            selection.clear();
        }
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Remove groups with only one transaction"), err)
        QString q = QStringLiteral(
            "UPDATE operation SET i_group_id=0 WHERE i_group_id<>0 AND (SELECT COUNT(1) FROM operation o WHERE o.i_group_id=operation.i_group_id)<2");
        int nb = selection.count();
        if (nb == 0) {
            err = m_currentBankDocument->executeSqliteOrder(q);
        } else {
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(selection.at(i));
                err = m_currentBankDocument->executeSqliteOrder(q % " AND id=" % SKGServices::intToString(op.getID()));
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Remove groups done.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Remove groups failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPlugin::onAlignComment()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        auto *act = qobject_cast<QAction *>(sender());
        if ((act == nullptr) || !act->data().toBool()) {
            // Clear selection to enable the model "All"
            selection.clear();
        }
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Align comment of subtransactions"), err)
        QString q = QStringLiteral(
            "UPDATE suboperation SET t_comment=(SELECT op.t_comment FROM operation op WHERE suboperation.rd_operation_id=op.id) WHERE suboperation.id IN "
            "(SELECT so.id FROM operation op, suboperation so WHERE so.rd_operation_id=op.id AND so.t_comment<>op.t_comment AND (SELECT COUNT(1) FROM "
            "suboperation so2 WHERE so2.rd_operation_id=op.id)=1)");
        int nb = selection.count();
        if (nb == 0) {
            err = m_currentBankDocument->executeSqliteOrder(q);
        } else {
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(selection.at(i));
                err = m_currentBankDocument->executeSqliteOrder(q % " AND rd_operation_id=" % SKGServices::intToString(op.getID()));
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Comments aligned.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Comments alignment failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPlugin::onAlignComment2()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        auto *act = qobject_cast<QAction *>(sender());
        if ((act == nullptr) || !act->data().toBool()) {
            // Clear selection to enable the model "All"
            selection.clear();
        }
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Align comment of transactions"), err)
        QString q = QStringLiteral(
            "UPDATE operation SET t_comment=(SELECT suboperation.t_comment FROM suboperation WHERE suboperation.rd_operation_id=operation.id) WHERE "
            "operation.id IN (SELECT operation.id FROM operation op, suboperation so WHERE so.rd_operation_id=op.id AND so.t_comment<>op.t_comment AND (SELECT "
            "COUNT(1) FROM suboperation so2 WHERE so2.rd_operation_id=op.id)=1)");
        int nb = selection.count();
        if (nb == 0) {
            err = m_currentBankDocument->executeSqliteOrder(q);
        } else {
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(selection.at(i));
                err = m_currentBankDocument->executeSqliteOrder(q % " AND id=" % SKGServices::intToString(op.getID()));
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Comments aligned.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Comments alignment failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPlugin::onAlignDate()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        auto *act = qobject_cast<QAction *>(sender());
        if ((act == nullptr) || !act->data().toBool()) {
            // Clear selection to enable the model "All"
            selection.clear();
        }
        SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Align date of subtransactions"), err)

        int nb = selection.count();
        if (nb == 0) {
            QString q = QStringLiteral(
                "UPDATE suboperation SET d_date=(SELECT op.d_date FROM operation op WHERE suboperation.rd_operation_id=op.id) WHERE suboperation.id IN (SELECT "
                "so.id FROM operation op, suboperation so WHERE so.rd_operation_id=op.id AND (so.d_date<op.d_date OR (so.d_date<>op.d_date AND (SELECT "
                "COUNT(1) "
                "FROM suboperation so2 WHERE so2.rd_operation_id=op.id)=1)))");
            err = m_currentBankDocument->executeSqliteOrder(q);
        } else {
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(selection.at(i));
                err = m_currentBankDocument->executeSqliteOrder(
                    "UPDATE suboperation SET d_date=(SELECT op.d_date FROM operation op WHERE suboperation.rd_operation_id=op.id) WHERE rd_operation_id="
                    % SKGServices::intToString(op.getID()));
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Dates aligned.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Dates alignment failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPlugin::onAlignWithCategoryOfPayee()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        auto *act = qobject_cast<QAction *>(sender());
        if ((act == nullptr) || !act->data().toBool()) {
            // Clear selection to enable the model "All"
            selection.clear();
        }
        SKGBEGINTRANSACTION(*m_currentBankDocument,
                            i18nc("Noun, name of the user action", "Align the category of all single transactions with the category of their payee"),
                            err)
        QString q = QStringLiteral(
            "UPDATE suboperation SET r_category_id=(SELECT p.r_category_id FROM v_suboperation_consolidated so INNER JOIN payee p ON so.r_payee_id=p.id WHERE "
            "so.id=suboperation.id AND p.r_category_id!=0 AND so.r_category_id!=p.r_category_id) WHERE suboperation.id IN (SELECT so.id FROM "
            "v_suboperation_consolidated so INNER JOIN payee p ON so.r_payee_id=p.id WHERE p.r_category_id!=0 AND so.r_category_id!=p.r_category_id AND "
            "so.i_NBSUBOPERATIONS)");
        int nb = selection.count();
        if (nb == 0) {
            err = m_currentBankDocument->executeSqliteOrder(q);
        } else {
            for (int i = 0; !err && i < nb; ++i) {
                SKGOperationObject op(selection.at(i));
                err = m_currentBankDocument->executeSqliteOrder(q % " AND rd_operation_id=" % SKGServices::intToString(op.getID()));
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Message for successful user action", "Categories aligned.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Categories alignment failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPlugin::onMergeSubOperations()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    if ((SKGMainPanel::getMainPanel() != nullptr) && (m_currentBankDocument != nullptr)) {
        SKGObjectBase::SKGListSKGObjectBase selection = SKGMainPanel::getMainPanel()->getSelectedObjects();
        int nb = selection.count();
        if (nb >= 2) {
            SKGBEGINTRANSACTION(*m_currentBankDocument, i18nc("Noun, name of the user action", "Merge sub transactions"), err)
            SKGOperationObject op(selection.at(0));
            for (int i = 1; !err && i < nb; ++i) {
                SKGOperationObject op2(selection.at(i));
                err = op.mergeSuboperations(op2);

                // Send message
                IFOKDO(err,
                       op.getDocument()->sendMessage(i18nc("An information to the user",
                                                           "The sub transactions of '%1' have been merged in the transaction '%2'",
                                                           op2.getDisplayName(),
                                                           op.getDisplayName()),
                                                     SKGDocument::Hidden))
            }
        }
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Transactions merged.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Merge failed"));
    }

    // Display error
    SKGMainPanel::displayErrorMessage(err);
}

void SKGOperationPlugin::onShowOpenWithMenu()
{
    if ((m_openOperationsWithMenu != nullptr) && (m_currentBankDocument != nullptr)) {
        // Clean Menu
        auto *m = qobject_cast<QMenu *>(sender());
        bool modeOperation = (m == m_openOperationsWithMenu);
        m->clear();

        // Get Selection
        auto selection = SKGMainPanel::getMainPanel()->getSelectedObjects();

        // Build list od attribute to take into account
        QStringList existingAttributes;
        m_currentBankDocument->getAttributesList(modeOperation ? QStringLiteral("v_operation_display") : QStringLiteral("v_suboperation_consolidated"),
                                                 existingAttributes);

        QStringList attributes;
        auto schema = SKGServices::splitCSVLine(
            m_currentBankDocument->getDisplaySchemas(modeOperation ? QStringLiteral("operation") : QStringLiteral("suboperation")).at(0).schema);
        attributes.reserve(attributes.count());
        for (const auto &att : std::as_const(schema)) {
            auto a = SKGServices::splitCSVLine(att, u'|').value(0);
            if (existingAttributes.contains(a) && !a.endsWith(QStringLiteral("_INCOME")) && !a.endsWith(QStringLiteral("_EXPENSE"))) {
                attributes.push_back(a);
            }
        }

        // Build menus
        auto unitp = m_currentBankDocument->getPrimaryUnit();
        int nb = attributes.count();
        for (int i = 0; i < nb; ++i) {
            // Add actions
            QString att = attributes[i];
            QString attDisplay = m_currentBankDocument->getDisplay(att);
            QString value = selection[0].getAttribute(att);
            QString iconName = m_currentBankDocument->getIconName(att);
            QString wc = att;
            if (att.startsWith(QStringLiteral("f_"))) {
                wc += u'=' % SKGServices::stringToSqlString(value);
            } else {
                wc += "='" % SKGServices::stringToSqlString(value) % u'\'';
            }
            if (att.startsWith(QStringLiteral("f_"))) {
                auto unit = unitp;
                if (!att.contains(QStringLiteral("QUANTITY")) && !att.contains(QStringLiteral("f_BALANCE_ENTERED"))) {
                    value = SKGServices::toCurrencyString(SKGServices::stringToDouble(value), unit.Symbol, unit.NbDecimal);
                } else {
                    unit.NbDecimal = SKGServices::stringToInt(selection[0].getAttribute(QStringLiteral("i_NBDEC")));
                    if (unit.NbDecimal == 0) {
                        unit.NbDecimal = 2;
                    }
                    if (att != QStringLiteral("f_QUANTITYOWNED")) {
                        unit.Symbol = selection[0].getAttribute(QStringLiteral("t_UNIT"));
                    }
                    value = SKGServices::toCurrencyString(SKGServices::stringToDouble(value), unit.Symbol, unit.NbDecimal);
                }
            }
            QString title = i18nc("A condition", "%1 = %2", attDisplay, value);

            if (!value.isEmpty() && att != attDisplay) {
                QAction *act = m->addAction(m_currentBankDocument->getIcon(att), title);
                if (act != nullptr) {
                    connect(act, &QAction::triggered, this, [title, wc, iconName, modeOperation]() {
                        QString view = modeOperation ? QStringLiteral("v_operation_display") : QStringLiteral("v_suboperation_consolidated");
                        // Open
                        QDomDocument doc(QStringLiteral("SKGML"));
                        doc.setContent(SKGMainPanel::getMainPanel()->getDocument()->getParameter(
                            view != QStringLiteral("v_suboperation_consolidated") ? QStringLiteral("SKGOPERATION_DEFAULT_PARAMETERS")
                                                                                  : QStringLiteral("SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS")));
                        QDomElement root = doc.documentElement();
                        if (root.isNull()) {
                            root = doc.createElement(QStringLiteral("parameters"));
                            doc.appendChild(root);
                        }

                        root.setAttribute(QStringLiteral("operationTable"), view);
                        root.setAttribute(QStringLiteral("operationWhereClause"), wc);
                        root.setAttribute(QStringLiteral("title"), title);
                        root.setAttribute(QStringLiteral("title_icon"), std::move(iconName));

                        SKGMainPanel::getMainPanel()->openPage(SKGMainPanel::getMainPanel()->getPluginByName(QStringLiteral("Skrooge operation plugin")),
                                                               -1,
                                                               doc.toString(),
                                                               modeOperation ? QString() : i18nc("Noun, a list of items", "Sub transactions"));
                    });
                }
            }
        }
    }
}

#include <skgoperationplugin.moc>
