/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGREPORTPLUGINWIDGET_H
#define SKGREPORTPLUGINWIDGET_H
/** @file
 * This file is Skrooge plugin for bank management.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qdom.h>
#include <qstringlist.h>
#include <qtimer.h>

#include "skgtabpage.h"
#include "ui_skgreportpluginwidget_base.h"

class SKGDocumentBank;

/**
 * This file is Skrooge plugin to generate report
 */
class SKGReportPluginWidget : public SKGTabPage
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     * @param iMinimmumMode the panel will be displayed with minimum widgets
     */
    explicit SKGReportPluginWidget(QWidget *iParent, SKGDocumentBank *iDocument, bool iMinimmumMode = false);

    /**
     * Default Destructor
     */
    ~SKGReportPluginWidget() override;

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
     * Get the zoomable widget.
     * The default implementation returns the main widget.
     * @return the zoomable widget.
     */
    QWidget *zoomableWidget() override;

    /**
     * Get the printable widgets.
     * The default implementation returns the main widget.
     * @return the printable widgets.
     */
    QList<QWidget *> printableWidgets() override;

public Q_SLOTS:
    /**
     * Refresh the panel
     */
    void refresh();

    /**
     * data are modified
     * @param iTableName table name
     * @param iIdTransaction the id of the transaction for direct modifications of the table (update of modify objects is enough)
     *or 0 in case of modifications by impact (full table must be refreshed)
     */
    virtual void dataModified(const QString &iTableName = QString(), int iIdTransaction = 0);

private Q_SLOTS:
    void pageChanged();
    void onDoubleClick(int row, int column);
    void onOpen();
    void onOpenReport();
    void onSelectionChanged();
    void onOneLevelMore();
    void onOneLevelLess();
    void onAddLine();
    void onRemoveLine();
    void onBtnModeClicked(int mode);

    void setSettings();

private:
    Q_DISABLE_COPY(SKGReportPluginWidget)

    QString getConsolidatedWhereClause(QString *oWhereClausForPreviousData = nullptr, QString *oWhereClausForForecastData = nullptr);
    void getWhereClauseAndTitleForItem(int row, int column, QString &oWc, QString &oTitle);
    void getWhereClauseAndTitleForSelection(QString &oWc, QString &oTitle);
    QString getWhereClauseForProperty(const QString &iProperty) const;

    Ui::skgreportplugin_base ui{};
    QString m_previousParametersUsed;
    QStringList m_attsForColumns;
    QStringList m_attsForLines;
    QStringList m_attsForLinesAdded;
    QAction *m_openReportAction;
    QAction *m_openAction;
    QTimer m_timer;
    int m_mode;
    int m_nbLevelLines;
    int m_nbLevelColumns;
    bool m_refreshNeeded;
};

#endif // SKGREPORTPLUGINWIDGET_H
