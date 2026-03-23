/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGSERVICES_H
#define SKGSERVICES_H
/** @file
 * This file defines classes SKGServices.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qurl.h>

#include <qdatetime.h>
#include <qhash.h>
#include <qicon.h>
#include <qstringlist.h>

#include <kpluginmetadata.h>

#include "skgbasemodeler_export.h"
#include "skgdefine.h"
#include "skgerror.h"

/**
 * @var SQLLITEERROR
 * Must be used to generate a SKGError on an error coming from sqlite's API
 * Example of usage:
 * @code
 * int rc=QSqlDatabase_close ( currentDatabase );
 * if(rc!=SQLITE_OK) err=SKGError(SQLLITEERROR+rc, QSqlDatabase_errmsg(currentDatabase));
 * @see SKGError
 * @endcode

 */
static const int SQLLITEERROR = 10000;

class QSqlDatabase;
class QTextStream;
class QDomDocument;
class SKGDocument;

/**
 * A map of strings ==> SKGQStringQStringMap
 */
using SKGQStringQStringMap = QHash<QString, QString>;

/**
 * A list of QStringList ==> SKGStringListList
 */
using SKGStringListList = QList<QStringList>;

/**
 * A list of int ==> SKGIntList
 */
using SKGIntList = QList<int>;

/**
 * This class containing various static services
 */
class SKGBASEMODELER_EXPORT SKGServices : public QObject
{
    Q_OBJECT
public:
    /**
     * Unit value
     */
    struct SKGUnitInfo {
        /** The name of the unit */
        QString Name;
        /** The amount of the unit */
        double Value = 0.0;
        /** The number of decimal of the unit*/
        int NbDecimal = 2;

        /** The symbol of the unit */
        QString Symbol;
        /** The country of the unit */
        QString Country;
        /** The Internet code of the unit */
        QString Internet;
        /** The download source */
        QString Source;
        /** The parent unit of the unit */
        QString Parent;
        /** The date of the unit */
        QDate Date;
        /** To know if the unit is obsolete or not */
        bool Obsolete = false;
    };

    /**
     * This structure defines attributes types
     */
    enum AttributeType {
        TEXT, /**< Text */
        INTEGER, /**< Integer */
        FLOAT, /**< Float */
        DATE, /**< Date */
        ID, /**< Unique identifier */
        LINK, /**< Link */
        BLOB, /**< Blob */
        BOOL, /**< Boolean (Y, N) */
        TRISTATE, /**< Tri-state (N, P, C) */
        OTHER /**< Other or undefined */
    };

    /**
     * This enumerate defines attributes types
     */
    Q_ENUM(AttributeType)

    /**
     * Describe an attribute
     */
    struct SKGAttributeInfo {
        QString name; /**< The internal name of the attribute */
        QString display; /**< The name for display of the attribute */
        QIcon icon; /**< The icon for display of the attribute */
        AttributeType type; /**< The type of the attribute */
        bool notnull{}; /**< To know if the attribute can be null or not */
        QString defaultvalue; /**< The default value of the attribute */
    };

    /**
     * Search criteria
     */
    struct SKGSearchCriteria {
        QChar mode; /**< The mode '+' or '-' */
        QStringList words; /**< The list of words */
    };

    /**
     * A list of SKGSearchCriteria ==> SKGAttributesList
     */
    using SKGSearchCriteriaList = QVector<SKGSearchCriteria>;

    /**
     * A list of SKGAttributeInfo ==> SKGAttributesList
     */
    using SKGAttributesList = QVector<SKGAttributeInfo>;

    /**
     * This enumerate defines dump modes
     */
    enum DumpMode {
        DUMP_CSV, /**< To dump in CSV mode */
        DUMP_TEXT /**< To dump in DUMP_TEXT mode */
    };

    /**
     * This enumerate defines dump modes
     */
    Q_ENUM(DumpMode)

