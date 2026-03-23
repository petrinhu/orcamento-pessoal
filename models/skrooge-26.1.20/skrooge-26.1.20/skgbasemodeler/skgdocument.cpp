/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGDocument.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdocument.h"

#include <kcolorscheme.h>

#include <qapplication.h>
#ifdef SKG_DBUS
#include <qdbusconnection.h>
#endif
#include <qdir.h>
#include <qfile.h>
#include <qhash.h>
#include <qicon.h>
#include <qjsondocument.h>
#include <qprocess.h>
#include <qregularexpression.h>
#include <qsqldatabase.h>
#include <qsqldriver.h>
#include <qsqlerror.h>
#include <qsqlquery.h>
#include <qtconcurrentrun.h>
#include <qthread.h>
#include <qurl.h>
#include <quuid.h>
#include <qvariant.h>

#include <cmath>
#include <sqlite3.h>
#include <utility>

#include "skgdocumentprivate.h"
#include "skgerror.h"
#include "skgpropertyobject.h"
#include "skgreport.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

#define SQLDRIVERNAME QStringLiteral("SKGSQLCIPHER")

/**
 * Custom sqlite function.
 */
static void sleepFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);

    if (Q_LIKELY(data1)) {
        auto s = SKGServices::stringToInt(QString(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar)));
        QThread::sleep(s);

        sqlite3_result_text(context, "OK", 2, SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void periodFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        const auto &date = SKGServices::stringToDate(QString(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar)));
        const auto &format = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar)).toUpper();
        const auto &period = SKGServices::dateToPeriod(date, format);
        QByteArray output = period.toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void formattedDateFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        QString string(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
        const auto &format = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar));

        const auto &date = QDate::fromString(string, QStringLiteral("yyyy-MM-dd")).toString(format);

        QByteArray output = date.toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void dateFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        QString string(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
        const auto &format = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar));

        auto date = SKGServices::dateToSqlString(string, format).trimmed();
        if (date.isEmpty()) {
            date = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
        }

        QByteArray output = date.toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void weekYearFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);

    if (Q_LIKELY(data1)) {
        QString string(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));

        const auto &date = SKGServices::dateToPeriod(SKGServices::stringToDate(string), QStringLiteral("W"));

        QByteArray output = date.toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void currencyFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        auto string = SKGServices::stringToDouble(QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar)));
        const auto &symbol = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar));

        const auto &currency = SKGServices::toCurrencyString(string, symbol);

        QByteArray output = currency.toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void xorFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        auto string = QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar)).toUtf8();
        auto key = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar)).toUtf8();

        if (string.startsWith(QByteArray("# "))) {
            // Decrypt
            string = QByteArray::fromHex(string.right(string.length() - 2));
            QByteArray estring;
            for (int i = 0; i < string.size(); ++i) {
                estring += static_cast<char>(string[i] ^ key[i % key.size()]);
            }
            QByteArray output = estring;
            sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
        } else {
            // Encrypt
            QByteArray estring;
            for (int i = 0; i < string.size(); ++i) {
                estring += static_cast<char>(string[i] ^ key[i % key.size()]);
            }
            QByteArray output = "# " + estring.toHex();
            sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
        }
    }
}

static void xordoubleFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        auto d = SKGServices::stringToDouble(QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar)).toUtf8());
        auto key = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar)).toUtf8();
        int kk = 0;
        for (int i = 0; i < key.size(); ++i) {
            kk += key[i];
        }
        QByteArray output = SKGServices::doubleToString(static_cast<double>(kk) - d).toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void wordFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        QString string1(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
        string1 = string1.simplified();
        static QRegularExpression re(QStringLiteral("(\\w+)"));
        QRegularExpressionMatchIterator i = re.globalMatch(string1);
        QStringList list;
        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString word = match.captured(1);
            list << word;
        }

        int pos = SKGServices::stringToInt(QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar)));
        if (pos == 0) {
            pos = 1;
        } else if (pos > list.count()) {
            pos = list.count();
        } else if (pos < -list.count()) {
            pos = 1;
        } else if (pos < 0) {
            pos = list.count() + pos + 1;
        }

        QByteArray output = list[pos - 1].toUtf8();

        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void wildcardFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        QString string1(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
        QString string2 = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar));

        QRegularExpression pattern(QRegularExpression::anchoredPattern(QRegularExpression::wildcardToRegularExpression(string1)),
                                   QRegularExpression::CaseInsensitiveOption);
        if (pattern.isValid()) {
            sqlite3_result_int(context, static_cast<int>(pattern.match(string2).hasMatch()));
        } else {
            sqlite3_result_error(context, pattern.errorString().toUtf8().constData(), -1);
        }
    }
}

/**
 * Custom sqlite function.
 */
static void regexpFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);

    if (Q_LIKELY(data1 && data2)) {
        QString string1(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
        QString string2 = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar));

        QRegularExpression pattern(QRegularExpression::anchoredPattern(string1), QRegularExpression::CaseInsensitiveOption);
        if (pattern.isValid()) {
            sqlite3_result_int(context, static_cast<int>(pattern.match(string2).hasMatch()));
        } else {
            sqlite3_result_error(context, pattern.errorString().toUtf8().constData(), -1);
        }
    }
}

/**
 * Custom sqlite function.
 */
static void regexpCaptureFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);
    int len2 = sqlite3_value_bytes16(argv[1]);
    const void *data2 = sqlite3_value_text16(argv[1]);
    int len3 = sqlite3_value_bytes16(argv[2]);
    const void *data3 = sqlite3_value_text16(argv[2]);
    if (Q_LIKELY(data1 && data2 && data3)) {
        int pos = SKGServices::stringToInt(QString::fromRawData(reinterpret_cast<const QChar *>(data3), len3 / sizeof(QChar)));

        QString string1(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
        QString string2 = QString::fromRawData(reinterpret_cast<const QChar *>(data2), len2 / sizeof(QChar));

        QRegularExpression pattern(string1, QRegularExpression::CaseInsensitiveOption);
        if (pattern.isValid()) {
            auto match = pattern.match(string2);
            QByteArray output = match.capturedTexts().value(pos).toUtf8();
            sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
        } else {
            sqlite3_result_error(context, pattern.errorString().toUtf8().constData(), -1);
        }
    }
}

/**
 * Custom sqlite function.
 */
static void upperFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);

    if (Q_LIKELY(data1)) {
        QByteArray output = QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar)).toUpper().toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void nextFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);

    if (Q_LIKELY(data1)) {
        QByteArray output = SKGServices::getNextString(QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar))).toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void lowerFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);

    if (Q_LIKELY(data1)) {
        QByteArray output = QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar)).toLower().toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

/**
 * Custom sqlite function.
 */
static void capitalizeFunction(sqlite3_context *context, int /*argc*/, sqlite3_value **argv)
{
    int len1 = sqlite3_value_bytes16(argv[0]);
    const void *data1 = sqlite3_value_text16(argv[0]);

    if (Q_LIKELY(data1)) {
        QString str = QString::fromRawData(reinterpret_cast<const QChar *>(data1), len1 / sizeof(QChar));
        QByteArray output = (str.at(0).toUpper() + str.mid(1).toLower()).toUtf8();
        sqlite3_result_text(context, output.constData(), output.size(), SQLITE_TRANSIENT);
    }
}

static SKGError addSqliteAddon(QSqlDatabase *iDb)
{
    SKGError err;
    auto *sqlite_handle = iDb->driver()->handle().value<sqlite3 *>();
    if (sqlite_handle != nullptr) {
        sqlite3_create_function(sqlite_handle, "REGEXP", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &regexpFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "REGEXPCAPTURE", 3, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &regexpCaptureFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "WILDCARD", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &wildcardFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "WORD", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &wordFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "TODATE", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &dateFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "TOWEEKYEAR", 1, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &weekYearFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "TOFORMATTEDDATE", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &formattedDateFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "PERIOD", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &periodFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "SLEEP", 1, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &sleepFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "TOCURRENCY", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &currencyFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "XOR", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &xorFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "XORD", 2, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &xordoubleFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "UPPER", 1, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &upperFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "LOWER", 1, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &lowerFunction, nullptr, nullptr);
        sqlite3_create_function(sqlite_handle, "NEXT", 1, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &nextFunction, nullptr, nullptr);
        int rc = sqlite3_create_function(sqlite_handle, "CAPITALIZE", 1, SQLITE_UTF16 | SQLITE_DETERMINISTIC, nullptr, &capitalizeFunction, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            err = SKGError(SQLLITEERROR + rc, QStringLiteral("sqlite3_create_function failed"));
        }
    } else {
        SKGTRACE << "WARNING: Custom sqlite functions not added" << Qt::endl;
    }
    return err;
}

SKGDocument::SKGDocument()
    : d(new SKGDocumentPrivate())
{
    SKGTRACEINFUNC(10)
    // Set the QThreadPool
    // QThreadPool::globalInstance()->setMaxThreadCount(3*QThread::idealThreadCount());

    // DBUS registration
#ifdef SKG_DBUS
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerObject(QStringLiteral("/skg/skgdocument"), this, QDBusConnection::ExportAllContents);
    dbus.registerService(QStringLiteral("org.skg"));
#endif

    // Initialisation of undoable tables
    SKGListNotUndoable.push_back(QStringLiteral("T.doctransaction"));
    SKGListNotUndoable.push_back(QStringLiteral("T.doctransactionitem"));
    SKGListNotUndoable.push_back(QStringLiteral("T.doctransactionmsg"));

    // Database unique identifier
    ++SKGDocumentPrivate::m_databaseUniqueIdentifier;
    d->m_databaseIdentifier = "SKGDATABASE_" % SKGServices::intToString(SKGDocumentPrivate::m_databaseUniqueIdentifier);

    // Initialisation of backup file parameters
    setBackupParameters(QString(), QStringLiteral(".old"));

    // 320157 vvvv
    // Disable OS lock
    sqlite3_vfs *vfs = sqlite3_vfs_find("unix-none");
    if (Q_LIKELY(vfs)) {
        sqlite3_vfs_register(vfs, 1);
    } else {
        SKGTRACE << "WARNING: Impossible to use the 'unix-none' vfs mode of sqlite3. Use:'" << sqlite3_vfs_find(nullptr)->zName << "'" << Qt::endl;
    }
    // 320157 ^^^^
}

SKGDocument::~SKGDocument()
{
    SKGTRACEINFUNC(10)
    close(true);
    d->m_progressFunction = nullptr;
    d->m_progressData = nullptr;
    d->m_checkFunctions.clear();

    for (auto w : std::as_const(d->m_watchers)) {
        delete w;
    }
    d->m_watchers.clear();

    delete d->m_cacheSql;
    d->m_cacheSql = nullptr;

    delete d;
    d = nullptr;
}

QString SKGDocument::getUniqueIdentifier() const
{
    return d->m_uniqueIdentifier;
}

QString SKGDocument::getDatabaseIdentifier() const
{
    return d->m_databaseIdentifier;
}

SKGError SKGDocument::setProgressCallback(FuncProgress iProgressFunction, void *iData)
{
    d->m_progressFunction = std::move(iProgressFunction);
    d->m_progressData = iData;
    return SKGError();
}

SKGError SKGDocument::addEndOfTransactionCheck(SKGError (*iCheckFunction)(SKGDocument *))
{
    d->m_checkFunctions.append(iCheckFunction);
    return SKGError();
}

SKGError SKGDocument::stepForward(int iPosition, const QString &iText)
{
    SKGError err;

    // Increase the step for the last transaction
    if (Q_LIKELY(getDepthTransaction())) {
        d->m_posStepForTransaction.pop_back();
        d->m_posStepForTransaction.push_back(iPosition);
    }

    // Check if a callback function exists
    if (Q_LIKELY(d->m_progressFunction)) {
        // YES ==> compute
        double min = 0;
        double max = 100;

        bool emitevent = true;
        auto nbIt = d->m_nbStepForTransaction.constBegin();
        auto posIt = d->m_posStepForTransaction.constBegin();
        for (; emitevent && nbIt != d->m_nbStepForTransaction.constEnd(); ++nbIt) {
            int p = *posIt;
            int n = *nbIt;
            if (Q_UNLIKELY(p < 0 || p > n)) {
                p = n;
            }

            if (Q_LIKELY(n != 0)) {
                double pmin = min;
                double pmax = max;
                min = pmin + (pmax - pmin) * (static_cast<double>(p) / static_cast<double>(n));
                max = pmin + (pmax - pmin) * (static_cast<double>(p + 1) / static_cast<double>(n));
                if (Q_UNLIKELY(max > 100)) {
                    max = 100;
                }
            } else {
                emitevent = false;
            }

            ++posIt;
        }

        int posPercent = rint(min);

        // Call the call back
        if (emitevent) {
            d->m_inProgress = true;
            QString text;
            qint64 time = QDateTime::currentMSecsSinceEpoch() - d->m_timeBeginTransaction;
            if (Q_UNLIKELY(time > 3000)) {
                text = iText;
                if (text.isEmpty()) {
                    text = d->m_nameForTransaction.at(d->m_nameForTransaction.count() - 1);
                }
            }
            if (Q_LIKELY(d->m_progressFunction(posPercent, time, text, d->m_progressData) != 0)) {
                err.setReturnCode(ERR_ABORT).setMessage(
                    i18nc("User interrupted something that Skrooge was performing", "The current transaction has been interrupted"));

                // Remove all untransactionnal messaged
                m_unTransactionnalMessages.clear();
            }
            d->m_inProgress = false;
        }
    }
    return err;
}

SKGError SKGDocument::beginTransaction(const QString &iName, int iNbStep, const QDateTime &iDate, bool iRefreshViews)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    SKGTRACEL(10) << "Input parameter [name]=[" << iName << "]  [nb step]=[" << iNbStep << "]  [refresh]=["
                  << (iRefreshViews ? QStringLiteral("Y") : QStringLiteral("N")) << ']' << Qt::endl;
    bool overrideCursor = false;
    if (d->m_nbStepForTransaction.isEmpty()) {
        // Open SQLtransaction
        err = executeSqliteOrder(QStringLiteral("BEGIN;"));
        IFOK(err)
        {
            overrideCursor = true;

            // Create undo redo transaction
            err = executeSqliteOrder(QStringLiteral("insert into doctransaction (d_date, t_name, i_parent") %
                                     (!iRefreshViews ? ", t_refreshviews" : "") %
                                     ") values "
                                     "('" % SKGServices::timeToString(iDate) %
                                     "','" % SKGServices::stringToSqlString(iName) %
                                     "', " % SKGServices::intToString(getTransactionToProcess(SKGDocument::UNDO)) %
                                     (!iRefreshViews ? ",'N'" : "") %
                                     ");");
            addValueInCache(QStringLiteral("SKG_REFRESH_VIEW"), (iRefreshViews ? QStringLiteral("Y") : QStringLiteral("N")));
            d->m_currentTransaction = getTransactionToProcess(SKGDocument::UNDO);
            d->m_timeBeginTransaction = QDateTime::currentMSecsSinceEpoch();
        }
    } else {
        // A transaction already exists
        // Check if the child transaction is a opened in the progress callback
        if (d->m_inProgress) {
            err.setReturnCode(ERR_FAIL).setMessage(
                i18nc("Something went wrong with SQL transactions", "A transaction cannot be started during execution of another one"));
        }
    }
    IFOK(err)
    {
        d->m_nbStepForTransaction.push_back(iNbStep);
        d->m_posStepForTransaction.push_back(iNbStep);
        QString n = iName;
        n = n.remove(QStringLiteral("#INTERNAL#"));
        if (n.isEmpty() && !d->m_nameForTransaction.isEmpty()) {
            n = d->m_nameForTransaction.at(d->m_nameForTransaction.count() - 1);
        }
        d->m_nameForTransaction.push_back(n);

        if (iNbStep > 0) {
            err = stepForward(0);
        }
    }
    else
    {
        executeSqliteOrder(QStringLiteral("ROLLBACK;"));
    }

    if (Q_LIKELY(overrideCursor && !err && qobject_cast<QGuiApplication *>(qApp) != nullptr)) { // clazy:excludeall=unneeded-cast
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    }

    return err;
}

SKGError SKGDocument::checkExistingTransaction() const
{
    SKGError err;
    if (d->m_nbStepForTransaction.isEmpty()) {
        err.setReturnCode(ERR_ABORT).setMessage(i18nc("Something went wrong with SQL transactions", "A transaction must be opened to do this action"));
    }
    return err;
}

