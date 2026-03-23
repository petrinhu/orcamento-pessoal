/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a class managing widget.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtabpage.h"

#include <klocalizedstring.h>
#include <kmessagebox.h>

#include <qwidget.h>
#ifdef SKG_WEBENGINE
#include <qwebengineview.h>
#endif
#if !defined(SKG_WEBENGINE)
#include <qlabel.h>
#endif
#include <qmath.h>

#include <cmath>

#include "skgdocument.h"
#include "skghtmlboardwidget.h"
#include "skgmainpanel.h"
#include "skgnodeobject.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"
#include "skgtreeview.h"
#include <utility>

SKGTabPage::SKGTabPage(QWidget *iParent, SKGDocument *iDocument)
    : SKGWidget(iParent, iDocument)
    , m_pin(false)
{
    SKGTRACEINFUNC(5)

    // Save original size
    m_fontOriginalPointSize = this->font().pointSize(); // Use this instead of zoomableWidget()
}

SKGTabPage::~SKGTabPage()
{
    SKGTRACEINFUNC(5)
}

bool SKGTabPage::close(bool iForce)
{
    SKGTRACEINFUNC(5)
    int conf = KMessageBox::PrimaryAction;
    if (!iForce && isPin()) {
        QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
        conf = KMessageBox::questionTwoActions(this,
                                               i18nc("Question", "Do you really want to close this pinned page?"),
                                               i18nc("Question", "Pinned page"),
                                               KStandardGuiItem::apply(),
                                               KStandardGuiItem::cancel(),
                                               QStringLiteral("closepinnedpage"));
        QApplication::restoreOverrideCursor();
    }
    overwrite();
    if (conf == KMessageBox::SecondaryAction) {
        return false;
    }
    return QWidget::close();
}

void SKGTabPage::setBookmarkID(const QString &iId)
{
    m_bookmarkID = iId;
}

QString SKGTabPage::getBookmarkID()
{
    return m_bookmarkID;
}
SKGTabPage::SKGPageHistoryItemList SKGTabPage::getPreviousPages()
{
    return m_previousPages;
}

void SKGTabPage::setPreviousPages(const SKGTabPage::SKGPageHistoryItemList &iPages)
{
    m_previousPages = iPages;
}

SKGTabPage::SKGPageHistoryItemList SKGTabPage::getNextPages()
{
    return m_nextPages;
}

void SKGTabPage::setNextPages(const SKGTabPage::SKGPageHistoryItemList &iPages)
{
    m_nextPages = iPages;
}

bool SKGTabPage::isOverwriteNeeded()
{
    // Is this widget linked to a bookmark ?
    if (!m_bookmarkID.isEmpty()) {
        // Yes. Is state modified ?
        SKGNodeObject node(getDocument(), SKGServices::stringToInt(m_bookmarkID));
        if (node.exist()) {
            QStringList d = SKGServices::splitCSVLine(node.getData());
            if (d.count() > 2) {
                QString currentState = getState().trimmed();
                QString oldState = d[2].trimmed();
                currentState.remove('\n');
                oldState.remove('\n');
                SKGTRACEL(20) << "oldState      =[" << oldState << ']' << Qt::endl;
                SKGTRACEL(20) << "currentState  =[" << currentState << ']' << Qt::endl;
                SKGTRACEL(20) << "Bookmark diff =" << (currentState != oldState ? "TRUE" : "FALSE") << Qt::endl;
                return (currentState != oldState);
            }
        }
    } else {
        // No. It is a page opened from context or from another page
        QString name = getDefaultStateAttribute();
        if (!name.isEmpty()) {
            QString currentState = getState().trimmed();
            QString oldState = getDocument()->getParameter(name);
            currentState.remove('\n');
            oldState.remove('\n');
            SKGTRACEL(20) << "oldState      =[" << oldState << ']' << Qt::endl;
            SKGTRACEL(20) << "currentState  =[" << currentState << ']' << Qt::endl;
            SKGTRACEL(20) << "Page diff =" << (currentState != oldState ? "TRUE" : "FALSE") << Qt::endl;
            return (currentState != oldState);
        }
    }
    return false;
}

