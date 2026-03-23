/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDOCUMENT_H
#define SKGDOCUMENT_H
/** @file
 * This file defines classes SKGDocument.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qdatetime.h>
#include <qfuturewatcher.h>
#include <qhash.h>
#include <qicon.h>
#include <qsqldatabase.h>
#include <qstringlist.h>
#include <qvariant.h>

#include <functional>

#include "skgbasemodeler_export.h"
#include "skgobjectbase.h"
#include "skgservices.h"

class SKGObjectBase;
class SKGError;
class SKGPropertyObject;
class SKGReport;
class SKGDocumentPrivate;

using FuncSelect = std::function<void(const SKGStringListList &)>; // NOLINT(whitespace/parens)
using FuncExist = std::function<void(bool)>; // NOLINT(whitespace/parens)
using FuncProgress = std::function<int(int, qint64, const QString &, void *)>; // NOLINT(whitespace/parens)
/**
 * This class manages skg documents
 */
class SKGBASEMODELER_EXPORT SKGDocument : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.skg.SKGDocument")

    using progressFunction = int (*)(int, qint64, const QString &, void *);
    /**
     * The unique identifier
     */
    Q_PROPERTY(QString uuid READ getUniqueIdentifier NOTIFY modified)

    /**
     * The current file name
     */
    Q_PROPERTY(QString fileName READ getCurrentFileName NOTIFY modified)

    /**
     * To know if the document is read only
     */
    Q_PROPERTY(bool readOnly READ isReadOnly NOTIFY modified)

    /**
     * To know if the document is modifier
     */
    Q_PROPERTY(bool modified READ isFileModified NOTIFY transactionSuccessfullyEnded)

