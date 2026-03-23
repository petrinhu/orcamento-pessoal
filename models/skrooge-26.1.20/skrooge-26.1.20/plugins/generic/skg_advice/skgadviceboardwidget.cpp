/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is plugin for advice.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgadviceboardwidget.h"

#include <qdom.h>
#include <qlayoutitem.h>
#include <qmenu.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>

#include <klocalizedstring.h>

#include "skgdocument.h"
#include "skginterfaceplugin.h"
#include "skgmainpanel.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

SKGAdviceBoardWidget::SKGAdviceBoardWidget(QWidget *iParent, SKGDocument *iDocument)
    : SKGBoardWidget(iParent, iDocument, i18nc("Dashboard widget title", "Advice"))
    , m_maxAdvice(7)
    , m_refreshNeeded(true)
    , m_refresh(nullptr)
    , m_inapplyall(false)
{
    SKGTRACEINFUNC(10)

    // Create menu
    setContextMenuPolicy(Qt::ActionsContextMenu);

    auto g = new QWidget(this);
    m_layout = new QFormLayout(g);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setObjectName(QStringLiteral("Slayout"));
    m_layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_layout->setHorizontalSpacing(1);
    m_layout->setVerticalSpacing(1);
    setMainWidget(g);

    // menu
    auto menuResetAdvice = new QAction(SKGServices::fromTheme(QStringLiteral("edit-undo")), i18nc("Noun, a user action", "Activate all advice"), this);
    connect(menuResetAdvice, &QAction::triggered, this, &SKGAdviceBoardWidget::activateAllAdvice);
    addAction(menuResetAdvice);

    auto sep = new QAction(this);
    sep->setSeparator(true);
    addAction(sep);

    m_menuAuto = new QAction(i18nc("Noun, a type of refresh for advice", "Automatic refresh"), this);
    m_menuAuto->setCheckable(true);
    m_menuAuto->setChecked(true);
    connect(m_menuAuto, &QAction::triggered, this, &SKGAdviceBoardWidget::dataModifiedNotForce);
    addAction(m_menuAuto);

    // Refresh
    connect(getDocument(), &SKGDocument::transactionSuccessfullyEnded, this, &SKGAdviceBoardWidget::dataModifiedNotForce, Qt::QueuedConnection);
    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGAdviceBoardWidget::pageChanged, Qt::QueuedConnection);
    connect(
        this,
        &SKGAdviceBoardWidget::refreshNeeded,
        this,
        [this]() {
            this->dataModifiedNotForce();
        },
        Qt::QueuedConnection);
}

SKGAdviceBoardWidget::~SKGAdviceBoardWidget()
{
    SKGTRACEINFUNC(10)
    m_menuAuto = nullptr;
    m_refresh = nullptr;
}

QString SKGAdviceBoardWidget::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGBoardWidget::getState());
    QDomElement root = doc.documentElement();

    root.setAttribute(QStringLiteral("maxAdvice"), SKGServices::intToString(m_maxAdvice));
    root.setAttribute(QStringLiteral("automatic"), (m_menuAuto->isChecked() ? QStringLiteral("Y") : QStringLiteral("N")));
    return doc.toString();
}

void SKGAdviceBoardWidget::setState(const QString &iState)
{
    SKGBoardWidget::setState(iState);

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString maxAdviceS = root.attribute(QStringLiteral("maxAdvice"));
    if (maxAdviceS.isEmpty()) {
        maxAdviceS = '7';
    }
    m_maxAdvice = SKGServices::stringToInt(maxAdviceS);

    QString automatic = root.attribute(QStringLiteral("automatic"));
    if (automatic.isEmpty()) {
        automatic = 'Y';
    }

    if (m_menuAuto != nullptr) {
        bool previous = m_menuAuto->blockSignals(true);
        m_menuAuto->setChecked(automatic == QStringLiteral("Y"));
        m_menuAuto->blockSignals(previous);
    }

    dataModifiedForce();
}

void SKGAdviceBoardWidget::pageChanged()
{
    if (m_refreshNeeded) {
        dataModifiedNotForce();
    }
}

void SKGAdviceBoardWidget::dataModifiedNotForce()
{
    dataModified(false);
}

void SKGAdviceBoardWidget::dataModifiedForce()
{
    dataModified(true);
}

