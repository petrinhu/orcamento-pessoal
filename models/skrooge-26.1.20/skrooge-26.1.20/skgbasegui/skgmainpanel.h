/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGMAINPANEL_H
#define SKGMAINPANEL_H
/** @file
 * This file defines a main panel.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <kxmlguiwindow.h>

#include <qsystemtrayicon.h>
#include <qurl.h>

#include "skgbasegui_export.h"
#include "skgbasegui_settings.h"
#include "skginterfaceplugin.h"
#include "skgobjectbase.h"
#include "skgtabpage.h"
#include "skgtabwidget.h"

#include "ui_skgmainpanel_base.h"
#include "ui_skgmainpanel_pref.h"

class SKGDocument;
class SKGMainPanelPrivate;

class QSplashScreen;
class KMessageWidget;

class QListWidgetItem;

/**
 * This class serves as the main window.  It handles the
 * menus, toolbars, and status bars.
 */
class SKGBASEGUI_EXPORT SKGMainPanel : public KXmlGuiWindow
{
    Q_OBJECT
public:
    /**
     * Default Constructor
     * @param iSplashScreen the splash screen
     * @param iDocument the document to manage data
     */
    explicit SKGMainPanel(QSplashScreen *iSplashScreen, SKGDocument *iDocument);

    /**
     * Default Destructor
     */
    ~SKGMainPanel() override;

    /**
     * Return the document of the main panel
     * @return document of the main panel
     */
    SKGDocument *getDocument() const;

    /**
     * Creates a modal file dialog and returns the selected filename or an empty string if none was chosen.
     * A confirmation message is displayed if needed
     * @param iStartDir this can either be
     * @li the URL of the directory to start in.
     * @li a QUrl) to start in the current working directory, or the last directory where a file has been selected.
     * @li an URL starting with 'kfiledialog:///\<keyword\>' to start in the directory last used by a filedialog in the same application that specified the same
     * keyword.
     * @li an URL starting with 'kfiledialog:///\<keyword\>?global' to start in the directory last used by a filedialog in any application that specified the
     * same keyword.
     * @param iFilter a shell glob or a mime-type-filter that specifies which files to display. The preferred option is to set a list of mimetype names, see
     * setMimeFilter() for details. Otherwise you can set the text to be displayed for the each glob, and provide multiple globs, see setFilter() for details.
     * @param iParent the widget the dialog will be centered on initially.
     * @param iCodec a valid QString to get the codec or nullptr.
     * @return the file name
     */
    static QString getSaveFileName(const QString &iStartDir, const QString &iFilter, QWidget *iParent, QString *iCodec = nullptr);

    /**
     * Display an error message
     * @param iError the error
     * @param iNotifyIfNoError to launch a notification even if there is no error
     * @return the message widget
     */
    static KMessageWidget *displayErrorMessage(const SKGError &iError, bool iNotifyIfNoError = false);

    /**
     * Display an error message
     * @param iError the error
     * @param iAction the additional action to add
     * @param iNotifyIfNoError to launch a notification even if there is no error
     * @return the message widget
     */
    static KMessageWidget *displayErrorMessage(const SKGError &iError, QAction *iAction, bool iNotifyIfNoError = false);

    /**
     * Fill a widget with distinct values
     * @param iWidgets the widgets
     * @param iDoc document
     * @param iTable table
     * @param iAttribut attribute
     * @param iWhereClause where clause
     * @param iAddoperators to add operators (=upper, =lower to the list)
     */
    static void fillWithDistinctValue(const QList<QWidget *> &iWidgets,
                                      SKGDocument *iDoc,
                                      const QString &iTable,
                                      const QString &iAttribut,
                                      const QString &iWhereClause,
                                      bool iAddoperators = false);

    /**
     * Return main panel
     */
    static SKGMainPanel *getMainPanel();

    /**
     * Get main config group
     * @return main config group
     */
    static KConfigGroup getMainConfigGroup();

