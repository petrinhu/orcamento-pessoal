/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGTraces.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgtraces.h"
#include <utility>

#include <qiodevice.h>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/time.h>
#endif
#include "skgerror.h"
#include "skgservices.h"

/**
 * To generate a colorized string
 */
#define COLORED(TEXT)                                                                                                                                          \
    (QStringLiteral("\x1b[3") % QString::number(1 + (((SKGTraces::SKGIndentTrace.size() - 2) / 2) % 7)) % QLatin1Char('m') % (TEXT)                            \
     % QStringLiteral("\x1b[39m"))
/**
 * To generate a red string
 */
#define COLOREDRED(TEXT) (QStringLiteral("\x1b[31m") % (TEXT) % QStringLiteral("\x1b[39m"))

// ===================================================================
int SKGTraces::SKGLevelTrace = SKGServices::stringToInt(SKGServices::getEnvVariable(QStringLiteral("SKGTRACE")));
bool SKGTraces::SKGPerfo = !SKGServices::getEnvVariable(QStringLiteral("SKGTRACEPERFO")).isEmpty();
QString SKGTraces::SKGIndentTrace = QStringLiteral("##");
SKGPerfoMap SKGTraces::m_SKGPerfoMethode;
SKGQStringStack SKGTraces::m_SKGPerfoPathMethode;
QTextStream SKGTraces::SKGCout(stdout, QIODevice::WriteOnly);
// ===================================================================
SKGTraces::SKGTraces(int iLevel, const char *iName, SKGError *iRC)
{
    init(iLevel, iName, iRC);
}

SKGTraces::SKGTraces(int iLevel, const QString &iName, SKGError *iRC)
{
    init(iLevel, iName, iRC);
}

void SKGTraces::init(int iLevel, const QString &iName, SKGError *iRC)
{
    IFSKGTRACEL(iLevel)
    {
        m_mame = iName;
        m_output = true;

        m_rc = iRC;
        SKGIndentTrace += QStringLiteral("  ");

        SKGTRACE << COLORED(QLatin1Char('>') % m_mame) << Qt::endl;
    }
    else
    {
        m_rc = nullptr;
        m_output = false;
    }

    if (SKGPerfo) {
        m_profiling = true;
        m_mame = iName;

        // Push the method in the stack
        SKGTraces::m_SKGPerfoPathMethode.push(m_mame);

        // Pour les mesures de perfos
#ifdef Q_OS_WIN
        m_elapse = static_cast<double>(GetTickCount());
#else
        struct timeval tv {
        };
        struct timezone tz {
        };
        gettimeofday(&tv, &tz);
        m_elapse = (static_cast<double>(1000.0 * tv.tv_sec)) + (static_cast<double>(tv.tv_usec / 1000.0));
#endif

        // Searching the key in the map
        m_it = SKGTraces::m_SKGPerfoMethode.find(m_mame);
        if (m_it == SKGTraces::m_SKGPerfoMethode.end()) {
            // Not found ==> initialisation
            SKGPerfoInfo init{};
            init.NbCall = 0;
            init.Time = 0;
            init.TimePropre = 0;
            init.TimeMin = 99999999;
            init.TimeMax = -1;

            // Add the line
            SKGTraces::m_SKGPerfoMethode[m_mame] = init;

            // find again
            m_it = SKGTraces::m_SKGPerfoMethode.find(m_mame);
        }
    } else {
        m_profiling = false;
        m_elapse = -1;
    }
}

