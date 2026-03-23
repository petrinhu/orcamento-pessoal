/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGGRAPHICSSCENE_H
#define SKGGRAPHICSSCENE_H
/** @file
 * This file defines a graphic scene with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include <qgraphicsscene.h>
/**
 * A graphic scene with more features
 */
class SKGBASEGUI_EXPORT SKGGraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    /**
     * Default constructor
     * @param iParent the parent
     */
    explicit SKGGraphicsScene(QObject *iParent = nullptr);

    /**
     * Destructor
     */
    ~SKGGraphicsScene() override;

Q_SIGNALS:
    /**
     * This signal is launched when a double click is done
     */
    void doubleClicked();

protected:
    /**
     * This event handler, for event mouseEvent, can be reimplemented in a subclass to receive mouse doubleclick events for the scene.
     * @param mouseEvent the event
     */
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *mouseEvent) override;

private:
    Q_DISABLE_COPY(SKGGraphicsScene)
};

#endif
