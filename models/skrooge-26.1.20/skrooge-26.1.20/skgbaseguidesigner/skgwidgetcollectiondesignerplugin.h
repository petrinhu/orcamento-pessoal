/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGWIDGETCOLLECTIONDESIGNERPLUGIN_H
#define SKGWIDGETCOLLECTIONDESIGNERPLUGIN_H
/** @file
 * A collection of widgets (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <QtUiPlugin/customwidget.h>

#include "skgbaseguidesigner_export.h"
/**
 * QDesigner plugin collection
 */
class SKGBASEGUIDESIGNER_EXPORT SKGWidgetCollectionDesignerPlugin : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QDesignerCustomWidgetCollectionInterface")
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)

public:
    /**
     * Constructor
     * @param iParent the parent
     */
    explicit SKGWidgetCollectionDesignerPlugin(QObject *iParent = nullptr);

    /**
     * To get the list of widgets
     * @return the list of widgets
     */
    QList<QDesignerCustomWidgetInterface *> customWidgets() const override;

private:
    QList<QDesignerCustomWidgetInterface *> m_widgets;
};

#endif // SKGWIDGETCOLLECTIONDESIGNERPLUGIN_H