void SKGAdviceBoardWidget::dataModified(bool iForce)
{
    SKGTRACEINFUNC(10)
    SKGTabPage *page = SKGTabPage::parentTabPage(this);
    if (m_inapplyall || (!iForce && ((page != nullptr && page != SKGMainPanel::getMainPanel()->currentPage()) || !m_menuAuto->isChecked()))) {
        m_refreshNeeded = true;
        if (m_refresh != nullptr) {
            m_refresh->show();
        }
        return;
    }
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    m_refreshNeeded = false;

    // Remove all item of the layout
    while (m_layout->count() != 0) {
        QLayoutItem *child = m_layout->takeAt(0);
        if (child != nullptr) {
            QWidget *w = child->widget();
            delete w;
            delete child;
        }
    }

    // Get list of advice
    double elapse = SKGServices::getMicroTime();
    SKGAdviceList globalAdviceList = SKGMainPanel::getMainPanel()->getAdvice();
    elapse = SKGServices::getMicroTime() - elapse;

    // Get list of ignored advice
    QString currentMonth = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
    QStringList ignoredAdvice = getDocument()->getParameters(QStringLiteral("advice"), "t_value='I' OR t_value='I_" % currentMonth % u'\'');
    if (elapse > 3000 && m_menuAuto->isChecked() && !ignoredAdvice.contains(QStringLiteral("skgadviceboardwidget_verylong"))) {
        SKGAdvice ad;
        ad.setUUID(QStringLiteral("skgadviceboardwidget_verylong"));
        ad.setPriority(2);
        ad.setShortMessage(i18nc("Advice on making the best (short)", "Advice are very long to compute"));
        ad.setLongMessage(
            i18nc("Advice on making the best (long)", "To improve performances, you should switch the widget in 'Manual refresh' (see contextual menu)."));
        globalAdviceList.push_back(ad);
    }

    // Fill layout
    int nbDisplayedAdvice = 0;
    int nb = globalAdviceList.count();
    m_recommendedActions.clear();
    for (int i = 0; i < m_maxAdvice && i < nb; ++i) {
        // Get advice
        const SKGAdvice &ad = globalAdviceList.at(i);

        // Create icon
        QString iconName = (ad.getPriority() == -1      ? QStringLiteral("dialog-information")
                                : ad.getPriority() >= 8 ? QStringLiteral("security-low")
                                : ad.getPriority() <= 4 ? QStringLiteral("security-high")
                                                        : QStringLiteral("security-medium"));
        QString toolTipString = i18n("<p><b>Priority %1:</b>%2</p>", SKGServices::intToString(ad.getPriority()), ad.getLongMessage());

        // Add ignored action
        SKGAdvice::SKGAdviceActionList autoCorrections = ad.getAutoCorrections();
        {
            SKGAdvice::SKGAdviceAction dismiss;
            dismiss.Title = i18nc("Dismiss an advice provided", "Dismiss");
            dismiss.IconName = QStringLiteral("edit-delete");
            dismiss.IsRecommended = false;
            autoCorrections.push_back(dismiss);
        }
        {
            SKGAdvice::SKGAdviceAction dismiss;
            dismiss.Title = i18nc("Dismiss an advice provided", "Dismiss during current month");
            dismiss.IconName = QStringLiteral("edit-delete");
            dismiss.IsRecommended = false;
            autoCorrections.push_back(dismiss);
        }
        {
            SKGAdvice::SKGAdviceAction dismiss;
            dismiss.Title = i18nc("Dismiss an advice provided", "Dismiss this kind");
            dismiss.IconName = QStringLiteral("edit-delete");
            dismiss.IsRecommended = false;
            autoCorrections.push_back(dismiss);
        }
        {
            SKGAdvice::SKGAdviceAction dismiss;
            dismiss.Title = i18nc("Dismiss an advice provided", "Dismiss this kind during current month");
            dismiss.IconName = QStringLiteral("edit-delete");
            dismiss.IsRecommended = false;
            autoCorrections.push_back(dismiss);
        }

        int nbSolution = autoCorrections.count();

        // Build button
        QStringList overlays;
        overlays.push_back(nbSolution > 2 ? QStringLiteral("system-run") : QStringLiteral("edit-delete"));
        auto icon = new QToolButton(this);
        icon->setObjectName(ad.getUUID());
        icon->setIcon(SKGServices::fromTheme(iconName, overlays));
        icon->setIconSize(QSize(24, 24));
        icon->setMaximumSize(QSize(24, 24));
        icon->setCursor(Qt::PointingHandCursor);
        icon->setAutoRaise(true);

        auto menu = new QMenu(this);
        menu->setIcon(icon->icon());
        for (int k = 0; k < nbSolution; ++k) {
            SKGAdvice::SKGAdviceAction adviceAction = autoCorrections.at(k);
            QString actionText = adviceAction.Title;
            QAction *action = SKGMainPanel::getMainPanel()->getGlobalAction(QString(actionText).remove(QStringLiteral("skg://")), false);
            QAction *act;
            if (action != nullptr) {
                // This is an action
                act = menu->addAction(action->icon(),
                                      action->text(),
                                      SKGMainPanel::getMainPanel(),
                                      static_cast<bool (SKGMainPanel::*)()>(&SKGMainPanel::openPage));
                act->setData(actionText);
            } else {
                // This is a text
                act = menu->addAction(SKGServices::fromTheme(adviceAction.IconName.isEmpty() ? QStringLiteral("system-run") : adviceAction.IconName),
                                      autoCorrections.at(k).Title,
                                      this,
                                      &SKGAdviceBoardWidget::adviceClicked);
                if (act != nullptr) {
                    act->setProperty("id", ad.getUUID());
                    act->setProperty("solution", k < nbSolution - 4 ? k : k - nbSolution);
                }
            }

            if ((act != nullptr) && adviceAction.IsRecommended) {
                act->setToolTip(act->text());
                act->setText(act->text() % i18nc("To recommend this action", " (recommended)"));
                m_recommendedActions.append(act);
            }
        }

        icon->setMenu(menu);
        icon->setPopupMode(QToolButton::InstantPopup);

        icon->setToolTip(toolTipString);

        // Create text
        auto label = new QLabel(this);
        label->setText(ad.getShortMessage());
        label->setToolTip(toolTipString);

        // Add them
        m_layout->addRow(icon, label);

        ++nbDisplayedAdvice;
    }

    // Add apply all recommended actions
    QPushButton *apply = nullptr;
    int nb2 = m_recommendedActions.count();
    if (nb2 != 0) {
        apply = new QPushButton(this);
        apply->setIcon(SKGServices::fromTheme(QStringLiteral("games-solve")));
        apply->setIconSize(QSize(22, 22));
        apply->setMaximumSize(QSize(22, 22));
        apply->setCursor(Qt::PointingHandCursor);
        QString ToolTip;
        for (int i = 0; i < nb2; ++i) {
            if (i > 0) {
                ToolTip += '\n';
            }
            ToolTip += m_recommendedActions.at(i)->toolTip();
        }
        apply->setToolTip(ToolTip);
        connect(apply, &QPushButton::clicked, this, &SKGAdviceBoardWidget::applyRecommended, Qt::QueuedConnection);
    }

    // Add more
    if (nb > m_maxAdvice) {
        auto more = new QPushButton(this);
        more->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-down-double")));
        more->setIconSize(QSize(22, 22));
        more->setMaximumSize(QSize(22, 22));
        more->setCursor(Qt::PointingHandCursor);
        more->setToolTip(i18nc("Information message", "Display all advices"));
        connect(more, &QPushButton::clicked, this, &SKGAdviceBoardWidget::moreAdvice, Qt::QueuedConnection);

        if (apply != nullptr) {
            m_layout->addRow(more, apply);
            apply = nullptr;
        } else {
            m_layout->addRow(more, new QLabel(this));
        }
    } else if (nbDisplayedAdvice > 7) {
        // Add less
        auto less = new QPushButton(this);
        less->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-up-double")));
        less->setIconSize(QSize(22, 22));
        less->setMaximumSize(QSize(22, 22));
        less->setCursor(Qt::PointingHandCursor);
        less->setToolTip(i18nc("Information message", "Display less advices"));
        connect(less, &QPushButton::clicked, this, &SKGAdviceBoardWidget::lessAdvice, Qt::QueuedConnection);
        if (apply != nullptr) {
            m_layout->addRow(less, apply);
            apply = nullptr;
        } else {
            m_layout->addRow(less, new QLabel(this));
        }
    }

    if (apply != nullptr) {
        m_layout->addRow(apply, new QLabel(this));
    }

    // Add manual refresh button
    m_refresh = new QPushButton(this);
    m_refresh->setIcon(SKGServices::fromTheme(QStringLiteral("view-refresh")));
    m_refresh->setIconSize(QSize(22, 22));
    m_refresh->setMaximumSize(QSize(22, 22));
    m_refresh->setCursor(Qt::PointingHandCursor);
    m_refresh->setToolTip(i18nc("Information message", "Refresh advices"));
    m_refresh->hide();
    connect(m_refresh, &QPushButton::clicked, this, &SKGAdviceBoardWidget::dataModifiedForce, Qt::QueuedConnection);

    m_layout->addRow(m_refresh, new QLabel(this));

    QApplication::restoreOverrideCursor();
}

