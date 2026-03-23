/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGHTMLBOARDWIDGET_H
#define SKGHTMLBOARDWIDGET_H
/** @file
 * This file is a generic Skrooge plugin for html/qml reports.
 * @see SKGReport
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgbasegui_export.h"
#include "skgboardwidget.h"
#include "skgsimpleperiodedit.h"

class QLabel;
class QQuickWidget;
class KLineEdit;
class SKGReport;

/**
 * This file is a generic Skrooge plugin for html/qml reports
 */
class SKGBASEGUI_EXPORT SKGHtmlBoardWidget : public SKGBoardWidget
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     * @param iDocument the document
     * @param iTitle the title
     * @param iTemplate the html template (.html) or the qml file (.qml)
     * @param iTablesRefreshing the list of table refreshing the report when the table is updated. (empty means all)
     * @param iOptions to enable options in menu
     * @param iEnableFilter to enable the filter
     */
    explicit SKGHtmlBoardWidget(QWidget *iParent,
                                SKGDocument *iDocument,
                                const QString &iTitle,
                                const QString &iTemplate,
                                QStringList iTablesRefreshing = QStringList(),
                                SKGSimplePeriodEdit::Modes iOptions = SKGSimplePeriodEdit::NONE,
                                const QStringList &iAttributesForFilter = QStringList());

    /**
     * Default Destructor
     */
    ~SKGHtmlBoardWidget() override;

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
     * Set the font point size
     * @param iPointSize font point size
     */
    virtual void setPointSize(int iPointSize) const;

protected Q_SLOTS:
    void pageChanged();
    void onTextFilterChanged(const QString &iFilter);
    virtual void dataModified(const QString &iTableName = QString(), int iIdTransaction = 0);

protected:
    QQuickWidget *m_Quick;
    SKGReport *m_Report;

private:
    Q_DISABLE_COPY(SKGHtmlBoardWidget)

    QLabel *m_Text;
    QString m_Template;
    QStringList m_TablesRefreshing;
    bool m_refreshNeeded;

    SKGSimplePeriodEdit *m_period;
    KLineEdit *m_filter;
    QStringList m_attributes;
};

#endif // SKGHTMLBOARDWIDGET_H
