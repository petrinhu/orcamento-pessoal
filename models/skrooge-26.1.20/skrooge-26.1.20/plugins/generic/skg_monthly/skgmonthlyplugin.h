/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGMONTHLYPLUGIN_H
#define SKGMONTHLYPLUGIN_H
/** @file
 * A plugin for monthly report.
 *
 * @author Stephane MANKOWSKI
 */
#include "skginterfaceplugin.h"

#ifdef SKG_WEBENGINE
class QWebEngineView;
#endif
#if !defined(SKG_WEBENGINE)
class QLabel;
#endif

/**
 * A plugin for monthly report
 */
class SKGMonthlyPlugin : public SKGInterfacePlugin
{
    Q_OBJECT
    Q_INTERFACES(SKGInterfacePlugin)

public:
    /**
     * Default Constructor
     */
    explicit SKGMonthlyPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData &metaData, const QVariantList &iArg);

    /**
     * Default Destructor
     */
    ~SKGMonthlyPlugin() override;

    /**
     * Called to initialise the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    bool setupActions(SKGDocument *iDocument) override;

    /**
     * This function is called when the application is launched again with new arguments
     * @param iArgument the arguments
     * @return the rest of arguments to treat
     */
    QStringList processArguments(const QStringList &iArgument) override;

    /**
     * The page widget of the plugin.
     * @return The page widget of the plugin
     */
    SKGTabPage *getWidget() override;

    /**
     * The title of the plugin.
     * @return The title of the plugin
     */
    QString title() const override;

    /**
     * The icon of the plugin.
     * @return The icon of the plugin
     */
    QString icon() const override;

    /**
     * The toolTip of the plugin.
     * @return The toolTip of the plugin
     */
    QString toolTip() const override;

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

    /**
     * Must be implemented to know if a plugin must be display in pages chooser.
     * @return true of false (default = false)
     */
    bool isInPagesChooser() const override;

    /**
     * Must be modified to close properly the plugin.
     */
    virtual void close() override;

private Q_SLOTS:
    void refreshMainPage();

private:
    Q_DISABLE_COPY(SKGMonthlyPlugin)

    SKGDocument *m_currentBankDocument;
#ifdef SKG_WEBENGINE
    QWebEngineView *m_mainPage;
#endif
#if !defined(SKG_WEBENGINE)
    QLabel *m_mainPage;
#endif
};

#endif // SKGMonthlyPLUGIN_H
