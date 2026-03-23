/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A QTabWidget with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtabwidget.h"

#include <klocalizedstring.h>

#include <qicon.h>
#include <qpushbutton.h>
#include <qstringlist.h>
#include <qtabbar.h>
#include <qtimer.h>

#include "skgmainpanel.h"
#include "skgtabpage.h"
#include "skgtraces.h"

SKGTabWidget::SKGTabWidget(QWidget *iParent)
    : QTabWidget(iParent)
{
    m_timerSave.setSingleShot(true);
    connect(&m_timerSave, &QTimer::timeout, this, &SKGTabWidget::onRefreshSaveIcon, Qt::QueuedConnection);
    connect(tabBar(), &QTabBar::tabMoved, this, &::SKGTabWidget::onMoveTab);
    if (iParent != nullptr) {
        connect(qobject_cast<SKGMainPanel *>(iParent), &SKGMainPanel::currentPageChanged, this, &SKGTabWidget::onCurrentChanged);
    }

    // Use new KDE for moving tabs.
    tabBar()->setMovable(true);

    m_timerSave.start(1000);
}

SKGTabWidget::~SKGTabWidget() = default;

void SKGTabWidget::onCurrentChanged()
{
    if ((currentWidget() != nullptr) && !m_tabIndexSaveButton.contains(currentWidget())) {
        // Build widgets
        auto kSave = new QPushButton(this);
        kSave->setIcon(SKGServices::fromTheme(QStringLiteral("document-save")));
        kSave->setToolTip(i18nc("Verb", "Save the state of this tab"));
        kSave->setFlat(true);
        kSave->setMaximumSize(QSize(16, 16));
        kSave->show();

        connect(kSave, &QPushButton::clicked, this, &SKGTabWidget::onSaveRequested);
        tabBar()->setTabButton(currentIndex(), QTabBar::LeftSide, kSave);

        m_tabIndexSaveButton.insert(currentWidget(), kSave);
    }

    // Rebuild indexes
    QHash<QWidget *, QPushButton *> TabIndexSaveButton2;

    int nb = count();
    for (int i = 0; i < nb; ++i) {
        QWidget *w = widget(i);
        QPushButton *save = m_tabIndexSaveButton.value(w);
        if ((w != nullptr) && (save != nullptr)) {
            save->setVisible(false);
            TabIndexSaveButton2[w] = save;
        }
    }

    m_tabIndexSaveButton = TabIndexSaveButton2;

    onRefreshSaveIcon();
}

void SKGTabWidget::removeTab(int index)
{
    m_tabIndexSaveButton.clear();
    QTabWidget::removeTab(index);
}

void SKGTabWidget::onMoveTab(int oldPos, int newPos)
{
    Q_UNUSED(oldPos)
    Q_UNUSED(newPos)
    m_tabIndexSaveButton.clear();
    onCurrentChanged();
}

void SKGTabWidget::onSaveRequested()
{
    auto *page = qobject_cast<SKGTabPage *>(currentWidget());
    if (page != nullptr) {
        page->overwrite(false);
        onRefreshSaveIcon();
    }
}

void SKGTabWidget::onRefreshSaveIcon()
{
    auto *page = qobject_cast<SKGTabPage *>(currentWidget());
    if (page != nullptr) {
        QPushButton *save = m_tabIndexSaveButton.value(page);
        if (save != nullptr) {
            if (page->isOverwriteNeeded()) {
                save->show();
                save->setEnabled(true);
                QStringList overlay;
                if (page->isPin()) {
                    overlay.push_back(QStringLiteral("document-encrypt"));
                }
                if (!page->getBookmarkID().isEmpty()) {
                    overlay.push_back(QStringLiteral("bookmarks"));
                }
                save->setIcon(SKGServices::fromTheme(QStringLiteral("document-save"), overlay));
            } else if (page->isPin()) {
                save->show();
                save->setEnabled(false);
                save->setIcon(SKGServices::fromTheme(QStringLiteral("document-encrypt")));
            } else {
                save->hide();
            }
        }

        m_timerSave.start(1000);
    }
}
