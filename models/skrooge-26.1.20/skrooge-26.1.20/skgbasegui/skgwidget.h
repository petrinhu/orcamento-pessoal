/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGWIDGET_H
#define SKGWIDGET_H
/** @file
 * This file is a class managing widget.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qwidget.h>

#include "skgbasegui_export.h"
#include "skgobjectbase.h"

class SKGDocument;

/**
 * This file is a tab widget used by plugins
 */
class SKGBASEGUI_EXPORT SKGWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGWidget(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGWidget() override;

    /**
     * Get main document
     * @return pointer on main document
     */
    virtual SKGDocument *getDocument() const;

    /**
     * Get the current state
     * MUST BE OVERWRITTEN
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    virtual QString getState();

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    virtual void setState(const QString &iState);

    /**
     * Get attribute name to save the default state
     * MUST BE OVERWRITTEN
     * @return attribute name to save the default state.
     */
    virtual QString getDefaultStateAttribute();

    /**
     * Get the current selection
     * MUST BE OVERWRITTEN
     * @return selected objects
     */
    virtual SKGObjectBase::SKGListSKGObjectBase getSelectedObjects();

    /**
     * Get the first selected object
     * @return first selected object
     */
    virtual SKGObjectBase getFirstSelectedObject();

    /**
     * Get the number of selected object
     * CAN BE OVERWRITTEN FOR OPTIMIZATION
     * @return number of selected objects
     */
    virtual int getNbSelectedObjects();

    /**
     * To know if the widget having the selection has the focus
     * Default implementation is based on mainWidget
     * Don't forget to do mainWidget()->installEventFilter(this);
     * @return true of false
     */
    virtual bool hasSelectionWithFocus();

    /**
     * Get the main widget
     * @return a widget
     */
    virtual QWidget *mainWidget();

Q_SIGNALS:
    /**
     * This signal must be launched when the selection is modified
     */
    void selectionChanged();

    /**
     * This signal must be launched when the widget having the selection has win or lost the focus
     */
    void selectionFocusChanged();

protected:
    /**
     * Event filtering
     * @param iObject object
     * @param iEvent event
     * @return In your reimplementation of this function, if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return
     * false.
     */
    bool eventFilter(QObject *iObject, QEvent *iEvent) override;

private:
    Q_DISABLE_COPY(SKGWidget)

    SKGDocument *m_document;
};

#endif // SKGWIDGET_H
