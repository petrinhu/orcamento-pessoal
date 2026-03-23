/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for actions.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestactions.h"
#include "skgboardwidget.h"
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgtestmacro.h"
#include "skgtraces.h"
#include <utility>

void SKGTESTActions::Test()
{
    KLocalizedString::setApplicationDomain("skrooge");

    // Initialize document
    SKGDocumentBank doc;

    // Create main panel
    SKGMainPanel mainpanel(nullptr, &doc);
    mainpanel.show();

    QVERIFY2(!doc.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/advice.skg"), "Load document failed");

    {
        SKGError err;
        SKGBEGINTRANSACTION(doc, QStringLiteral("DELETE"), err)
        doc.executeSqliteOrder(QStringLiteral("DELETE FROM operation WHERE d_date<'") + SKGServices::dateToSqlString(QDate::currentDate().addMonths(-2))
                               + '\'');
    }

    // Scenario
    QStringList actionsToExecute;
    actionsToExecute
        // Reinitialize document
        << QStringLiteral("tab_closeall") << QStringLiteral("fullscreen") << QStringLiteral("fullscreen") << QStringLiteral("fullscreen")
        << QStringLiteral("edit_undolastsave") << QStringLiteral("view_unlock") << QStringLiteral("view_unlock") << QStringLiteral("options_show_menubar")
        << QStringLiteral("options_show_menubar")
        // Bank page
        << QStringLiteral("page_Skrooge bank plugin") << QStringLiteral("enable_editor") << QStringLiteral("edit_select_all") << QStringLiteral("edit_copy")
        << QStringLiteral("edit_switch_highlight") << QStringLiteral("edit_select_all")
        << QStringLiteral("edit_delete")
        // Undo
        << QStringLiteral("edit_undo") << QStringLiteral("edit_redo") << QStringLiteral("edit_undolastsave")
        << QStringLiteral("edit_clear_history")
        // Clean
        << QStringLiteral("edit_select_all") << QStringLiteral("open_report") << QStringLiteral("edit_find") << QStringLiteral("page_Skrooge bank plugin")
        << QStringLiteral("edit_select_all") << QStringLiteral("edit_reconcile") << QStringLiteral("page_Skrooge bank plugin")
        << QStringLiteral("edit_select_all")
        << QStringLiteral("edit_find")

        // Transaction
        << QStringLiteral("page_Skrooge operation plugin") << QStringLiteral("enable_editor") << QStringLiteral("fast_edition")
        << QStringLiteral("edit_select_all") << QStringLiteral("edit_switch_highlight") << QStringLiteral("edit_mark_selected_operation")
        << QStringLiteral("merge_sub_operations")
        // Too long << "edit_select_all" << "edit_duplicate_operation"
        << QStringLiteral("edit_select_all") << QStringLiteral("edit_group_operation") << QStringLiteral("merge_imported_operation")
        << QStringLiteral("edit_select_all") << QStringLiteral("open_report") << QStringLiteral("page_Skrooge operation plugin")
        << QStringLiteral("edit_select_all")
        << QStringLiteral("edit_find")

        // Category
        << QStringLiteral("page_Skrooge categories plugin") << QStringLiteral("edit_expandall") << QStringLiteral("edit_collapseal")
        << QStringLiteral("edit_select_all") << QStringLiteral("open_report") << QStringLiteral("page_Skrooge categories plugin")
        << QStringLiteral("edit_select_all")
        << QStringLiteral("edit_find")

        // Search page
        << QStringLiteral("page_Skrooge search plugin") << QStringLiteral("edit_select_all") << QStringLiteral("execute_all")
        << QStringLiteral("execute_notchecked") << QStringLiteral("execute_imported") << QStringLiteral("execute_not_validated")
        << QStringLiteral("open_report")

        // All other pages
        << QStringLiteral("page_Dashboard plugin") << QStringLiteral("500") << QStringLiteral("page_Skrooge budget plugin")
        << QStringLiteral("tool_process_budget_rules") << QStringLiteral("edit_select_all") << QStringLiteral("open_report")
        << QStringLiteral("page_Skrooge budget plugin") << QStringLiteral("edit_select_all") << QStringLiteral("edit_find")
        << QStringLiteral("page_Skrooge calculator plugin") << QStringLiteral("page_Skrooge payee plugin") << QStringLiteral("edit_select_all")
        << QStringLiteral("open_report") << QStringLiteral("page_Skrooge payee plugin") << QStringLiteral("edit_select_all") << QStringLiteral("edit_find")
        << QStringLiteral("page_Skrooge report plugin") << QStringLiteral("page_Skrooge scheduled plugin") << QStringLiteral("edit_select_all")
        << QStringLiteral("open_report") << QStringLiteral("page_Skrooge scheduled plugin") << QStringLiteral("edit_select_all") << QStringLiteral("edit_find")
        << QStringLiteral("page_Skrooge tracker plugin") << QStringLiteral("edit_select_all") << QStringLiteral("open_report")
        << QStringLiteral("page_Skrooge tracker plugin") << QStringLiteral("edit_select_all") << QStringLiteral("edit_find")
        << QStringLiteral("page_Skrooge unit plugin") << QStringLiteral("edit_select_all") << QStringLiteral("open_report")
        << QStringLiteral("page_Skrooge unit plugin") << QStringLiteral("edit_select_all")
        << QStringLiteral("edit_find")

        // Go
        << QStringLiteral("page_Monthly plugin") << QStringLiteral("go_previous") << QStringLiteral("go_next") << QStringLiteral("go_home")
        << QStringLiteral("new_tab")

        // imports
        << QStringLiteral("import") << QStringLiteral("import_backends") << QStringLiteral("import_standard_bookmarks")
        << QStringLiteral("import_standard_categories")

        // Processing
        << QStringLiteral("align_comment") << QStringLiteral("clean_delete_unused_categories") << QStringLiteral("clean_delete_unused_payees")
        << QStringLiteral("clean_delete_unused_units") << QStringLiteral("clean_remove_group_with_one_operation") << QStringLiteral("process_banks")
        << QStringLiteral("process_foundtransfer") << QStringLiteral("process_validate") << QStringLiteral("processing")
        << QStringLiteral("switch_validation_imported_operation")

        // Views
        << QStringLiteral("view_bookmarks") << QStringLiteral("view_context") << QStringLiteral("view_contextmenu") << QStringLiteral("view_lock")
        << QStringLiteral("view_menu") << QStringLiteral("view_open_duplicates") << QStringLiteral("view_open_highlight")
        << QStringLiteral("view_open_last_modified") << QStringLiteral("view_open_not_validated") << QStringLiteral("view_open_operation_in_group_of_one")
        << QStringLiteral("view_open_operation_with_comment_not_aligned") << QStringLiteral("view_open_operation_without_category")
        << QStringLiteral("view_open_operation_without_mode") << QStringLiteral("view_open_operation_without_payee")
        << QStringLiteral("view_open_suboperations") << QStringLiteral("view_open_transfers_without_category")
        << QStringLiteral("view_open_transfers_without_payee") << QStringLiteral("view_open_very_old_operations") << QStringLiteral("view_properties")
        << QStringLiteral("view_transactions")
        // Close
        << QStringLiteral("tab_close") << QStringLiteral("tab_closeallother")
        << QStringLiteral("tab_closeall")
        // Tabs
        << QStringLiteral("tab_reopenlastclosed") << QStringLiteral("tab_resetdefaultstate") << QStringLiteral("tab_savedefaultstate")
        << QStringLiteral("tab_switchpin")
        << QStringLiteral("tab_overwritebookmark")

        // File
        << QStringLiteral("file_save") << QStringLiteral("file_new");

    // Dump actions
    QMap<QString, QPointer<QAction>> actions = mainpanel.getGlobalActions();
    QStringList keys = actions.keys();
    for (const auto &key : std::as_const(keys)) {
        if (!actionsToExecute.contains(key)) {
            SKGTRACE << "  [" << key << "]" << Qt::endl;
        }
    }

    // Trigger actions
    for (const auto &key : std::as_const(actionsToExecute)) {
        if (key == QStringLiteral("edit_select_all")) {
            SKGTRACE << "      Wait" << Qt::endl;
            QTest::qWait(10);
        }
        SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")) << "  [" << key << "]: ";
        QAction *act = mainpanel.getGlobalAction(key);
        if (act != nullptr) {
            act->trigger();
            SKGTRACESUITE << "DONE" << Qt::endl;
        } else {
            SKGTRACESUITE << "UNKNOWN" << Qt::endl;
        }

        if (key == QStringLiteral("edit_select_all")) {
            SKGTabPage *page = mainpanel.currentPage();
            if (page != nullptr) {
                SKGTRACE << "      Current page:" << page->objectName() << Qt::endl;
            }
            SKGTRACE << "      Nb objects selected:" << mainpanel.getNbSelectedObjects() << Qt::endl;
        }
    }
}

QTEST_MAIN(SKGTESTActions)
