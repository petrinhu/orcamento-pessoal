/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSEARCHPLUGINWIDGET_H
#define SKGSEARCHPLUGINWIDGET_H
/** @file
 * A skrooge plugin to search and process transactions
 *
 * @author Stephane MANKOWSKI
 */
#include "skgruleobject.h"
#include "skgtabpage.h"
#include "ui_skgsearchpluginwidget_base.h"

/**
 * A skrooge plugin to search and process transactions
 */
class SKGSearchPluginWidget : public SKGTabPage
{
    Q_OBJECT
public:
    /**
     * This enumerate defines the open mode
     */
    enum OpenMode {
        TABLE, /**< open in a table*/
        REPORT /**< open in report page*/
    };
    /**
     * This enumerate defines the open mode
     */
    Q_ENUM(OpenMode)
public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGSearchPluginWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGSearchPluginWidget() override;

    /**
     * Get the current selection
     * @return selected objects
     */
    SKGObjectBase::SKGListSKGObjectBase getSelectedObjects() override;

    /**
     * Get the number of selected object
     * @return number of selected objects
     */
    int getNbSelectedObjects() override;

    /**
     * Get the current state
     * MUST BE OVERWRITTEN
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    QString getState() override;

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    void setState(const QString &iState) override;

    /**
     * Get attribute name to save the default state
     * MUST BE OVERWRITTEN
     * @return attribute name to save the default state.
     */
    QString getDefaultStateAttribute() override;

    /**
     * To know if this page contains an editor. MUST BE OVERWRITTEN
     * @return the editor state
     */
    bool isEditor() override;

    /**
     * To activate the editor by setting focus on right widget. MUST BE OVERWRITTEN
     */
    void activateEditor() override;

    /**
     * Get the main widget
     * @return a widget
     */
    QWidget *mainWidget() override;

    /**
     * Open transactions corresponding to a rule
     * @param iRule a rule
     * @param iMode the mode for the open
     */
    static void open(const SKGRuleObject &iRule, OpenMode iMode = TABLE);

protected:
    /**
     * Event filtering
     * @param iObject object
     * @param iEvent event
     * @return In your reimplementation of this function, if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return
     * false.
     */
    bool eventFilter(QObject *iObject, QEvent *iEvent) override;

private Q_SLOTS:
    void dataModified(const QString &iTableName, int iIdTransaction);
    void onAddRule();
    void onModifyRule();
    void onOpen();
    void onSelectionChanged();
    void onTop();
    void onUp();
    void onDown();
    void onBottom();
    void onEditorModified();
    void cleanEditor();

private:
    Q_DISABLE_COPY(SKGSearchPluginWidget)

    QString getXMLActionDefinition();
    Ui::skgsearchplugin_base ui{};
};

#endif // SKGSEARCHPLUGINWIDGET_H