SKGError SKGDocument::endTransaction(bool succeedded)
{
    SKGError err;
    SKGError errOverwritten;
    SKGTRACEINFUNCRC(5, err)
    if (Q_UNLIKELY(d->m_nbStepForTransaction.empty())) {
        err.setReturnCode(ERR_ABORT).setMessage(
            i18nc("Something went wrong with SQL transactions", "Closing transaction failed because too many transactions ended"));
    } else {
        stepForward(d->m_nbStepForTransaction.at(d->m_nbStepForTransaction.count() - 1)); // =100%
        if (Q_LIKELY(d->m_nbStepForTransaction.size())) { // This test is needed. It is a security in some cases.
            d->m_nbStepForTransaction.pop_back();
            d->m_posStepForTransaction.pop_back();
            d->m_nameForTransaction.pop_back();
        }
        QString currentTransactionString = SKGServices::intToString(getCurrentTransaction());

        if (d->m_nbStepForTransaction.empty()) {
            QStringList listModifiedTables;

            // Check
            if (succeedded) {
                auto cachepointer = d->m_cache;
                d->m_cache = QHash<QString, QString>();

                for (auto check : std::as_const(d->m_checkFunctions)) {
                    errOverwritten = check(this);
                    IFKO(errOverwritten)
                    {
                        succeedded = false;
                        SKGTRACEL(5) << "Transaction cancelled by a check" << Qt::endl;
                        break;
                    }
                }

                d->m_cache = std::move(cachepointer);
            }

            if (succeedded) {
                // Link items on current transaction
                IFOK(err)
                {
                    err =
                        executeSqliteOrder("UPDATE doctransactionitem set rd_doctransaction_id=" % currentTransactionString % " WHERE rd_doctransaction_id=0;");
                }

                // Optimization of the current transaction
                IFOK(err)
                {
                    SKGStringListList listTmp;
                    err = executeSelectSqliteOrder("SELECT count(1) FROM doctransactionitem where rd_doctransaction_id=" % currentTransactionString, listTmp);
                    IFOK(err)
                    {
                        int nbItem = SKGServices::stringToInt(listTmp.at(1).at(0));
                        if (nbItem == 0) {
                            // Optimization is needed
                            // Get non hidden messages
                            SKGMessageList messages;
                            getMessages(getCurrentTransaction(), messages, false);

                            // Delete current transaction
                            err = executeSqliteOrder("DELETE FROM doctransaction WHERE id=" % currentTransactionString);

                            int nb = messages.count();
                            for (int i = 0; i < nb; ++i) {
                                m_unTransactionnalMessages.push_back(messages.at(i));
                            }
                        }
                    }
                }

                // Optimization 2: remove duplicate orders
                IFOK(err)
                {
                    QString wc = "DELETE FROM doctransactionitem WHERE id IN "
                                 "(SELECT a.id FROM doctransactionitem a INDEXED BY idx_doctransactionitem_optimization, doctransactionitem b INDEXED BY idx_doctransactionitem_optimization "
                                 "WHERE a.rd_doctransaction_id=" % currentTransactionString % " AND b.rd_doctransaction_id=" % currentTransactionString %
                                 " AND a.i_object_id=b.i_object_id AND a.t_object_table=b.t_object_table AND b.t_action=a.t_action AND b.t_sqlorder=a.t_sqlorder AND a.id>b.id );";
                    err = executeSqliteOrder(wc);
                }

                // Get current transaction information to be able to emit event in case of SKG_UNDO_MAX_DEPTH=0
                IFOK(err)
                {
                    err = this->getDistinctValues(QStringLiteral("doctransactionitem"),
                                                  QStringLiteral("t_object_table"),
                                                  "rd_doctransaction_id=" % currentTransactionString,
                                                  listModifiedTables);
                }

                // Remove oldest transaction
                IFOK(err)
                {
                    QString maxdepthstring = getParameter(QStringLiteral("SKG_UNDO_MAX_DEPTH"));
                    if (maxdepthstring.isEmpty()) {
                        maxdepthstring = QStringLiteral("-1");
                    }
                    int maxdepth = SKGServices::stringToInt(maxdepthstring);
                    if (maxdepth >= 0) {
                        err = executeSqliteOrder(
                            "delete from doctransaction where id in (select id from doctransaction limit max(0,((select count(1) from doctransaction)-("
                            % maxdepthstring % "))))");
                    }
                }

                // Remove SKGDocument::REDO transactions if we are not in a undo / redo transaction
                if (d->m_inundoRedoTransaction == 0) {
                    int i = 0;
                    while (((i = getTransactionToProcess(SKGDocument::REDO)) != 0) && !err) {
                        err = executeSqliteOrder("delete from doctransaction where id=" % SKGServices::intToString(i));
                    }
                }

                // Commit the transaction
                IFOK(err)
                {
                    err = executeSqliteOrder(QStringLiteral("COMMIT;"));
                }
            }

            // clean cache sql (must be done before event emit)
            d->m_cacheSql->clear();

            if (!succeedded || err) {
                // Rollback the transaction
                SKGError err2 = executeSqliteOrder(QStringLiteral("ROLLBACK;"));
                // delete the transaction
                IFOKDO(err2, executeSqliteOrder("delete from doctransaction where id=" % currentTransactionString))

                if (err2) {
                    err.addError(err2.getReturnCode(), err2.getMessage());
                }
            } else {
                // For better performance, events are submitted only for the first recursive undo
                if (Q_UNLIKELY(d->m_inundoRedoTransaction <= 1)) {
                    // Is it a light transaction?
                    bool lightTransaction = (getCachedValue(QStringLiteral("SKG_REFRESH_VIEW")) != QStringLiteral("Y"));

                    // Emit modification events
                    QStringList tablesRefreshed;
                    tablesRefreshed.reserve(listModifiedTables.count());
                    for (const auto &table : std::as_const(listModifiedTables)) {
                        Q_EMIT tableModified(table, getCurrentTransaction(), lightTransaction);
                        tablesRefreshed.push_back(table);
                    }

                    // Remove temporary transaction if needed
                    IFOKDO(err, executeSqliteOrder(QStringLiteral("delete from doctransaction where t_name LIKE '#INTERNAL#%';")))

                    Q_EMIT tableModified(QStringLiteral("doctransaction"), getCurrentTransaction(), lightTransaction);
                    Q_EMIT tableModified(QStringLiteral("doctransactionitem"), getCurrentTransaction(), lightTransaction);

                    // WARNING: list is modified during treatment
                    for (int i = 0; !err && i < listModifiedTables.count(); ++i) {
                        QString table = listModifiedTables.at(i);
                        QStringList toAdd = getImpactedViews(table);
                        int nbToAdd = toAdd.count();
                        for (int j = 0; !err && j < nbToAdd; ++j) {
                            const QString &toAddTable = toAdd.at(j);
                            if (!listModifiedTables.contains(toAddTable)) {
                                // Compute materialized view of modified table
                                if (!lightTransaction) {
                                    err = computeMaterializedViews(toAddTable);
                                }
                                listModifiedTables.push_back(toAddTable);
                            }
                        }
                    }

                    // Emit events
                    for (int i = tablesRefreshed.count(); i < listModifiedTables.count(); ++i) {
                        Q_EMIT tableModified(listModifiedTables.at(i), 0, lightTransaction);
                    }

                    Q_EMIT transactionSuccessfullyEnded(getCurrentTransaction());
                }
            }

            // clean cache
            d->m_cache.clear();

            d->m_currentTransaction = 0;

            if (Q_LIKELY(qobject_cast<QGuiApplication *>(qApp) != nullptr)) { // clazy:excludeall=unneeded-cast
                QApplication::restoreOverrideCursor();
            }
        }
    }

    IFOK(err)
    {
        err = errOverwritten;
    }
    return err;
}

SKGError SKGDocument::removeAllTransactions()
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Check if a transaction is still opened
    err = checkExistingTransaction();
    IFOK(err)
    err.setReturnCode(ERR_ABORT).setMessage(i18nc("Something went wrong with SQL transactions", "Remove of transactions is forbidden inside a transaction"));
    else
    {
        err = SKGDocument::beginTransaction(QStringLiteral("#INTERNAL#"));
        IFOKDO(err, executeSqliteOrder(QStringLiteral("delete from doctransaction")))
        SKGENDTRANSACTION(this, err)

        // Force the save
        d->m_lastSavedTransaction = -1;
    }
    return err;
}

SKGError SKGDocument::computeMaterializedViews(const QString &iTable) const
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)

    // Compute additional where clause
    QStringList tables;
    if (d->m_MaterializedViews.contains(iTable)) {
        tables = d->m_MaterializedViews[iTable];
    } else {
        QString wc;
        if (!iTable.isEmpty()) {
            QString t = iTable;
            if (t.startsWith(QStringLiteral("v_"))) {
                t.replace(0, 2, QStringLiteral("vm_"));
            }
            wc = " AND name='" % t % u'\'';
        }

        // Get list of materialized table
        err = getDistinctValues(QStringLiteral("sqlite_master"), QStringLiteral("name"), "type='table' AND name LIKE 'vm_%' " % wc, tables);
        d->m_MaterializedViews[iTable] = tables;
    }

    // Refresh tables
    int nb = tables.count();
    for (int i = 0; !err && i < nb; ++i) {
        const QString &table = tables.at(i);
        QString view = table;
        view.replace(0, 3, QStringLiteral("v_"));

        // Remove previous table
        {
            SKGTRACEINRC(5, "SKGDocument::computeMaterializedViews-drop-" % table, err)
            err = executeSqliteOrder("DROP TABLE IF EXISTS " % table);
        }
        {
            // Recreate table
            SKGTRACEINRC(5, "SKGDocument::computeMaterializedViews-create-" % table, err)
            IFOKDO(err, executeSqliteOrder("CREATE TABLE " % table % " AS SELECT * FROM " % view))
        }
    }

    return err;
}

SKGError SKGDocument::sendMessage(const QString &iMessage, MessageType iMessageType, const QString &iAction)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    // Associate message with transaction
    if (!checkExistingTransaction()) {
        SKGObjectBase msg(this, QStringLiteral("doctransactionmsg"));
        err = msg.setAttribute(QStringLiteral("rd_doctransaction_id"), SKGServices::intToString(getCurrentTransaction()));
        IFOKDO(err, msg.setAttribute(QStringLiteral("t_message"), iMessage))
        IFOKDO(err,
               msg.setAttribute(QStringLiteral("t_type"),
                                iMessageType == SKGDocument::Positive          ? QStringLiteral("P")
                                    : iMessageType == SKGDocument::Information ? QStringLiteral("I")
                                    : iMessageType == SKGDocument::Warning     ? QStringLiteral("W")
                                    : iMessageType == SKGDocument::Error       ? QStringLiteral("E")
                                                                               : QStringLiteral("H")))
        IFOKDO(err, msg.save())
    }

    if (checkExistingTransaction() || !iAction.isEmpty()) {
        // Addition message in global variable in case of no transaction opened
        bool found = false;
        for (const auto &m : std::as_const(m_unTransactionnalMessages)) {
            if (m.Text == iMessage) {
                found = true;
            }
        }
        if (iMessageType != SKGDocument::Hidden && !found) {
            SKGMessage m;
            m.Text = iMessage;
            m.Type = iMessageType;
            m.Action = iAction;
            m_unTransactionnalMessages.push_back(m);
        }
    }
    return err;
}

SKGError SKGDocument::removeMessages(int iIdTransaction)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    if (!checkExistingTransaction()) {
        err = executeSqliteOrder("DELETE FROM doctransactionmsg WHERE rd_doctransaction_id=" % SKGServices::intToString(iIdTransaction));
    }

    m_unTransactionnalMessages.clear();
    return err;
}

SKGError SKGDocument::getMessages(int iIdTransaction, SKGMessageList &oMessages, bool iAll)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    oMessages = m_unTransactionnalMessages;

    SKGStringListList listTmp;
    err = executeSelectSqliteOrder(QStringLiteral("SELECT t_message, t_type FROM doctransactionmsg WHERE ") % (iAll ? "" : "t_type<>'H' AND ")
                                       % "rd_doctransaction_id=" % SKGServices::intToString(iIdTransaction) % " ORDER BY id ASC",
                                   listTmp);

    int nb = listTmp.count();
    for (int i = 1; !err && i < nb; ++i) {
        QString msg = listTmp.at(i).at(0);
        QString type = listTmp.at(i).at(1);
        bool found = false;
        for (const auto &m : std::as_const(m_unTransactionnalMessages)) {
            if (m.Text == msg) {
                found = true;
            }
        }
        if (!found) {
            SKGMessage m;
            m.Text = std::move(msg);
            m.Type = type == QStringLiteral("P") ? SKGDocument::Positive
                : type == QStringLiteral("I")    ? SKGDocument::Information
                : type == QStringLiteral("W")    ? SKGDocument::Warning
                : type == QStringLiteral("E")    ? SKGDocument::Error
                                                 : SKGDocument::Hidden;
            oMessages.push_back(m);
        }
    }

    m_unTransactionnalMessages.clear();
    return err;
}

SKGError SKGDocument::getModifications(int iIdTransaction, SKGObjectModificationList &oModifications) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    oModifications.clear();

    SKGStringListList listTmp;
    err = executeSelectSqliteOrder("SELECT i_object_id,t_object_table,t_action FROM doctransactionitem WHERE rd_doctransaction_id="
                                       % SKGServices::intToString(iIdTransaction) % " ORDER BY id ASC",
                                   listTmp);
    int nb = listTmp.count();
    for (int i = 1; !err && i < nb; ++i) {
        SKGObjectModification mod;
        mod.id = SKGServices::stringToInt(listTmp.at(i).at(0));
        mod.table = listTmp.at(i).at(1);
        QString type = listTmp.at(i).at(2);
        mod.type = (type == QStringLiteral("D") ? I : (type == QStringLiteral("I") ? D : U)); // Normal because in database we have to sql order to go back.
        mod.uuid = listTmp.at(i).at(0) % u'-' % mod.table;

        oModifications.push_back(mod);
    }
    return err;
}

QStringList SKGDocument::getImpactedViews(const QString &iTable) const
{
    SKGTRACEINFUNC(10)
    if (Q_UNLIKELY(d->m_ImpactedViews.isEmpty())) {
        // Get list of tables and views
        QStringList tables;
        SKGStringListList result;
        executeSelectSqliteOrder(QStringLiteral("SELECT tbl_name FROM sqlite_master WHERE tbl_name NOT LIKE '%_delete' AND type IN ('table', 'view')"), result);
        int nb = result.count();
        tables.reserve(nb);
        for (int i = 1; i < nb; ++i) {
            tables.push_back(result.at(i).at(0));
        }

        // First computation
        executeSelectSqliteOrder(QStringLiteral("SELECT tbl_name, sql FROM sqlite_master WHERE tbl_name NOT LIKE '%_delete' AND type='view'"), result);
        nb = result.count();
        for (int i = 1; i < nb; ++i) {
            const QStringList &line = result.at(i);
            const QString &name = line.at(0);
            const QString &sql = line.at(1);

            QStringList words = SKGServices::splitCSVLine(sql, ' ', false);
            words.push_back(QStringLiteral("parameters"));
            int nbWords = words.count();
            for (int j = 0; j < nbWords; ++j) {
                QString word = words.at(j);
                word = word.remove(u',');
                if (word.startsWith(QStringLiteral("vm_"))) {
                    word.replace(0, 3, QStringLiteral("v_"));
                }
                if (word != name && tables.contains(word, Qt::CaseInsensitive)) {
                    QStringList l = d->m_ImpactedViews.value(word);
                    if (!l.contains(name)) {
                        l.push_back(name);
                    }
                    d->m_ImpactedViews[word] = std::move(l);
                }
            }
        }

        // Now, we have some thing like this
        // d->m_ImpactedViews[A]={ B, C, D}
        // d->m_ImpactedViews[B]={ E, F}
        // We must build d->m_ImpactedViews[A]={ B, C, D, E, F}
        QStringList keys = d->m_ImpactedViews.keys();
        for (const auto &k : std::as_const(keys)) {
            QStringList l = d->m_ImpactedViews.value(k);
            for (int i = 0; i < l.count(); ++i) { // Warning: the size of l will change in the loop
                QString item = l.at(i);
                if (d->m_ImpactedViews.contains(item)) {
                    const auto &tmp = d->m_ImpactedViews.value(item);
                    for (const auto &name : std::as_const(tmp)) {
                        if (!l.contains(name)) {
                            l.push_back(name);
                        }
                    }
                }
            }
            d->m_ImpactedViews[k] = std::move(l);
        }
    }
    return d->m_ImpactedViews.value(iTable);
}

