/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSHOW_H
#define SKGSHOW_H
/** @file
 * A widget to select what to show.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qmap.h>
#include <qmenu.h>
#include <qtimer.h>
#include <qtoolbutton.h>

#include "skgbasegui_export.h"

class QAction;
class QActionGroup;

class SKGBASEGUI_EXPORT SKGMenu : public QMenu
{
    Q_OBJECT
public:
    explicit SKGMenu(QWidget *parent = nullptr)
        : QMenu(parent)
    {
    }

protected:
    bool focusNextPrevChild(bool next) override
    {
        return QWidget::focusNextPrevChild(next);
    }
};

/**
 * This file is a widget to select what to show
 */
class SKGBASEGUI_EXPORT SKGShow : public QToolButton
{
    Q_OBJECT

    /**
     * Mode
     */
    Q_PROPERTY(OperatorMode mode READ getMode WRITE setMode NOTIFY modified)

    /**
     * Display Title
     */
    Q_PROPERTY(bool displayTitle READ getDisplayTitle WRITE setDisplayTitle NOTIFY modified)
public:
    /**
     * This enumerate defines type of operator
     */
    enum OperatorMode {
        AND, /**< AND*/
        OR /**< OR*/
    };

    /**
     * This enumerate defines type of operator
     */
    Q_ENUM(OperatorMode)

    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGShow(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGShow() override;

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
     * Get the current mode
     * @return the mode
     */
    virtual OperatorMode getMode();

    /**
     * Set the current mode
     * @param iMode the mode
     */
    virtual void setMode(OperatorMode iMode);

    /**
     * Set the default state
     * @param iState a string containing all activated item identifiers (separated by ;)
     */
    virtual void setDefaultState(const QString &iState);

    /**
     * Get the current display mode
     * @return true of false
     */
    virtual bool getDisplayTitle();

    /**
     * Set the current display mode
     * @param iDisplay true if you want to see the selected filter of false
     */
    virtual void setDisplayTitle(bool iDisplay);

    /**
     * Get the current where clause
     * @return a where clause string
     */
    virtual QString getWhereClause() const;

    /**
     * Remove all items
     */
    virtual void clear();

    /**
     * @brief Get the number of items
     *
     * @return the number of items
     **/
    virtual int count();

    /**
     * @brief Get the action for an identifier
     *
     * @param iIdentifier unique identifier of the item
     * @return the action
     **/
    virtual QAction *getAction(const QString &iIdentifier) const;

    /**
     * @brief Add an item to the menu
     *
     * @param iIdentifier unique identifier of the item
     * @param iText text
     * @param iIcon icon Defaults to "".
     * @param iWhereClose the where clause Defaults to "".
     * @param iListIdToCheckWhenChecked list of item identifiers (separated by ;) to check when checked Defaults to "".
     * @param iListIdToUncheckWhenChecked list of item identifiers (separated by ;) to uncheck when unchecked Defaults to "".
     * @param iListIdToCheckWhenUnchecked list of item identifiers (separated by ;) to check when checked Defaults to "".
     * @param iListIdToUncheckWhenUnchecked list of item identifiers (separated by ;) to uncheck when unchecked Defaults to "".
     * @param iShortcut the associated shortcut.
     * @return the index of the new item
     **/
    virtual int addItem(const QString &iIdentifier,
                        const QString &iText,
                        const QString &iIcon = QString(),
                        const QString &iWhereClose = QString(),
                        const QString &iListIdToCheckWhenChecked = QString(),
                        const QString &iListIdToUncheckWhenChecked = QString(),
                        const QString &iListIdToCheckWhenUnchecked = QString(),
                        const QString &iListIdToUncheckWhenUnchecked = QString(),
                        const QKeySequence &iShortcut = QKeySequence());

    /**
     * @brief Add a period item to the menu
     *
     * @param iIdentifier unique identifier of the item
     * @return the index of the new item
     **/
    virtual int addPeriodItem(const QString &iIdentifier);

    /**
     * @brief Add an item to the menu
     *
     * @param iIdentifier unique identifier of the item
     * @param iText text
     * @param iIcon icon Defaults to "".
     * @param iWhereClose icon Defaults to "".
     * @param iGroup the group of actions.
     * @param iShortcut the associated shortcut.
     * @return the index of the new item
     **/
    virtual int addGroupedItem(const QString &iIdentifier,
                               const QString &iText,
                               const QString &iIcon = QString(),
                               const QString &iWhereClose = QString(),
                               const QString &iGroup = QString(),
                               const QKeySequence &iShortcut = QKeySequence());

    /**
     * @brief Set the list of items to check when iIndex is checked
     *
     * @param iIndex index of the item (@see addItem)
     * @param iIds list of item identifiers (separated by ;)
     **/
    virtual void setListIdToCheckWhenChecked(int iIndex, const QString &iIds);

    /**
     * @brief Set the list of items to uncheck when iIndex is checked
     *
     * @param iIndex index of the item (@see addItem)
     * @param iIds list of item identifiers (separated by ;)
     **/
    virtual void setListIdToUncheckWhenChecked(int iIndex, const QString &iIds);

    /**
     * @brief Set the list of items to check when iIndex is unchecked
     *
     * @param iIndex index of the item (@see addItem)
     * @param iIds list of item identifiers (separated by ;)
     **/
    virtual void setListIdToCheckWhenUnchecked(int iIndex, const QString &iIds);

    /**
     * @brief Set the list of items to uncheck when iIndex is unchecked
     *
     * @param iIndex index of the item (@see addItem)
     * @param iIds list of item identifiers (separated by ;)
     **/
    virtual void setListIdToUncheckWhenUnchecked(int iIndex, const QString &iIds);

    /**
     * @brief Add a separator
     *
     * @return void
     **/
    virtual void addSeparator();

Q_SIGNALS:
    /**
     * @brief Emitted when an item is changed
     *
     * @return void
     **/
    void stateChanged();

    /**
     * This signal is launched when the error is modified
     */
    void modified();

private Q_SLOTS:
    /**
     * @brief trigger
     **/
    void trigger();

    /**
     * @brief trigger
     **/
    void triggerRefreshOnly();

private:
    Q_DISABLE_COPY(SKGShow)

    QString getTitle() const;
    void refreshTitle();

    SKGMenu *m_menu;
    QTimer m_timer;
    QString m_defaultState;
    OperatorMode m_mode;
    bool m_inTrigger;
    bool m_displayTitle;

    QList<QAction *> m_actions;
    QStringList m_icons;
    QMap<QAction *, QString> m_check_to_check;
    QMap<QAction *, QString> m_uncheck_to_check;
    QMap<QAction *, QString> m_check_to_uncheck;
    QMap<QAction *, QString> m_uncheck_to_uncheck;
    QMap<QAction *, QString> m_whereclause;
    QMap<QString, QActionGroup *> m_groups;
};

#endif // SKGSHOW_H
