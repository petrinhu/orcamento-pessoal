/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGUNITPLUGINWIDGET_H
#define SKGUNITPLUGINWIDGET_H
/** @file
 * This file is Skrooge plugin for unit management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtabpage.h"
#include "skgunitobject.h"
#include "ui_skgunitpluginwidget_base.h"

class SKGDocumentBank;

/**
 * This file is Skrooge plugin for unit management
 */
class SKGUnitPluginWidget : public SKGTabPage
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGUnitPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument);

    /**
     * Default Destructor
     */
    ~SKGUnitPluginWidget() override;

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
     * Download values for a unit
     * @param iUnit unit
     * @param iMode the download mode
     * @return an object managing the error
     *   @see SKGError
     */
    static SKGError downloadUnitValue(const SKGUnitObject &iUnit, SKGUnitObject::UnitDownloadMode iMode);

    /**
     * Get the download mode defined in settings
     * @return the download mode
     */
    static SKGUnitObject::UnitDownloadMode getDownloadModeFromSettings();

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
    void onSelectionChanged();
    void onSelectionValueChanged();
    void onUnitCreatorModified();
    void onAddUnit();
    void onModifyUnit();
    void onDownloadUnitValue();
    void onSimplify();
    void onDeleteSource();
    void onAddSource();
    void onSourceChanged();
    void onGetNewHotStuff();
    void onPutNewHotStuff();
    void onOpenURL();
    void cleanEditor();
    void refreshUnitList();
    void fillSourceList();

private:
    Q_DISABLE_COPY(SKGUnitPluginWidget)

    Ui::skgunitplugin_base ui{};
    QTimer m_timer;

    QAction *m_upload;
    SKGComboBox *m_unitValueGraphCmb;
};

#endif // SKGDEBUGPLUGIN_H
