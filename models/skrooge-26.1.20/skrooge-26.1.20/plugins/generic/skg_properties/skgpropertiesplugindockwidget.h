/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPROPERTIESPLUGINDOCKWIDGET_H
#define SKGPROPERTIESPLUGINDOCKWIDGET_H
/** @file
 * A plugin to manage properties on objects
 *
 * @author Stephane MANKOWSKI
 */
#include "skgwidget.h"
#include "ui_skgpropertiesplugindockwidget_base.h"

/**
 * A plugin to manage properties on objects
 */
class SKGPropertiesPluginDockWidget : public SKGWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGPropertiesPluginDockWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGPropertiesPluginDockWidget() override;

    /**
     * Get the main widget
     * @return a widget
     */
    QWidget *mainWidget() override;

public Q_SLOTS:
    /**
     * Refresh the content.
     */
    virtual void refresh();

private Q_SLOTS:
    void onSelectionChanged();
    void onAddProperty();
    void onRenameProperty();
    void onRemoveProperty();
    void onSelectFile();
    void onOpenFile();
    void onOpenPropertyFileByUrl();
    void cleanEditor();

private:
    Q_DISABLE_COPY(SKGPropertiesPluginDockWidget)

    void openPropertyFile(const SKGPropertyObject &iProp);

    Ui::skgpropertiesdockplugin_base ui{};
};

#endif // SKGPROPERTIESPLUGINDOCKWIDGET_H
