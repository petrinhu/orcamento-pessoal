/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGREPORTBOARDWIDGET_H
#define SKGREPORTBOARDWIDGET_H
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgboardwidget.h"

class SKGReportPluginWidget;
class SKGDocumentBank;

class QAction;

/**
 * This file is Skrooge plugin for bank management
 */
class SKGReportBoardWidget : public SKGBoardWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     */
    explicit SKGReportBoardWidget(QWidget *iParent, SKGDocumentBank *iDocument);

    /**
     * Default Destructor
     */
    ~SKGReportBoardWidget() override;

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
    void dataModified(const QString &iTableName, int iIdTransaction);
    void onOpen();

private:
    Q_DISABLE_COPY(SKGReportBoardWidget)
    SKGReportPluginWidget *m_graph;
};

#endif // SKGREPORTBOARDWIDGET_H
