/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A table view with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtableview.h"
#include <qheaderview.h>

SKGTableView::SKGTableView(QWidget *iParent)
    : SKGTreeView(iParent)
{
    this->setAllColumnsShowFocus(true);
    this->setRootIsDecorated(false);
    this->setUniformRowHeights(true);
    header()->setStretchLastSection(false);
}

SKGTableView::~SKGTableView() = default;