    /**
     * Convert a QDate into a QString based on application settings.
     * @param iDate the date
     * @return the converted QString
     */
    // cppcheck-suppress passedByValue
    static QString dateToString(QDate iDate);

    /**
     * Get the first selected object
     * @return first selected object
     */
    SKGObjectBase getFirstSelectedObject() const;

    /**
     * Get the current selection
     * @return selected objects
     */
    SKGObjectBase::SKGListSKGObjectBase getSelectedObjects() const;

    /**
     * Get the number of selected object
     * @return number of selected objects
     */
    int getNbSelectedObjects() const;

    /**
     * To know if the widget having the selection has the focus
     * Default implementation is based on mainWidget
     * @return true of false
     */
    bool hasSelectionWithFocus();

    /**
     * To know if the closure of the application is authorized
     * @return true if close is authorized else false
     */
    bool queryClose() override;

    /**
     * To know if the closure of the file is authorized
     * @return true if close is authorized else false
     */
    bool queryFileClose();

    /**
     * Return the plugin number by index
     * @param iIndex the index of the plugin
     * @return the plugin pointer. Can be nullptr. Mustn't be deleted
     */
    SKGInterfacePlugin *getPluginByIndex(int iIndex);

    /**
     * Return the plugin number by name
     * @param iName the name of the plugin
     * @return the plugin pointer. Can be nullptr. Mustn't be deleted
     */
    SKGInterfacePlugin *getPluginByName(const QString &iName);

    /**
     * Get the label for normal message in status bar
     * @return the label
     */
    QLabel *statusNormalMessage() const;

    /**
     * Get the current splash screen. nullptr if the splash screen is closed.
     * @return the splash screen
     */
    QSplashScreen *splashScreen() const;

    /**
     * Set the main widget
     * @param iWidget the widget to display when all pages are closed
     */
    void setMainWidget(QWidget *iWidget);

    /**
     * Get the tab widget.
     * @return the tab widget
     */
    SKGTabWidget *getTabWidget() const;

    /**
     * Get the history item of the current page
     * @return the history item
     */
    SKGTabPage::SKGPageHistoryItem currentPageHistoryItem() const;

    /**
     * Get the index of the current page
     * @return index of the current page
     */
    int currentPageIndex() const;

    /**
     * Get the current page
     * @return the current page
     */
    SKGTabPage *currentPage() const;

    /**
     * Get a index of a page
     * @param iPage the page
     * @return the index (-1 if not found)
     */
    int pageIndex(SKGTabPage *iPage) const;

    /**
     * Get a page
     * @param iIndex an index
     * @return the page
     */
    SKGTabPage *page(int iIndex) const;

    /**
     * Get then number of pages
     * @return the number of pages
     */
    int countPages() const;

    /**
     * Register a global action
     * @param iIdentifier identifier of the action
     * @param iAction action pointer
     * @param iAddInCollection to add or not the action in the main panel collection
     * @param iListOfTable list of table where this action must be enabled (empty list means all)
     *                 You can also add only one item like this to set the list dynamically:
     *                 query:the sql condition on sqlite_master
     * @param iMinSelection the minimum number of selected item to enable the action
     *                  0 : no need selection but need a page opened containing a table
     *                 -1 : no need selection and need a page opened (not containing a table)
     *                 -2 : no need selection and no need a page opened
     * @param iMaxSelection the maximum number of selected item to enable the action (-1 = infinite)
     * @param iRanking the ranking to sort actions in contextual menus
     * @param iRanking the ranking to sort actions in contextual menus
     *                  -1: automatic by creation order
     *                   0: not in contextual menu
     * @param iSelectionMustHaveFocus the action will be activated only if the widget containing the selection has the focus
     *
     *                   Actions can be set in different groups by changing hundred:
     *                      0 to  99 is a group
     *                    100 to 200 is another group
     */
    void registerGlobalAction(const QString &iIdentifier,
                              QAction *iAction,
                              bool iAddInCollection = true,
                              const QStringList &iListOfTable = QStringList(),
                              int iMinSelection = -2,
                              int iMaxSelection = -1,
                              int iRanking = -1,
                              bool iSelectionMustHaveFocus = false);

