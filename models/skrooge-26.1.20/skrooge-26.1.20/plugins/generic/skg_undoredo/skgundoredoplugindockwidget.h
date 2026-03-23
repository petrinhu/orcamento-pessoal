/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNDOREDOPLUGINDOCKWIDGET_H
#define SKGUNDOREDOPLUGINDOCKWIDGET_H
/** @file
 * This file is a undoredo for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgwidget.h"
#include "ui_skgundoredoplugindockwidget_base.h"

/**
 * This file is a plugin for undoredo management
 */
class SKGUndoRedoPluginDockWidget : public SKGWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGUndoRedoPluginDockWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGUndoRedoPluginDockWidget() override;

    /**
     * Get the main widget
     * @return a widget
     */
    QWidget *mainWidget() override;

private Q_SLOTS:
    void onUndoRedo(const QModelIndex &index);
    void onClearHistory();

private:
    Q_DISABLE_COPY(SKGUndoRedoPluginDockWidget)

    Ui::skgundoredoplugindockwidget_base ui{};
};

#endif // SKGUNDOREDOPLUGINDOCKWIDGET_H
