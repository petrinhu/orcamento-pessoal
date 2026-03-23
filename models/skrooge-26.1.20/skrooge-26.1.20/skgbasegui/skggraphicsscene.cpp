/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines classes SKGGraphicsScene.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skggraphicsscene.h"

SKGGraphicsScene::SKGGraphicsScene(QObject *iParent)
    : QGraphicsScene(iParent)
{
}

SKGGraphicsScene::~SKGGraphicsScene() = default;

void SKGGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent)
{
    Q_EMIT doubleClicked();

    QGraphicsScene::mouseDoubleClickEvent(mouseEvent);
}
