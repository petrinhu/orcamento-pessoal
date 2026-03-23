/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A collection of widgets for skrooge (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbkwidgetcollectiondesignerplugin.h"

#include "skgpredicatcreatordesignerplugin.h"
#include "skgquerycreatordesignerplugin.h"
#include "skgunitcomboboxdesignerplugin.h"

SKGBKWidgetCollectionDesignerPlugin::SKGBKWidgetCollectionDesignerPlugin(QObject *iParent)
    : QObject(iParent)
{
    m_widgets.append(new SKGQueryCreatorDesignerPlugin(this));
    m_widgets.append(new SKGPredicatCreatorDesignerPlugin(this));
    m_widgets.append(new SKGUnitComboBoxDesignerPlugin(this));
}

QList<QDesignerCustomWidgetInterface *> SKGBKWidgetCollectionDesignerPlugin::customWidgets() const
{
    return m_widgets;
}
