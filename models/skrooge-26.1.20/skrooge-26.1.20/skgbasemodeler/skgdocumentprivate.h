/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDOCUMENTPRIVATE_H
#define SKGDOCUMENTPRIVATE_H
/** @file
 * This file defines classes SKGDocumentPrivate.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include <qfuturewatcher.h>
#include <qhash.h>
#include <qsqldatabase.h>
#include <qstringlist.h>

#include <functional>

#include "skgdefine.h"
#include "skgerror.h"
#include "skgservices.h"

using FuncProgress = std::function<int(int, qint64, const QString &, void *)>;
using checksFunction = SKGError (*)(SKGDocument *);

/**
 * This class manages skg documents
 */
class SKGDocumentPrivate
{
public:
    /**
     * Constructor
     */
    explicit SKGDocumentPrivate();

    /**
     * Destructor
     */
    virtual ~SKGDocumentPrivate();

    static SKGError m_lastCallbackError;

    static int m_databaseUniqueIdentifier;

    int m_lastSavedTransaction{0};
    FuncProgress m_progressFunction{nullptr};
    QList<checksFunction> m_checkFunctions;
    void *m_progressData{nullptr};
    QString m_currentFileName;
    QString m_databaseIdentifier;
    QSqlDatabase m_currentDatabase;
    SKGIntList m_nbStepForTransaction;
    SKGIntList m_posStepForTransaction;
    QStringList m_nameForTransaction;
    int m_inundoRedoTransaction{0};
    int m_currentTransaction{0};
    qint64 m_timeBeginTransaction{0};
    QString m_temporaryFile;
    QString m_uniqueIdentifier;
    // SKGMessageList m_unTransactionnalMessages;
    QMap<QString, QStringList> m_ImpactedViews;
    QMap<QString, QStringList> m_MaterializedViews;

    QHash<QString, QString> m_cache;
    QHash<QString, SKGStringListList> *m_cacheSql;
    bool m_inProgress{false};
    QString m_backupPrefix;
    QString m_backupSuffix;
    bool m_directAccessDb{false};
    bool m_modeReadOnly{false};
    bool m_modeSQLCipher{false};
    QList<QFutureWatcher<SKGStringListList> *> m_watchers;

    bool m_blockEmits{false};
    QMutex m_mutex;
    QString m_password;
    bool m_password_got{false};
};
#endif
