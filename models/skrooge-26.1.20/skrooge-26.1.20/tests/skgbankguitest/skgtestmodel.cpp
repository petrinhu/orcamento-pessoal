/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGObjectModel component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestmodel.h"
#include "modeltest.h"
#include "skgobjectmodel.h"
#include "skgsortfilterproxymodel.h"
#include "skgtestmacro.h"
#include <utility>

void SKGTESTModel::Test()
{
    KLocalizedString::setApplicationDomain("skrooge");

    int wait = 1;

    // Initialize document
    SKGDocumentBank doc;
    QVERIFY2(!doc.load(SKGTest::getTestPath(QStringLiteral("IN")) % "/advice.skg"), "Load document failed");

    QStringList tables;
    doc.getDistinctValues(
        QStringLiteral("sqlite_master"),
        QStringLiteral("name"),
        QStringLiteral("type='view' AND name LIKE 'v_%_display' OR "
                       "name IN ('v_rule', 'v_account', 'v_recurrentoperation', 'v_operation_display_all', 'v_unit', 'v_operation_displayname', 'v_node')"),
        tables);

    {
        SKGError err;
        SKGBEGINTRANSACTION(doc, QStringLiteral("DELETE"), err)
        doc.executeSqliteOrder(QStringLiteral("DELETE FROM operation WHERE d_date<'") + SKGServices::dateToSqlString(QDate::currentDate().addMonths(-2))
                               + '\'');
    }

    for (const auto &table : std::as_const(tables)) {
        SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "..:: " << table << " ::.." << Qt::endl;

        // Initialization model
        auto model = new SKGObjectModel(
            &doc,
            table,
            (table == QStringLiteral("v_account_display") || table == QStringLiteral("v_refund_display") ? QStringLiteral("t_close='N'") : QString()),
            nullptr,
            (table == QStringLiteral("v_category_display") ? QStringLiteral("rd_category_id") : QString()));

        if (table == QStringLiteral("v_rule")) {
            model->setGroupBy(QStringLiteral("t_action_type"));
        }
        if (table == QStringLiteral("v_account")) {
            model->setGroupBy(QStringLiteral("t_close"));
        }
        if (table == QStringLiteral("v_recurrentoperation")) {
            model->setGroupBy(QStringLiteral("i_nb_times"));
        }
        if (table == QStringLiteral("v_operation_display_all")) {
            model->setGroupBy(QStringLiteral("t_status"));
        }
        if (table == QStringLiteral("v_operation")) {
            model->setGroupBy(QStringLiteral("d_date"));
        }
        if (table == QStringLiteral("v_unit")) {
            model->setGroupBy(QStringLiteral("t_TYPENLS"));
        }
        if (table == QStringLiteral("v_operation_displayname")) {
            model->setGroupBy(QStringLiteral("p_myproperty"));
        }
        model->getGroupBy();
        model->refresh();
        model->supportedDragActions();
        QTest::qWait(wait);

        int nbCols = model->columnCount();
        SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "     Nb rows=" << model->rowCount() << Qt::endl;
        SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "     Nb Cols=" << nbCols << Qt::endl;
        for (int i = 0; i < nbCols; ++i) {
            SKGTRACE << "         " << model->getAttribute(i) << Qt::endl;
        }

        // Set Data
        if (table == QStringLiteral("v_account_display")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_close, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_close"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);
        }
        if (table == QStringLiteral("v_refund_display")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_close, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_close"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);
        }
        if (table == QStringLiteral("v_operation_display")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_status, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_status"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);
        }
        if (table == QStringLiteral("v_recurrentoperation_display")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(i_warn_days, Checked)"
                     << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("i_warn_days"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);

            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(i_auto_write_days, Checked)"
                     << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("i_auto_write_days"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);

            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(i_nb_times, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("i_nb_times"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);
        }
        if (table == QStringLiteral("v_budgetrule_display")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(i_year, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("i_year"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);

            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(i_month, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("i_month"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);

            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_CATEGORYCONDITION, Checked)"
                     << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_CATEGORYCONDITION"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);

            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_CATEGORY, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_CATEGORY"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);
        }
        if (table == QStringLiteral("v_budget_display")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_CATEGORY, Checked)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_CATEGORY"))),
                           QVariant(static_cast<unsigned int>(Qt::Checked)),
                           Qt::CheckStateRole);
        }
        if (table == QStringLiteral("node")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_name, new name)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_name"))), QVariant("new name"), Qt::EditRole);
        }
        if (table == QStringLiteral("v_unit_display")) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             setData(t_name, new name)" << Qt::endl;
            model->setData(model->index(1, model->getIndexAttribute(QStringLiteral("t_name"))), QVariant("new name"), Qt::EditRole);
        }

        if ((table == QStringLiteral("v_node") || table.endsWith(QStringLiteral("_display"))) && (model->supportedDragActions() & Qt::MoveAction) != 0u
            && (model->supportedDropActions() & Qt::MoveAction) != 0u) {
            SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             Drag & Drop" << Qt::endl;
            model->mimeTypes();
            QMimeData *md = model->mimeData(QModelIndexList() << model->index(0, 0));
            QCOMPARE(model->dropMimeData(md, Qt::MoveAction, 1, 0, model->index(1, 0)), true);
        }

        // test
        SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             new ModelTest" << Qt::endl;
        auto modeltest = new ModelTest(model);
        QTest::qWait(wait);

        delete modeltest;
        delete model;
        SKGTRACE << QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")) << "             end" << Qt::endl;
    }
}

QTEST_MAIN(SKGTESTModel)