public:
    /**
     * This enumerate defines a type of modification
     */
    enum ModificationType {
        U, /**< Update */
        I, /**< Insert */
        D /**< Delete */
    };
    /**
     * This enumerate defines a type of modification
     */
    Q_ENUM(ModificationType)

    /**
     * This enumerate defines a type of message
     */
    enum MessageType {
        Positive, /**< Positive */
        Information, /**< Information */
        Warning, /**< Warning */
        Error, /**< Error */
        Hidden /**< Hidden */
    };

    /**
     * This enumerate defines a type of message
     */
    Q_ENUM(MessageType)

    /**
     * Describe a modification of an object
     */
    struct SKGObjectModification {
        QString uuid; /**< The uuid of the object */
        int id{}; /**< The id of the object */
        QString table; /**< The table of the object */
        ModificationType type; /**< The type of modification */
    };

    /**
     * Describe a modification of an object
     */
    struct SKGMessage {
        QString Text; /**< The text of the message */
        MessageType Type; /**< The type of the message */
        QString Action; /**< The action associated */
    };

    /**
     * This structure represents a template to display data
     */
    struct SKGModelTemplate {
        QString id; /**< Identifier of the schema */
        QString name; /**< The nls name */
        QString icon; /**< The icon */
        QString schema; /**< The schema. The format of this string is the following one: attribute name[|visibility Y or N[|size]];attribute name[|visibility Y
                           or N[|size]];… */
    };

    /**
     * A list of SKGModelTemplate ==> SKGModelTemplateList
     */
    using SKGModelTemplateList = QVector<SKGModelTemplate>;

    /**
     * A list of SKGObjectModification ==> SKGObjectModificationList
     */
    using SKGObjectModificationList = QVector<SKGObjectModification>;

    /**
     * A list of SKGMessage ==> SKGObjectModificationList
     */
    using SKGMessageList = QVector<SKGMessage>;

    /**
     * This enumerate defines the direction of the UNDO / REDO mechanism
     */
    enum UndoRedoMode {
        UNDOLASTSAVE, /**< To do/get an undo=cancel of the last successfully extecuted transactions until last save */
        UNDO, /**< To do/get an undo=cancel of the last successfully extecuted transaction */
        REDO /**< To do/get a redo=replay the last cancelled transaction */
    };
    /**
     * This enumerate defines the direction of the UNDO / REDO mechanism
     */
    Q_ENUM(UndoRedoMode)

    /**
     * Constructor
     */
    explicit SKGDocument();

    /**
     * Destructor
     */
    ~SKGDocument() override;

    /**
     * Set the callback function to follow the progress of the transaction.
     * the first parameter is the progress between 0% and 100%.
     * the callback must return 0 to continue and !=0 to cancel the transaction.
     * @param iProgressFunction the pointer of the function
     * @param iData the data for the progress call back
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setProgressCallback(FuncProgress iProgressFunction, void *iData);

    /**
     * Add a check function at the end of successfully executed transaction.
     * If an error is returned, the transaction will be cancelled.
     * @param iCheckFunction the pointer of the function
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError addEndOfTransactionCheck(SKGError (*iCheckFunction)(SKGDocument *));

    /**
     * Get unique identifier
     * @return the unique identifier
     */
    virtual QString getUniqueIdentifier() const;

    /**
     * Get database identifier
     * @return the database identifier
     */
    virtual QString getDatabaseIdentifier() const;

    /**
     * Get message attached to a transaction.
     * @param iIdTransaction the identifier of a transaction
     * @param oMessages the messages
     * @param iAll to get all message (true) or only non hidden messages (false)
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getMessages(int iIdTransaction, SKGMessageList &oMessages, bool iAll = true);

    /**
     * Remove all message attached to a transaction.
     * @param iIdTransaction the identifier of a transaction
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError removeMessages(int iIdTransaction);

    /**
     * Get list of direct modifications done in a transaction
     * @param iIdTransaction the identifier of a transaction
     * @param oModifications list of modifications
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getModifications(int iIdTransaction, SKGObjectModificationList &oModifications) const;

    /**
     * Undo or redo the last transaction.
     * @param iMode the mode
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError undoRedoTransaction(UndoRedoMode iMode = SKGDocument::UNDO);

    /**
     * Group transactions
     * @param iFrom the first id transaction of the group, it will be the master of the group.
     * @param iTo the last id transaction of the group.
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError groupTransactions(int iFrom, int iTo);

    /**
     * Return the number of transaction stored including the current one
     * @param iMode the mode
     * @return the number of transaction
     */
    virtual int getNbTransaction(UndoRedoMode iMode = SKGDocument::UNDO) const;

    /**
     * Return the internal identifier of the transaction
     * which must be treated for an undo or a redo
     * @param iMode the mode
     * @param oName if you want also the name of the transaction
     * @param oSaveStep if you want also to know if it is a save step
     * @param oDate if you want also the date of the transaction
     * @param oRefreshViews if you want also to know if views must be refreshed
     * @return the internal identifier of the last transaction
     * 0 if no transaction found
     */
    virtual int getTransactionToProcess(UndoRedoMode iMode,
                                        QString *oName = nullptr,
                                        bool *oSaveStep = nullptr,
                                        QDateTime *oDate = nullptr,
                                        bool *oRefreshViews = nullptr) const;

    /**
     * Return the identifier of the current transaction
     * @return the internal identifier of the current transaction
     * 0 if no transaction found
     */
    virtual int getCurrentTransaction() const;

    /**
     * Return the depth of the current transaction
     * @return the depth
     */
    virtual int getDepthTransaction() const;

    /**
     * To know if a transaction is opened or not
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError checkExistingTransaction() const;

    /**
     * Get the current password.
     * @return the password
     */
    virtual QString getPassword() const;

    /**
     * To know if the current document has been modified or not.
     * @return true: the document has been modified, save is possible/needed.
     *         false: the document hasn't been modified, save is not needed.
     */
    virtual bool isFileModified() const;

    /**
     * To know if the document is loaded in read only.
     * @return true: the document is loaded in read only.
     *         false: the document is loaded in read write.
     */
    virtual bool isReadOnly() const;

    /**
     * Return the file name of the current document.
     * To set it, you must use saveAs.
     * @return the file name of the current document.
     */
    virtual QString getCurrentFileName() const;

    /**
     * Set a parameter.
     * WARNING: This method must be used in a transaction.
     * @see beginTransaction
     * @param iName the parameter unique identifier.
     * @param iValue the parameter value.
     * @param iFileName the file name.
     * @param iParentUUID the unique identifier of the object owning this parameter.
     * @param oObjectCreated the parameter object created. Can be nullptr
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError setParameter(const QString &iName,
                                  const QString &iValue,
                                  const QString &iFileName = QString(),
                                  const QString &iParentUUID = QStringLiteral("document"),
                                  SKGPropertyObject *oObjectCreated = nullptr) const;

    /**
     * Set a parameter.
     * WARNING: This method must be used in a transaction.
     * @see beginTransaction
     * @param iName the parameter unique identifier.
     * @param iValue the parameter value.
     * @param iBlob the parameter blob.
     * @param iParentUUID the unique identifier of the object owning this parameter.
     * @param oObjectCreated the parameter object created. Can be nullptr
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError setParameter(const QString &iName,
                                  const QString &iValue,
                                  const QVariant &iBlob,
                                  const QString &iParentUUID = QStringLiteral("document"),
                                  SKGPropertyObject *oObjectCreated = nullptr) const;

    /**
     * Get the list of parameters
     * @param iParentUUID the unique identifier of the object owning parameters.
     * @param iWhereClause the additional where clause.
     * @return the list of parameters.
     */
    virtual QStringList getParameters(const QString &iParentUUID, const QString &iWhereClause = QString());

    /**
     * Get a parameter value
     * @param iName the parameter unique identifier.
     * @param iParentUUID the unique identifier of the object owning this parameter.
     * @return the value.
     */
    virtual QString getParameter(const QString &iName, const QString &iParentUUID = QStringLiteral("document")) const;

    /**
     * Get a parameter blob
     * @param iName the parameter unique identifier.
     * @param iParentUUID the unique identifier of the object owning this parameter.
     * @return the blob.
     */
    virtual QVariant getParameterBlob(const QString &iName, const QString &iParentUUID = QStringLiteral("document")) const;

    /**
     * Get the database pointer.
     * This is the database connection of the main thread
     * @return the database pointer.
     *   MUST NOT BE REMOVED
     */
    QSqlDatabase *getMainDatabase() const;

    /**
     * Get the database pointer.
     * This is the database connection of the current thread
     * @return the database pointer.
     */
    virtual QSqlDatabase getThreadDatabase() const;

    /**
     * dump the document in the std output.
     * It is useful for debug.
     * @param iMode the select what you want to dump.
     * @code
     * document->dump (DUMPPARAMETERS|DUMPTRANSACTIONS);
     * @endcode
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError dump(int iMode = DUMPPARAMETERS | DUMPTRANSACTIONS) const;

    /**
     * Create a consolidated view
     * @param iTable Table name
     * @param iAsColumn Attribute used as column names
     * @param iAsRow Attribute used as lines names
     * @param iAttribute Attribute
     * @param iOpAtt Transaction to apply on @p iAttribute
     * @param iWhereClause Where clause
     * @param oTable the consolidated view
     * @param iMissingValue value to put if no value found
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError getConsolidatedView(const QString &iTable,
                                         const QString &iAsColumn,
                                         const QString &iAsRow,
                                         const QString &iAttribute,
                                         const QString &iOpAtt,
                                         const QString &iWhereClause,
                                         SKGStringListList &oTable,
                                         const QString &iMissingValue = QStringLiteral("0")) const;

    /**
     * Get the display string for any modeler object (table, attribute)
     * @param iString the name of the object (example: v_operation, v_unit.t_name)
     * @return the display string
     */
    virtual QString getDisplay(const QString &iString) const;

    /**
     * Get display schemas
     * @param iRealTable the real table name
     * @return list of schemas
     */
    virtual SKGDocument::SKGModelTemplateList getDisplaySchemas(const QString &iRealTable) const;

    /**
     * Get the icon for attribute
     * @param iString the name of the attribute
     * @return the icon name
     */
    virtual QString getIconName(const QString &iString) const;

    /**
     * Get the icon for attribute
     * @param iString the name of the attribute
     * @return the icon
     */
    virtual QIcon getIcon(const QString &iString) const;

    /**
     * Get the real attribute
     * @param iString the name of the attribute (something like t_BANK)
     * @return the real attribute (something like bank.rd_bank_id.t_name)
     */
    virtual QString getRealAttribute(const QString &iString) const;

    /**
     * Get the attribute type
     * @param iAttributeName the name of an attribute
     * @return the type
     */
    virtual SKGServices::AttributeType getAttributeType(const QString &iAttributeName) const;

    /**
     * Get unit. WARNING: This value can be not uptodated in a transaction.
     * @param iPrefixInCache the prefix of the unit in the cache
     * @return unit.
     */
    virtual SKGServices::SKGUnitInfo getUnit(const QString &iPrefixInCache) const;

    /**
     * Get the cached sql result
     * @param iKey key
     * @return the sql result
     */
    virtual SKGStringListList getCachedSqlResult(const QString &iKey) const;

    /**
     * Get the cached value
     * @param iKey key
     * @return the value
     */
    virtual QString getCachedValue(const QString &iKey) const;

    /**
     * Get the corresponding backup file
     * @param iFileName the file to save
     * @return the corresponding backup file
     */
    virtual QString getBackupFile(const QString &iFileName) const;

    /**
     * Get the temporary file for a file
     * @param iFileName the file
     * @param iForceReadOnly force the read only mode
     * @return the temporary file
     */
    static QString getTemporaryFile(const QString &iFileName, bool iForceReadOnly = false);

    /**
     * Get the temporary file for a document
     * @return the temporary file
     */
    virtual QString getCurrentTemporaryFile() const;

    /**
     * Get the file extension for this kind of document (must be overwritten)
     * @return file extension (like skg)
     */
    virtual QString getFileExtension() const;

    /**
     * Get the header of the file (useful for magic mime type).
     * @return document header
     */
    virtual QString getDocumentHeader() const;

    /**
     * Return a number of objects (@p oNbObjects) corresponding to a where clause (@p iWhereClause )
     * @param iTable the table where to search
     * @param iWhereClause the where clause
     * @param oNbObjects the result
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getNbObjects(const QString &iTable, const QString &iWhereClause, int &oNbObjects) const;

    /**
     * To execute a function when at least one object exist corresponding to a where clause (@p iWhereClause)
     * @param iTable the table where to search
     * @param iWhereClause the where clause
     * @param iFunction the function to call
     * @param iExecuteInMainThread to execute the function in the main thread or not
     */
    virtual void concurrentExistObjects(const QString &iTable, const QString &iWhereClause, const FuncExist &iFunction, bool iExecuteInMainThread = true);

    /**
     * To know if at least one object exist corresponding to a where clause (@p iWhereClause)
     * @param iTable the table where to search
     * @param iWhereClause the where clause
     * @param oExist the result
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError existObjects(const QString &iTable, const QString &iWhereClause, bool &oExist) const;

    /**
     * Return a list of objects (@p oListObject) corresponding to a where clause (@p iWhereClause )
     * @param iTable the table where to search
     * @param iWhereClause the where clause
     * @param oListObject the result
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getObjects(const QString &iTable, const QString &iWhereClause, SKGObjectBase::SKGListSKGObjectBase &oListObject) const;

    /**
     * Return the object (@p oObject) corresponding to a where clause (@p iWhereClause ).
     * If more than one objects are returned by the query, then an error is generated
     * If 0 object is returned by the query, then an error is generated
     * @param iTable the table where to search
     * @param iWhereClause the where clause
     * @param oObject the result
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getObject(const QString &iTable, const QString &iWhereClause, SKGObjectBase &oObject) const;

    /**
     * Return the object (@p oObject) corresponding to an id (@p iId ).
     * If more than one objects are returned by the query, then an error is generated
     * If 0 object is returned by the query, then an error is generated
     * @param iTable the table where to search
     * @param iId id of the object in @p iTable
     * @param oObject the result
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError getObject(const QString &iTable, int iId, SKGObjectBase &oObject) const;

    /**
     * Retrieve list of tables
     * @param oResult the output result
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError getTablesList(QStringList &oResult) const;

    /**
     * Retrieve all distinct values of an attribute of a table
     * @param iTable the table where to look for
     * @param iAttribute the attribute wanted (only one)
     * @param iWhereClause a whereclause
     * @param oResult the output result
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError getDistinctValues(const QString &iTable, const QString &iAttribute, const QString &iWhereClause, QStringList &oResult) const;

    /**
     * Retrieve all distinct values of an attribute of a table
     * @param iTable the table where to look for
     * @param iAttribute the attribute wanted (only one)
     * @param oResult the output result
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError getDistinctValues(const QString &iTable, const QString &iAttribute, QStringList &oResult) const;

    /**
     * Execute a sqlite orders
     * @param iSqlOrders the sql orders
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError executeSqliteOrders(const QStringList &iSqlOrders) const;

    /**
     * Execute a sqlite order
     * @param iSqlOrder the sql order
     * @param iBind the binded variables and values
     * @param iLastId to retrieve the id of the last created object
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError executeSqliteOrder(const QString &iSqlOrder, const QMap<QString, QVariant> &iBind, int *iLastId) const;

    /**
     * Execute a sqlite order
     * @param iSqlOrder the sql order
     * @param iLastId to retrieve the id of the last created object. Can be nullptr
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError executeSqliteOrder(const QString &iSqlOrder, int *iLastId) const;

    /**
     * Execute a sqlite order
     * @param iSqlOrder the sql order
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError executeSqliteOrder(const QString &iSqlOrder) const;

    /**
     * Execute a select sqlite order returning one value and return the result in @p oResult
     * @param iSqlOrder the sql order
     * @param oResult the result of the select
     * @param iUseCache to use the cache
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError executeSingleSelectSqliteOrder(const QString &iSqlOrder, QString &oResult, bool iUseCache = true) const;

    /**
     * Execute a select sqlite order and return the result in @p oResult
     * @param iSqlOrder the sql order
     * @param oResult the result of the select. It is a vector of vector of QString
     * @param iUseCache to use the cache
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError executeSelectSqliteOrder(const QString &iSqlOrder, SKGStringListList &oResult, bool iUseCache = true) const;

    /**
     * Execute a select sqlite order and pass the result to the function @p iFunction. This is done in another thread.
     * @param iSqlOrder the sql order
     * @param iFunction the function to call
     * @param iExecuteInMainThread to execute the function in the main thread or not
     */
    virtual void concurrentExecuteSelectSqliteOrder(const QString &iSqlOrder, const FuncSelect &iFunction, bool iExecuteInMainThread = true) const;

    /**
     * dump a select sqlite order
     * @param iSqlOrder the sql order
     * @param oStream the output stream, nullptr for std output (cout)
     * @param iMode dump mode
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError
    dumpSelectSqliteOrder(const QString &iSqlOrder, QTextStream *oStream = nullptr, SKGServices::DumpMode iMode = SKGServices::DUMP_TEXT) const;

    /**
     * dump a select sqlite order
     * @param iSqlOrder the sql order
     * @param oResult the output
     * @param iMode dump mode
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError dumpSelectSqliteOrder(const QString &iSqlOrder, QString &oResult, SKGServices::DumpMode iMode = SKGServices::DUMP_TEXT) const;

    /**
     * dump a select sqlite order
     * @param iSqlOrder the sql order
     * @param oResult the output
     * @param iMode dump mode
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError dumpSelectSqliteOrder(const QString &iSqlOrder, QStringList &oResult, SKGServices::DumpMode iMode = SKGServices::DUMP_TEXT) const;

    /**
     * Retrieve description of each attribute of a table
     * @param iTable the table where to look for
     * @param oResult the output result
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError getAttributesDescription(const QString &iTable, SKGServices::SKGAttributesList &oResult) const;

    /**
     * Retrieve list of attributes
     * @param iTable the table where to look for
     * @param oResult the output result
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError getAttributesList(const QString &iTable, QStringList &oResult) const;

    /**
     * Get the report
     * Do not forget to delete the pointer
     * @return the report
     */
    virtual SKGReport *getReport() const;

    /**
     * Copy a document into an JSON document.
     * @param oDocument the json document
     * @return An object managing the error
     *   @see SKGError
     */
    virtual SKGError copyToJson(QString &oDocument) const;

    /**
     * Refresh all views and indexes in the database
     * @param iForce force the refresh
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError refreshViewsIndexesAndTriggers(bool iForce = false) const;

    /**
     * Get formatted money
     * @param iValue value
     * @param iUnit unit
     * @param iHtml colored output
     * @return formatted value in red or black
     */
    Q_INVOKABLE virtual QString formatMoney(double iValue, const SKGServices::SKGUnitInfo &iUnit, bool iHtml = true) const;

    /**
     * Get formatted money in primary unit
     * @param iValue value
     * @param iForcedNbOfDigit the number of digit (-1 means nb of digit of the unit)
     * @return formatted value in red or black
     */
    Q_INVOKABLE virtual QString formatPrimaryMoney(double iValue, int iForcedNbOfDigit = -1) const;

    /**
     * Get formatted money in primary unit
     * @param iValue value
     * @param iForcedNbOfDigit the number of digit (-1 means nb of digit of the unit)
     * @return formatted value in red or black
     */
    Q_INVOKABLE virtual QString formatSecondaryMoney(double iValue, int iForcedNbOfDigit = -1) const;

    /**
     * Get formatted percentage
     * @param iValue value
     * @param iInvertColors to set positive value in red and negative values in green
     * @return formatted value in red or black
     */
    Q_INVOKABLE virtual QString formatPercentage(double iValue, bool iInvertColors = false) const;