    /**
     * Get list of search criteria of a string (example: +cat1 -abc +auto)
     * @param iString a string to analyze
     * @return the search criteria list
     */
    static SKGServices::SKGSearchCriteriaList stringToSearchCriterias(const QString &iString);

    /**
     * Compute a SQL where clause with criteria and list of attributes
     * @param iSearchCriterias the list of criteria
     * @param iAttributes the list of attributes
     * @param iDocument if a document is given, this document will be used to find the synonym of attributes
     * @param iForDisplay to build the human readable string explaining the query
     * @return the where clause
     */
    static QString searchCriteriasToWhereClause(const SKGServices::SKGSearchCriteriaList &iSearchCriterias,
                                                const QStringList &iAttributes,
                                                const SKGDocument *iDocument,
                                                bool iForDisplay = false);

    /**
     * Get environment variable
     * @param iAttribute name of the variable
     * @return value of the variable
     */
    static QString getEnvVariable(const QString &iAttribute);

    /**
     * Convert a integer into a QString
     * @param iNumber the integer to convert
     * @return the converted QString
     */
    static QString intToString(qlonglong iNumber);

    /**
     * Convert a QString into an integer
     * @param iNumber the QString to convert
     * @return the converted integer
     */
    static qlonglong stringToInt(const QString &iNumber);

    /**
     * Convert a QString into a csv QString
     * @param iNumber the QString to convert
     * @return the converted csv string
     */
    static QString stringToCsv(const QString &iNumber);

    /**
     * Convert a QStringList into a csv QString
     * @param iList the QStringList to convert
     * @param iSeparator the separator
     * @return the converted csv string
     */
    static QString stringsToCsv(const QStringList &iList, QChar iSeparator = u';');

    /**
     * Convert a QString to a html QString
     * @param iString the QString to convert
     * @return the converted html string
     */
    static QString stringToHtml(const QString &iString);

    /**
     * Convert a html QString to a QString
     * @param iString the QString to convert
     * @return the converted string
     */
    static QString htmlToString(const QString &iString);

    /**
     * Convert a double into a QString
     * @param iNumber the double to convert
     * @return the converted QString
     */
    static QString doubleToString(double iNumber);

    /**
     * Convert a QString into an double
     * @param iNumber the QString to convert
     * @param iUseLocale try to use the locale
     * @return the converted double
     */
    static double stringToDouble(const QString &iNumber, bool iUseLocale = false);

    /**
     * Convert a QDateTime into a QString ready for sql order.
     * The output format is "%Y-%m-%d %H-%M-%S"
     * @param iDateTime the time
     * @return the converted QString
     */
    static QString timeToString(const QDateTime &iDateTime);

    /**
     * Convert a date in a period
     * @param iDate the time
     * @param iPeriod the period.
     * D for day, example: 2013-05-21
     * W for week, example: 2013-W10
     * M for month, example: 2013-05
     * Q for quarter, example: 2013-Q2
     * S for semester, example: 2013-S1
     * Y for year, example: 2013
     * @return the period string
     */
    // cppcheck-suppress passedByValue
    static QString dateToPeriod(QDate iDate, const QString &iPeriod);

    /**
     * Compute the number of working days between two dates.
     * @param iFrom first date
     * @param iTo seconde date
     * @return the number of working days.
     */
    // cppcheck-suppress passedByValue
    static int nbWorkingDays(QDate iFrom, QDate iTo);

    /**
     * Convert a QString to QDateTime.
     * @param iDateString the time. The format must be "%Y-%m-%d %H-%M-%S" or "%Y-%m-%d"
     * @return the converted QDateTime
     */
    static QDateTime stringToTime(const QString &iDateString);

    /**
     * Convert a QString to QDate.
     * @param iDateString the date. The format must be "%Y-%m-%d"
     * @return the converted QDate
     */
    static QDate stringToDate(const QString &iDateString);

