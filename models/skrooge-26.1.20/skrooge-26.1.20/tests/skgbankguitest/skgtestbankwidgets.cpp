/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for bank widgets.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestbankwidgets.h"
#include "skgbkwidgetcollectiondesignerplugin.h"
#include "skgdocumentbank.h"
#include "skgquerycreator.h"
#include "skgtestmacro.h"
#include "skgunitcombobox.h"
#include <utility>

void SKGTESTBankWidgets::TestSKGUnitComboBox()
{
    SKGDocumentBank doc;
    QVERIFY2(!doc.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/advice.skg"), "Load document failed");

    SKGUnitComboBox unitWidget(nullptr);
    unitWidget.setDocument(&doc);
    unitWidget.setWhereClauseCondition(QStringLiteral("t_type='S'"));
    SKGUnitObject unit = unitWidget.getUnit();
    unitWidget.setUnit(unit);

    unitWidget.setText(QStringLiteral("newunit"));
    SKGUnitObject newunit = unitWidget.getUnit();
}

void SKGTESTBankWidgets::TestSKGQueryCreator()
{
    SKGDocumentBank doc;
    QVERIFY2(!doc.load(SKGTest::getTestPath(QStringLiteral("OUT")) % "/advice.skg"), "Load document failed");

    QStringList attributeForQuery;
    attributeForQuery << QStringLiteral("d_date") << QStringLiteral("t_number") << QStringLiteral("t_mode") << QStringLiteral("t_PAYEE")
                      << QStringLiteral("t_comment") << QStringLiteral("t_REALCOMMENT") << QStringLiteral("t_REALCATEGORY") << QStringLiteral("t_status")
                      << QStringLiteral("t_bookmarked") << QStringLiteral("t_imported") << QStringLiteral("t_TRANSFER") << QStringLiteral("t_UNIT")
                      << QStringLiteral("t_ACCOUNT") << QStringLiteral("t_TOACCOUNT") << QStringLiteral("f_REALCURRENTAMOUNT") << QStringLiteral("t_REALREFUND")
                      << QStringLiteral("f_BALANCE");

    SKGQueryCreator creator(nullptr);
    creator.setParameters(&doc, QStringLiteral("v_operation"), attributeForQuery);
    QCOMPARE(creator.getColumnsCount(), 0);
    QCOMPARE(creator.getLinesCount(), 1);

    QString cond = QStringLiteral(
        "<!DOCTYPE SKGML><element> <!--OR--> <element>  <!--AND-->  <element operator=\"#ATT# LIKE '%#V1S#%'\" att2s=\"\" attribute=\"t_PAYEE\" att2=\"\" "
        "value=\"VIR CAF \" value2=\"\"/> </element></element>");
    creator.setXMLCondition(cond);
    QTest::qWait(100);
    QString result = creator.getXMLCondition();

    QVERIFY(result.contains(QStringLiteral("operator=\"#ATT# LIKE '%#V1S#%'\"")));
    QVERIFY(result.contains(QStringLiteral("att2s=\"\"")));
    QVERIFY(result.contains(QStringLiteral("attribute=\"t_PAYEE\"")));
    QVERIFY(result.contains(QStringLiteral("att2=\"\"")));
    QVERIFY(result.contains(QStringLiteral("value=\"VIR CAF \"")));
    QVERIFY(result.contains(QStringLiteral("value2=\"\"")));

    QCOMPARE(creator.getColumnsCount(), 1);
    QCOMPARE(creator.getLinesCount(), 2);

    creator.addNewLine();
    QCOMPARE(creator.getColumnsCount(), 1);
    QCOMPARE(creator.getLinesCount(), 3);

    creator.removeLine(0);
    creator.removeLine(0);
    QCOMPARE(creator.getColumnsCount(), 1);
    QCOMPARE(creator.getLinesCount(), 1);

    creator.removeColumn(0);
    QCOMPARE(creator.getColumnsCount(), 0);
    QCOMPARE(creator.getLinesCount(), 1);

    creator.removeLine(0);
    QCOMPARE(creator.getColumnsCount(), 0);
    QCOMPARE(creator.getLinesCount(), 1);
}

void SKGTESTBankWidgets::TestSKGBKWidgetCollectionDesignerPlugin()
{
    SKGBKWidgetCollectionDesignerPlugin col(nullptr);
    QList<QDesignerCustomWidgetInterface *> items = col.customWidgets();
    for (auto item : std::as_const(items)) {
        QCOMPARE(item != nullptr, true);

        item->isContainer();
        QCOMPARE(item->isInitialized(), false);
        item->initialize(nullptr);
        QCOMPARE(item->isInitialized(), true);
        item->icon();
        QCOMPARE(item->domXml() != QString(), true);
        QCOMPARE(item->group(), QStringLiteral("SKG Widgets"));
        QCOMPARE(item->includeFile() != QString(), true);
        QCOMPARE(item->name() != QString(), true);
        QCOMPARE(item->toolTip() != QString(), true);
        QCOMPARE(item->whatsThis() != QString(), true);
        QCOMPARE(item->createWidget(nullptr) != nullptr, true);
    }
}

QTEST_MAIN(SKGTESTBankWidgets)
