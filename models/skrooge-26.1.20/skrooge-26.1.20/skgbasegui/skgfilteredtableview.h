/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGFILTEREDTABLEVIEW_H
#define SKGFILTEREDTABLEVIEW_H
/** @file
 * A filtered SKGTableView.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qtimer.h>
#include <qwidget.h>

#include "skgbasegui_export.h"
#include "ui_skgfilteredtableview.h"

class SKGShow;
class SKGObjectModelBase;

/**
 * This file is a filtered SKGTableView
 */
class SKGBASEGUI_EXPORT SKGFilteredTableView : public QWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGFilteredTableView(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGFilteredTableView() override;

    /**
     * Get the current state
     * @return a string containing all activated item identifiers (separated by ;)
     */
    virtual QString getState();

    /**
     * Set the current state
     * @param iState a string containing all activated item identifiers (separated by ;)
     */
    virtual void setState(const QString &iState);

    /**
     * @brief Get show widget
     *
     * @return SKGShow
     **/
    virtual SKGShow *getShowWidget() const;

    /**
     * @brief Get table or tree view
     *
     * @return SKGTreeView
     **/
    virtual SKGTreeView *getView() const;

    /**
     * @brief Get the search field
     *
     * @return QLineEdit
     **/
    virtual QLineEdit *getSearchField() const;

    /**
     * @brief Set model
     *
     * @param iModel the model
     * @return void
     **/
    virtual void setModel(SKGObjectModelBase *iModel);

public Q_SLOTS:
    /**
     * @brief Set the filter. This filter disable the "show" menu
     *
     * @param iIcon the icon
     * @param iText the text to display
     * @param iWhereClause the where clause
     * @return void
     **/
    virtual void setFilter(const QIcon &iIcon, const QString &iText, const QString &iWhereClause);

    /**
     * @brief Reset the filter
     * @return void
     **/
    virtual void resetFilter();

private Q_SLOTS:
    void pageChanged();
    void onFilterChanged();
    void onTextFilterChanged();
    void dataModified(const QString &iTableName, int iIdTransaction);

private:
    Q_DISABLE_COPY(SKGFilteredTableView)

    Ui::skgfilteredtableview_base ui{};
    SKGObjectModelBase *m_objectModel;
    bool m_refreshNeeded;
    QTimer m_timer;
};

#endif // SKGFILTEREDTABLEVIEW_H
