/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGPRINTPLUGIN_H
#define SKGPRINTPLUGIN_H
/** @file
 * A plugin to print pages.
 *
 * @author Stephane MANKOWSKI
 */
#include <qprinter.h>
#ifdef SKG_WEBENGINE
#include <qwebengineview.h>
#endif
#if !defined(SKG_WEBENGINE)
#include <qtextbrowser.h>
#endif

#include "skginterfaceplugin.h"
#include "ui_skgprintpluginwidget_pref.h"

/**
 * A plugin to print pages
 */
class SKGPrintPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGPrintPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGPrintPlugin() override;

    /**
     * Called to initialise the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

    /**
     * The preference skeleton of the plugin.
     * @return The preference skeleton of the plugin
     */
    KConfigSkeleton *getPreferenceSkeleton() override;

    /**
     * The title of the plugin.
     * @return The title of the plugin
     */
    QString title() const override;

    /**
     * The tips list of the plugin.
     * @return The tips list of the plugin
     */
    QStringList tips() const override;

    /**
     * Must be implemented to set the position of the plugin.
     * @return integer value between 0 and 999 (default = 999)
     */
    int getOrder() const override;

private Q_SLOTS:
    void onPrint();
    void onPrintPreview();
    void onPrintHtml();
    void print(QPrinter *iPrinter);

private:
    Q_DISABLE_COPY(SKGPrintPlugin)

    SKGError getHtml(QPrinter *iPrinter, QString &oHtml) const;

    SKGDocument *m_currentDocument;
    QPrinter m_printer;
#ifdef SKG_WEBENGINE
    QWebEngineView m_toPrint;
#endif
#if !defined(SKG_WEBENGINE)
    QTextEdit m_toPrint;
#endif
    Ui::skgprintplugin_pref ui{};
};

#endif // SKGPRINTPLUGIN_H
