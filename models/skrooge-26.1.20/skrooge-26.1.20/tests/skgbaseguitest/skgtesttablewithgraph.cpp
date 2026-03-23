/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGTableWithGraph component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtesttablewithgraph.h"
#include "skgtablewithgraph.h"
#include "skgtestmacro.h"

void SKGTESTTableWithGraph::Test()
{
    KLocalizedString::setApplicationDomain("skrooge");

    SKGTableWithGraph graph(nullptr);

    SKGStringListList data;

    data.append(QStringList() << QStringLiteral("Category") << QStringLiteral("2013-01") << QStringLiteral("2013-02") << QStringLiteral("2013-03")
                              << QStringLiteral("2013-04") << QStringLiteral("2013-05") << QStringLiteral("2013-06"));
    data.append(QStringList() << QStringLiteral("Auto") << QStringLiteral("-50") << QStringLiteral("-40.5") << QStringLiteral("-40") << QStringLiteral("-60")
                              << QStringLiteral("-70") << QStringLiteral("-100"));
    data.append(QStringList() << QStringLiteral("Food") << QStringLiteral("-510.0") << QStringLiteral("-520") << QStringLiteral("-530.55") << QString()
                              << QStringLiteral("-535") << QStringLiteral("-520"));
    data.append(QStringList() << QStringLiteral("s1") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s2") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s3") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s4") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s5") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s6") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s7") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s8") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s9") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s10") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s11") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));
    data.append(QStringList() << QStringLiteral("s12") << QStringLiteral("1") << QStringLiteral("2") << QStringLiteral("3") << QString() << QStringLiteral("4")
                              << QStringLiteral("5"));

    SKGServices::SKGUnitInfo info1;
    info1.Symbol = 'P';
    info1.Name = 'P';
    info1.Date = QDate::currentDate();
    info1.NbDecimal = 2;
    info1.Value = 1;

    SKGServices::SKGUnitInfo info2;
    info2.Symbol = 'S';
    info2.Name = 'S';
    info2.Date = QDate::currentDate();
    info2.NbDecimal = 4;
    info2.Value = 0.5;
    graph.setData(data, info1, info2);

    graph.setBackgroundColor(Qt::black);
    QCOMPARE(graph.isGraphVisible(), true);
    QCOMPARE(graph.isTableVisible(), true);
    QCOMPARE(graph.isTextReportVisible(), false);
    QCOMPARE(graph.isGraphTypeSelectorVisible(), true);

    QCOMPARE(graph.switchLegendVisibility(), true);

    QCOMPARE(graph.switchLinearRegressionVisibility(), false);
    QCOMPARE(graph.switchLinearRegressionVisibility(), true);

    QCOMPARE(graph.switchLimitsVisibility(), false);
    QCOMPARE(graph.switchLimitsVisibility(), true);

    QCOMPARE(graph.swithOriginVisibility(), false);
    QCOMPARE(graph.swithOriginVisibility(), true);

    QCOMPARE(graph.getAdditionalDisplayMode(), SKGTableWithGraph::ALL);

    graph.setShadowVisible(true);
    QCOMPARE(graph.isShadowVisible(), true);

    graph.setShadowVisible(false);
    QCOMPARE(graph.isShadowVisible(), false);

    graph.setSelectable(true);
    QCOMPARE(graph.isSelectable(), true);

    graph.setSelectable(false);
    QCOMPARE(graph.isSelectable(), false);

    for (int i = 0; i < 9; ++i) {
        graph.setGraphType(static_cast<SKGTableWithGraph::GraphType>(i));
        QCOMPARE(graph.getGraphType(), static_cast<SKGTableWithGraph::GraphType>(i));
        QTest::qWait(1000);
        graph.graph()->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGGraphicsView_" + SKGServices::intToString(i)
                                    + ".png");
    }

    // Graph
    QTest::qWait(2000);

    graph.resetColors();
    SKGGraphicsView *g = graph.graph();
    g->onCopy();
    g->onSwitchToolBarVisibility();
    g->onSwitchToolBarVisibility();
    g->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGGraphicsView.svg");
    g->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGGraphicsView.pdf");
    g->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGGraphicsView.jpeg");
    g->setAntialiasing(false);
    g->setAntialiasing(true);

    SKGShow *show = graph.getShowWidget();
    show->setState(QStringLiteral("text"));
    show->getMode();
    show->setDisplayTitle(true);
    QCOMPARE(show->getDisplayTitle(), true);
    show->setDisplayTitle(false);
    QCOMPARE(show->getDisplayTitle(), false);

    /*QCOMPARE(graph.isGraphVisible(), true);
    QCOMPARE(graph.isTableVisible(), true);
    QCOMPARE(graph.isTextReportVisible(), true);*/

    SKGWebView *t = graph.textReport();
    QTest::qWait(2000);
    t->onZoomIn();
    t->onZoomOriginal();
    t->onZoomOut();
    t->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGWebView.odt");
    t->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGWebView.pdf");
    t->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGWebView.html");
    t->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGWebView.htm");
    t->exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGWebView.png");

    graph.exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGTableWithGraph.csv");
    graph.exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtesttablewithgraph/SKGTableWithGraph.txt");

    graph.getTable();
}

QTEST_MAIN(SKGTESTTableWithGraph)
