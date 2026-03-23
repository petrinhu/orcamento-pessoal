/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test for SKGWidgetCollectionDesignerPlugin component.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtestwidgetcollection.h"

#include <qicon.h>

#include "skggraphicsscene.h"
#include "skgtestmacro.h"
#include "skgwebview.h"
#include "skgwidgetcollectiondesignerplugin.h"
#include "skgzoomselector.h"

void SKGTESTWidgetCollection::Test()
{
    KLocalizedString::setApplicationDomain("skrooge");

    SKGWidgetCollectionDesignerPlugin col(this);
    QList<QDesignerCustomWidgetInterface *> widgets = col.customWidgets();
    for (int i = 0; i < widgets.count(); ++i) {
        QDesignerCustomWidgetInterface *widget = widgets.at(i);
        QCOMPARE(widget != nullptr, true);

        widget->isContainer();
        QCOMPARE(widget->isInitialized(), false);
        widget->initialize(nullptr);
        QCOMPARE(widget->isInitialized(), true);
        widget->icon();
        QCOMPARE(widget->domXml() != QString(), true);
        QCOMPARE(widget->group(), QStringLiteral("SKG Widgets"));
        QCOMPARE(widget->includeFile() != QString(), true);
        QCOMPARE(widget->name() != QString(), true);
        QCOMPARE(widget->toolTip() != QString(), true);
        QCOMPARE(widget->whatsThis() != QString(), true);
        QCOMPARE(widget->createWidget(nullptr) != nullptr, true);
    }

    // Test zoom
    SKGZoomSelector zoom(nullptr);
    zoom.zoomIn();
    zoom.zoomOut();
    zoom.initializeZoom();

    // Test web view
    SKGWebView web(nullptr);
    web.onZoomIn();
    web.onZoomOut();
    web.exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestwidgetcollection/export.pdf");
    web.exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestwidgetcollection/export.html");
    web.exportInFile(SKGTest::getTestPath(QStringLiteral("OUT")) % "/skgtestwidgetcollection/export.jpg");

    SKGGraphicsScene graphScene;
}

QTEST_MAIN(SKGTESTWidgetCollection)
