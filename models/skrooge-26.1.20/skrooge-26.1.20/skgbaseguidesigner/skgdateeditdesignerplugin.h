/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDATEEDITDESIGNERPLUGIN_H
#define SKGDATEEDITDESIGNERPLUGIN_H
/** @file
 * A date edit with more features (qt designer plugin).
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <QtUiPlugin/customwidget.h>

/**
 * QDesigner plugin for SKGDateEdit
 */
class SKGDateEditDesignerPlugin : public QObject, public QDesignerCustomWidgetInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)

public:
    /**
     * Constructor
     * @param iParent parent
     */
    explicit SKGDateEditDesignerPlugin(QObject *iParent = nullptr);

    /**
     * To know if the component is a container
     * @return true or false
     */
    bool isContainer() const override;

    /**
     * To know if the component is initialized
     * @return true or false
     */
    bool isInitialized() const override;

    /**
     * To get the icon for this component
     * @return the icon
     */
    QIcon icon() const override;

    /**
     * To get the icon for this component
     * @return
     */
    QString domXml() const override;

    /**
     * To get the group for this component
     * @return group
     */
    QString group() const override;

    /**
     * To get the include file for this component
     * @return the include file
     */
    QString includeFile() const override;

    /**
     * To get the name for this component
     * @return name
     */
    QString name() const override;

    /**
     * To get the "tool tip" for this component
     * @return the "tool tip"
     */
    QString toolTip() const override;

    /**
     * To get the "whats this" for this component
     * @return the "whats this"
     */
    QString whatsThis() const override;

    /**
     * To get the widget representing the component
     * @param iParent the parent of the widget
     * @return the widget
     */
    QWidget *createWidget(QWidget *iParent) override;

    /**
     * Initialize the component
     * @param iCore interface
     */
    void initialize(QDesignerFormEditorInterface *iCore) override;

private:
    bool m_initialized;
};

#endif
