/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGWEBVIEW_H
#define SKGWEBVIEW_H
/** @file
 * A web viewer with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qprinter.h>
#ifdef SKG_WEBENGINE
#include <qwebengineview.h>
#else
#include <qlabel.h>
#include <qscrollarea.h>
#endif
#include "skgbasegui_export.h"

/**
 * This file is a web viewer
 */
#ifdef SKG_WEBENGINE
class SKGBASEGUI_EXPORT SKGWebView : public QWebEngineView
#else
class SKGBASEGUI_EXPORT SKGWebView : public QScrollArea
#endif
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param iParent the parent
     * @param name name
     */
#ifdef SKG_WEBENGINE
    explicit SKGWebView(QWidget *iParent, const char *name = nullptr, bool iWithContextualMenu = true);

    /**
     * To emit that a link is clicked
     * @param iURL the url of the link
     */
    void emitLinkClicked(const QUrl &iURL);

#else
    explicit SKGWebView(QWidget *iParent, const char *name = nullptr);
#endif
    /**
     * Destructor
     */
    ~SKGWebView() override;

    /**
     * Get the current state
     * @return a string containing all information needed to set the same state.
     * Could be an XML stream
     */
    virtual QString getState();

    /**
     * Set the current state
     * MUST BE OVERWRITTEN
     * @param iState must be interpreted to set the state of the widget
     */
    virtual void setState(const QString &iState);

    /**
     * Export in the file (PDF, ODT, png, jpeg, …)
     * @param iFileName the file name
     */
    virtual void exportInFile(const QString &iFileName);

#if !defined(SKG_WEBENGINE)
    /**
     * Set html
     * @param iFileName the file name
     */
    virtual void setHtml(const QString &iHtml)
    {
        qobject_cast<QLabel *>(widget())->setText(iHtml);
    };
#endif

Q_SIGNALS:
    /**
     * Emitted when zoom changed
     * @param iZoomPosition zoom position (-10<=zoom position<=10)
     */
    void zoomChanged(int iZoomPosition);

    /**
     * Emitted when a file is exported
     * @param iFileName the exported file name
     */
    void fileExporter(const QString &iFileName);

#ifdef SKG_WEBENGINE
    /**
     * Emitted when a link is clicked
     * @param iURL the url of the link
     */
    void linkClicked(const QUrl &iURL);
#endif

public Q_SLOTS:
    /**
     * Zoom in
     */
    virtual void onZoomIn();

    /**
     * Zoom out
     */
    virtual void onZoomOut();

    /**
     * Fit on scene
     */
    virtual void onZoomOriginal();

    /**
     * Print
     */
    virtual void onPrint();

    /**
     * Export
     */
    virtual void onExport();

    /**
     * Print preview
     */
    virtual void onPrintPreview();

protected:
#if defined(SKG_WEBENGINE)
    /**
     * Contextual event
     * @param iEvent the event
     */
    void contextMenuEvent(QContextMenuEvent *iEvent) override;
#endif

    /**
     * Event filtering
     * @param iObject object
     * @param iEvent event
     * @return In your reimplementation of this function, if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return
     * false.
     */
    bool eventFilter(QObject *iObject, QEvent *iEvent) override;

private:
    Q_DISABLE_COPY(SKGWebView)
    QPrinter m_printer;

#ifdef SKG_WEBENGINE
    bool m_ContextualMenu;
#endif
};

#endif
