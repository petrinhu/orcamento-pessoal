/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGINTERFACEPLUGIN_H
#define SKGINTERFACEPLUGIN_H
/** @file
 * This file is a plugin interface definition.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <kparts/readonlypart.h>

#include <qstringlist.h>

#include "skgadvice.h"
#include "skgbasegui_export.h"
#include "skgdocument.h"
#include "skgerror.h"
#include "skgtabpage.h"
class SKGBoardWidget;
class QAction;
class KConfigSkeleton;
class QDockWidget;

/**
 * This file is a plugin interface definition.
 */
class SKGBASEGUI_EXPORT SKGInterfacePlugin : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    /**
     * Default constructor
     * @param iParent the parent of the plugin
     */
    explicit SKGInterfacePlugin(QObject *iParent = nullptr);

    /**
     * Default destructor
     */
    ~SKGInterfacePlugin() override;

    /**
     * Called to initialise the plugin
     * @param iDocument the main document
     * @return true if the plugin is compatible with the document
     */
    virtual bool setupActions(SKGDocument *iDocument) = 0;

    /**
     * Register a global action
     * @param iIdentifier identifier of the action
     * @param iAction action pointer
     * @param iListOfTable list of table where this action must be enabled (empty list means all)
     *                 You can also add only one item like this to set the list dynamically:
     *                 query:the sql condition on sqlite_master
     * @param iMinSelection the minimum number of selected item to enable the action
     *                  0 : no need selection but need a page opened containing a table
     *                 -1 : no need selection and need a page opened (not containing a table)
     *                 -2 : no need selection and no need a page opened
     * @param iMaxSelection the maximum number of selected item to enable the action (-1 = infinite)
     * @param iRanking the ranking to sort actions in contextual menus
     *                  -1: automatic by creation order
     *                   0: not in contextual menu
     * @param iSelectionMustHaveFocus the action will be activated only if the widget containing the selection has the focus
     *
     *                   Actions can be set in different groups by changing hundred:
     *                      0 to  99 is a group
     *                    100 to 200 is an other group
     */
    virtual void registerGlobalAction(const QString &iIdentifier,
                                      QAction *iAction,
                                      const QStringList &iListOfTable = QStringList(),
                                      int iMinSelection = -2,
                                      int iMaxSelection = -1,
                                      int iRanking = -1,
                                      bool iSelectionMustHaveFocus = false);

    /**
     * This function is called when the application is launched again with new arguments
     * @param iArgument the arguments
     * @return the rest of arguments to treat
     */
    virtual QStringList processArguments(const QStringList &iArgument);

    /**
     * Must be modified to close properly the plugin.
     */
    virtual void close();

    /**
     * Must be modified to refresh widgets after a modification.
     */
    virtual void refresh();

    /**
     * The page widget of the plugin.
     * @return The page widget of the plugin
     */
    virtual SKGTabPage *getWidget();

    /**
     * The number of dashboard widgets of the plugin.
     * @return The number of dashboard widgets of the plugin
     */
    virtual int getNbDashboardWidgets();

    /**
     * Get a dashboard widget title of the plugin.
     * @param iIndex the index of the widget
     * @return The title
     */
    virtual QString getDashboardWidgetTitle(int iIndex);

    /**
     * Get a dashboard widget of the plugin.
     * @param iIndex the index of the widget
     * @return The dashboard widget of the plugin
     */
    virtual SKGBoardWidget *getDashboardWidget(int iIndex);

    /**
     * The dock widget of the plugin.
     * @return The dock widget of the plugin
     */
    virtual QDockWidget *getDockWidget();

    /**
     * Initialize the preferences. This is use full for settings stored in the document
     */
    virtual void initPreferences();

    /**
     * The preference widget of the plugin.
     * @return The preference widget of the plugin
     */
    virtual QWidget *getPreferenceWidget();

    /**
     * The preference skeleton of the plugin.
     * @return The preference skeleton of the plugin
     */
    virtual KConfigSkeleton *getPreferenceSkeleton();

    /**
     * This function is called when preferences have been modified. Must be used to save some parameters into the document.
     * A transaction is already opened
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError savePreferences() const;

    /**
     * The title of the plugin.
     * @return The title of the plugin
     */
    virtual QString title() const = 0;

    /**
     * The icon of the plugin.
     * @return The icon of the plugin
     */
    virtual QString icon() const;

    /**
     * The statusTip of the plugin.
     * @return The toolTip of the plugin
     */
    virtual QString statusTip() const;

    /**
     * The toolTip of the plugin.
     * @return The toolTip of the plugin
     */
    virtual QString toolTip() const;

    /**
     * The tips list of the plugin.
     * @return The tips list of the plugin
     */
    virtual QStringList tips() const;

    /**
     * The sub plugins services types list of the plugin.
     * This will be used to display authors in the "About" of the application
     * @return The sub plugins list of the plugin
     */
    virtual QStringList subPlugins() const;

    /**
     * Must be implemented to set the position of the plugin.
     * @return integer value between 0 and 999 (default = 999)
     */
    virtual int getOrder() const;

    /**
     * Must be implemented to know if a plugin must be display in pages chooser.
     * @return true of false (default = false)
     */
    virtual bool isInPagesChooser() const;

    /**
     * Must be implemented to know if this plugin is enabled
     * @return true of false (default = true)
     */
    virtual bool isEnabled() const;

    /**
     * The advice list of the plugin.
     * @return The advice list of the plugin
     */
    virtual SKGAdviceList advice(const QStringList &iIgnoredAdvice);

    /**
     * Must be implemented to execute the automatic correction for the advice.
     * @param iAdviceIdentifier the identifier of the advice
     * @param iSolution the identifier of the possible solution
     * @return an object managing the error. MUST return ERR_NOTIMPL if iAdviceIdentifier is not known
     *   @see SKGError
     */
    virtual SKGError executeAdviceCorrection(const QString &iAdviceIdentifier, int iSolution);

private:
    Q_DISABLE_COPY(SKGInterfacePlugin)
};

/**
 * This plugin interface definition.
 */
Q_DECLARE_INTERFACE(SKGInterfacePlugin, "skrooge.com.SKGInterfacePlugin/1.0")

#endif // SKGINTERFACEPLUGIN_H