    /**
     * Convert a QString to QDate.
     * @param iDateString the date string. Could be a partial date like 1/12.
     * @param iFixupBackward the date string is fixed in backward mode.
     * @return the converted QDate
     */
    static QDate partialStringToDate(const QString &iDateString, bool iFixupBackward = true);

    /**
     * Convert a QDateTime into a QString ready for sql order.
     * The output format is "%Y-%m-%d"
     * @param iDateTime the time
     * @return the converted QString
     */
    static QString dateToSqlString(const QDateTime &iDateTime);

    /**
     * Convert a QDate into a QString ready for sql order.
     * The output format is "%Y-%m-%d"
     * @param iDate the date
     * @return the converted QString
     */
    // cppcheck-suppress passedByValue
    static QString dateToSqlString(QDate iDate);

    /**
     * Format a date.
     * The output format is "YYYY-MM-DD"
     * @param iDate the QString representing the input date
     * @param iFormat the format of the input date. Must be in the following list:
     * "YYYYMMDD","DDMMYYYY", "MMDDYYYY" "YYYY-MM-DD"
     * "MM/DD/YY", "MM-DD-YY", "DD/MM/YY", "DD-MM-YY"
     * "MM/DD/YYYY", "MM-DD-YYYY", "DD/MM/YYYY", "DD-MM-YYYY"
     * "DD/MMM/YY" "DD-MMM-YY" DDMMMYY
     * "DD/MMM/YYYY" "DD-MMM-YYYY" DDMMMYYYY
     * @return the converted QString
     */
    static QString dateToSqlString(const QString &iDate, const QString &iFormat);

    /**
     * Get the sql where clause corresponding to a period date.
     * @param iPeriod the period date (some thing like "ALL" "2014" or "2014-04" or "2014-Q2" or "2014-S2"
     * @param iDateAttribute the date attribute
     * @param iComparator the sql comparator (= by default. But you can use < or >)
     * @return the where clause
     */
    static QString
    getPeriodWhereClause(const QString &iPeriod, const QString &iDateAttribute = QStringLiteral("d_date"), const QString &iComparator = QStringLiteral("="));

    /**
     * Get the last date of a period.
     * @param iPeriod the period date
     * @return the last date
     */
    static QDate periodToDate(const QString &iPeriod);

    /**
     * Get the neighboring period.
     * @param iPeriod the period date (some thing like "ALL" "2014" or "2014-04" or "2014-Q2" or "2014-S2"
     * @param iDelta the delta to apply (-1 = previous, +1 = next)
     * @return the neighboring period
     */
    static QString getNeighboringPeriod(const QString &iPeriod, int iDelta = -1);

    /**
     * Get the format of a list of string dates
     * @param iDates a list of string dates
     * @return the supported format of all of these dates.
     */
    static QString getDateFormat(const QStringList &iDates);

    /**
     * Get the string representing the percentage
     * @param iAmount the amount
     * @param iNbDecimal the number of decimals
     * @return the string.
     */
    static QString toPercentageString(double iAmount, int iNbDecimal = 2);

    /**
     * Get the string representing the amount
     * @param iAmount the amount
     * @param iSymbol the symbol
     * @param iNbDecimal the number of decimals
     * @return the string.
     */
    static QString toCurrencyString(double iAmount, const QString &iSymbol = QString(), int iNbDecimal = 2);

    /**
     * Return string ready for sql order
     * @param iString the string
     * @return the escaped string
     */
    static QString stringToSqlString(const QString &iString);

    /**
     * Split a csv line
     * @param iString the csv line
     * @param iSeparator the csv separator
     * @param iCoteDefineBlock to indicate if the character double cote define a block
     * @param oRealSeparator the real csv separator. If the csv string is strictly all enclosed then oRealSeparator will contain the real separator. Can be
     * nullptr
     * @return the list of all values in this csv line. Return an empty list if some cote are opened and not closed
     */
    static QStringList splitCSVLine(const QString &iString, QChar iSeparator, bool iCoteDefineBlock, QChar *oRealSeparator);

