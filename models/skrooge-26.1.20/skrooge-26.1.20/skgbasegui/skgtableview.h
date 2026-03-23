/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTABLEVIEW_H
#define SKGTABLEVIEW_H
/** @file
 * A table view with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbasegui_export.h"
#include "skgtreeview.h"

/**
 * This file is a tab widget used by plugins
 */
class SKGBASEGUI_EXPORT SKGTableView : public SKGTreeView
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGTableView(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGTableView() override;
};

#endif // SKGTABLEVIEW_H