SKGTraces::~SKGTraces()
{
    // Get delta time
    if (m_elapse >= 0) {
#ifdef Q_OS_WIN
        m_elapse = static_cast<double>(GetTickCount());
#else
        struct timeval tv {
        };
        struct timezone tz {
        };
        gettimeofday(&tv, &tz);
        m_elapse = (static_cast<double>(1000.0 * tv.tv_sec)) + (static_cast<double>(tv.tv_usec / 1000.0)) - m_elapse;
#endif
    }

    if (m_output) {
        SKGTRACESUITE << SKGTraces::SKGIndentTrace << COLORED(QLatin1Char('<') % m_mame);
        if (m_rc != nullptr) {
            SKGTRACESUITE << (m_rc->isSucceeded() ? QString(COLORED(QStringLiteral(" RC=") % m_rc->getFullMessage()))
                                                  : QString(COLOREDRED(QStringLiteral(" RC=") % m_rc->getFullMessage())));
        }
        if (m_profiling) {
            SKGTRACESUITE << QStringLiteral(" TIME=") << m_elapse << QStringLiteral(" ms");
        }
        SKGTRACESUITE << Qt::endl;
        SKGIndentTrace.resize(SKGIndentTrace.length() - 2);

        m_rc = nullptr;
    }

    if (m_profiling) {
        // Update values
        ++(m_it.value().NbCall);
        m_it.value().Time += m_elapse;
        m_it.value().TimePropre += m_elapse;
        if (m_elapse > m_it.value().TimeMax) {
            m_it.value().TimeMax = m_elapse;
        }
        if (m_elapse < m_it.value().TimeMin) {
            m_it.value().TimeMin = m_elapse;
        }

        if (!SKGTraces::m_SKGPerfoPathMethode.empty() && SKGTraces::m_SKGPerfoPathMethode.top() == m_mame) {
            // Remove current method from stack
            SKGTraces::m_SKGPerfoPathMethode.pop();

            // Get previous method name
            if (!SKGTraces::m_SKGPerfoPathMethode.empty()) {
                QString previousMethode = std::as_const(SKGTraces::m_SKGPerfoPathMethode).top();

                // Searching the key in the map
                m_it = SKGTraces::m_SKGPerfoMethode.find(previousMethode);
                if (m_it != SKGTraces::m_SKGPerfoMethode.end()) {
                    m_it.value().TimePropre -= m_elapse;
                }
            }
        }
    }
}

void SKGTraces::cleanProfilingStatistics()
{
    SKGTraces::m_SKGPerfoMethode.clear();
}

void SKGTraces::dumpProfilingStatistics()
{
    QStringList dump = getProfilingStatistics();

    int nbl = dump.count();
    for (int i = 0; i < nbl; ++i) {
        SKGTRACE << dump.at(i) << Qt::endl;
    }
}

QStringList SKGTraces::getProfilingStatistics()
{
    QStringList output;
    if (SKGPerfo) {
        output.push_back(QStringLiteral("method ; nb call ; millisecondes ; average ; min ; max ; own time ; average own time"));

        SKGPerfoMap SKGPerfoMethodeCopy = m_SKGPerfoMethode;
        while (!SKGPerfoMethodeCopy.empty()) {
            // Recheche du temps propre maximal
            double maxtime = -1;
            SKGPerfoMapIterator max;
            SKGPerfoMapIterator it2;
            for (it2 = SKGPerfoMethodeCopy.begin(); it2 != SKGPerfoMethodeCopy.end(); ++it2) {
                if (it2.value().TimePropre > maxtime || maxtime == -1) {
                    maxtime = it2.value().TimePropre;
                    max = it2;
                }
            }

            // dump max
            if (maxtime != -1) {
                output.push_back(max.key() % QStringLiteral(" ; ") % SKGServices::intToString(max.value().NbCall) % QStringLiteral(" ; ")
                                 % SKGServices::doubleToString(max.value().Time) % QStringLiteral(" ; ")
                                 % SKGServices::doubleToString((max.value().Time) / (static_cast<double>(max.value().NbCall))) % QStringLiteral(" ; ")
                                 % SKGServices::doubleToString(max.value().TimeMin) % QStringLiteral(" ; ") % SKGServices::doubleToString(max.value().TimeMax)
                                 % QStringLiteral(" ; ") % SKGServices::doubleToString(max.value().TimePropre) % QStringLiteral(" ; ")
                                 % SKGServices::doubleToString((max.value().TimePropre) / (static_cast<double>(max.value().NbCall))));

                // Remove it
                SKGPerfoMethodeCopy.erase(max);
            }
        }
    }

    return output;
}
