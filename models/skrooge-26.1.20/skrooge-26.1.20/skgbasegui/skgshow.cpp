/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A widget to select what to show.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgshow.h"

#include <klocalizedstring.h>
#include <qactiongroup.h>
#include <qdom.h>
#include <qicon.h>
#include <qwidgetaction.h>

#include "skgperiodedit.h"
#include "skgservices.h"
#include <utility>

SKGShow::SKGShow(QWidget *iParent)
    : QToolButton(iParent)
    , m_mode(OR)
    , m_inTrigger(false)
    , m_displayTitle(true)
{
    setPopupMode(QToolButton::InstantPopup);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setAutoRaise(true);

    m_menu = new SKGMenu(this);
    setMenu(m_menu);

    // Time to Q_EMIT stateChanged
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGShow::stateChanged, Qt::QueuedConnection);

    hide();
}

SKGShow::~SKGShow()
{
    m_menu = nullptr;
}

SKGShow::OperatorMode SKGShow::getMode()
{
    return m_mode;
}

void SKGShow::setMode(SKGShow::OperatorMode iMode)
{
    if (m_mode != iMode) {
        m_mode = iMode;
        Q_EMIT modified();
    }
}

QString SKGShow::getState()
{
    QStringList itemsChecked;
    if (m_menu != nullptr) {
        QList<QAction *> actionsList = m_menu->actions();
        int nb = actionsList.count();
        itemsChecked.reserve(nb);
        for (int i = 0; i < nb; ++i) {
            QAction *act = actionsList.at(i);
            if (act != nullptr) {
                auto *wact = qobject_cast<QWidgetAction *>(act);
                if (wact != nullptr) {
                    auto *pedit = qobject_cast<SKGPeriodEdit *>(wact->defaultWidget());
                    itemsChecked.push_back(act->data().toString() % ":" % pedit->getState());
                } else {
                    if (act->isChecked()) {
                        itemsChecked.push_back(act->data().toString());
                    }
                }
            }
        }
    }
    return SKGServices::stringsToCsv(itemsChecked);
}

void SKGShow::setDefaultState(const QString &iState)
{
    m_defaultState = iState;
    setState(m_defaultState);
}

void SKGShow::setState(const QString &iState)
{
    if (m_menu != nullptr) {
        QStringList itemsChecked = SKGServices::splitCSVLine(iState.isEmpty() ? m_defaultState : iState);

        int nb = m_actions.count();
        for (int i = 0; i < nb; ++i) {
            QAction *act = m_actions.at(i);
            if (act != nullptr) {
                QString identifier = m_actions.at(i)->data().toString();
                auto *wact = qobject_cast<QWidgetAction *>(act);
                if (wact != nullptr) {
                    auto *pedit = qobject_cast<SKGPeriodEdit *>(wact->defaultWidget());
                    for (const auto &item : std::as_const(itemsChecked)) {
                        if (item.startsWith(identifier % ":")) {
                            pedit->setState(item.right(item.length() - identifier.length() - 1));
                            break;
                        }
                    }
                } else {
                    act->setChecked(itemsChecked.contains(identifier));
                }
            }
        }

        // Change tooltip
        setToolTip(getTitle());

        // Emit event
        Q_EMIT stateChanged();
    }
}

QString SKGShow::getWhereClause() const
{
    QString wc;
    if (m_menu != nullptr) {
        QList<QAction *> actionsList = m_menu->actions();
        int nb = actionsList.count();
        bool noCheck = true;
        for (int i = 0; i < nb; ++i) {
            QAction *act = actionsList.at(i);

            if (act != nullptr) {
                auto *wact = qobject_cast<QWidgetAction *>(act);
                if (wact != nullptr) {
                    auto *pedit = qobject_cast<SKGPeriodEdit *>(wact->defaultWidget());
                    if (!wc.isEmpty()) {
                        wc += (m_mode == OR ? QStringLiteral(" OR ") : QStringLiteral(" AND "));
                    }
                    wc += u'(' % pedit->getWhereClause() % u')';
                    noCheck = false;

                } else {
                    if (act->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += (m_mode == OR ? QStringLiteral(" OR ") : QStringLiteral(" AND "));
                        }
                        wc += u'(' % m_whereclause.value(act) % u')';
                        noCheck = false;

                        if (m_whereclause.value(act).isEmpty()) {
                            wc = QString();
                            break;
                        }
                    }
                }
            }
        }
        if ((nb != 0) && noCheck) {
            wc = QStringLiteral("1=0");
        }
    }
    return wc;
}