    /**
     * Split a csv line
     * @param iString the csv line
     * @param iSeparator the csv separator
     * @param iCoteDefineBlock to indicate if the character double cote define a block
     * @return the list of all values in this csv line. Return an empty list if some cote are opened and not closed
     */
    static QStringList splitCSVLine(const QString &iString, QChar iSeparator = u';', bool iCoteDefineBlock = true);

    /**
     * To dump a table
     * @param iTable A table
     * @param iMode dump mode
     * @return the dump of the table
     */
    static QStringList tableToDump(const SKGStringListList &iTable, SKGServices::DumpMode iMode);

    /**
     * Get the table name corresponding to a table or a view
     * @param iTable table or view
     * @return table name corresponding
     */
    static QString getRealTable(const QString &iTable);

    /**
     * Copy a sqlite database from memory to file or from file to memory.
     * It is used to do a load or a save
     * @param iFileDb the sqlite pointer corresponding to a file database
     * @param iMemoryDb the sqlite pointer corresponding to  memory database
     * @param iFromFileToMemory
     *        true: the copy is done from iFileDb to iMemoryDb (needed of a load)
     *        false: the copy is done from iMemoryDb to iFileDb (needed of a save)
     * @param iPassword the password
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError
    copySqliteDatabase(const QSqlDatabase &iFileDb, const QSqlDatabase &iMemoryDb, bool iFromFileToMemory, const QString &iPassword = QString());

    /**
     * Copy a sqlite database into an XML document.
     * @param iDb the sqlite pointer corresponding to database
     * @param oDocument the xml document
     * @param oDocument the xml document
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError copySqliteDatabaseToXml(const QSqlDatabase &iDb, QDomDocument &oDocument, QVector<QString> *iIgnore = nullptr);

    /**
     * Execute a sqlite orders
     * @param iDb a database pointer
     * @param iSqlOrders the sql orders
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError executeSqliteOrders(const QSqlDatabase &iDb, const QStringList &iSqlOrders);

    /**
     * Execute a sqlite order
     * @param iDb a database pointer
     * @param iSqlOrder the sql order
     * @param iBind the binded variables and values
     * @param iLastId to retrieve the id of the last created object. Can be nullptr
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError executeSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, const QMap<QString, QVariant> &iBind, int *iLastId);

    /**
     * Execute a sqlite order
     * @param iDb a database pointer
     * @param iSqlOrder the sql order
     * @param iLastId to retrieve the id of the last created object. Can be nullptr
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError executeSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, int *iLastId = nullptr);

    /**
     * Execute a select sqlite order and return the result in @p oResult
     * @param iDb A database pointer
     * @param iSqlOrder the sql order
     * @param oResult the result of the select. It is a vector of vector of QString
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError executeSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, SKGStringListList &oResult);

    /**
     * Execute a select sqlite order returning one value and return the result in @p oResult
     * @param iDb A database pointer
     * @param iSqlOrder the sql order
     * @param oResult the result of the select
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError executeSingleSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QString &oResult);

    /**
     * dump a select sqlite order
     * @param iDb A database pointer
     * @param iSqlOrder the sql order
     * @param iMode dump mode
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, SKGServices::DumpMode iMode = DUMP_TEXT);

    /**
     * dump a select sqlite order
     * @param iDb A database pointer
     * @param iSqlOrder the sql order
     * @param oStream the output stream, nullptr for std output (cout)
     * @param iMode dump mode
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QTextStream *oStream, SKGServices::DumpMode iMode = DUMP_TEXT);

    /**
     * dump a select sqlite order
     * @param iDb A database pointer
     * @param iSqlOrder the sql order
     * @param oResult the output
     * @param iMode dump mode
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QString &oResult, SKGServices::DumpMode iMode = DUMP_TEXT);

    /**
     * dump a select sqlite order
     * @param iDb A database pointer
     * @param iSqlOrder the sql order
     * @param oResult the output
     * @param iMode dump mode
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError dumpSelectSqliteOrder(const QSqlDatabase &iDb, const QString &iSqlOrder, QStringList &oResult, SKGServices::DumpMode iMode = DUMP_TEXT);

    /**
     * Read a property file
     * @param iFileName A file name
     * @param oProperties the properties
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError readPropertyFile(const QString &iFileName, QHash<QString, QString> &oProperties);

    /**
     * Return the current time in millisecond
     * @return current
     */
    static double getMicroTime();