void SKGAdviceBoardWidget::moreAdvice()
{
    m_maxAdvice = 9999999;
    dataModifiedForce();
}

void SKGAdviceBoardWidget::lessAdvice()
{
    m_maxAdvice = 7;
    dataModifiedForce();
}

void SKGAdviceBoardWidget::applyRecommended()
{
    SKGError err;
    SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Apply all recommended corrections"), err)
    m_inapplyall = true;
    int nb = m_recommendedActions.count();
    for (int i = 0; i < nb; ++i) {
        m_recommendedActions.at(i)->trigger();
    }
    m_inapplyall = false;
}

void SKGAdviceBoardWidget::activateAllAdvice()
{
    SKGError err;
    {
        SKGBEGINTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Activate all advice"), err)
        err = getDocument()->executeSqliteOrder(QStringLiteral("DELETE FROM parameters WHERE t_uuid_parent='advice'"));
    }

    // status bar
    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Advice activated.")))
    else
    {
        err.addError(ERR_FAIL, i18nc("Error message", "Advice activation failed"));
    }

    SKGMainPanel::displayErrorMessage(err);
}

void SKGAdviceBoardWidget::adviceClicked()
{
    // Get advice identifier
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        QString uuid = act->property("id").toString();
        if (!uuid.isEmpty()) {
            // Get solution clicker
            int solution = sender()->property("solution").toInt();

            if (solution < 0) {
                // We have to ignore this advice
                SKGError err;
                {
                    SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Dismiss advice"), err)
                    QString currentMonth = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));

                    // Create dismiss
                    if (solution == -1 || solution == -2) {
                        uuid = SKGServices::splitCSVLine(uuid, '|').at(0);
                    }
                    IFOKDO(err,
                           getDocument()->setParameter(uuid,
                                                       solution == -2 || solution == -4 ? QStringLiteral("I") : QString("I_" % currentMonth),
                                                       QVariant(),
                                                       QStringLiteral("advice")))

                    // Delete useless dismiss
                    IFOKDO(err,
                           getDocument()->executeSqliteOrder("DELETE FROM parameters WHERE t_uuid_parent='advice' AND t_value like 'I_ % ' AND t_value!='I_"
                                                             % currentMonth % QLatin1Char('\'')))
                }

                // status bar
                IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Advice dismissed.")))
                else
                {
                    err.addError(ERR_FAIL, i18nc("Error message", "Advice dismiss failed"));
                }
            } else {
                // Get last transaction id
                int previous = getDocument()->getTransactionToProcess(SKGDocument::UNDO);

                // Execute the advice correction on all plugin
                QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
                int index = 0;
                while (index >= 0) {
                    SKGInterfacePlugin *plugin = SKGMainPanel::getMainPanel()->getPluginByIndex(index);
                    if (plugin != nullptr) {
                        SKGError err = plugin->executeAdviceCorrection(uuid, solution);
                        if (!err || err.getReturnCode() != ERR_NOTIMPL) {
                            // The correction has been done or failed. This is the end.
                            index = -2;
                        }
                    } else {
                        index = -2;
                    }
                    ++index;
                }

                // Get last transaction id
                int next = getDocument()->getTransactionToProcess(SKGDocument::UNDO);

                // If this is the same transaction, it means that an action has been done outside the document ==> a refresh is needed
                if (next == previous) {
                    Q_EMIT refreshNeeded();
                }

                QApplication::restoreOverrideCursor();
            }
        }
    }
}
