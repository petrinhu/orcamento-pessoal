/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGBKWIDGETCOLLECTIONDESIGNERPLUGIN_H
#define SKGBKWIDGETCOLLECTIONDESIGNERPLUGIN_H
/** @file
 * A collection of widgets for skrooge (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <QtUiPlugin/customwidget.h>

#include "skgbankguidesigner_export.h"
/**
 * QDesigner plugin collection
 */
class SKGBANKGUIDESIGNER_EXPORT SKGBKWidgetCollectionDesignerPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface")
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
    /**
     * Constructor
     * @param iParent the parent
     */
    explicit SKGBKWidgetCollectionDesignerPlugin(QObject *iParent = nullptr);

    /**
     * To get the list of widgets
     * @return the list of widgets
     */
    QList<QDesignerCustomWidgetInterface *> customWidgets() const override;

private:
    QList<QDesignerCustomWidgetInterface *> m_widgets;
};

#endif // SKGBKWIDGETCOLLECTIONDESIGNERPLUGIN_H