    /**
     * Encrypt or decrypt a file
     * @param iFileSource source file
     * @param iFileTarget target file
     * @param iPassword password
     * @param iEncrypt true to encrypt, false to decrypt
     * @param iHeaderFile the header file (useful for a magic mime type)
     * @param oModeSQLCipher to know if the file is in SQLite or SQLCipher mode
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError cryptFile(const QString &iFileSource,
                              const QString &iFileTarget,
                              const QString &iPassword,
                              bool iEncrypt,
                              const QString &iHeaderFile,
                              bool &oModeSQLCipher);

    /**
     * Download a non local URL to a string
     * @param iSourceUrl the non local URL
     * @param oStream the downloaded string
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError downloadToStream(const QUrl &iSourceUrl, QByteArray &oStream);

    /**
     * Download a non local URL to a temporary file
     * @param iSourceUrl the non local URL
     * @param oTemporaryFile a temporary file. You have to delete it
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError download(const QUrl &iSourceUrl, QString &oTemporaryFile);

    /**
     * Upload from a source to a destination
     * @param iSourceUrl the source
     * @param iDescUrl the destination
     * @return An object managing the error
     *   @see SKGError
     */
    static SKGError upload(const QUrl &iSourceUrl, const QUrl &iDescUrl);

    /**
     * To enable, disable sql traces
     */
    static int SKGSqlTraces;

    /**
     * Create a table with all values expressed in % of the column
     * @param iTable table
     * @param iOfColumns false to compute percentage of lines, true to compute perceentage of columns
     * @param iAbsolute transform all values in absolute
     * @return the table
     */
    static SKGStringListList getPercentTable(const SKGStringListList &iTable, bool iOfColumns = true, bool iAbsolute = false);

    /**
     * Create a table in base 100
     * @param iTable table
     * @return the table
     */
    static SKGStringListList getBase100Table(const SKGStringListList &iTable);

    /**
     * Create an historized table.
     * Each column is the sum of previous ones
     * @param iTable table
     * @return the table
     */
    static SKGStringListList getHistorizedTable(const SKGStringListList &iTable);

    /**
     * Encode a string for an url
     * @param iString the decoded string
     * @return the encoded string
     */
    static QString encodeForUrl(const QString &iString);

    /**
     * Return the icon form theme with expected overlays
     *
     * @param iName The name of the icon
     * @param iOverlays List of overlays
     * @return The icon
     */
    static QIcon fromTheme(const QString &iName, const QStringList &iOverlays = QStringList());

    /**
     * Get a major version (ex: 4.3) of a version (ex: 4.3.1)
     * @param iVersion the version
     * @return the major version
     */
    static QString getMajorVersion(const QString &iVersion);

    /**
     * Get a full path command line
     * @param iCommandLine command line
     * @return the full path command line
     */
    static QString getFullPathCommandLine(const QString &iCommandLine);

    /**
     * Get the next string
     * @param iString the string
     * @return the next string
     */
    static QString getNextString(const QString &iString);

    /**
     * Search data plugin
     * @param type the type of plugins
     * @return the plugins found
     */
    static QVector<KPluginMetaData> findDataPlugins(const QString &type);

private:
    Q_DISABLE_COPY(SKGServices)
    static SKGError m_lastCallbackError;
};
#endif