    /**
     * Get a registered global action
     * @param iIdentifier identifier of the action
     * @param iWarnIfNotExist warn if the action does not exist
     * @return action pointer
     */
    QPointer<QAction> getGlobalAction(const QString &iIdentifier, bool iWarnIfNotExist = true);

    /**
     * Get a list of actions enable for a contextual menu
     * @param iTable the table
     * @return the list of actions
     */
    QList<QPointer<QAction>> getActionsForContextualMenu(const QString &iTable);

    /**
     * Get all registered global actions
     * @return actions
     */
    QMap<QString, QPointer<QAction>> getGlobalActions() const;

    /**
     * Get the tips of days
     * @return the tips of days
     */
    QStringList getTipsOfDay() const;

    /**
     * Get the tip of days
     * @return the tip of days
     */
    QString getTipOfDay() const;

    /**
     * Define if the document must be saved when closed
     * @param iSaveOnClose the save on close mode
     */
    void setSaveOnClose(bool iSaveOnClose);

    /**
     * Get all advice
     * @return the list of advice
     */
    SKGAdviceList getAdvice() const;

public Q_SLOTS:
    /**
     * Display a message
     * @param iMessage the message
     * @param iType the type
     * @param iAction the associated action
     * @return the message widget
     */
    KMessageWidget *displayMessage(const QString &iMessage, SKGDocument::MessageType iType = SKGDocument::Information, const QString &iAction = QString());

    /**
     * Display an error message
     * @param iMessage the error message. If the message is "", the data of the sender will be used
     * @return the message widget
     */
    KMessageWidget *displayErrorMessage(const QString &iMessage = QString());

    /**
     * This function is called when the application is launched again with new arguments
     * @param iArgument the arguments
     * @return the rest of arguments to treat
     */
    QStringList processArguments(const QStringList &iArgument);

    /**
     * Set the current page
     * @param iIndex the current page
     */
    void setCurrentPage(int iIndex);

    /**
     * Set the context item visibility
     * @param iPage index of the page in the pages chooser
     * @param iVisibility the visibility
     */
    void setContextVisibility(int iPage, bool iVisibility);

    /**
     * Set the context item visibility
     * @param iItem item in the pages chooser
     * @param iVisibility the visibility
     */
    void setContextVisibility(QListWidgetItem *iItem, bool iVisibility);

    /**
     * Open a plugin in a page described in sender()->data()
     * @return true if the url has been opened
     */
    bool openPage();

    /**
     * Open a plugin in a page
     * @param iUrl the url like this "skg://plugin_name/?param1=value1&param2=value2&…" to open a special page
     * or "skg://action" to trigger an action
     * or "https://…" to open a web page
     * @param iNewPage to open a new page or not
     * @return true if the url has been opened
     */
    bool openPage(const QUrl &iUrl, bool iNewPage = true);

    /**
     * Open a plugin in a page
     * @param iUrl the url like this "skg://plugin_name/?param1=value1&param2=value2&…" to open a special page
     * or "skg://action/?param1=value1&param2=value2&…" to trigger an action, parameters can be found as property on the sender QAction
     * or "https://…" to open a web page
     * if empty then url will be get from sender()->data()
     * @param iNewPage to open a new page or not
     * @return true if the url has been opened
     */
    bool openPage(const QString &iUrl, bool iNewPage = true);

    /**
     * Open a plugin in a page
     * @param iPage index of the page in the pages chooser
     * @param iNewPage to open a new page or not
     * @return the opened tab
     */
    SKGTabPage *openPage(int iPage, bool iNewPage = true);

