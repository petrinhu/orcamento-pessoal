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
#include "skgwidget.h"

#include <qwidget.h>

#include "skgtraces.h"
#include "skgtreeview.h"

SKGWidget::SKGWidget(QWidget *iParent, SKGDocument *iDocument)
    : QWidget(iParent)
    , m_document(iDocument){SKGTRACEINFUNC(5)}

    SKGWidget::~SKGWidget()
{
    SKGTRACEINFUNC(5)
    m_document = nullptr;
}

SKGDocument *SKGWidget::getDocument() const
{
    return m_document;
}

QString SKGWidget::getState()
{
    return QString();
}

QString SKGWidget::getDefaultStateAttribute()
{
    return QString();
}

void SKGWidget::setState(const QString & /*iState*/)
{
}

SKGObjectBase::SKGListSKGObjectBase SKGWidget::getSelectedObjects()
{
    SKGObjectBase::SKGListSKGObjectBase selection;
    auto *treeView = qobject_cast<SKGTreeView *>(mainWidget());
    if (treeView != nullptr) {
        selection = treeView->getSelectedObjects();
    }

    return selection;
}

SKGObjectBase SKGWidget::getFirstSelectedObject()
{
    SKGObjectBase first;
    auto *treeView = qobject_cast<SKGTreeView *>(mainWidget());
    if (treeView != nullptr) {
        first = treeView->getFirstSelectedObject();
    }

    return first;
}

int SKGWidget::getNbSelectedObjects()
{
    int output = 0;
    auto *treeView = qobject_cast<SKGTreeView *>(mainWidget());
    if (treeView != nullptr) {
        output = treeView->getNbSelectedObjects();
    } else {
        output = getSelectedObjects().count();
    }

    return output;
}

bool SKGWidget::hasSelectionWithFocus()
{
    return (mainWidget()->hasFocus());
}

bool SKGWidget::eventFilter(QObject *iObject, QEvent *iEvent)
{
    if (iObject == mainWidget() && (iEvent != nullptr) && (iEvent->type() == QEvent::FocusIn || iEvent->type() == QEvent::FocusOut)) {
        Q_EMIT selectionFocusChanged();
    }
    return QObject::eventFilter(iObject, iEvent);
}

QWidget *SKGWidget::mainWidget()
{
    return this;
}
