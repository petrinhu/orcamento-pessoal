/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTRACES_H
#define SKGTRACES_H
/** @file
 * This file defines classes SKGTraces an macros.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qmap.h>
#include <qstack.h>
#include <qstringlist.h>
#include <qtextstream.h>

#include "skgdefine.h"

/**
 * Macro for traces
 */
#define SKGTRACE SKGTRACESUITE << SKGTraces::SKGIndentTrace
/**
 * Macro for traces
 */
#define SKGTRACESUITE SKGTraces::SKGCout
/**
 * Macro for traces
 */
#define SKGTRACESEPARATOR SKGTRACE << "##############################################\n" << Qt::flush;

#ifdef SKGNOTRACES
/**
 * Macro for traces
 */
#define IFSKGTRACEL(Level) if (false)
#else
/**
 * Macro for traces
 */
#define IFSKGTRACEL(Level) if ((Level) <= SKGTraces::SKGLevelTrace)
#endif

/**
 * Macro for traces
 */
#define SKGTRACEL(Level) SKGTRACESUITEL(Level) << SKGTraces::SKGIndentTrace
/**
 * Macro for traces
 */
#define SKGTRACESUITEL(Level) IFSKGTRACEL(Level) SKGTRACESUITE

/**
 * This structure represents one performance measure
 */
struct SKGPerfoInfo {
    /** The number of call for the method */
    int NbCall;
    /** The global time passed in the method */
    double Time;
    /** The time consumed by the method */
    double TimePropre;
    /** The minimum time passed in the method */
    double TimeMin;
    /** The maximum time passed in the method */
    double TimeMax;
};

/**
 * A map of strings ==> SKGPerfoInfo
 * @see SKGPerfoMapIterator
 */
using SKGPerfoMap = QMap<QString, SKGPerfoInfo>;

/**
 * A iterator for SKGPerfoMap ==> SKGPerfoInfo
 * @see SKGPerfoMap
 */
using SKGPerfoMapIterator = QMap<QString, SKGPerfoInfo>::Iterator;

/**
 * A stack of strings
 */
using SKGQStringStack = QStack<QString>;

class SKGError;

/**
 * This class manages traces
 */
class SKGBASEMODELER_EXPORT SKGTraces final
{
public:
    /**
     * Constructor
     * @param iName The message to display
     * @param iLevel The level to display this error.
     * The error will be display if the level of traces asked is greater or equal than
     * the level of this trace (iLevel)
     * @param iRC A pointer of the error object of the calling method
     *   The SKGError
     */
    explicit SKGTraces(int iLevel, const char *iName, SKGError *iRC);

    /**
     * Constructor
     * @param iName The message to display
     * @param iLevel The level to display this error.
     * The error will be display if the level of traces asked is greater or equal than
     * the level of this trace (iLevel)
     * @param iRC A pointer of the error object of the calling method
     *   The SKGError
     */
    SKGTraces(int iLevel, const QString &iName, SKGError *iRC);

    /**
     * Destructor
     */
    ~SKGTraces();

    /**
     * Clean profiling statistics
     */
    static void cleanProfilingStatistics();

    /**
     * Get profiling statistics
     */
    static QStringList getProfilingStatistics();

    /**
     * Dump profiling statistics
     */
    static void dumpProfilingStatistics();

    /**
     * Standard output stream for traces
     */
    static QTextStream SKGCout;

    /**
     * The current level of indentation
     */
    static QString SKGIndentTrace;

    /**
     * The current level of taces
     */
    static int SKGLevelTrace;

    /**
     * To enable, disable profiling
     */
    static bool SKGPerfo;

private:
    Q_DISABLE_COPY(SKGTraces)
    void init(int iLevel, const QString &iName, SKGError *iRC);

    QString m_mame;
    bool m_output = false;
    bool m_profiling = false;
    SKGError *m_rc = nullptr;
    double m_elapse = 0.0;
    SKGPerfoMapIterator m_it;

    static SKGPerfoMap m_SKGPerfoMethode;
    static SKGQStringStack m_SKGPerfoPathMethode;
};

/**
 * Macro for traces
 */
#define TOKENPASTE(x, y) x##y

/**
 * Macro for traces
 */
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#ifdef SKGNOTRACES
/**
 * Macro for traces
 */
#define SKGTRACEINRC(Level, Name, RC)

/**
 * Macro for traces
 */
#define SKGTRACEIN(Level, Name)
#else
/**
 * Macro for traces
 */
#define SKGTRACEINRC(Level, Name, RC) SKGTraces TOKENPASTE2(trace1_, __LINE__)(Level, Name, &(RC));

/**
 * Macro for traces
 */
#define SKGTRACEIN(Level, Name) SKGTraces TOKENPASTE2(trace2_, __LINE__)(Level, Name, nullptr);
#endif

#ifdef SKGFULLTRACES
/**
 * Macro for traces
 */
#define _SKGTRACEINRC(Level, Name, RC) SKGTRACEINRC(Level, Name, RC)
/**
 * Macro for traces
 */
#define _SKGTRACEIN(Level, Name) SKGTRACEIN(Level, Name)
#else
/**
 * Macro for traces
 */
#define _SKGTRACEINRC(Level, Name, RC)
/**
 * Macro for traces
 */
#define _SKGTRACEIN(Level, Name)
#endif

/**
 * Macro for traces
 */
#define SKGTRACEINFUNCRC(Level, RC) SKGTRACEINRC(Level, Q_FUNC_INFO, RC)

/**
 * Macro for traces
 */
#define SKGTRACEINFUNC(Level) SKGTRACEIN(Level, Q_FUNC_INFO)

/**
 * Macro for traces
 */
#define _SKGTRACEINFUNCRC(Level, RC) _SKGTRACEINRC(Level, Q_FUNC_INFO, RC)

/**
 * Macro for traces
 */
#define _SKGTRACEINFUNC(Level) _SKGTRACEIN(Level, Q_FUNC_INFO)

#endif