    /**
     * Open a plugin in a page
     * @param plugin the plugin
     * @param index index of the tab to replace or -1 to add a new one
     * @param parameters parameters of the plugin
     * @param title title of the page
     * @param iID id of the new page
     * @param iSetCurrent to set the new page as the current one
     * @return the opened tab
     */
    SKGTabPage *openPage(SKGInterfacePlugin *plugin,
                         int index = -1,
                         const QString &parameters = QString(),
                         const QString &title = QString(),
                         const QString &iID = QString(),
                         bool iSetCurrent = true);

    /**
     * Switch the pin state of the page
     * @param iWidget the page to remove (nullptr means current one)
     */
    void switchPinPage(QWidget *iWidget);

    /**
     * Close a page
     * @param iIndex the page to remove (-1 means current one)
     */
    void closePageByIndex(int iIndex = -1);

    /**
     * Close a page
     * @param iWidget the page to remove (nullptr means current one)
     * @param iForce to close pinned pages too
     */
    void closePage(QWidget *iWidget, bool iForce = false);

    /**
     * Close the current page
     */
    void closeCurrentPage();

    /**
     * Close all other pages
     * @param iWidget the page to keep (nullptr means current one)
     */
    void closeAllOtherPages(QWidget *iWidget);

    /**
     * Close all pages
     * @param iForce to close pinned pages too
     */
    void closeAllPages(bool iForce = false);

    /**
     * Force the refresh of the object.
     */
    void refresh();

    /**
     * Send notifications corresponding to the transaction.
     * @param iTransaction the transaction identifier
     */
    void notify(int iTransaction = 0);

    /**
     * Open settings panel on dedicated page.
     * @param iPluginName the name of the plugin (pluginInterface->objectName())
     */
    void optionsPreferences(const QString &iPluginName = QString());

    /**
     * Unregister a global action
     * @param iAction action pointer
     */
    void unRegisterGlobalAction(QObject *iAction);

Q_SIGNALS:
    /**
     * This signal is sent when a new page is opened.
     */
    void pageOpened();

    /**
     * This signal is sent when a new page is opened.
     */
    void pageClosed();

    /**
     * This signal is sent when the current page changes.
     */
    void currentPageChanged();

    /**
     * This signal is sent when settings changes.
     */
    void settingsChanged();

    /**
     * This signal is sent when selection changes.
     */
    void selectionChanged();

protected:
    /**
     * event
     * @param e event
     */
    void changeEvent(QEvent *e) override;
    /**
     * Event filtering
     * @param iObject object
     * @param iEvent event
     * @return In your reimplementation of this function, if you want to filter the event out, i.e. stop it being handled further, return true; otherwise return
     * false.
     */
    bool eventFilter(QObject *iObject, QEvent *iEvent) override;

private Q_SLOTS:
    KMessageWidget *getMessageWidget(const QString &iMessage, SKGDocument::MessageType iType, const QString &iAction, bool iAutoKillOnClick);
    void showMenu(QPoint iPos);

    void onSettingsChanged();
    void onCancelCurrentAction();
    void onQuitAction();
    void addTab();
    void onBeforeOpenContext();
    void onOpenContext();
    void saveDefaultState();
    void resetDefaultState();
    void overwriteBookmarkState();
    void enableEditor();
    void onPrevious();
    void onNext();
    void onReopenLastClosed();
    void onFullScreen();
    void onShowPreviousMenu();
    void onShowNextMenu();
    void onZoomChanged();
    void onShowMenuBar();
    void onShowButtonMenu();
    void onHideContextItem();
    void onShowAllContextItems();
    void onLockDocks();
    void onUnlockDocks();
    void onConfigureNotifications();
    void onClearMessages();
    void onMigrateToSQLCipher();

private:
    Q_DISABLE_COPY(SKGMainPanel)
    void setupActions();

    SKGMainPanelPrivate *const d;
};

#endif // SKGMAINPANEL_H