SKGError SKGDocument::groupTransactions(int iFrom, int iTo)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)

    ++d->m_inundoRedoTransaction; // It is a kind of undo redo

    // Check if a transaction is still opened
    err = checkExistingTransaction();
    IFOK(err)
    err.setReturnCode(ERR_ABORT).setMessage(
        i18nc("Something went wrong with SQL transactions", "Creation of a group of transactions is forbidden inside a transaction"));
    else
    {
        int iidMaster = qMax(iFrom, iTo);
        QString smin = SKGServices::intToString(qMin(iFrom, iTo));
        QString smax = SKGServices::intToString(iidMaster);

        // Get transaction
        SKGStringListList transactions;
        err = executeSelectSqliteOrder(QStringLiteral("SELECT id, t_name, t_mode, i_parent FROM doctransaction WHERE id BETWEEN ") % smin % " AND " % smax
                                           % " ORDER BY id ASC",
                                       transactions);

        // Check and get main parameter for the group
        int nb = transactions.count();
        QString transactionMode;
        QString communParent;
        QString name;
        for (int i = 1; !err && i < nb; ++i) { // We forget header
            const QStringList &transaction = transactions.at(i);
            const QString &mode = transaction.at(2);
            if (!name.isEmpty()) {
                name += ',';
            }
            name += transaction.at(1);

            if (!transactionMode.isEmpty() && mode != transactionMode) {
                err = SKGError(ERR_INVALIDARG, QStringLiteral("Undo and Redo transactions cannot be grouped"));
            } else {
                transactionMode = mode;
            }

            if (i == 1) {
                communParent = transaction.at(3);
            }
        }

        // Group
        IFOK(err)
        {
            err = SKGDocument::beginTransaction(QStringLiteral("#INTERNAL#"));
            // Group items
            IFOKDO(err,
                   executeSqliteOrder(QStringLiteral("UPDATE doctransactionitem set rd_doctransaction_id=") % smax % " where rd_doctransaction_id BETWEEN "
                                      % smin % " AND " % smax))
            IFOKDO(err,
                   executeSqliteOrder(QStringLiteral("UPDATE doctransaction set i_parent=") % communParent % ", t_name='" % SKGServices::stringToSqlString(name)
                                      % "' where id=" % smax))

            IFOKDO(err,
                   executeSqliteOrder(QStringLiteral("DELETE FROM doctransaction WHERE id BETWEEN ") % smin % " AND "
                                      % SKGServices::intToString(qMax(iFrom, iTo) - 1)))

            SKGENDTRANSACTION(this, err)
        }
    }

    --d->m_inundoRedoTransaction;
    return err;
}

SKGError SKGDocument::undoRedoTransaction(UndoRedoMode iMode)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    // Check if a transaction is still opened
    err = checkExistingTransaction();
    IFOK(err) err.setReturnCode(ERR_ABORT).setMessage(i18nc("Something went wrong with SQL transactions", "Undo / Redo is forbidden inside a transaction"));
    else
    {
        if (iMode == SKGDocument::UNDOLASTSAVE) {
            // Create group
            SKGStringListList transactions;
            err = executeSelectSqliteOrder(QStringLiteral("SELECT id, t_savestep FROM doctransaction WHERE t_mode='U' ORDER BY id DESC"), transactions);
            int nb = transactions.count();
            int min = 0;
            int max = 0;
            for (int i = 1; !err && i < nb; ++i) {
                const QStringList &transaction = transactions.at(i);
                if (i == 1) {
                    max = SKGServices::stringToInt(transaction.at(0));
                }
                if (i != 1 && transaction.at(1) == QStringLiteral("Y")) {
                    break;
                }
                min = SKGServices::stringToInt(transaction.at(0));
            }
            if (min == 0) {
                min = max;
            }
            if (!err && min != max && min != 0) {
                err = groupTransactions(min, max);
            }
        } else {
            err = SKGError(); // To ignore error generated by checkExistingTransaction.
        }

        // Get ID of the transaction to undo
        IFOK(err)
        {
            QString name;
            bool saveStep = false;
            QDateTime date;
            bool refreshViews;
            int id = getTransactionToProcess(iMode, &name, &saveStep, &date, &refreshViews);
            if (id == 0) {
                // No transaction found ==> generate an error
                err = SKGError(ERR_INVALIDARG, QStringLiteral("No transaction found. Undo / Redo impossible."));
            } else {
                // Undo transaction
                SKGTRACEL(5) << "Undoing transaction [" << id << "]- [" << name << "]…" << Qt::endl;
                SKGStringListList listSqlOrder;
                err = executeSelectSqliteOrder("SELECT t_sqlorder FROM doctransactionitem WHERE rd_doctransaction_id=" % SKGServices::intToString(id)
                                                   % " ORDER BY id DESC",
                                               listSqlOrder);
                IFOK(err)
                {
                    int nb = listSqlOrder.count();
                    err = SKGDocument::beginTransaction(name, nb + 3, date, refreshViews);
                    IFOK(err)
                    {
                        ++d->m_inundoRedoTransaction; // Because we will be in a undo/redo transaction
                        // Normal the first element is ignored because it is the header
                        for (int i = 1; !err && i < nb; ++i) {
                            err = executeSqliteOrder(listSqlOrder.at(i).at(0));

                            IFOKDO(err, stepForward(i))
                        }

                        IFOK(err)
                        {
                            // Set the NEW transaction in redo mode
                            int lastredo = getTransactionToProcess(
                                (iMode == SKGDocument::UNDO || iMode == SKGDocument::UNDOLASTSAVE ? SKGDocument::REDO : SKGDocument::UNDO));
                            int newredo = getTransactionToProcess(iMode);
                            IFOKDO(err,
                                   executeSqliteOrder(
                                       QStringLiteral("UPDATE doctransaction set t_mode=")
                                       % (iMode == SKGDocument::UNDO || iMode == SKGDocument::UNDOLASTSAVE ? QStringLiteral("'R'") : QStringLiteral("'U'"))
                                       % ", i_parent=" % SKGServices::intToString(lastredo) % " where id=" % SKGServices::intToString(newredo)))
                            IFOKDO(err, stepForward(nb))

                            // Move messages from previous transaction to new one
                            IFOKDO(err,
                                   executeSqliteOrder("UPDATE doctransactionmsg set rd_doctransaction_id=" % SKGServices::intToString(getCurrentTransaction())
                                                      % " where rd_doctransaction_id=" % SKGServices::intToString(id)))
                            IFOKDO(err, stepForward(nb + 1))

                            // delete treated transaction
                            IFOKDO(err, executeSqliteOrder("DELETE from doctransaction where id=" % SKGServices::intToString(id)))
                            IFOKDO(err, stepForward(nb + 2))

                            // Check that new transaction has exactly the same number of item
                            /* IFOK (err) {
                                     SKGStringListList listSqlOrder;
                                     err=executeSelectSqliteOrder(
                                                     "SELECT count(1) FROM doctransactionitem WHERE rd_doctransaction_id=" %
                                                     SKGServices::intToString(getCurrentTransaction()),
                                                     listSqlOrder);
                                     if (!err && SKGServices::stringToInt(listSqlOrder.at(1).at(0))!=nb-1) {
                                             err=SKGError(ERR_ABORT, i18nc("Error message", "Invalid number of item after undo/redo. Expected (%1) != Result
                             (%2)",nb-1,listSqlOrder.at(1).at(0)));
                                     }
                             }*/

                            IFOKDO(err, stepForward(nb + 3))
                        }

                        SKGENDTRANSACTION(this, err)
                        --d->m_inundoRedoTransaction; // We left the undo / redo transaction
                    }
                }
            }
        }
    }

    return err;
}

int SKGDocument::getDepthTransaction() const
{
    return d->m_nbStepForTransaction.size();
}

int SKGDocument::getNbTransaction(UndoRedoMode iMode) const
{
    SKGTRACEINFUNC(10)
    int output = 0;
    if (Q_LIKELY(getMainDatabase())) {
        QString sqlorder = QStringLiteral("select count(1) from doctransaction where t_mode='");
        sqlorder += (iMode == SKGDocument::UNDO || iMode == SKGDocument::UNDOLASTSAVE ? QStringLiteral("U") : QStringLiteral("R"));
        sqlorder += '\'';
        QSqlQuery query(sqlorder, *getMainDatabase());
        if (query.next()) {
            output = query.value(0).toInt();
        }
    }
    return output;
}

int SKGDocument::getTransactionToProcess(UndoRedoMode iMode, QString *oName, bool *oSaveStep, QDateTime *oDate, bool *oRefreshViews) const
{
    SKGTRACEINFUNC(10)
    // initialisation
    int output = 0;
    if (oName != nullptr) {
        *oName = QString();
    }
    if (Q_LIKELY(getMainDatabase())) {
        QString sqlorder = QStringLiteral(
            "select A.id , A.t_name, A.t_savestep, A.d_date, A.t_refreshviews from doctransaction A where "
            "NOT EXISTS(select 1 from doctransaction B where B.i_parent=A.id) "
            "and A.t_mode='");
        sqlorder += (iMode == SKGDocument::UNDO || iMode == SKGDocument::UNDOLASTSAVE ? QStringLiteral("U") : QStringLiteral("R"));
        sqlorder += '\'';
        QSqlQuery query(sqlorder, *getMainDatabase());
        if (query.next()) {
            output = query.value(0).toInt();
            if (oName != nullptr) {
                *oName = query.value(1).toString();
            }
            if (oSaveStep != nullptr) {
                *oSaveStep = (query.value(2).toString() == QStringLiteral("Y"));
            }
            if (oDate != nullptr) {
                *oDate = SKGServices::stringToTime(query.value(3).toString());
            }
            if (oRefreshViews != nullptr) {
                *oRefreshViews = (query.value(4).toString() == QStringLiteral("Y"));
            }
        }
    }
    return output;
}

int SKGDocument::getCurrentTransaction() const
{
    SKGTRACEINFUNC(10)
    return d->m_currentTransaction;
}

QString SKGDocument::getPassword() const
{
    if (!d->m_password_got) {
        d->m_password = getParameter(QStringLiteral("SKG_PASSWORD"));
        d->m_password_got = true;
    }
    return d->m_password;
}

SKGError SKGDocument::changePassword(const QString &iNewPassword)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    IFOK(checkExistingTransaction())
    err.setReturnCode(ERR_ABORT).setMessage(i18nc("Something went wrong with SQL transactions", "Change password is forbidden inside a transaction"));
    else
    {
        IFOKDO(err,
               executeSqliteOrder("PRAGMA REKEY = '" % SKGServices::stringToSqlString(iNewPassword.isEmpty() ? QStringLiteral("DEFAULTPASSWORD") : iNewPassword)
                                  % "'"))
        IFOKDO(err, beginTransaction(QStringLiteral("#INTERNAL#"), 0, QDateTime::currentDateTime(), false))
        IFOKDO(err, setParameter(QStringLiteral("SKG_PASSWORD"), iNewPassword))
        IFOKDO(err, setParameter(QStringLiteral("SKG_PASSWORD_LASTUPDATE"), SKGServices::dateToSqlString(QDate::currentDate())))
        IFOKDO(err,
               sendMessage(iNewPassword.isEmpty()
                               ? i18nc("Inform the user that the password protection was removed", "The document password has been removed.")
                               : i18nc("Inform the user that the password was successfully changed", "The document password has been modified."),
                           SKGDocument::Positive))
        SKGENDTRANSACTION(this, err)

        // Force the save
        IFOK(err)
        {
            d->m_lastSavedTransaction = -1;

            d->m_password = iNewPassword;
            d->m_password_got = true;

            // Close all thread connection
            auto conNameMainConnection = getMainDatabase()->connectionName();
            const auto conNames = QSqlDatabase::connectionNames();
            for (const auto &conName : conNames) {
                if (conName.startsWith(conNameMainConnection % "_")) {
                    /* NO NEED
                    {
                        auto con = QSqlDatabase::database(conName, false);
                        con.close();
                    }*/
                    QSqlDatabase::removeDatabase(conName);
                }
            }
        }
    }
    return err;
}

SKGError SKGDocument::setLanguage(const QString &iLanguage)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    QString previousLanguage = getParameter(QStringLiteral("SKG_LANGUAGE"));
    if (previousLanguage != iLanguage) {
        // Save language into the document
        IFOKDO(err, beginTransaction(QStringLiteral("#INTERNAL#"), 0, QDateTime::currentDateTime(), false))
        IFOKDO(err, setParameter(QStringLiteral("SKG_LANGUAGE"), iLanguage))

        // Migrate view for new language
        IFOKDO(err, refreshViewsIndexesAndTriggers())

        // close temporary transaction
        SKGENDTRANSACTION(this, err)
    }
    return err;
}

SKGError SKGDocument::initialize()
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    err = load(QString(), QString());
    return err;
}

SKGError SKGDocument::recover(const QString &iName, const QString &iPassword, QString &oRecoveredFile)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    SKGTRACEL(10) << "Input parameter [name]=[" << iName << ']' << Qt::endl;

    QString sqliteFile = QString(iName % "_recovered.sqlite").replace(QStringLiteral(".skg_"), QStringLiteral("_"));
    oRecoveredFile = QString(iName % "_recovered.skg").replace(QStringLiteral(".skg_"), QStringLiteral("_"));
    bool mode;
    err = SKGServices::cryptFile(iName, sqliteFile, iPassword, false, getDocumentHeader(), mode);
    IFOK(err)
    {
        QFile(oRecoveredFile).remove();
        QString cmd = "echo .dump | sqlcipher \"" % sqliteFile % "\" | sed -e 's/ROLLBACK; -- due to errors/COMMIT;/g' | sqlcipher \"" % oRecoveredFile % u'"';
        QProcess p;
        p.start(QStringLiteral("sh"), QStringList() << QStringLiteral("-c") << cmd);
        if (!p.waitForFinished(1000 * 60 * 2) || p.exitCode() != 0) {
            err.setReturnCode(ERR_FAIL).setMessage(i18nc("Error message", "The following command line failed with code %2:\n'%1'", cmd, p.exitCode()));
        }

        // Try to load the recovered file
        IFOKDO(err, load(oRecoveredFile, QString()))
        IFOK(err){SKGBEGINTRANSACTION(*this, i18nc("Noun", "Recovery"), err) IFOKDO(err, refreshViewsIndexesAndTriggers(true))} IFOKDO(err, save())

            // Reset the current document
            initialize();

        // Clean useless file
        IFOK(err)
        {
            // We keep only the recovered
            QFile(sqliteFile).remove();
        }
        else
        {
            // We keep the sqlite file in case of
            QFile(oRecoveredFile).remove();
            err.addError(ERR_FAIL, i18nc("Error message", "Impossible to recover this file"));
        }
    }

    return err;
}

