/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A collection of widgets (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgwidgetcollectiondesignerplugin.h"

#include "skgcalculatoreditdesignerplugin.h"
#include "skgcolorbuttondesignerplugin.h"
#include "skgcomboboxdesignerplugin.h"
#include "skgdateeditdesignerplugin.h"
#include "skgfilteredtableviewdesignerplugin.h"
#include "skggraphicsviewdesignerplugin.h"
#include "skgperiodeditdesignerplugin.h"
#include "skgprogressbardesignerplugin.h"
#include "skgshowdesignerplugin.h"
#include "skgsimpleperiodeditdesignerplugin.h"
#include "skgtableviewdesignerplugin.h"
#include "skgtablewidgetdesignerplugin.h"
#include "skgtablewithgraphdesignerplugin.h"
#include "skgtabwidgetdesignerplugin.h"
#include "skgtreeviewdesignerplugin.h"
#include "skgwebviewdesignerplugin.h"
#include "skgwidgetselectordesignerplugin.h"
#include "skgzoomselectordesignerplugin.h"

SKGWidgetCollectionDesignerPlugin::SKGWidgetCollectionDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_widgets.append(new SKGCalculatorEditDesignerPlugin(this));
    m_widgets.append(new SKGComboBoxDesignerPlugin(this));
    m_widgets.append(new SKGColorButtonDesignerPlugin(this));
    m_widgets.append(new SKGDateEditDesignerPlugin(this));
    m_widgets.append(new SKGFilteredTableViewDesignerPlugin(this));
    m_widgets.append(new SKGGraphicsViewDesignerPlugin(this));
    m_widgets.append(new SKGShowDesignerPlugin(this));
    m_widgets.append(new SKGTableViewDesignerPlugin(this));
    m_widgets.append(new SKGTreeViewDesignerPlugin(this));
    m_widgets.append(new SKGTableWithGraphDesignerPlugin(this));
    m_widgets.append(new SKGTabWidgetDesignerPlugin(this));
    m_widgets.append(new SKGTableWidgetDesignerPlugin(this));
    m_widgets.append(new SKGWebViewDesignerPlugin(this));
    m_widgets.append(new SKGWidgetSelectorDesignerPlugin(this));
    m_widgets.append(new SKGZoomSelectorDesignerPlugin(this));
    m_widgets.append(new SKGProgressBarDesignerPlugin(this));
    m_widgets.append(new SKGPeriodEditDesignerPlugin(this));
    m_widgets.append(new SKGSimplePeriodEditDesignerPlugin(this));
}

QList<QDesignerCustomWidgetInterface *> SKGWidgetCollectionDesignerPlugin::customWidgets() const
{
    return m_widgets;
}