QString SKGShow::getTitle() const
{
    QString wc;
    if (m_menu != nullptr) {
        int nb = m_actions.count();
        for (int i = 0; i < nb; ++i) {
            QAction *act = m_actions.at(i);
            if (act != nullptr) {
                auto *wact = qobject_cast<QWidgetAction *>(act);
                if (wact != nullptr) {
                    auto *pedit = qobject_cast<SKGPeriodEdit *>(wact->defaultWidget());
                    if (!wc.isEmpty()) {
                        wc += (m_mode == OR ? QStringLiteral(" + ") : QStringLiteral(" , "));
                    }
                    wc += pedit->text();
                } else {
                    if (act->isChecked()) {
                        if (!wc.isEmpty()) {
                            wc += (m_mode == OR ? QStringLiteral(" + ") : QStringLiteral(" , "));
                        }
                        wc += act->toolTip();
                    }
                }
            }
        }
    }
    return wc;
}

void SKGShow::clear()
{
    m_check_to_check.clear();
    m_uncheck_to_check.clear();
    m_check_to_uncheck.clear();
    m_uncheck_to_uncheck.clear();
    m_actions.clear();
    m_icons.clear();
    m_whereclause.clear();
    m_defaultState.clear();
    m_menu->clear();
}

int SKGShow::count()
{
    return m_check_to_check.count();
}

int SKGShow::addGroupedItem(const QString &iIdentifier,
                            const QString &iText,
                            const QString &iIcon,
                            const QString &iWhereClose,
                            const QString &iGroup,
                            const QKeySequence &iShortcut)
{
    if (m_menu != nullptr) {
        QActionGroup *groupAction = m_groups.value(iGroup);
        if (groupAction == nullptr) {
            groupAction = new QActionGroup(this);
            m_groups[iGroup] = groupAction;
        }

        QString name = iText;
        name = name.replace('&', QStringLiteral("&&"));
        QAction *act = m_menu->addAction(name);
        if (act != nullptr) {
            act->setToolTip(name);
            act->setIcon(SKGServices::fromTheme(iIcon));
            act->setData(iIdentifier);
            act->setCheckable(true);
            if (!iShortcut.isEmpty()) {
                act->setShortcuts(QList<QKeySequence>() << iShortcut << QKeySequence::fromString("Ctrl+Alt+" % iShortcut.toString()));
            }

            m_check_to_check[act] = QString();
            m_check_to_uncheck[act] = QString();
            m_uncheck_to_check[act] = QString();
            m_uncheck_to_uncheck[act] = QString();
            m_actions.push_back(act);
            m_icons.push_back(iIcon);

            m_whereclause[act] = iWhereClose;

            connect(act, &QAction::toggled, this, &SKGShow::trigger);

            groupAction->addAction(act);
        }

        show();

        return (m_actions.count() - 1);
    }
    return -1;
}

int SKGShow::addPeriodItem(const QString &iIdentifier)
{
    if (m_menu != nullptr) {
        auto m_periodEdit1 = new SKGPeriodEdit(this);

        // Set default
        QDomDocument doc(QStringLiteral("SKGML"));
        QDomElement root = doc.createElement(QStringLiteral("parameters"));
        doc.appendChild(root);
        root.setAttribute(QStringLiteral("period"), SKGServices::intToString(static_cast<int>(SKGPeriodEdit::ALL)));
        m_periodEdit1->setState(doc.toString());

        // Add widget in menu
        auto act = new QWidgetAction(this);
        if (act != nullptr) {
            act->setData(iIdentifier);
            act->setDefaultWidget(m_periodEdit1);

            m_check_to_check[act] = QString();
            m_check_to_uncheck[act] = QString();
            m_uncheck_to_check[act] = QString();
            m_uncheck_to_uncheck[act] = QString();
            m_actions.push_back(act);
            m_icons.push_back(QString());

            m_whereclause[act] = QString();

            connect(m_periodEdit1, &SKGPeriodEdit::changed, this, &SKGShow::triggerRefreshOnly);

            m_menu->addAction(act);
        }

        show();

        return (m_actions.count() - 1);
    }
    return -1;
}

int SKGShow::addItem(const QString &iIdentifier,
                     const QString &iText,
                     const QString &iIcon,
                     const QString &iWhereClose,
                     const QString &iListIdToCheckWhenChecked,
                     const QString &iListIdToUncheckWhenChecked,
                     const QString &iListIdToCheckWhenUnchecked,
                     const QString &iListIdToUncheckWhenUnchecked,
                     const QKeySequence &iShortcut)
{
    if (m_menu != nullptr) {
        QString name = iText;
        name = name.replace('&', QStringLiteral("&&"));
        QAction *act = m_menu->addAction(name);
        if (act != nullptr) {
            act->setToolTip(name);
            act->setIcon(SKGServices::fromTheme(iIcon));
            act->setData(iIdentifier);
            act->setCheckable(true);
            if (!iShortcut.isEmpty()) {
                act->setShortcuts(QList<QKeySequence>() << iShortcut << QKeySequence::fromString("Ctrl+Alt+" % iShortcut.toString()));
            }

            m_check_to_check[act] = iListIdToCheckWhenChecked;
            m_check_to_uncheck[act] = iListIdToUncheckWhenChecked;
            m_uncheck_to_check[act] = iListIdToCheckWhenUnchecked;
            m_uncheck_to_uncheck[act] = iListIdToUncheckWhenUnchecked;
            m_actions.push_back(act);
            m_icons.push_back(iIcon);

            m_whereclause[act] = iWhereClose;

            connect(act, &QAction::toggled, this, &SKGShow::trigger);
        }

        show();

        return (m_actions.count() - 1);
    }
    return -1;
}