SKGError SKGDocument::load(const QString &iName, const QString &iPassword, bool iRestoreTmpFile, bool iForceReadOnly)
{
    // Close previous document
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    SKGTRACEL(10) << "Input parameter [name]=[" << iName << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iRestoreTmpFile]=[" << (iRestoreTmpFile ? "TRUE" : "FALSE") << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iForceReadOnly]=[" << (iForceReadOnly ? "TRUE" : "FALSE") << ']' << Qt::endl;

    d->m_lastSavedTransaction = -1; // To avoid double event emission
    d->m_modeSQLCipher = true;
    d->m_blockEmits = true;
    err = close();
    d->m_blockEmits = false;
    IFOK(err)
    {
        if (!iName.isEmpty()) {
            // File exist
            QFileInfo fi(iName);
            d->m_modeReadOnly = iForceReadOnly || !fi.permission(QFile::WriteUser);

            // Temporary file
            d->m_temporaryFile = SKGDocument::getTemporaryFile(iName, d->m_modeReadOnly);
            bool temporaryFileExisting = QFile(d->m_temporaryFile).exists();
            SKGTRACEL(10) << "Temporary file: [" << d->m_temporaryFile << ']' << Qt::endl;
            SKGTRACEL(10) << "Temporary file existing: [" << (temporaryFileExisting ? "TRUE" : "FALSE") << ']' << Qt::endl;
            if (!iRestoreTmpFile || !temporaryFileExisting) {
                SKGTRACEL(10) << "Create the temporary file" << Qt::endl;
                QFile::remove(d->m_temporaryFile); // Must remove it to be able to copy
                err = SKGServices::cryptFile(iName, d->m_temporaryFile, iPassword, false, getDocumentHeader(), d->m_modeSQLCipher);
            } else {
                SKGTRACEL(10) << "The temporary file is existing, try a restore but we must check if the file is password protected first" << Qt::endl;
                // 249955: Check if password protected vvv
                // Temporary file will be loaded but first we must check if original document is password protected
                QString temporaryFile2 = d->m_temporaryFile % u'2';
                err = SKGServices::cryptFile(iName, temporaryFile2, iPassword, false, getDocumentHeader(), d->m_modeSQLCipher);

                // Try an open to check if well descrypted
                IFOK(err)
                {
                    QSqlDatabase tryOpen(QSqlDatabase::addDatabase(SQLDRIVERNAME, QStringLiteral("tryOpen")));
                    tryOpen.setDatabaseName(temporaryFile2);
                    if (!tryOpen.open()) {
                        // Set error message
                        QSqlError sqlErr = tryOpen.lastError();
                        err = SKGError(SQLLITEERROR + sqlErr.nativeErrorCode().toInt(), sqlErr.text());
                    }
                    if (d->m_modeSQLCipher) {
                        IFOKDO(
                            err,
                            SKGServices::executeSqliteOrder(
                                tryOpen,
                                "PRAGMA KEY = '" % SKGServices::stringToSqlString(iPassword.isEmpty() ? QStringLiteral("DEFAULTPASSWORD") : iPassword) % "'"))
                        IFKO(err)
                        {
                            SKGTRACEL(10) << "Wrong installation of sqlcipher (doesn't support encryption)" << Qt::endl;
                            err = SKGError(ERR_ENCRYPTION, i18nc("Error message", "Wrong installation"));
                        }

                        // Migrate to the last version of SQLCipher
                        IFOKDO(err, SKGServices::executeSqliteOrder(tryOpen, QStringLiteral("PRAGMA cipher_migrate")))

                        // Test the password
                        IFOKDO(err, SKGServices::executeSqliteOrder(tryOpen, QStringLiteral("SELECT count(*) FROM sqlite_master")))
                        IFKO(err)
                        {
                            SKGTRACEL(10) << "Wrong password in restore mode" << Qt::endl;
                            err = SKGError(ERR_ENCRYPTION, i18nc("Error message", "Wrong password"));
                        }
                    }
                    IFOKDO(err, SKGServices::executeSqliteOrder(tryOpen, QStringLiteral("PRAGMA synchronous = OFF")))
                }
                QSqlDatabase::removeDatabase(QStringLiteral("tryOpen"));
                QFile::remove(temporaryFile2);

                // To avoid deletion of temporary file during next try
                IFKO(err) d->m_temporaryFile = QString();
                // 249955: Check if password protected ^^^
            }

            // Create file database
            IFOK(err)
            {
                d->m_currentDatabase = QSqlDatabase::addDatabase(SQLDRIVERNAME, d->m_databaseIdentifier);
                d->m_currentDatabase.setDatabaseName(d->m_temporaryFile);
                if (!d->m_currentDatabase.open()) {
                    // Set error message
                    QSqlError sqlErr = d->m_currentDatabase.lastError();
                    err = SKGError(SQLLITEERROR + sqlErr.nativeErrorCode().toInt(), sqlErr.text());
                }

                d->m_directAccessDb = true;
                if (QUrl::fromUserInput(iName).isLocalFile()) {
                    d->m_currentFileName = iName;
                }
            }
        } else {
            // Temporary file
            d->m_temporaryFile = QDir::tempPath() % "/skg_" % QUuid::createUuid().toString() % ".skg";

            // Create memory database
            d->m_currentDatabase = QSqlDatabase::addDatabase(SQLDRIVERNAME, d->m_databaseIdentifier);
            d->m_currentDatabase.setConnectOptions(QStringLiteral("QSQLITE_OPEN_URI"));
            d->m_currentDatabase.setDatabaseName(QStringLiteral("file:") + d->m_databaseIdentifier + QStringLiteral("?mode=memory&cache=shared"));
            if (!d->m_currentDatabase.open()) {
                // Set error message
                QSqlError sqlErr = d->m_currentDatabase.lastError();
                err = SKGError(SQLLITEERROR + sqlErr.nativeErrorCode().toInt(), sqlErr.text());
            }

            d->m_directAccessDb = false;
        }
        if (d->m_modeSQLCipher) {
            // This is an encrypted data base
            IFOKDO(err,
                   executeSqliteOrder("PRAGMA KEY = '" % SKGServices::stringToSqlString(iPassword.isEmpty() ? QStringLiteral("DEFAULTPASSWORD") : iPassword)
                                      % "'"))
            IFKO(err)
            {
                SKGTRACEL(10) << "Wrong installation of sqlcipher (doesn't support encryption)" << Qt::endl;
                err = SKGError(ERR_ENCRYPTION, i18nc("Error message", "Wrong installation"));
            }

            // Migrate to the last version of SQLCipher
            IFOKDO(err, executeSqliteOrder(QStringLiteral("PRAGMA cipher_migrate")))

            // Test the password
            IFOKDO(err, executeSqliteOrder(QStringLiteral("SELECT count(*) FROM sqlite_master")))
            IFKO(err)
            {
                SKGTRACEL(10) << "Wrong password on temporary file" << Qt::endl;
                err = SKGError(ERR_ENCRYPTION, i18nc("Error message", "Wrong password"));
            }
        }

        // Check if the database is correct
        IFOK(err)
        {
            IFOKDO(err, executeSqliteOrder(QStringLiteral("PRAGMA journal_mode=MEMORY")))
            IFKO(err)
            {
                err.addError(ERR_CORRUPTION, i18nc("Error message", "Oups, this file seems to be corrupted"));
            }
        }

        // Optimization
        QStringList optimization;
        optimization << QStringLiteral("PRAGMA case_sensitive_like=true") << QStringLiteral("PRAGMA journal_mode=MEMORY")
                     << QStringLiteral("PRAGMA temp_store=MEMORY")
                     // << QStringLiteral("PRAGMA locking_mode=EXCLUSIVE")
                     << QStringLiteral("PRAGMA synchronous = OFF")
                     << QStringLiteral("PRAGMA legacy_alter_table=ON") // For migration on sqlite >=3.25 (see https://sqlite.org/lang_altertable.html)
                     << QStringLiteral("PRAGMA recursive_triggers=true");
        IFOKDO(err, executeSqliteOrders(optimization))

        // Add custom sqlite functions
        IFOKDO(err, addSqliteAddon(getMainDatabase()))

        if (!d->m_directAccessDb) {
            // Create parameter and undo redo table
            /**
             * This constant is used to initialized the data model (table creation)
             */
            QStringList InitialDataModel;

            // ==================================================================
            // Table parameters
            InitialDataModel << QStringLiteral("CREATE TABLE parameters "
                                              "(id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                              "t_uuid_parent TEXT NOT NULL DEFAULT '',"
                                              "t_name TEXT NOT NULL,"
                                              "t_value TEXT NOT NULL DEFAULT '',"
                                              "b_blob BLOB,"
                                              "d_lastmodifdate DATE NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                                              "i_tmp INTEGER NOT NULL DEFAULT 0"
                                              ")")

                             // ==================================================================
                             // Table node
                             << "CREATE TABLE node ("
                             "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                             "t_name TEXT NOT NULL DEFAULT '' CHECK (t_name NOT LIKE '%" % OBJECTSEPARATOR % "%'),"
                             "t_fullname TEXT,"
                             "t_icon TEXT DEFAULT '',"
                             "f_sortorder FLOAT,"
                             "t_autostart VARCHAR(1) DEFAULT 'N' CHECK (t_autostart IN ('Y', 'N')),"
                             "t_data TEXT,"
                             "rd_node_id INT CONSTRAINT fk_id REFERENCES node(id) ON DELETE CASCADE)"

                             // ==================================================================
                             // Table doctransaction
                             << QStringLiteral("CREATE TABLE doctransaction ("
                                              "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                                              "t_name TEXT NOT NULL,"
                                              "t_mode VARCHAR(1) DEFAULT 'U' CHECK (t_mode IN ('U', 'R')),"
                                              "d_date DATE NOT NULL,"
                                              "t_savestep VARCHAR(1) DEFAULT 'N' CHECK (t_savestep IN ('Y', 'N')),"
                                              "t_refreshviews VARCHAR(1) DEFAULT 'Y' CHECK (t_refreshviews IN ('Y', 'N')),"
                                              "i_parent INTEGER)")

                             // ==================================================================
                             // Table doctransactionitem
                             << QStringLiteral("CREATE TABLE doctransactionitem ("
                                              "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                                              "rd_doctransaction_id INTEGER NOT NULL,"
                                              "i_object_id INTEGER NOT NULL,"
                                              "t_object_table TEXT NOT NULL,"
                                              "t_action VARCHAR(1) DEFAULT 'I' CHECK (t_action IN ('I', 'U', 'D')),"
                                              "t_sqlorder TEXT NOT NULL DEFAULT '')")

                             << QStringLiteral("CREATE TABLE doctransactionmsg ("
                                              "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                                              "rd_doctransaction_id INTEGER NOT NULL,"
                                              "t_message TEXT NOT NULL DEFAULT '',"
                                              "t_type VARCHAR(1) DEFAULT 'I' CHECK (t_type IN ('P', 'I', 'W', 'E', 'H')))");  // Positive, Information, Warning, Error, Hidden

            IFOKDO(err, executeSqliteOrders(InitialDataModel))
            IFOKDO(err, SKGDocument::refreshViewsIndexesAndTriggers())
        }
    }

    // migrate
    IFOK(err)
    {
        bool mig = false;
        err = migrate(mig);

        if (!err && getParameter(QStringLiteral("SKG_DATABASE_TYPE")) != SQLDRIVERNAME && !getPassword().isEmpty()) {
            err = sendMessage(i18nc("Information message",
                                    "This document is protected by a password but the database is still in SQLite mode.\nDo you know that the SQLCipher mode "
                                    "is more secured because even the temporary file is encrypted?"),
                              SKGDocument::Warning,
                              QStringLiteral("skg://migrate_sqlcipher"));
        }
        if (!err && getParameter(QStringLiteral("SKG_PASSWORD_LASTUPDATE")) == QString() && !getPassword().isEmpty()) {
            err = sendMessage(
                i18nc("Information message",
                      "A security hole has been detected and corrected on this version of the application. We strongly encourage you to change your password."),
                SKGDocument::Warning,
                QStringLiteral("skg://file_change_password"));
        }

        // To authorize manual repair of document in case of error during migration
        // the error is not caught if traces are activated
        if (err && (SKGTraces::SKGLevelTrace != 0)) {
            err = sendMessage(i18nc("Popup message", "The migration failed but the document has been loaded without error because debug mode is activated"),
                              SKGDocument::Warning);
        }

        if (!err && mig && !iName.isEmpty()) {
            err = sendMessage(i18nc("The document has been upgraded to the latest Skrooge version format", "The document has been migrated"),
                              SKGDocument::Positive);
        }
    }

    // Optimization
    IFOK(err)
    {
        d->m_lastSavedTransaction = getTransactionToProcess(SKGDocument::UNDO);
        executeSqliteOrder(QStringLiteral("ANALYZE"));
    }

    // Creation undo/redo triggers
    IFOKDO(err, createUndoRedoTemporaryTriggers())

    IFOK(err)
    {
        QString sqliteQtVersion = getParameter(QStringLiteral("SKG_SQLITE_LAST_VERSION"));
        QString sqliteSystemVersion(sqlite3_libversion());
        QProcess sqlite3Process;
        QString mode;
        sqlite3Process.start(QStringLiteral("sqlcipher"), QStringList() << QStringLiteral("-version"));
        mode = QStringLiteral("SQLCipher");
        if (sqlite3Process.waitForFinished()) {
            sqliteSystemVersion = SKGServices::splitCSVLine(sqlite3Process.readAll(), ' ').value(0);
        }
        SKGTRACEL(5) << "SQLite version of Qt        :" << sqliteQtVersion << Qt::endl;
        SKGTRACEL(5) << "SQLite version of the system:" << sqliteSystemVersion << Qt::endl;
        if (!sqliteQtVersion.isEmpty() && !sqliteSystemVersion.isEmpty() && sqliteQtVersion != sqliteSystemVersion) {
            QString message = i18nc("Error message",
                                    "This application can not run correctly because the %3 version of the system (%1) is not aligned with the %4 version "
                                    "embedded in Qt (%2). You should rebuild Qt with the option -system-sqlite.",
                                    sqliteSystemVersion,
                                    sqliteQtVersion,
                                    mode,
                                    mode);
            err = sendMessage(message, Warning);
            SKGTRACE << "WARNING:" << message << Qt::endl;
        }
    }

    if (err && !iName.isEmpty()) {
        close();
    } else {
        // Send event
        d->m_uniqueIdentifier = QUuid::createUuid().toString();
        d->m_password = iPassword;
        d->m_password_got = true;
        Q_EMIT tableModified(QString(), 0, false);
        Q_EMIT modified();
    }

    return err;
}

bool SKGDocument::isReadOnly() const
{
    return d->m_modeReadOnly;
}

bool SKGDocument::isFileModified() const
{
    // Get last executed transaction
    int last = getTransactionToProcess(SKGDocument::UNDO);
    //  if (nbStepForTransaction.size()) --last;
    return (d->m_lastSavedTransaction != last);
}

void SKGDocument::setFileNotModified() const
{
    d->m_lastSavedTransaction = getTransactionToProcess(SKGDocument::UNDO);
}

QString SKGDocument::getCurrentFileName() const
{
    return d->m_currentFileName;
}

SKGError SKGDocument::save()
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    if (d->m_currentFileName.isEmpty()) {
        err = SKGError(ERR_INVALIDARG,
                       i18nc("Error message: Can not save a file if it has no name yet", "Save not authorized because the file name is not yet defined"));
    } else {
        // save
        err = saveAs(d->m_currentFileName, true);
    }
    return err;
}