public Q_SLOTS:

    /**
     * Call the progress callstack.
     * @param iPosition the position in the current transaction.
     * The value must be between 0 and the value passed to beginTransaction.
     * @param iText the text to display. If empty then the name of the last transaction is used.
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError stepForward(int iPosition, const QString &iText = QString());

    /**
     * Start a transaction.
     * A transaction is needed to modify the SKGDocument.
     * This transaction is also used to manage the undo/redo.
     * @see endTransaction
     * @param iName the name of the transaction
     * @param iNbStep the number of step in this transaction.
     * It is used to call the progress callback.
     * @param iDate date of the transaction.
     * @param iRefreshViews at the end of the transaction, computed views will be recomputed.
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError beginTransaction(const QString &iName, int iNbStep = 0, const QDateTime &iDate = QDateTime::currentDateTime(), bool iRefreshViews = true);

    /**
     * Close the current transaction.
     * A transaction is needed to modify the SKGDocument.
     * This transaction is also used to manage the undo/redo.
     * @see beginTransaction
     * @param succeedded : true to indicate that current transaction has been successfully executed
     *                   : false to indicate that current transaction has failed
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError endTransaction(bool succeedded);

    /**
     * Remove all transactions of the history.
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError removeAllTransactions();

    /**
     * Send a message attached to the current transaction.
     * @param iMessage the message
     * @param iMessageType the message type
     * @param iAction the associated action
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError sendMessage(const QString &iMessage, SKGDocument::MessageType iMessageType = SKGDocument::Information, const QString &iAction = QString());

    /**
     * Change the password of the document.
     * WARNING: This method must NOT be used in a transaction.
     * @see beginTransaction
     * @param iNewPassword the new password
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError changePassword(const QString &iNewPassword);

    /**
     * Change the language of the document.
     * @param iLanguage the new language
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError setLanguage(const QString &iLanguage);

    /**
     * Initialize a new document.
     * WARNING: This method must NOT be used in a transaction.
     * @see endTransaction
     * @return an object managing the error
     *   @see SKGError
     */
    virtual SKGError initialize();

    /**
     * Recover a corrupted file.
     * WARNING: This method must NOT be used in a transaction.
     * @see endTransaction
     * @param iName the file name to load.
     * @param iPassword the password of the SKGDocument.
     * @param oRecoveredFile the recovered file.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError recover(const QString &iName, const QString &iPassword, QString &oRecoveredFile);

    /**
     * Load an existing document.
     * WARNING: This method must NOT be used in a transaction.
     * @see endTransaction
     * @param iName the file name to load.
     * @param iPassword the password of the SKGDocument.
     * @param iRestoreTmpFile restore the temporary file if existing.
     * @param iForceReadOnly force the read only mode.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError load(const QString &iName, const QString &iPassword = QString(), bool iRestoreTmpFile = false, bool iForceReadOnly = false);

    /**
     * Set the file not modified.
     */
    virtual void setFileNotModified() const;

    /**
     * save the current SKGDocument.
     * WARNING: This method must NOT be used in a transaction.
     * @see endTransaction.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError save();

    /**
     * save the current SKGDocument.
     * WARNING: This method must NOT be used in a transaction.
     * @see endTransaction
     * @param iName the file name to save.
     * @param iOverwrite to authorize the overwrite or not.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError saveAs(const QString &iName, bool iOverwrite = false);

    /**
     * close the current SKGDocument.
     * @param iDeletingDocument to know if the deletion of the document is ongoing.
     * @return an object managing the error.
     *   @see SKGError
     */
    SKGError close(bool iDeletingDocument = false);

    /**
     * Add a sql result in the cache. Cache is cleaned after each transaction
     * @param iKey the key
     * @param iValue the sql result
     */
    virtual void addSqlResultInCache(const QString &iKey, const SKGStringListList &iValue) const;

    /**
     * Add a value in the cache. Cache is cleaned after each transaction
     * @param iKey the key
     * @param iValue the value
     */
    virtual void addValueInCache(const QString &iKey, const QString &iValue) const;

    /**
     * Refresh the case.
     * @param iTable the modified table triggering the cache refresh.
     */
    virtual void refreshCache(const QString &iTable) const;

    /**
     * Set backup parameters.
     * The key word \<DATE\> is supported.
     * Call setBackupParameters() to avoid backup creation.
     * @param iPrefix the prefix for the backup file
     * @param iSuffix the suffix for backup file
     */
    void setBackupParameters(const QString &iPrefix = QString(), const QString &iSuffix = QString()) const;

