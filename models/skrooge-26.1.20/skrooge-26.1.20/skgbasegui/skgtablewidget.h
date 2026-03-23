/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTABLEWIDGET_H
#define SKGTABLEWIDGET_H
/** @file
 * A table widget with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include <qtablewidget.h>
/**
 * This file is a combo box with more features.
 */
class SKGBASEGUI_EXPORT SKGTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGTableWidget(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGTableWidget() override;

    /**
     * Set the horizontal scroll bar stick to the maximum
     * @param iStick the stick state
     */
    virtual void setStickHorizontal(bool iStick);

    /**
     * Get the horizontal scroll bar stick to the maximum
     * @return the stick state
     */
    virtual bool stickHorizontal() const;

    /**
     * Set the vertical scroll bar stick to the maximum
     * @param iStick the stick state
     */
    virtual void setStickVertical(bool iStick);

    /**
     * Get the vertical scroll bar stick to the maximum
     * @return the stick state
     */
    virtual bool stickVertical() const;

Q_SIGNALS:
    /**
     * Line must be removed
     */
    void removeLine(int /*_t1*/);

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
    void copy();
    void onActionTriggered();
    void onRangeChanged();

private:
    bool stickH;
    bool stickV;
};

#endif // SKGTABLEWIDGET_H
