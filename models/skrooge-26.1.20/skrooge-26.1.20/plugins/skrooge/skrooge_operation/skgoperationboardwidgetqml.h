/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGOPERATIONBOARDWIDGETQML_H
#define SKGOPERATIONBOARDWIDGETQML_H
/** @file
 * This file is Skrooge plugin for transaction management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skghtmlboardwidget.h"

class QAction;
class SKGPeriodEdit;

/**
 * This file is Skrooge plugin for transaction management
 */
class SKGOperationBoardWidgetQml : public SKGHtmlBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGOperationBoardWidgetQml(QWidget *iParent, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGOperationBoardWidgetQml() override;

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

private Q_SLOTS:
    void settingsModified();

private:
    Q_DISABLE_COPY(SKGOperationBoardWidgetQml)

    QAction *m_menuOpen;
    QAction *m_menuGroup;
    QAction *m_menuTransfer;
    QAction *m_menuTracked;
    QAction *m_menuSuboperation;

    SKGPeriodEdit *m_periodEdit1;
    SKGPeriodEdit *m_periodEdit2;
};

#endif // SKGOPERATIONBOARDWIDGETQML_H