SKGError SKGDocument::saveAs(const QString &iName, bool iOverwrite)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    SKGTRACEL(10) << "Input parameter [name]=[" << iName << ']' << Qt::endl;

    bool simulateFileSystemFull = !SKGServices::getEnvVariable(QStringLiteral("SKGFILESYSTEMFULL")).isEmpty();

    // Check if a transaction is still opened
    err = checkExistingTransaction();
    IFOK(err)
    err.setReturnCode(ERR_ABORT).setMessage(
        i18nc("Cannot save the file while the application is still performing an SQL transaction", "Save is forbidden if a transaction is still opened"));
    else
    {
        err = SKGError();
        if (getParameter(QStringLiteral("SKG_UNDO_CLEAN_AFTER_SAVE")) == QStringLiteral("Y")) {
            err = executeSqliteOrder(QStringLiteral("delete from doctransaction"));
        }

        // No transaction opened ==> it is ok
        // We mark the last transaction as a save point
        IFOKDO(err,
               executeSqliteOrder(QStringLiteral("update doctransaction set t_savestep='Y' where id in (select A.id from doctransaction A where "
                                                 "NOT EXISTS(select 1 from doctransaction B where B.i_parent=A.id) "
                                                 "and A.t_mode='U')")))
        Q_EMIT tableModified(QStringLiteral("doctransaction"), 0, false);

        // Optimization
        IFOK(err)
        {
            err = executeSqliteOrder(QStringLiteral("VACUUM;"));
            IFOK(err)
            {
                // Check if file already exist
                if (!iOverwrite && QFile(iName).exists()) {
                    err.setReturnCode(ERR_INVALIDARG).setMessage(i18nc("There is already a file with the same name", "File '%1' already exist", iName));
                } else {
                    // Get backup file name
                    bool backupFileMustBeRemoved = false;
                    QString backupFileName = getBackupFile(iName);
                    if (backupFileName.isEmpty()) {
                        backupFileName = iName % ".tmp";
                        backupFileMustBeRemoved = true;
                    }

                    // Create backup file
                    QFile::remove(backupFileName % u'~');
                    QFile::rename(backupFileName, backupFileName % u'~');
                    if (QFile(iName).exists() && (simulateFileSystemFull || !QFile(iName).copy(backupFileName))) {
                        this->sendMessage(i18nc("Error message: Could not create a backup file", "Creation of backup file %1 failed", backupFileName), Warning);
                    }

                    // Save database
                    IFOK(err)
                    {
                        QFile::remove(iName % u'~');
                        QFile::rename(iName, iName % u'~');

                        // To be sure that db is flushed
                        IFOKDO(err, executeSqliteOrder(QStringLiteral("PRAGMA synchronous = FULL")))
                        QString pwd = getPassword();

                        // Copy memory to tmp db
                        if (!d->m_directAccessDb && !err) {
                            QFile::remove(d->m_temporaryFile);
                            auto fileDb = QSqlDatabase::addDatabase(SQLDRIVERNAME, d->m_databaseIdentifier % "_tmp");
                            fileDb.setDatabaseName(d->m_temporaryFile);
                            if (!fileDb.open()) {
                                // Set error message
                                QSqlError sqlErr = fileDb.lastError();
                                err = SKGError(SQLLITEERROR + sqlErr.nativeErrorCode().toInt(), sqlErr.text());
                            } else {
                                IFOKDO(err,
                                       SKGServices::executeSqliteOrder(
                                           fileDb,
                                           "PRAGMA KEY = '" % SKGServices::stringToSqlString(pwd.isEmpty() ? QStringLiteral("DEFAULTPASSWORD") : pwd) % "'"))
                                addSqliteAddon(&fileDb);
                                IFOKDO(err,
                                       SKGServices::copySqliteDatabase(fileDb,
                                                                       d->m_currentDatabase,
                                                                       false,
                                                                       pwd.isEmpty() ? QStringLiteral("DEFAULTPASSWORD") : std::move(pwd)))
                            }

                            fileDb.close();
                            QSqlDatabase::removeDatabase(d->m_databaseIdentifier % "_tmp");
                        }

                        // To simulate a file system full
                        if (!err && simulateFileSystemFull) {
                            err = SKGError(ERR_WRITEACCESS, i18nc("Error message: writing a file failed", "Write file '%1' failed", iName));
                        }

                        // Crypt the file
                        if (!err) {
                            bool mode;
                            err = SKGServices::cryptFile(d->m_temporaryFile, iName, pwd, true, getDocumentHeader(), mode);
                        }
                        if (!d->m_directAccessDb && !err) {
                            QFile(d->m_temporaryFile).remove();
                        }

                        // For performances
                        IFOKDO(err, executeSqliteOrder(QStringLiteral("PRAGMA synchronous = OFF")))
                    }

                    if (backupFileMustBeRemoved) {
                        QFile::remove(backupFileName);
                    }

                    IFOK(err)
                    {
                        // The document is not modified
                        QString oldtemporaryFile = d->m_temporaryFile;
                        d->m_currentFileName = iName;
                        d->m_modeReadOnly = false;
                        d->m_temporaryFile = getTemporaryFile(d->m_currentFileName);
                        if (oldtemporaryFile != d->m_temporaryFile) {
                            QFile(oldtemporaryFile).rename(d->m_temporaryFile);
                        }
                        d->m_lastSavedTransaction = getTransactionToProcess(SKGDocument::UNDO);

                        // Commit save
                        QFile::remove(backupFileName % u'~');
                        QFile::remove(iName % u'~');
                    }
                    else
                    {
                        // Rollback file
                        QFile::remove(backupFileName);
                        QFile::rename(backupFileName % u'~', backupFileName);

                        QFile::remove(iName);
                        QFile::rename(iName % u'~', iName);
                    }
                }
            }
        }

        Q_EMIT transactionSuccessfullyEnded(0);
    }
    return err;
}

SKGError SKGDocument::close(bool iDeletingDocument)
{
    SKGTRACEINFUNC(5)
    if (getMainDatabase() != nullptr) {
        QString conNameMainConnection = getMainDatabase()->connectionName();
        const auto &conNames = QSqlDatabase::connectionNames();
        for (const auto &conName : conNames) {
            if (conName.startsWith(conNameMainConnection % "_")) {
                /* NO NEED
                {
                    auto con = QSqlDatabase::database(conName, false);
                    con.close();
                }*/
                QSqlDatabase::removeDatabase(conName);
            }
        }
        getMainDatabase()->close();
        d->m_currentDatabase = QSqlDatabase(); // To avoid warning on remove
        QSqlDatabase::removeDatabase(d->m_databaseIdentifier);
    }

    if (!d->m_temporaryFile.isEmpty()) {
        QFile(d->m_temporaryFile).remove();
        d->m_temporaryFile = QString();
    }

    // Emit events ?
    bool emitEvent = (d->m_lastSavedTransaction != -1);

    // Init fields
    d->m_currentFileName = QString();
    d->m_lastSavedTransaction = 0;
    d->m_nbStepForTransaction.clear();
    d->m_posStepForTransaction.clear();
    d->m_nameForTransaction.clear();
    d->m_password.clear();
    d->m_password_got = false;

    // Send event
    if (!d->m_blockEmits && emitEvent && !iDeletingDocument) {
        Q_EMIT tableModified(QString(), 0, false);
        Q_EMIT transactionSuccessfullyEnded(0);
        Q_EMIT modified();
    }

    return SKGError();
}

SKGError SKGDocument::dropViewsAndIndexes(const QStringList &iTables) const
{
    SKGError err;
    // Drop all views
    SKGStringListList list;
    err = executeSelectSqliteOrder(QStringLiteral("SELECT tbl_name, name, type FROM sqlite_master WHERE type IN ('view','index')"), list);
    int nb = list.count();
    for (int i = 1; !err && i < nb; ++i) {
        QString name = list.at(i).at(1);
        QString table = SKGServices::getRealTable(list.at(i).at(0));
        QString type = list.at(i).at(2);
        if (iTables.contains(table)) {
            QString sql = "DROP " % type % " IF EXISTS " % name;
            err = this->executeSqliteOrder(sql);
        }
    }
    return err;
}

#include "skgdocument2.cpp"

SKGError SKGDocument::migrate(bool &oMigrationDone)
{
    SKGError err;
    SKGTRACEINFUNCRC(5, err)
    oMigrationDone = false;

    {
        SKGBEGINPROGRESSTRANSACTION(*this, "#INTERNAL#" % i18nc("Progression step", "Migrate document"), err, 3)
        if (getParameter(QStringLiteral("SKG_UNDO_MAX_DEPTH")).isEmpty()) {
            IFOKDO(err, setParameter(QStringLiteral("SKG_UNDO_MAX_DEPTH"), SKGServices::intToString(SKG_UNDO_MAX_DEPTH)))
        }

        if (getParameter(QStringLiteral("SKG_UNDO_CLEAN_AFTER_SAVE")).isEmpty()) {
            IFOKDO(err, setParameter(QStringLiteral("SKG_UNDO_CLEAN_AFTER_SAVE"), QStringLiteral("N")))
        }

        if (!err && getParameter(QStringLiteral("SKG_DATABASE_TYPE")) != (d->m_modeSQLCipher ? SQLDRIVERNAME : QStringLiteral("QSQLITE"))) {
            IFOKDO(err, setParameter(QStringLiteral("SKG_DATABASE_TYPE"), d->m_modeSQLCipher ? SQLDRIVERNAME : QStringLiteral("QSQLITE")))
        }

        QString version = getParameter(QStringLiteral("SKG_DB_VERSION"));
        QString initialversion = version;
        QString lastversion = QStringLiteral("1.6");

        if (!err && version.isEmpty()) {
            // First creation
            SKGTRACEL(10) << "Migration from 0 to " << lastversion << Qt::endl;

            // Set new version
            version = lastversion;
            IFOKDO(err, setParameter(QStringLiteral("SKG_DB_VERSION"), version))

            // Set sqlite creation version
            SKGStringListList listTmp;
            IFOKDO(err, executeSelectSqliteOrder(QStringLiteral("select sqlite_version()"), listTmp))
            if (!err && listTmp.count() == 2) {
                err = setParameter(QStringLiteral("SKG_SQLITE_CREATION_VERSION"), listTmp.at(1).at(0));
            }
            oMigrationDone = true;
        }

        if (!err && SKGServices::stringToDouble(version) > SKGServices::stringToDouble(lastversion)) {
            err = SKGError(ERR_ABORT, i18nc("Error message", "Impossible to load a document generated by a more recent version"));
        }

        {
            // Migration steps
            if (!err && version == QStringLiteral("0.1")) {
                // Migration from version 0.1 to 0.2
                SKGTRACEL(10) << "Migration from 0.1 to 0.2" << Qt::endl;

                // ==================================================================
                // Table doctransactionmsg
                QStringList sqlOrders;
                sqlOrders << QStringLiteral("DROP TABLE IF EXISTS doctransactionmsg")
                          << QStringLiteral(
                                 "CREATE TABLE doctransactionmsg ("
                                 "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                                 "rd_doctransaction_id INTEGER NOT NULL,"
                                 "t_message TEXT NOT NULL DEFAULT '')");
                err = executeSqliteOrders(sqlOrders);

                // Set new version
                version = QStringLiteral("0.2");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.2")) {
                // Migration from version 0.2 to 0.3
                SKGTRACEL(10) << "Migration from 0.2 to 0.3" << Qt::endl;

                err = executeSqliteOrder(QStringLiteral("UPDATE node set f_sortorder=id"));

                // Set new version
                version = QStringLiteral("0.3");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.3")) {
                // Migration from version 0.3 to 0.4
                SKGTRACEL(10) << "Migration from 0.3 to 0.4" << Qt::endl;

                err = executeSqliteOrder(QStringLiteral("ALTER TABLE node ADD COLUMN t_autostart VARCHAR(1) DEFAULT 'N' CHECK (t_autostart IN ('Y', 'N'))"));
                IFOKDO(err,
                       executeSqliteOrder("UPDATE node set t_autostart='Y' where t_name='"
                                          % i18nc("Verb, automatically load when the application is started", "autostart") % QLatin1Char('\'')))

                // Set new version
                version = QStringLiteral("0.4");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.4")) {
                // Migration from version 0.4 to 0.5
                SKGTRACEL(10) << "Migration from 0.4 to 0.5" << Qt::endl;

                err =
                    executeSqliteOrder(QStringLiteral("ALTER TABLE doctransactionmsg ADD COLUMN t_popup VARCHAR(1) DEFAULT 'Y' CHECK (t_popup IN ('Y', 'N'))"));

                // Set new version
                version = QStringLiteral("0.5");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.5")) {
                // Migration from version 0.5 to 0.6
                SKGTRACEL(10) << "Migration from 0.5 to 0.6" << Qt::endl;

                err = executeSqliteOrder(QStringLiteral("UPDATE node set t_autostart='N' where t_autostart NOT IN ('Y', 'N')"));

                // Set new version
                version = QStringLiteral("0.6");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.6")) {
                // Migration from version 0.6 to 0.7
                SKGTRACEL(10) << "Migration from 0.6 to 0.7" << Qt::endl;

                err = executeSqliteOrder(QStringLiteral("ALTER TABLE parameters ADD COLUMN b_blob BLOB"));

                // Set new version
                version = QStringLiteral("0.7");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.7")) {
                // Migration from version 0.7 to 0.8
                SKGTRACEL(10) << "Migration from 0.7 to 0.8" << Qt::endl;

                err = executeSqliteOrder(QStringLiteral("UPDATE parameters set t_name='SKG_LANGUAGE' where t_name='SKGLANGUAGE'"));

                // Set new version
                version = QStringLiteral("0.8");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.8")) {
                SKGTRACEL(10) << "Migration from 0.8 to 0.9" << Qt::endl;

                QStringList sql;
                sql << QStringLiteral("ALTER TABLE parameters ADD COLUMN i_tmp INTEGER NOT NULL DEFAULT 0") << QStringLiteral("UPDATE parameters set i_tmp=0");

                err = executeSqliteOrders(sql);

                // Set new version
                version = QStringLiteral("0.9");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("0.9")) {
                SKGTRACEL(10) << "Migration from 0.9 to 1.0" << Qt::endl;

                err = SKGDocument::setParameter(QStringLiteral("SKG_UNIQUE_ID"), QString());

                // Set new version
                version = QStringLiteral("1.0");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("1.0")) {
                // Migration from version 1.0 to 1.1
                SKGTRACEL(10) << "Migration from 1.0 to 1.1" << Qt::endl;

                err = executeSqliteOrder(QStringLiteral("ALTER TABLE node ADD COLUMN t_icon TEXT DEFAULT ''"));
                IFOK(err)
                {
                    SKGStringListList result;
                    err = executeSelectSqliteOrder(QStringLiteral("SELECT id,t_data from node"), result);
                    int nb = result.count();
                    for (int i = 1; !err && i < nb; ++i) {
                        const QStringList &line = result.at(i);
                        QString icon = QStringLiteral("folder-bookmark");
                        QStringList data = SKGServices::splitCSVLine(line.at(1));
                        if (data.count() > 2) {
                            icon = data.at(2);
                            data.removeAt(2);
                        }
                        err = executeSqliteOrder("UPDATE node set t_icon='" % SKGServices::stringToSqlString(icon) % "', t_data='"
                                                 % SKGServices::stringToSqlString(SKGServices::stringsToCsv(data)) % "' where id=" % line.at(0));
                    }
                }

                // Set new version
                version = QStringLiteral("1.1");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("1.1")) {
                // Migration from version 1.1 to 1.2
                SKGTRACEL(10) << "Migration from 1.1 to 1.2" << Qt::endl;

                QStringList sql;
                sql << QStringLiteral("ALTER TABLE doctransaction ADD COLUMN t_refreshviews VARCHAR(1) DEFAULT 'Y' CHECK (t_refreshviews IN ('Y', 'N'))")
                    << QStringLiteral("UPDATE doctransaction set t_refreshviews='Y'");

                err = executeSqliteOrders(sql);

                // Set new version
                version = QStringLiteral("1.2");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("1.2")) {
                // Migration from version 1.2 to 1.3
                SKGTRACEL(10) << "Migration from 1.2 to 1.3" << Qt::endl;

                err = SKGDocument::refreshViewsIndexesAndTriggers();

                QStringList sql;
                sql << QStringLiteral(
                    "DELETE FROM node WHERE (r_node_id IS NULL OR r_node_id='') AND EXISTS (SELECT 1 FROM node n WHERE n.t_name=node.t_name AND r_node_id=0)")
                    << QStringLiteral("UPDATE node SET t_name=t_name");
                IFOKDO(err, executeSqliteOrders(sql))

                // Set new version
                version = QStringLiteral("1.3");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("1.3")) {
                // Migration from version 1.3 to 1.4
                SKGTRACEL(10) << "Migration from 1.3 to 1.4" << Qt::endl;

                QStringList sql;
                sql   << "CREATE TABLE node2 ("
                      "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"
                      "t_name TEXT NOT NULL DEFAULT '' CHECK (t_name NOT LIKE '%" % OBJECTSEPARATOR % "%'),"
                      "t_fullname TEXT,"
                      "t_icon TEXT DEFAULT '',"
                      "f_sortorder FLOAT,"
                      "t_autostart VARCHAR(1) DEFAULT 'N' CHECK (t_autostart IN ('Y', 'N')),"
                      "t_data TEXT,"
                      "rd_node_id INT CONSTRAINT fk_id REFERENCES node(id) ON DELETE CASCADE)"

                      << QStringLiteral("INSERT INTO node2 (id, t_name, t_fullname, t_icon, f_sortorder, t_autostart, t_data, rd_node_id) "
                                       "SELECT id, t_name, t_fullname, t_icon, f_sortorder, t_autostart, t_data, r_node_id FROM node")

                      << QStringLiteral("DROP TABLE IF EXISTS node")
                      << QStringLiteral("ALTER TABLE node2 RENAME TO node");

                err = executeSqliteOrders(sql);

                // Set new version
                version = QStringLiteral("1.4");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("1.4")) {
                // Migration from version 1.4 to 1.5
                SKGTRACEL(10) << "Migration from 1.4 to 1.5" << Qt::endl;

                err = SKGDocument::refreshViewsIndexesAndTriggers();

                QStringList sql;
                sql << QStringLiteral("UPDATE parameters SET t_uuid_parent='advice' WHERE t_uuid_parent='advices'");
                IFOKDO(err, executeSqliteOrders(sql))

                // Set new version
                version = QStringLiteral("1.5");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
            if (!err && version == QStringLiteral("1.5")) {
                // Migration from version 1.5 to 1.6
                SKGTRACEL(10) << "Migration from  1.5 to 1.6" << Qt::endl;

                err = SKGDocument::refreshViewsIndexesAndTriggers();

                QStringList sql;
                sql << QStringLiteral("DROP TABLE IF EXISTS doctransactionmsg2")
                    << QStringLiteral(
                           "CREATE TABLE doctransactionmsg2 ("
                           "id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                           "rd_doctransaction_id INTEGER NOT NULL,"
                           "t_message TEXT NOT NULL DEFAULT '',"
                           "t_type VARCHAR(1) DEFAULT 'I' CHECK (t_type IN ('P', 'I', 'W', 'E', 'H')))") // Positive, Information, Warning, Error, Hidden
                    << QStringLiteral(
                           "INSERT INTO doctransactionmsg2 (id, rd_doctransaction_id, t_message, t_type) SELECT id, rd_doctransaction_id, t_message, (CASE "
                           "WHEN t_popup='Y' THEN 'I' ELSE 'H' END)  FROM doctransactionmsg")

                    << QStringLiteral("DROP TABLE IF EXISTS doctransactionmsg") << QStringLiteral("ALTER TABLE doctransactionmsg2 RENAME TO doctransactionmsg");
                IFOKDO(err, executeSqliteOrders(sql))

                // Set new version
                version = QStringLiteral("1.6");
                IFOKDO(err, SKGDocument::setParameter(QStringLiteral("SKG_DB_VERSION"), version))
                oMigrationDone = true;
            }
        }
        IFOKDO(err, stepForward(1, i18nc("Progression step", "Refresh views")))

        // Set sqlite last version
        SKGStringListList listTmp;
        IFOKDO(err, executeSelectSqliteOrder(QStringLiteral("select sqlite_version()"), listTmp))
        if (!err && listTmp.count() == 2) {
            err = setParameter(QStringLiteral("SKG_SQLITE_LAST_VERSION"), listTmp.at(1).at(0));
        }

        // Refresh views
        IFOKDO(err, refreshViewsIndexesAndTriggers())
        IFOKDO(err, stepForward(2, i18nc("Progression step", "Update materialized views")))

        // Refresh materialized views
        if (!err && oMigrationDone) {
            err = computeMaterializedViews();
        }
        IFOKDO(err, stepForward(3))

        IFKO(err)
        err.addError(
            ERR_FAIL,
            i18nc("Error message: Could not perform database migration", "Database migration from version %1 to version %2 failed", initialversion, version));
    }

    return err;
}

SKGError SKGDocument::createUndoRedoTemporaryTriggers() const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Create triggers
    QStringList tables;
    err = this->getTablesList(tables);
    int nbTables = tables.count();
    for (int i = 0; !err && i < nbTables; ++i) {
        // Get table name
        const QString &table = tables.at(i);

        // Do we have to treat this table
        if (!SKGListNotUndoable.contains("T." % table) && !table.startsWith(QStringLiteral("vm_"))) {
            // YES
            // Get attributes name
            QStringList attributes;
            err = getAttributesList(table, attributes);

            // Build sqlorder for update and insert
            QString sqlorderForUpdate2;
            QString sqlorderForInsert1;
            QString sqlorderForInsert2;
            int nbAttributes = attributes.count();
            for (int j = 0; !err && j < nbAttributes; ++j) {
                // Get attribute
                const QString &att = attributes.at(j);

                // Do we have to treat this attribute
                if (!SKGListNotUndoable.contains("A." % table % u'.' % att)) {
                    // Build for update
                    if (!sqlorderForUpdate2.isEmpty()) {
                        sqlorderForUpdate2 += ',';
                    }
                    sqlorderForUpdate2 += att % "='||quote(old." % att % ")||'";

                    // Build for insert part 1
                    if (!sqlorderForInsert1.isEmpty()) {
                        sqlorderForInsert1 += ',';
                    }
                    sqlorderForInsert1 += att;

                    // Build for insert part 2
                    if (!sqlorderForInsert2.isEmpty()) {
                        sqlorderForInsert2 += ',';
                    }
                    sqlorderForInsert2 += "'||quote(old." % att % ")||'";
                }
            }

            // Create specific triggers for the current transaction
            QStringList sqlOrders;
            // DROP DELETE trigger
            sqlOrders << "DROP TRIGGER IF EXISTS UR_" % table % "_IN"

                      // Create DELETE trigger
                      << "CREATE TEMP TRIGGER UR_" % table % "_IN "
                      "AFTER  INSERT ON " % table % " BEGIN "
                      "INSERT INTO doctransactionitem (rd_doctransaction_id, t_sqlorder,i_object_id,t_object_table,t_action) VALUES(0,'DELETE FROM " % table %
                      " WHERE id='||new.id,new.id,'" % table % "','D');END"

                      // DROP UPDATE trigger
                      << "DROP TRIGGER IF EXISTS UR_" % table % "_UP"

                      // Create UPDATE trigger
                      << "CREATE TEMP TRIGGER UR_" % table % "_UP "
                      "AFTER UPDATE ON " % table % " BEGIN "
                      "INSERT INTO doctransactionitem  (rd_doctransaction_id, t_sqlorder,i_object_id,t_object_table,t_action) VALUES(0,'UPDATE " % table %
                      " SET " % sqlorderForUpdate2 %
                      " WHERE id='||new.id,new.id,'" % table % "','U');END"

                      // DROP INSERT trigger
                      << "DROP TRIGGER IF EXISTS UR_" % table % "_DE"

                      // Create INSERT trigger
                      << "CREATE TEMP TRIGGER UR_" % table % "_DE "
                      "AFTER DELETE ON " % table %
                      " BEGIN "
                      "INSERT INTO doctransactionitem  (rd_doctransaction_id, t_sqlorder,i_object_id,t_object_table,t_action) VALUES(0,'INSERT INTO " % table %
                      u'(' % sqlorderForInsert1 % ") VALUES(" % sqlorderForInsert2 % ")',old.id,'" % table % "','I'); END";
            err = executeSqliteOrders(sqlOrders);
        }
    }
    return err;
}