void SKGShow::setListIdToCheckWhenChecked(int iIndex, const QString &iIds)
{
    m_check_to_check[m_actions.at(iIndex)] = iIds;
}

void SKGShow::setListIdToCheckWhenUnchecked(int iIndex, const QString &iIds)
{
    m_uncheck_to_check[m_actions.at(iIndex)] = iIds;
}

void SKGShow::setListIdToUncheckWhenChecked(int iIndex, const QString &iIds)
{
    m_check_to_uncheck[m_actions.at(iIndex)] = iIds;
}

void SKGShow::setListIdToUncheckWhenUnchecked(int iIndex, const QString &iIds)
{
    m_uncheck_to_uncheck[m_actions.at(iIndex)] = iIds;
}

void SKGShow::addSeparator()
{
    if (m_menu != nullptr) {
        m_menu->addSeparator();
    }
}

void SKGShow::trigger()
{
    auto *act = qobject_cast<QAction *>(sender());
    if ((act != nullptr) && !m_inTrigger) {
        m_inTrigger = true;

        // Apply rules
        if (act->isChecked()) {
            {
                // Check items
                QStringList items = SKGServices::splitCSVLine(m_check_to_check.value(act));
                int nb = items.count();
                for (int i = 0; i < nb; ++i) {
                    QAction *act2 = getAction(items.at(i));
                    if ((act2 != nullptr) && act2 != act) {
                        act2->setChecked(true);
                    }
                }
            }

            {
                // Uncheck items
                QStringList items = SKGServices::splitCSVLine(m_check_to_uncheck.value(act));
                int nb = items.count();
                for (int i = 0; i < nb; ++i) {
                    QAction *act2 = getAction(items.at(i));
                    if ((act2 != nullptr) && act2 != act) {
                        act2->setChecked(false);
                    }
                }
            }
        } else {
            {
                // Check items
                QStringList items = SKGServices::splitCSVLine(m_uncheck_to_check.value(act));
                int nb = items.count();
                for (int i = 0; i < nb; ++i) {
                    QAction *act2 = getAction(items.at(i));
                    if ((act2 != nullptr) && act2 != act) {
                        act2->setChecked(true);
                    }
                }
            }

            {
                // Uncheck items
                QStringList items = SKGServices::splitCSVLine(m_uncheck_to_uncheck.value(act));
                int nb = items.count();
                for (int i = 0; i < nb; ++i) {
                    QAction *act2 = getAction(items.at(i));
                    if ((act2 != nullptr) && act2 != act) {
                        act2->setChecked(false);
                    }
                }
            }
        }

        // Change tooltip
        setToolTip(getTitle());

        // Change icon
        QStringList icons;
        QString mainIcon;
        if (m_menu != nullptr) {
            int nb = m_actions.count();
            icons.reserve(nb);
            for (int i = 0; i < nb; ++i) {
                QAction *act2 = m_actions.at(i);
                if ((act2 != nullptr) && act2->isChecked()) {
                    if (!m_icons.at(i).isEmpty()) {
                        if (mainIcon.isEmpty()) {
                            mainIcon = m_icons.at(i);
                        } else {
                            icons.push_back(m_icons.at(i));
                        }
                    } else {
                        if (mainIcon.isEmpty()) {
                            mainIcon = QStringLiteral("show-menu");
                        }
                    }
                }
            }
        }
        if (mainIcon.isEmpty()) {
            mainIcon = QStringLiteral("show-menu");
        }
        setIcon(SKGServices::fromTheme(mainIcon, icons));

        triggerRefreshOnly();

        m_inTrigger = false;
    }
}

void SKGShow::triggerRefreshOnly()
{
    // Emit event
    m_timer.start(300);

    // Change title
    refreshTitle();
}

bool SKGShow::getDisplayTitle()
{
    return m_displayTitle;
}

void SKGShow::setDisplayTitle(bool iDisplay)
{
    if (m_displayTitle != iDisplay) {
        m_displayTitle = iDisplay;
        refreshTitle();
        Q_EMIT modified();
    }
}

void SKGShow::refreshTitle()
{
    if (m_displayTitle) {
        setText(i18n("Show: %1", getTitle()));
    } else {
        setText(i18n("Show"));
    }
}

QAction *SKGShow::getAction(const QString &iIdentifier) const
{
    QAction *output = nullptr;
    if (m_menu != nullptr) {
        QList<QAction *> actionsList = m_menu->actions();
        int nb = actionsList.count();
        for (int i = 0; (output == nullptr) && i < nb; ++i) {
            QAction *act = actionsList.at(i);
            if ((act != nullptr) && act->data().toString() == iIdentifier) {
                output = act;
            }
        }
    }
    return output;
}