void SKGTabPage::overwrite(bool iUserConfirmation)
{
    SKGTRACEINFUNC(10)
    // Is this widget linked to a bookmark ?
    if (!m_bookmarkID.isEmpty()) {
        // Yes. Is state modified ?
        SKGNodeObject node(getDocument(), SKGServices::stringToInt(m_bookmarkID));
        if (node.exist()) {
            QStringList d = SKGServices::splitCSVLine(node.getData());
            QString fullname = node.getFullName();
            if (d.count() > 2) {
                QString currentState = getState().trimmed();
                QString oldState = d[2].trimmed();
                currentState.remove('\n');
                oldState.remove('\n');
                SKGTRACEL(20) << "oldState      =[" << oldState << ']' << Qt::endl;
                SKGTRACEL(20) << "currentState  =[" << currentState << ']' << Qt::endl;
                if (currentState != oldState) {
                    QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                    int conf = KMessageBox::PrimaryAction;
                    if (iUserConfirmation && !oldState.isEmpty()) {
                        conf = KMessageBox::questionTwoActions(
                            this,
                            i18nc("Question", "Bookmark '%1' has been modified. Do you want to update it with the current state?", fullname),
                            i18nc("Question", "Bookmark has been modified"),
                            KStandardGuiItem::apply(),
                            KStandardGuiItem::cancel(),
                            QStringLiteral("updateBookmarkOnClose"));
                    }
                    QApplication::restoreOverrideCursor();
                    if (conf == KMessageBox::PrimaryAction) {
                        SKGError err;
                        {
                            SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Bookmark update '%1'", fullname), err)
                            d[2] = std::move(currentState);
                            IFOKDO(err, node.setData(SKGServices::stringsToCsv(d)))
                            IFOKDO(err, node.save())
                        }
                        IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Bookmark updated")))
                        SKGMainPanel::displayErrorMessage(err);
                    }
                }
            }
        }
    } else {
        // No. It is a page opened from context or from another page
        QString name = getDefaultStateAttribute();
        if (!name.isEmpty()) {
            QString currentState = getState().trimmed();
            QString oldState = getDocument()->getParameter(name);
            SKGTRACEL(20) << "oldState      =[" << oldState << ']' << Qt::endl;
            SKGTRACEL(20) << "currentState  =[" << currentState << ']' << Qt::endl;
            currentState.remove('\n');
            oldState.remove('\n');
            if (currentState != oldState) {
                QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor));
                int conf = KMessageBox::PrimaryAction;
                if (iUserConfirmation && !oldState.isEmpty()) {
                    conf = KMessageBox::questionTwoActions(this,
                                                           i18nc("Question", "Page has been modified. Do you want to update it with the current state?"),
                                                           i18nc("Question", "Page has been modified"),
                                                           KStandardGuiItem::apply(),
                                                           KStandardGuiItem::cancel(),
                                                           QStringLiteral("updateContextOnClose"));
                }
                QApplication::restoreOverrideCursor();
                if (conf == KMessageBox::PrimaryAction) {
                    SKGError err;
                    {
                        SKGBEGINLIGHTTRANSACTION(*getDocument(), i18nc("Noun, name of the user action", "Save default state"), err)
                        err = getDocument()->setParameter(name, currentState);
                    }
                    IFOKDO(err, SKGError(0, i18nc("Successful message after an user action", "Default state saved")))
                    SKGMainPanel::displayErrorMessage(err);
                }
            }
        }
    }
}

bool SKGTabPage::isEditor()
{
    return false;
}

void SKGTabPage::activateEditor()
{
}

QWidget *SKGTabPage::zoomableWidget()
{
    return mainWidget();
}

QList<QWidget *> SKGTabPage::printableWidgets()
{
    QList<QWidget *> output;
    output.push_back(mainWidget());
    return output;
}

bool SKGTabPage::isZoomable()
{
    return (zoomableWidget() != nullptr);
}

void SKGTabPage::setZoomPosition(int iValue)
{
    QWidget *widget = zoomableWidget();
    auto *treeView = qobject_cast<SKGTreeView *>(widget);
    if (treeView != nullptr) {
        treeView->setZoomPosition(iValue);
    } else {
#ifdef SKG_WEBENGINE
        auto webView = qobject_cast<QWebEngineView *>(widget);
#endif
#if defined(SKG_WEBENGINE)
        if (webView != nullptr) {
            webView->setZoomFactor(qPow(10, static_cast<qreal>(iValue) / 30.0));
        } else {
            int pointSize = qMax(1, m_fontOriginalPointSize + iValue);
            QFont f = widget->font();
            f.setPointSize(pointSize);
            widget->setFont(f);

            auto cs = widget->findChildren<SKGHtmlBoardWidget *>();
            for (auto c : std::as_const(cs)) {
                c->setPointSize(pointSize);
            }
        }
#endif
    }
}

int SKGTabPage::zoomPosition()
{
    int output = 0;
    QWidget *widget = zoomableWidget();
    auto *treeView = qobject_cast<SKGTreeView *>(widget);
    if (treeView != nullptr) {
        output = treeView->zoomPosition();
    } else {
#ifdef SKG_WEBENGINE
        auto webView = qobject_cast<QWebEngineView *>(widget);
#endif
#if defined(SKG_WEBENGINE)
        if (webView != nullptr) {
            output = qRound(30.0 * log10(webView->zoomFactor()));
        } else if (widget != nullptr) {
            output = widget->font().pointSize() - m_fontOriginalPointSize;
        }
#endif
    }
    return output;
}

SKGTabPage *SKGTabPage::parentTabPage(QWidget *iWidget)
{
    auto *output = qobject_cast<SKGTabPage *>(iWidget);
    if ((output == nullptr) && (iWidget != nullptr)) {
        QWidget *iParent = iWidget->parentWidget();
        if (iParent != nullptr) {
            output = SKGTabPage::parentTabPage(iParent);
        }
    }
    return output;
}

bool SKGTabPage::isPin() const
{
    return m_pin;
}

void SKGTabPage::setPin(bool iPin)
{
    m_pin = iPin;
}