QStringList SKGDocument::getParameters(const QString &iParentUUID, const QString &iWhereClause)
{
    SKGTRACEINFUNC(10)
    QStringList output;
    QString wc = "t_uuid_parent='" % SKGServices::stringToSqlString(iParentUUID) % u'\'';
    if (!iWhereClause.isEmpty()) {
        wc += " AND (" % iWhereClause % u')';
    }
    this->getDistinctValues(QStringLiteral("parameters"), QStringLiteral("t_name"), wc, output);
    return output;
}

QString SKGDocument::getParameter(const QString &iName, const QString &iParentUUID) const
{
    SKGTRACEINFUNC(10)
    SKGTRACEL(10) << "Input parameter [iName]=[" << iName << ']' << Qt::endl;
    QString output;

    // Get parameter
    SKGObjectBase param;
    SKGError err = getObject(QStringLiteral("parameters"),
                             "t_name='" % SKGServices::stringToSqlString(iName) % "' AND t_uuid_parent='" % SKGServices::stringToSqlString(iParentUUID) % u'\'',
                             param);
    IFOK(err)
    {
        output = param.getAttribute(QStringLiteral("t_value"));
    }
    return output;
}

QVariant SKGDocument::getParameterBlob(const QString &iName, const QString &iParentUUID) const
{
    SKGTRACEINFUNC(10)
    SKGTRACEL(10) << "Input parameter [iName]=[" << iName << ']' << Qt::endl;

    QVariant output;
    if (getMainDatabase() != nullptr) {
        QString sqlQuery = QStringLiteral("SELECT b_blob FROM parameters WHERE t_name=? AND t_uuid_parent=?");
        QSqlQuery query(*getMainDatabase());
        query.prepare(sqlQuery);
        query.addBindValue(iName);
        query.addBindValue(iParentUUID);
        if (Q_LIKELY(!query.exec())) {
            QSqlError sqlError = query.lastError();
            SKGTRACE << "WARNING: " << sqlQuery << Qt::endl;
            SKGTRACE << "         returns :" << sqlError.text() << Qt::endl;
        } else {
            if (query.next()) {
                output = query.value(0);
            }
        }
    }

    return output;
}

SKGError SKGDocument::setParameter(const QString &iName,
                                   const QString &iValue,
                                   const QString &iFileName,
                                   const QString &iParentUUID,
                                   SKGPropertyObject *oObjectCreated) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(10) << "Input parameter [iName]    =[" << iName << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iValue]   =[" << iValue << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iFileName]=[" << iFileName << ']' << Qt::endl;
    QVariant blob;
    QString value = iValue;
    QFile file(iFileName);
    if (file.exists()) {
        QFileInfo fileInfo(iFileName);
        if (fileInfo.isDir()) {
            value = "file://" % iFileName;
        } else {
            // Open file
            if (Q_UNLIKELY(!file.open(QIODevice::ReadOnly))) {
                err = SKGError(ERR_INVALIDARG, i18nc("Error message: Could not open a file", "Open file '%1' failed", iFileName));
            } else {
                QByteArray blob_bytes = file.readAll();
                if (blob_bytes.isEmpty()) {
                    err = SKGError(ERR_INVALIDARG, i18nc("Error message: Could not open a file", "Open file '%1' failed", iFileName));
                } else {
                    blob = blob_bytes;
                    value = fileInfo.fileName();
                }

                // close file
                file.close();
            }
        }
    }

    IFOKDO(err, setParameter(iName, value, blob, iParentUUID, oObjectCreated))
    return err;
}

SKGError SKGDocument::setParameter(const QString &iName,
                                   const QString &iValue,
                                   const QVariant &iBlob,
                                   const QString &iParentUUID,
                                   SKGPropertyObject *oObjectCreated) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(10) << "Input parameter [iName]    =[" << iName << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iValue]   =[" << iValue << ']' << Qt::endl;
    if (getMainDatabase() == nullptr) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
    } else {
        SKGPropertyObject param(const_cast<SKGDocument *>(this));
        IFOKDO(err, param.setName(iName))
        IFOKDO(err, param.setValue(iValue))
        IFOKDO(err, param.setParentId(iParentUUID))
        IFOKDO(err, param.save(true, oObjectCreated != nullptr))

        if (!err && !iBlob.isNull()) {
            err = param.load();
            IFOK(err)
            {
                // Set blob
                QString sqlQuery = QStringLiteral("UPDATE parameters SET b_blob=? WHERE id=?");
                QSqlQuery query(*getMainDatabase());
                query.prepare(sqlQuery);
                query.addBindValue(iBlob);
                query.addBindValue(param.getID());
                if (Q_LIKELY(!query.exec())) {
                    QSqlError sqlError = query.lastError();
                    QString msg = sqlQuery % u':' % sqlError.text();
                    err = SKGError(SQLLITEERROR + sqlError.nativeErrorCode().toInt(), std::move(msg));
                }
            }
        }
        if (!err && oObjectCreated != nullptr) {
            *oObjectCreated = param;
        }
    }
    return err;
}

SKGError SKGDocument::dump(int iMode) const
{
    SKGError err;
    // dump parameters
    SKGTRACE << "=== START DUMP ===" << Qt::endl;
    if ((iMode & DUMPSQLITE) != 0) {
        SKGTRACE << "=== DUMPSQLITE ===" << Qt::endl;
        err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM sqlite_master order by type")));

        SKGTRACE << "=== DUMPSQLITE (TEMPORARY) ===" << Qt::endl;
        err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM sqlite_temp_master order by type")));
    }

    if ((iMode & DUMPPARAMETERS) != 0) {
        SKGTRACE << "=== DUMPPARAMETERS ===" << Qt::endl;
        err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM parameters order by id")));
    }

    if ((iMode & DUMPNODES) != 0) {
        SKGTRACE << "=== DUMPNODES ===" << Qt::endl;
        err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM node order by id")));
    }

    if ((iMode & DUMPTRANSACTIONS) != 0) {
        // dump transaction
        SKGTRACE << "=== DUMPTRANSACTIONS ===" << Qt::endl;
        err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM doctransaction order by id")));

        // dump transaction
        SKGTRACE << "=== DUMPTRANSACTIONS (ITEMS) ===" << Qt::endl;
        err.addError(dumpSelectSqliteOrder(QStringLiteral("SELECT * FROM doctransactionitem order by rd_doctransaction_id, id")));
    }
    SKGTRACE << "=== END DUMP ===" << Qt::endl;
    return err;
}

QSqlDatabase *SKGDocument::getMainDatabase() const
{
    if (!d->m_currentDatabase.isOpen()) {
        return nullptr;
    }
    return const_cast<QSqlDatabase *>(&d->m_currentDatabase);
}

QSqlDatabase SKGDocument::getThreadDatabase() const
{
    if (qApp->thread() != QThread::currentThread()) {
        d->m_mutex.lock();
        QString pwd = getPassword();
        QString dbName = getMainDatabase()->databaseName();
        QString conName = getMainDatabase()->connectionName();

        QString id = conName % "_" % QString::number((quint64)QThread::currentThread(), 16);
        d->m_mutex.unlock();

        auto tmpDatabase = QSqlDatabase::database(id);
        if (!tmpDatabase.isValid()) {
            tmpDatabase = QSqlDatabase::addDatabase(SQLDRIVERNAME, id);
        }
        if (tmpDatabase.databaseName() != dbName) {
            tmpDatabase.setConnectOptions(QStringLiteral("QSQLITE_OPEN_URI"));
            tmpDatabase.setDatabaseName(dbName);
            if (tmpDatabase.open()) {
                addSqliteAddon(&tmpDatabase);
                if (d->m_modeSQLCipher) {
                    SKGServices::executeSqliteOrder(
                        tmpDatabase,
                        "PRAGMA KEY = '" % SKGServices::stringToSqlString(pwd.isEmpty() ? QStringLiteral("DEFAULTPASSWORD") : std::move(pwd)) % "'");
                }
            }
        }
        return tmpDatabase;
    }
    return d->m_currentDatabase;
}

