/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGTreeView component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtesttreeview.h"
#include "skgfilteredtableview.h"
#include "skgobjectmodel.h"
#include "skgsortfilterproxymodel.h"
#include "skgtestmacro.h"
#include "skgtreeview.h"

void SKGTESTTreeView::Test()
{
    KLocalizedString::setApplicationDomain("skrooge");

    // Initialize document
    SKGDocumentBank doc;
    QVERIFY2(!doc.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/advice.skg"), "Load document failed");

    // Initialization view and model
    SKGFilteredTableView tableView(nullptr);
    auto m_objectModel = new SKGObjectModel(&doc, QStringLiteral("v_account_display"), QString(), nullptr);
    tableView.setModel(m_objectModel);
    m_objectModel->setTable(QStringLiteral("v_operation_display_all"));
    QCOMPARE(m_objectModel->getGroupBy(), QString());
    m_objectModel->setGroupBy(QStringLiteral("d_date"));
    QCOMPARE(m_objectModel->getGroupBy(), QStringLiteral("d_date"));
    m_objectModel->setGroupBy(QStringLiteral("t_status"));
    m_objectModel->refresh();
    QTest::qWait(300);

    // SKGFilteredTableView
    tableView.setState(tableView.getState());

    QTest::qWait(2000);

    // SKGTreeView
    SKGTreeView *tree = tableView.getView();
    tree->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttreeview/SKGTreeView.csv");
    tree->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttreeview/SKGTreeView.txt");
    tree->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttreeview/SKGTreeView.svg");
    tree->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttreeview/SKGTreeView.pdf");
    tree->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttreeview/SKGTreeView.html");
    tree->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttreeview/SKGTreeView.odt");

    tree->getTable();
    tree->getFirstSelectedObject();

    tree->setTextResizable(true);
    QCOMPARE(tree->isTextResizable(), true);
    tree->setTextResizable(false);
    QCOMPARE(tree->isTextResizable(), false);

    tree->setZoomPosition(5);
    QCOMPARE(tree->zoomPosition(), 5);
    tree->setZoomPosition(6);
    QCOMPARE(tree->zoomPosition(), 6);

    tree->switchAutoResize();
    tree->setState(tree->getState());

    tree->getCurrentSchema();

    tableView.getSearchField()->setText(QStringLiteral("e +a -i"));
    tree->copy();

    for (int i = 0; i < 10; ++i) {
        tree->sortByColumn(i, Qt::AscendingOrder);
        QTest::qWait(1000);

        tree->sortByColumn(i, Qt::DescendingOrder);
        QTest::qWait(1000);
    }
}

QTEST_MAIN(SKGTESTTreeView)