Q_SIGNALS:
    /**
     * This signal is launched by endTransaction on all tables modified when a huge modification occurs on the model
     * @param iTableName the name of the modified table. iTableName="" if all tables must be refreshed
     * @param iIdTransaction the id of the transaction for direct modifications of the table (update of modify objects is enough)
     * @param iLightTransaction to if the transaction is light
     *or 0 in case of modifications by impact (full table must be refreshed)
     */
    void tableModified(const QString &iTableName, int iIdTransaction, bool iLightTransaction);

    /**
     * This signal is launched by endTransaction when a transaction is successfully ended
     * @param iIdTransaction the id of the transaction for direct modifications of the table (update of modify objects is enough)
     */
    void transactionSuccessfullyEnded(int iIdTransaction);

    /**
     * This signal is launched when the status of the document is changed (read only, modified)
     */
    void modified();

protected:
    /**
     * Drop views and indexes attached to a list of tables
     * @param iTables the list of tables.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError dropViewsAndIndexes(const QStringList &iTables) const;

    /**
     * Migrate the current SKGDocument to the latest version of the data model.
     * WARNING: This method must be used in a transaction.
     * @see beginTransaction
     * @param oMigrationDone to know if a migration has been done or not.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError migrate(bool &oMigrationDone);

    /**
     * Create dynamic triggers for "undoable" tables.
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError createUndoRedoTemporaryTriggers() const;

    /**
     * This list must contain a list of object
     * where the undo / redo is NOT applicable.
     * For a full table, the syntax is: T.nameofthetable
     * For an attribute, the syntax is: A.nameofthetable.nameoftheattribute
     */
    QStringList SKGListNotUndoable;

    /**
     * Get impacted views if one object of @p iTable is modifier.
     * @param iTable name of a table
     * @return impacted views
     */
    virtual QStringList getImpactedViews(const QString &iTable) const;

    /**
     * Compute all materialized views.
     * @param iTable Compute only materialized views linked to this table. If empty then compute all materialized views
     * @return an object managing the error.
     *   @see SKGError
     */
    virtual SKGError computeMaterializedViews(const QString &iTable = QString()) const;

private:
    Q_DISABLE_COPY(SKGDocument)

    SKGMessageList m_unTransactionnalMessages;
    SKGDocumentPrivate *d;
};
/**
 * Declare the class
 */
Q_DECLARE_TYPEINFO(SKGDocument, Q_COMPLEX_TYPE);
#endif