SKGError SKGDocument::getConsolidatedView(const QString &iTable,
                                          const QString &iAsColumn,
                                          const QString &iAsRow,
                                          const QString &iAttribute,
                                          const QString &iOpAtt,
                                          const QString &iWhereClause,
                                          SKGStringListList &oTable,
                                          const QString &iMissingValue) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    SKGTRACEL(10) << "Input parameter [iTable]=[" << iTable << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iAsColumn]=[" << iAsColumn << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iAsRow]=[" << iAsRow << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iAttribute]=[" << iAttribute << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iOpAtt]=[" << iOpAtt << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iWhereClause]=[" << iWhereClause << ']' << Qt::endl;
    SKGTRACEL(10) << "Input parameter [iMissingValue]=[" << iMissingValue << ']' << Qt::endl;

    // Mode
    int mode = 0;
    if (!iAsColumn.isEmpty()) {
        mode += 1;
    }
    if (!iAsRow.isEmpty()) {
        mode += 2;
    }

    oTable.clear();
    oTable.push_back(QStringList());

    QStringList titles = oTable.at(0);

    if (mode == 3) {
        titles.push_back(iAsRow % u'/' % iAsColumn);
    } else {
        if (mode == 1) {
            titles.push_back(iAsColumn);

            QStringList sums;
            sums.push_back(i18nc("Noun, the numerical sum of a list of values", "Sum"));
            oTable.push_back(sums);
        } else {
            if (mode == 2) {
                titles.push_back(iAsRow);
                titles.push_back(i18nc("Noun, the numerical sum of a list of values", "Sum"));
            }
        }
    }
    oTable.removeAt(0);
    oTable.insert(0, titles);

    // Create sqlorder
    QString asColumn = iAsColumn;
    if (asColumn.startsWith(QStringLiteral("p_"))) {
        QString propertyName = asColumn.right(asColumn.length() - 2);
        asColumn = "(SELECT t_value FROM parameters WHERE t_uuid_parent=" % iTable % ".id||'-" % SKGServices::getRealTable(iTable) % "' AND t_name='"
            % propertyName % "')";
    }
    QString asRow = iAsRow;
    if (asRow.startsWith(QStringLiteral("p_"))) {
        QString propertyName = asRow.right(asRow.length() - 2);
        asRow = "(SELECT t_value FROM parameters WHERE t_uuid_parent=" % iTable % ".id||'-" % SKGServices::getRealTable(iTable) % "' AND t_name='"
            % propertyName % "')";
    }

    QString att = asColumn;
    if (!att.isEmpty() && !asRow.isEmpty()) {
        att += ',';
    }
    att += asRow;

    QString sort = std::move(asRow);
    if (!sort.isEmpty() && !asColumn.isEmpty()) {
        sort += ',';
    }
    sort += asColumn;

    if (!att.isEmpty()) {
        QString sql = "SELECT " % att % u',' % iOpAtt % u'(' % iAttribute % ") FROM " % iTable;
        if (!iWhereClause.isEmpty()) {
            sql += " WHERE " % iWhereClause;
        }
        if (!iOpAtt.isEmpty()) {
            sql += " GROUP BY " % att;
        }
        sql += " ORDER BY " % sort;

        QHash<QString, int> cols;
        QHash<QString, int> rows;

        SKGTRACEL(10) << "sqlorder=[" << sql << ']' << Qt::endl;
        SKGStringListList listTmp;
        err = executeSelectSqliteOrder(sql, listTmp);
        int nb = listTmp.count();
        for (int i = 1; !err && i < nb; ++i) { // Title is ignored
            const QStringList &line = listTmp.at(i);
            int rowindex = -1;
            int colindex = -1;
            if (mode >= 2) {
                const QString &rowname = line.at(mode == 3 ? 1 : 0);

                if (!rows.contains(rowname)) {
                    QStringList r;
                    int nbx = oTable.at(0).count();
                    r.reserve(nbx);
                    r.push_back(rowname);
                    for (int j = 1; j < nbx; ++j) {
                        r.push_back(iMissingValue);
                    }

                    oTable.push_back(r);

                    rowindex = oTable.count() - 1;
                    rows.insert(rowname, rowindex);
                } else {
                    rowindex = rows[rowname];
                }
            } else {
                rowindex = 1;
            }

            if (mode == 1 || mode == 3) {
                const QString &colname = line.at(0);

                if (!cols.contains(colname)) {
                    // Search better position of this column
                    colindex = -1;
                    {
                        QHashIterator<QString, int> cols_i(cols);
                        while (cols_i.hasNext()) {
                            cols_i.next();
                            if (colname > cols_i.key() && cols_i.value() > colindex) {
                                colindex = cols_i.value();
                            }
                        }
                    }
                    if (colindex == -1) {
                        colindex = 1;
                    } else {
                        ++colindex;
                    }

                    int nbx = oTable.count();
                    for (int j = 0; j < nbx; ++j) {
                        if (j == 0) {
                            oTable[j].insert(colindex, colname);
                        } else {
                            oTable[j].insert(colindex, iMissingValue);
                        }
                    }

                    {
                        QHash<QString, int> tmp;
                        QHashIterator<QString, int> cols_i(cols);
                        while (cols_i.hasNext()) {
                            cols_i.next();
                            tmp.insert(cols_i.key(), cols_i.value() + (cols_i.value() >= colindex ? 1 : 0));
                        }

                        cols = std::move(tmp);
                    }
                    cols.insert(colname, colindex);

                } else {
                    colindex = cols[colname];
                }
            } else {
                colindex = 1;
            }

            const QString &sum = line.at(mode == 3 ? 2 : 1);

            oTable[rowindex][colindex] = sum;
        }

        IFSKGTRACEL(10)
        {
            QStringList dump2 = SKGServices::tableToDump(oTable, SKGServices::DUMP_TEXT);
            int nbl = dump2.count();
            for (int i = 0; i < nbl; ++i) {
                SKGTRACE << dump2.at(i) << Qt::endl;
            }
        }

        // Correction bug 205466 vvv
        // If some months or years are missing, we must add them.
        if (asColumn.startsWith(QStringLiteral("d_"))) {
            for (int c = 1; c < oTable[0].count() - 1; ++c) { // Dynamic size
                bool forecast = false;
                QString title = oTable.at(0).at(c);
                if (title.isEmpty()) {
                    title = QStringLiteral("0000");
                }

                if (title.endsWith(QStringLiteral("999"))) {
                    title = title.left(title.size() - 3);
                    forecast = true;
                }
                QString nextTitle = oTable.at(0).at(c + 1);
                if (nextTitle.endsWith(QStringLiteral("999"))) {
                    nextTitle = nextTitle.left(nextTitle.size() - 3);
                    forecast = true;
                }

                QString dateFormat =
                    (asColumn == QStringLiteral("d_date")
                         ? QStringLiteral("yyyy-MM-dd")
                         : (asColumn == QStringLiteral("d_DATEMONTH")
                                ? QStringLiteral("yyyy-MM")
                                : (asColumn == QStringLiteral("d_DATEQUARTER")
                                       ? QStringLiteral("yyyy-QM")
                                       : (asColumn == QStringLiteral("d_DATESEMESTER")
                                              ? QStringLiteral("yyyy-SM")
                                              : (asColumn == QStringLiteral("d_DATEWEEK") ? QStringLiteral("yyyy-WM") : QStringLiteral("yyyy"))))));
                QDate nextExpected = QDate::fromString(title, dateFormat);
                QString nextExpectedString;
                if (asColumn == QStringLiteral("d_DATEWEEK")) {
                    QStringList items = SKGServices::splitCSVLine(oTable.at(0).at(c), '-');
                    auto y = SKGServices::stringToInt(items.at(0));
                    QString ws = items.value(1);
                    ws.remove('W');
                    auto w = SKGServices::stringToInt(ws);
                    if (w == 0) {
                        w = 1;
                    }

                    nextExpected = QDate(y, 1, 1).addDays(7 * w);
                    nextExpectedString = SKGServices::dateToPeriod(nextExpected, "W");
                    if (nextExpectedString == title) {
                        nextExpected = nextExpected.addDays(7);
                        nextExpectedString = SKGServices::dateToPeriod(nextExpected, "W");
                    }
                } else if (asColumn == QStringLiteral("d_DATEMONTH")) {
                    nextExpected = nextExpected.addMonths(1);
                    nextExpectedString = nextExpected.toString(dateFormat);
                } else if (asColumn == QStringLiteral("d_DATEQUARTER")) {
                    nextExpected = nextExpected.addMonths(nextExpected.month() * 3 - nextExpected.month()); // convert quarter in month
                    nextExpected = nextExpected.addMonths(3);
                    nextExpectedString = nextExpected.toString(QStringLiteral("yyyy-Q"))
                        % (nextExpected.month() <= 3 ? '1' : (nextExpected.month() <= 6 ? '2' : (nextExpected.month() <= 9 ? '3' : '4')));
                } else if (asColumn == QStringLiteral("d_DATESEMESTER")) {
                    nextExpected = nextExpected.addMonths(nextExpected.month() * 6 - nextExpected.month()); // convert semester in month
                    nextExpected = nextExpected.addMonths(6);
                    nextExpectedString = nextExpected.toString(QStringLiteral("yyyy-S")) % (nextExpected.month() <= 6 ? '1' : '2');
                } else if (asColumn == QStringLiteral("d_DATEYEAR")) {
                    nextExpected = nextExpected.addYears(1);
                    nextExpectedString = nextExpected.toString(dateFormat);
                } else {
                    nextExpected = nextExpected.addDays(1);
                    nextExpectedString = nextExpected.toString(dateFormat);
                }
                if (title != QStringLiteral("0000") && nextTitle != nextExpectedString && nextTitle != title) {
                    int colindex = c + 1;
                    if (forecast) {
                        nextExpectedString += QStringLiteral("999");
                    }

                    int nbx = oTable.count();
                    oTable[0].insert(colindex, nextExpectedString);
                    for (int j = 1; j < nbx; ++j) {
                        oTable[j].insert(colindex, iMissingValue);
                    }
                }
            }
        }
        // Correction bug 205466 ^^^
    }

    return err;
}

SKGDocument::SKGModelTemplateList SKGDocument::getDisplaySchemas(const QString &iRealTable) const
{
    SKGDocument::SKGModelTemplateList listSchema;

    // Build schemas
    if (iRealTable == QStringLiteral("doctransaction")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = QStringLiteral("t_name;t_value;d_lastmodifdate;t_savestep");
        listSchema.push_back(def);

        SKGModelTemplate minimum;
        minimum.id = QStringLiteral("minimum");
        minimum.name = i18nc("Noun, the minimum value of an item", "Minimum");
        minimum.icon = QString();
        minimum.schema = QStringLiteral("t_name;t_value;d_lastmodifdate|N;t_savestep|N");
        listSchema.push_back(minimum);
    } else if (iRealTable == QStringLiteral("parameters")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = QStringLiteral("t_name;t_value");
        listSchema.push_back(def);
    } else if (iRealTable == QStringLiteral("node")) {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = QStringLiteral("t_name");
        listSchema.push_back(def);
    } else {
        SKGModelTemplate def;
        def.id = QStringLiteral("default");
        def.name = i18nc("Noun, the default value of an item", "Default");
        def.icon = QStringLiteral("edit-undo");
        def.schema = QString();
        SKGStringListList lines;
        executeSelectSqliteOrder("PRAGMA table_info(" % iRealTable % ");", lines);
        for (const auto &line : std::as_const(lines)) {
            if (!def.schema.isEmpty()) {
                def.schema += ';';
            }
            def.schema += line[1];
        }
        listSchema.push_back(def);
    }

    return listSchema;
}

QString SKGDocument::getDisplay(const QString &iString) const
{
    QString output = iString.toLower();

    if (output.endsWith(QStringLiteral("t_name"))) {
        output = i18nc("Noun, the name of an item", "Name");
    } else if (output.endsWith(QStringLiteral("d_date"))) {
        output = i18nc("Noun, the date of an item", "Date");
    } else if (output.endsWith(QStringLiteral("t_savestep"))) {
        output = i18nc("Verb, save a document", "Save");
    } else if (output.endsWith(QStringLiteral("t_value"))) {
        output = i18nc("Noun, the value of an item", "Value");
    } else if (output.endsWith(QStringLiteral("d_lastmodifdate"))) {
        output = i18nc("Noun, date of last modification", "Last modification");
    } else if (output.startsWith(QStringLiteral("p_"))) {
        output = iString;
        output = output.right(output.size() - 2);
    } else if (output.contains(QStringLiteral(".p_"))) {
        output = iString;
        output = output.replace(QStringLiteral(".p_"), QStringLiteral("."));
    } else {
        output = iString;
    }
    return output;
}

QString SKGDocument::getIconName(const QString &iString) const
{
    QString output = iString.toLower();
    if (output.startsWith(QStringLiteral("p_")) || output.contains(QStringLiteral("p_"))) {
        return QStringLiteral("tag");
    }
    return QString();
}

QIcon SKGDocument::getIcon(const QString &iString) const
{
    return SKGServices::fromTheme(getIconName(iString));
}

QString SKGDocument::getRealAttribute(const QString &iString) const
{
    if (iString == iString.toLower()) {
        return iString;
    }
    return QString();
}

SKGServices::AttributeType SKGDocument::getAttributeType(const QString &iAttributeName) const
{
    SKGServices::AttributeType output = SKGServices::TEXT;
    if (iAttributeName.startsWith(QStringLiteral("d_"))) {
        output = SKGServices::DATE;
    } else if (iAttributeName.startsWith(QStringLiteral("i_"))) {
        output = SKGServices::INTEGER;
    } else if (iAttributeName.startsWith(QStringLiteral("rd_")) || iAttributeName.startsWith(QStringLiteral("rc_"))
               || iAttributeName.startsWith(QStringLiteral("r_")) || iAttributeName.startsWith(QStringLiteral("id_"))) {
        output = SKGServices::LINK;
    } else if (iAttributeName.startsWith(QStringLiteral("f_"))) {
        output = SKGServices::FLOAT;
    } else if (iAttributeName.startsWith(QStringLiteral("b_"))) {
        output = SKGServices::BLOB;
    } else if (iAttributeName == QStringLiteral("id")) {
        output = SKGServices::ID;
    } else if (iAttributeName == QStringLiteral("t_savestep") || iAttributeName == QStringLiteral("t_refreshviews")) {
        output = SKGServices::BOOL;
    }

    return output;
}

SKGServices::SKGUnitInfo SKGDocument::getUnit(const QString &iPrefixInCache) const
{
    SKGServices::SKGUnitInfo output;
    output.Name = getCachedValue(iPrefixInCache % "UnitCache");
    if (output.Name.isEmpty()) {
        refreshCache(QStringLiteral("unit"));
        output.Name = getCachedValue(iPrefixInCache % "UnitCache");
    }
    output.Symbol = getCachedValue(iPrefixInCache % "UnitSymbolCache");
    QString val = getCachedValue(iPrefixInCache % "UnitValueCache");
    if (!val.isEmpty()) {
        output.Value = SKGServices::stringToDouble(val);
    } else {
        output.Value = 1;
    }
    val = getCachedValue(iPrefixInCache % "UnitDecimalCache");
    if (!val.isEmpty()) {
        output.NbDecimal = SKGServices::stringToInt(val);
    } else {
        output.NbDecimal = 2;
    }

    return output;
}

QString SKGDocument::formatMoney(double iValue, const SKGServices::SKGUnitInfo &iUnit, bool iHtml) const
{
    QString val = SKGServices::toCurrencyString(iValue / iUnit.Value, iUnit.Symbol, iUnit.NbDecimal);
    if (iHtml) {
        // Return value
        if (iValue < 0) {
            // Get std colors
            KColorScheme scheme(QPalette::Normal);
            return QStringLiteral("<font color=\"") % scheme.foreground(KColorScheme::NegativeText).color().name() % "\">" % SKGServices::stringToHtml(val)
                % "</font>";
        }
        return SKGServices::stringToHtml(val);
    }
    return val;
}

QString SKGDocument::formatPrimaryMoney(double iValue, int iForcedNbOfDigit) const
{
    Q_UNUSED(iForcedNbOfDigit)
    return SKGServices::doubleToString(iValue);
}

QString SKGDocument::formatSecondaryMoney(double iValue, int iForcedNbOfDigit) const
{
    Q_UNUSED(iForcedNbOfDigit)
    return SKGServices::doubleToString(iValue);
}

QString SKGDocument::formatPercentage(double iValue, bool iInvertColors) const
{
    // Get std colors
    KColorScheme scheme(QPalette::Normal);
    QString negative = scheme.foreground(KColorScheme::NegativeText).color().name();
    QString positive = scheme.foreground(KColorScheme::PositiveText).color().name();

    // Return value
    QString p = SKGServices::toPercentageString(iValue);
    if (iValue > 0) {
        p = u'+' % p;
    }
    if (p.size() > 20 || std::isnan(iValue) || std::isinf(iValue)) {
        p = QChar(8734);
    }
    return "<font color=\"" % QString((iValue < 0 && !iInvertColors) || (iValue >= 0 && iInvertColors) ? negative : positive) % "\">"
        % SKGServices::stringToHtml(p) % "</font>";
}

QString SKGDocument::getFileExtension() const
{
    return QStringLiteral("skgc");
}

QString SKGDocument::getDocumentHeader() const
{
    return QStringLiteral("SKG");
}

void SKGDocument::addValueInCache(const QString &iKey, const QString &iValue) const
{
    d->m_cache[iKey] = iValue;
}

void SKGDocument::addSqlResultInCache(const QString &iKey, const SKGStringListList &iValue) const
{
    d->m_mutex.lock();
    (*d->m_cacheSql)[iKey] = iValue;
    d->m_mutex.unlock();
}

QString SKGDocument::getCachedValue(const QString &iKey) const
{
    return d->m_cache.value(iKey);
}

SKGStringListList SKGDocument::getCachedSqlResult(const QString &iKey) const
{
    return d->m_cacheSql->value(iKey);
}

void SKGDocument::refreshCache(const QString &iTable) const
{
    Q_UNUSED(iTable)
}

void SKGDocument::setBackupParameters(const QString &iPrefix, const QString &iSuffix) const
{
    d->m_backupPrefix = iPrefix;
    d->m_backupSuffix = iSuffix;
}

QString SKGDocument::getCurrentTemporaryFile() const
{
    return d->m_temporaryFile;
}

QString SKGDocument::getTemporaryFile(const QString &iFileName, bool iForceReadOnly)
{
    QString output;
    QFileInfo fi(iFileName);
    QFileInfo di(fi.dir().path());
    if (iForceReadOnly || !QUrl::fromUserInput(iFileName).isLocalFile() || !di.permission(QFile::WriteUser)) {
        output = QDir::tempPath();
    } else {
        output = fi.absolutePath();
    }
    return output += "/." % fi.fileName() % ".wrk";
}

QString SKGDocument::getBackupFile(const QString &iFileName) const
{
    QString output;
    if (!d->m_backupPrefix.isEmpty() || !d->m_backupSuffix.isEmpty()) {
        QFileInfo fi(iFileName);
        if (d->m_backupSuffix.endsWith(QStringLiteral(".skg"))) {
            output = d->m_backupPrefix % fi.baseName() % d->m_backupSuffix;
        } else {
            output = d->m_backupPrefix % fi.fileName() % d->m_backupSuffix;
        }
        output = output.replace(QStringLiteral("<DATE>"), SKGServices::dateToSqlString(QDateTime::currentDateTime().date()));
        output = output.replace(QStringLiteral("<TIME>"), SKGServices::timeToString(QDateTime::currentDateTime()));
        if (!QFileInfo(output).isAbsolute()) {
            output = fi.absolutePath() % u'/' % output;
        }
    }

    return output;
}

SKGError SKGDocument::getObjects(const QString &iTable, const QString &iWhereClause, SKGObjectBase::SKGListSKGObjectBase &oListObject) const
{
    SKGError err;

    // Initialisation
    oListObject.clear();

    // Execute sqlorder
    SKGStringListList result;
    err = executeSelectSqliteOrder(QString("SELECT * FROM " % iTable % (!iWhereClause.isEmpty() ? QString(" WHERE " % iWhereClause) : QString())), result);

    // Create output
    IFOK(err)
    {
        SKGStringListList::const_iterator itrow = result.constBegin();
        QStringList columns = *(itrow);
        ++itrow;
        for (; !err && itrow != result.constEnd(); ++itrow) {
            QStringList values = *(itrow);
            SKGObjectBase tmp(const_cast<SKGDocument *>(this), iTable);
            err = tmp.setAttributes(columns, values);
            oListObject.push_back(tmp);
        }
    }
    return err;
}

SKGError SKGDocument::existObjects(const QString &iTable, const QString &iWhereClause, bool &oExist) const
{
    SKGError err;

    // Initialisation
    oExist = false;

    // Execute sqlorder
    SKGStringListList result;
    err =
        executeSelectSqliteOrder("SELECT EXISTS(SELECT 1 FROM " % iTable % " WHERE " % (!iWhereClause.isEmpty() ? iWhereClause : QStringLiteral("1=1")) % u')',
                                 result);

    // Create output
    IFOK(err) oExist = (result.at(1).at(0) == QStringLiteral("1"));
    return err;
}

SKGError SKGDocument::getNbObjects(const QString &iTable, const QString &iWhereClause, int &oNbObjects) const
{
    SKGError err;

    // Initialisation
    oNbObjects = 0;

    // Execute sqlorder
    SKGStringListList result;
    err =
        executeSelectSqliteOrder(QString("SELECT count(1) FROM " % iTable % (!iWhereClause.isEmpty() ? QString(" WHERE " % iWhereClause) : QString())), result);

    // Create output
    IFOK(err) oNbObjects = SKGServices::stringToInt(result.at(1).at(0));
    return err;
}

SKGError SKGDocument::getObject(const QString &iTable, const QString &iWhereClause, SKGObjectBase &oObject) const
{
    SKGObjectBase::SKGListSKGObjectBase temporaryResult;
    oObject.resetID();
    SKGError err = SKGDocument::getObjects(iTable, iWhereClause, temporaryResult);
    IFOK(err)
    {
        int size = temporaryResult.size();
        if (Q_UNLIKELY(size > 1)) {
            err = SKGError(ERR_INVALIDARG,
                           i18nc("Error message: We expected only one object in the result, but got more",
                                 "More than one object returned in '%1' for '%2'",
                                 iTable,
                                 iWhereClause));
        } else {
            if (Q_UNLIKELY(size == 0)) {
                err = SKGError(ERR_INVALIDARG,
                               i18nc("Error message: We expected at least one object in the result, but got none",
                                     "No object returned in '%1' for '%2'",
                                     iTable,
                                     iWhereClause));
            } else {
                oObject = *(temporaryResult.begin());
            }
        }
    }
    return err;
}

SKGError SKGDocument::getObject(const QString &iTable, int iId, SKGObjectBase &oObject) const
{
    return getObject(iTable, "id=" % SKGServices::intToString(iId), oObject);
}

SKGError SKGDocument::getTablesList(QStringList &oResult) const
{
    return getDistinctValues(QStringLiteral("sqlite_master"), QStringLiteral("name"), QStringLiteral("type='table' AND name NOT LIKE 'sqlite_%'"), oResult);
}

SKGError SKGDocument::getDistinctValues(const QString &iTable, const QString &iAttribute, const QString &iWhereClause, QStringList &oResult) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // initialisation
    oResult.clear();

    // Search
    SKGStringListList temporaryResult;
    err = executeSelectSqliteOrder("SELECT DISTINCT " % iAttribute % " FROM " % iTable % " WHERE ("
                                       % (!iWhereClause.isEmpty() ? iWhereClause : QStringLiteral("1=1")) % ") ORDER BY "
                                       % iAttribute
                                       // Correction bug 202167 vvv
                                       % " COLLATE NOCASE"
                                   // Correction bug 202167 ^^^
                                   ,
                                   temporaryResult);
    IFOK(err)
    {
        SKGStringListList::const_iterator it = temporaryResult.constBegin();
        ++it; // to forget column name
        for (; it != temporaryResult.constEnd(); ++it) {
            oResult.push_back(*(it->constBegin()));
        }
    }

    return err;
}

SKGError SKGDocument::getDistinctValues(const QString &iTable, const QString &iAttribute, QStringList &oResult) const
{
    return getDistinctValues(iTable, iAttribute, iAttribute % " IS NOT NULL AND " % iAttribute % "!=''", oResult);
}

SKGError SKGDocument::executeSqliteOrder(const QString &iSqlOrder, int *iLastId) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (getMainDatabase() == nullptr) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
    } else {
        err = SKGServices::executeSqliteOrder(*getMainDatabase(), iSqlOrder, iLastId);
    }
    return err;
}

SKGError SKGDocument::executeSqliteOrder(const QString &iSqlOrder) const
{
    return SKGDocument::executeSqliteOrder(iSqlOrder, nullptr);
}

SKGError SKGDocument::executeSqliteOrders(const QStringList &iSqlOrders) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (getMainDatabase() == nullptr) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
    } else {
        err = SKGServices::executeSqliteOrders(*getMainDatabase(), iSqlOrders);
    }
    return err;
}

SKGError SKGDocument::executeSqliteOrder(const QString &iSqlOrder, const QMap<QString, QVariant> &iBind, int *iLastId) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (getMainDatabase() == nullptr) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
    } else {
        err = SKGServices::executeSqliteOrder(*getMainDatabase(), iSqlOrder, iBind, iLastId);
    }
    return err;
}

SKGError SKGDocument::dumpSelectSqliteOrder(const QString &iSqlOrder, QTextStream *oStream, SKGServices::DumpMode iMode) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (getMainDatabase() == nullptr) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
    } else {
        err = SKGServices::dumpSelectSqliteOrder(*getMainDatabase(), iSqlOrder, oStream, iMode);
    }
    return err;
}

SKGError SKGDocument::dumpSelectSqliteOrder(const QString &iSqlOrder, QString &oResult, SKGServices::DumpMode iMode) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (getMainDatabase() == nullptr) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
    } else {
        err = SKGServices::dumpSelectSqliteOrder(*getMainDatabase(), iSqlOrder, oResult, iMode);
    }
    return err;
}

SKGError SKGDocument::dumpSelectSqliteOrder(const QString &iSqlOrder, QStringList &oResult, SKGServices::DumpMode iMode) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    if (getMainDatabase() == nullptr) {
        err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
    } else {
        err = SKGServices::dumpSelectSqliteOrder(*getMainDatabase(), iSqlOrder, oResult, iMode);
    }
    return err;
}

SKGError SKGDocument::executeSingleSelectSqliteOrder(const QString &iSqlOrder, QString &oResult, bool iUseCache) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // Is the select in cache?
    if (iUseCache && d->m_cache.contains(iSqlOrder)) {
        // Yes => get the value
        oResult = getCachedValue(iSqlOrder);
        SKGTRACEL(10) << "Result retrieved from cache for:" << iSqlOrder << Qt::endl;

    } else {
        // No => Run the select
        oResult.clear();
        double elapse = SKGServices::getMicroTime();
        if (getMainDatabase() == nullptr) {
            err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
        } else {
            err = SKGServices::executeSingleSelectSqliteOrder(*getMainDatabase(), iSqlOrder, oResult);
        }

        // Is the execution time too long
        if (SKGServices::getMicroTime() - elapse > 50) {
            // Yes => put the result in cache
            addValueInCache(iSqlOrder, oResult);
        }
    }
    return err;
}

SKGError SKGDocument::executeSelectSqliteOrder(const QString &iSqlOrder, SKGStringListList &oResult, bool iUseCache) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // Is the select in cache?
    if (iUseCache && d->m_cacheSql->contains(iSqlOrder)) {
        // Yes => get the value
        oResult = getCachedSqlResult(iSqlOrder);
        SKGTRACEL(10) << "Result retrieved from cache for:" << iSqlOrder << Qt::endl;

    } else {
        // No => Run the select
        oResult.clear();
        double elapse = SKGServices::getMicroTime();
        if (getMainDatabase() == nullptr) {
            err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
        } else {
            err = SKGServices::executeSelectSqliteOrder(*getMainDatabase(), iSqlOrder, oResult);
        }

        // Is the execution time too long
        if (SKGServices::getMicroTime() - elapse > 50) {
            // Yes => put the result in cache
            addSqlResultInCache(iSqlOrder, oResult);
        }
    }
    return err;
}

void SKGDocument::concurrentExecuteSelectSqliteOrder(const QString &iSqlOrder, const FuncSelect &iFunction, bool iExecuteInMainThread) const
{
    // Is the select in cache?
    if (d->m_cacheSql->contains(iSqlOrder)) {
        // Yes => get the value and call the function
        iFunction(getCachedSqlResult(iSqlOrder));
        SKGTRACEL(10) << "Result retrieved from cache for:" << iSqlOrder << Qt::endl;

    } else {
        // No => Run the select asynchronously
        // Search a watcher free
        QFutureWatcher<SKGStringListList> *watcher = nullptr;
        if (iExecuteInMainThread) {
            for (auto w : std::as_const(d->m_watchers)) {
                if (w->isFinished()) {
                    watcher = w;
                    break;
                }
            }
            if (watcher == nullptr) {
                d->m_watchers.push_back(new QFutureWatcher<SKGStringListList>());
                watcher = d->m_watchers.at(d->m_watchers.count() - 1);
            }

            watcher->disconnect();
            connect(watcher, &QFutureWatcherBase::finished, this, [this, iFunction] {
                auto w = dynamic_cast<QFutureWatcher<SKGStringListList> *>(sender());
                iFunction(w->result());
            });
        }

        // Launch in another thread the select
        const auto &future = QtConcurrent::run([this, iSqlOrder, iFunction, iExecuteInMainThread] {
            const auto &tmpDatabase = this->getThreadDatabase();
            double elapse = SKGServices::getMicroTime();
            SKGStringListList listTmp;
            SKGServices::executeSelectSqliteOrder(tmpDatabase, iSqlOrder, listTmp);

            // Is the execution time too long
            if (SKGServices::getMicroTime() - elapse > 50) {
                // Yes => put the result in cache
                addSqlResultInCache(iSqlOrder, listTmp);
            }
            if (!iExecuteInMainThread) {
                iFunction(listTmp);
            }
            return listTmp;
        });
        if (watcher != nullptr) {
            watcher->setFuture(future);
        }
    }
}

void SKGDocument::concurrentExistObjects(const QString &iTable, const QString &iWhereClause, const FuncExist &iFunction, bool iExecuteInMainThread)
{
    // Execute sqlorder
    concurrentExecuteSelectSqliteOrder(
        "SELECT EXISTS(SELECT 1 FROM " % iTable % " WHERE " % (!iWhereClause.isEmpty() ? iWhereClause : QStringLiteral("1=1")) % u')',
        [iFunction](const SKGStringListList &iResult) {
            iFunction(iResult.count() > 0 && (iResult.at(1).at(0) == QStringLiteral("1")));
        },
        iExecuteInMainThread);
}

SKGError SKGDocument::getAttributesDescription(const QString &iTable, SKGServices::SKGAttributesList &oResult) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    // initialisation
    oResult.clear();

    // Search
    SKGStringListList temporaryResult;
    err = this->executeSelectSqliteOrder("PRAGMA table_info( " % iTable % " );", temporaryResult);
    IFOK(err)
    {
        int nblines = temporaryResult.count();
        QString realTable = SKGServices::getRealTable(iTable);

        for (int i = 1; i < nblines; ++i) { // the first one is ignored because it is the headers
            QStringList line = temporaryResult.at(i);

            SKGServices::SKGAttributeInfo attribute;
            attribute.name = line[1];

            QString attname = realTable % u'.' % attribute.name;
            attribute.display = getDisplay(attname);
            if (attribute.display == attname) {
                attribute.display = QString();
            }
            attribute.icon = getIcon(attname);
            attribute.type = getAttributeType(attribute.name);
            attribute.notnull = (line[3] == QStringLiteral("0"));
            attribute.defaultvalue = line[4];
            oResult.push_back(attribute);
        }
    }

    return err;
}

SKGError SKGDocument::getAttributesList(const QString &iTable, QStringList &oResult) const
{
    SKGError err;
    _SKGTRACEINFUNCRC(10, err)
    oResult.clear();
    SKGServices::SKGAttributesList attDesc;
    err = getAttributesDescription(iTable, attDesc);
    int nblines = attDesc.count();
    for (int i = 0; !err && i < nblines; ++i) {
        oResult.push_back(attDesc.at(i).name);
    }
    return err;
}

SKGReport *SKGDocument::getReport() const
{
    return new SKGReport(const_cast<SKGDocument *>(this));
}

SKGError SKGDocument::copyToJson(QString &oDocument) const
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)
    QVariantMap doc;

    // Copy the tables
    QVariantList list;
    QStringList listTables = getMainDatabase()->tables();
    int nb = listTables.count();
    for (int i = 0; !err && i < nb; ++i) {
        const QString &tableName = listTables.at(i);
        if (Q_UNLIKELY(!tableName.startsWith(QStringLiteral("sqlite_")) && !tableName.startsWith(QStringLiteral("vm_")))) {
            list.clear();

            SKGStringListList listRows;
            if (getMainDatabase() == nullptr) {
                err = SKGError(ERR_POINTER, i18nc("Error message", "No database defined"));
            } else {
                err = SKGServices::executeSelectSqliteOrder(*getMainDatabase(), "SELECT * FROM " % tableName, listRows);
            }
            int nbRows = listRows.count();
            if (Q_LIKELY(nbRows)) {
                QVariantMap item;
                const QStringList &titles = listRows.at(0);
                for (int j = 1; !err && j < nbRows; ++j) { // Forget title
                    const QStringList &values = listRows.at(j);

                    int nbVals = values.count();
                    for (int k = 0; k < nbVals; ++k) {
                        const QString &t = titles.at(k);
                        SKGServices::AttributeType type = getAttributeType(t);
                        if (type == SKGServices::ID || type == SKGServices::INTEGER || type == SKGServices::LINK) {
                            item.insert(t, SKGServices::stringToInt(values.at(k)));
                        } else if (type == SKGServices::FLOAT) {
                            item.insert(t, SKGServices::stringToDouble(values.at(k)));
                        } else if (type == SKGServices::BOOL) {
                            item.insert(t, values.at(k) == QStringLiteral("Y"));
                        } else {
                            item.insert(t, values.at(k));
                        }
                    }

                    list << item;
                }
            }
            doc.insert(tableName, list);
        }
    }

    QJsonDocument serializer = QJsonDocument::fromVariant(doc);
    oDocument = serializer.toJson(QJsonDocument::Compact);
    return err;
}

Q_DECLARE_OPAQUE_POINTER(sqlite3 *) // NOLINT(readability/casting)
Q_DECLARE_METATYPE(sqlite3 *) // NOLINT(readability/casting)
