/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTESTMACRO_H
#define SKGTESTMACRO_H
/** @file
 * This file contains macro for unit tests.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qcoreapplication.h>
#include <qdir.h>
#include <qfile.h>

#include <kaboutdata.h>

#include "skgdocument.h"
#include "skgerror.h"
#include "skgnodeobject.h"
#include "skgpropertyobject.h"
#include "skgservices.h"
#include "skgtraces.h"
#include "skgtransactionmng.h"

class SKGTest
{
public:
    /**
     * Return the test directory
     * @param iPath can be IN or OUT or REF
     * @return the test directory
     */
    static QString getTestPath(const QString &iPath)
    {
        QString pPath = SKGServices::getEnvVariable(iPath);
        if (pPath.isEmpty()) {
            return QStringLiteral("./tests/") % (iPath == QStringLiteral("IN") ? "input" : (iPath == QStringLiteral("OUT") ? "output" : "ref"));
        }
        return pPath;
    }
};

/**
 * @def SKGINITTEST(SHOWERRONLY)
 * Initialise the test
 */
#define SKGINITTEST(SHOWERRONLY)                                                                                                                               \
    QCoreApplication app(argc, argv);                                                                                                                          \
    app.setApplicationName(QStringLiteral("qttest"));                                                                                                          \
    KAboutData::setApplicationData(KAboutData(QStringLiteral("qttest"), QStringLiteral("qttest"), QStringLiteral("1.0")));                                     \
    KLocalizedString::setApplicationDomain("skrooge");                                                                                                         \
    KLocalizedString::setLanguages(QStringList() << QStringLiteral("en_GB"));                                                                                  \
    QLocale::setDefault(QLocale(QLocale::English));                                                                                                            \
    SKGTRACESEPARATOR;                                                                                                                                         \
    SKGTRACE << "STARTING TEST" << Qt::endl << Qt::flush;                                                                                                      \
    SKGTRACESEPARATOR;                                                                                                                                         \
    bool showonlyfailures = SHOWERRONLY;                                                                                                                       \
    if (showonlyfailures) {                                                                                                                                    \
        SKGTRACE << "Only failures will be displayed" << Qt::endl;                                                                                             \
    }                                                                                                                                                          \
    int nberror = 0;                                                                                                                                           \
    int nbcheck = 0;

/**
 * @def SKGENDTEST()
 * Exit test
 */
#define SKGENDTEST()                                                                                                                                           \
    if (nbcheck > 0) {                                                                                                                                         \
        SKGTRACE << nberror << " errors / " << nbcheck << " checks =" << (100.0 * (static_cast<double>(nberror)) / (static_cast<double>(nbcheck))) << "%"      \
                 << Qt::endl;                                                                                                                                  \
    }                                                                                                                                                          \
    SKGTraces::dumpProfilingStatistics();                                                                                                                      \
    return nberror;

/**
 * @def SKGRETURNTEST()
 * Return test
 */
#define SKGRETURNTEST()                                                                                                                                        \
    if (nbcheck > 0) {                                                                                                                                         \
        SKGTRACE << nberror << " errors / " << nbcheck << " checks =" << (100.0 * (static_cast<double>(nberror)) / (static_cast<double>(nbcheck))) << "%"      \
                 << Qt::endl;                                                                                                                                  \
    }                                                                                                                                                          \
    return nberror;

/**
 * @def SKGTEST(MESSAGE, RESULT, EXPECTEDRESULT)
 * To check the return of a method
 * Example of usage:
 * @code
 * SKGTEST(QStringLiteral("ERR:getHistoricalSize"), err.getHistoricalSize(), 0)
 * @endcode
 */
#define SKGTEST(MESSAGE, RESULT, EXPECTEDRESULT)                                                                                                               \
    if ((RESULT) == (EXPECTEDRESULT)) {                                                                                                                        \
        if (!showonlyfailures) {                                                                                                                               \
            SKGTRACE << "Test [" << (MESSAGE) << "] : OK" << Qt::endl;                                                                                         \
        }                                                                                                                                                      \
    } else {                                                                                                                                                   \
        SKGTRACE << "!!! Test [" << (MESSAGE) << "] : KO in line " << __LINE__ << Qt::endl;                                                                    \
        SKGTRACE << "     Expected: [" << (EXPECTEDRESULT) << ']' << Qt::endl;                                                                                 \
        SKGTRACE << "     Result  : [" << (RESULT) << ']' << Qt::endl;                                                                                         \
        ++nberror;                                                                                                                                             \
    }                                                                                                                                                          \
    ++nbcheck;
/**
 * @def SKGTESTBOOL(MESSAGE, RESULT, EXPECTEDRESULT)
 * To check the return of a method returning a boll
 * Example of usage:
 * @code
 * SKGTESTBOOL("isWarning", err.isWarning(), true)
 * @endcode
 */
#define SKGTESTBOOL(MESSAGE, RESULT, EXPECTEDRESULT)                                                                                                           \
    if ((RESULT) == (EXPECTEDRESULT)) {                                                                                                                        \
        if (!showonlyfailures) {                                                                                                                               \
            SKGTRACE << "Test [" << (MESSAGE) << "] : OK" << Qt::endl;                                                                                         \
        }                                                                                                                                                      \
    } else {                                                                                                                                                   \
        SKGTRACE << "!!! Test [" << (MESSAGE) << "] : KO in line " << __LINE__ << Qt::endl;                                                                    \
        SKGTRACE << "     Expected: [" << ((EXPECTEDRESULT) ? "TRUE" : "FALSE") << ']' << Qt::endl;                                                            \
        SKGTRACE << "     Result  : [" << ((RESULT) ? "TRUE" : "FALSE") << ']' << Qt::endl;                                                                    \
        ++nberror;                                                                                                                                             \
    }                                                                                                                                                          \
    ++nbcheck;

/**
 * @def SKGTESTERROR(MESSAGE, RESULT, EXPECTEDRESULT)
 * To check the return of a method
 * Example of usage:
 * @code
 * SKGTESTERROR(QStringLiteral("DOC:setParameter"), document1.setParameter(QStringLiteral("ATT1"), QStringLiteral("VAL1")), true)
 * @endcode
 */
#define SKGTESTERROR(MESSAGE, RESULT, EXPECTEDRESULT)                                                                                                          \
    {                                                                                                                                                          \
        SKGError _err_ = RESULT;                                                                                                                               \
        if (!_err_ == (EXPECTEDRESULT)) {                                                                                                                      \
            if (!showonlyfailures) {                                                                                                                           \
                SKGTRACE << "Test [" << (MESSAGE) << "] : OK" << Qt::endl;                                                                                     \
                if (!(EXPECTEDRESULT)) {                                                                                                                       \
                    SKGTRACE << "     Error Message  :\n" << _err_.getFullMessageWithHistorical() << Qt::endl;                                                 \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        } else {                                                                                                                                               \
            SKGTRACE << "!!! Test [" << (MESSAGE) << "] : KO in line " << __LINE__ << Qt::endl;                                                                \
            SKGTRACE << "     Expected: [" << ((EXPECTEDRESULT) ? "TRUE" : "FALSE") << ']' << Qt::endl;                                                        \
            SKGTRACE << "     Result  : [" << (!_err_ ? "TRUE" : "FALSE") << ']' << Qt::endl;                                                                  \
            SKGTRACE << "     Error Message  :\n" << _err_.getFullMessageWithHistorical() << Qt::endl;                                                         \
            ++nberror;                                                                                                                                         \
        }                                                                                                                                                      \
        ++nbcheck;                                                                                                                                             \
    }

/**
 * @def SKGTESTACCOUNT(DOC, ACCOUNT, AMOUNT)
 * To check the account amount
 * Example of usage:
 * @code
 * SKGTESTACCOUNT(document1, QStringLiteral("act1"), 12345);
 * @endcode
 */
#define SKGTESTACCOUNT(DOC, ACCOUNT, AMOUNT)                                                                                                                   \
    {                                                                                                                                                          \
        SKGAccountObject account(&(DOC));                                                                                                                      \
        SKGTESTERROR((ACCOUNT) % ".setName(QStringLiteral(" % (ACCOUNT) % "))", account.setName(ACCOUNT), true)                                                \
        SKGTESTERROR((ACCOUNT) % ".load(QStringLiteral(" % (ACCOUNT) % "))", account.load(), true)                                                             \
        if (qAbs(account.getCurrentAmount() - AMOUNT) > 10e-4) {                                                                                               \
            SKGTEST((ACCOUNT) % ".getCurrentAmount(" % (ACCOUNT) % ")",                                                                                        \
                    SKGServices::toCurrencyString(account.getCurrentAmount(), QStringLiteral("Euros"), 2),                                                     \
                    SKGServices::toCurrencyString(AMOUNT, QStringLiteral("Euros"), 2))                                                                         \
        }                                                                                                                                                      \
    }
#endif

/**
 * @def SKGTESTTRIGGERACTION(PLUGIN)
 * To trigger an action
 */
#define SKGTESTTRIGGERACTION(NAME)                                                                                                                             \
    {                                                                                                                                                          \
        QAction *act = plugin.action(NAME);                                                                                                                    \
        QVERIFY(act != nullptr);                                                                                                                               \
        act->trigger();                                                                                                                                        \
    }

/**
 * @def SKGTESTPLUGIN(PLUGIN)
 * To do common checks on a plugin
 */
#define SKGTESTPLUGIN(PLUGIN, DOC)                                                                                                                             \
    {                                                                                                                                                          \
        QVERIFY(!PLUGIN.setupActions(nullptr));                                                                                                                \
        QVERIFY(PLUGIN.setupActions(&DOC));                                                                                                                    \
        QVERIFY(!PLUGIN.title().isEmpty());                                                                                                                    \
        QVERIFY(!PLUGIN.icon().isEmpty());                                                                                                                     \
        QVERIFY(!PLUGIN.toolTip().isEmpty());                                                                                                                  \
        QVERIFY(!PLUGIN.statusTip().isEmpty());                                                                                                                \
        QVERIFY(PLUGIN.getOrder() > 0);                                                                                                                        \
        SKGTabPage *page = plugin.getWidget();                                                                                                                 \
        plugin.getPreferenceWidget();                                                                                                                          \
        plugin.getPreferenceSkeleton();                                                                                                                        \
        plugin.savePreferences();                                                                                                                              \
        if (page)                                                                                                                                              \
            page->setState(page->getState());                                                                                                                  \
        int nb = PLUGIN.getNbDashboardWidgets();                                                                                                               \
        for (int i = 0; i < nb; ++i) {                                                                                                                         \
            QVERIFY(!PLUGIN.getDashboardWidgetTitle(i).isEmpty());                                                                                             \
        }                                                                                                                                                      \
        auto tips = PLUGIN.tips();                                                                                                                             \
        for (int i = 0; i < tips.count(); ++i) {                                                                                                               \
            QVERIFY(!tips.at(i).isEmpty());                                                                                                                    \
        }                                                                                                                                                      \
        auto subplugins = PLUGIN.subPlugins();                                                                                                                 \
        for (int i = 0; i < subplugins.count(); ++i) {                                                                                                         \
            QVERIFY(!subplugins.at(i).isEmpty());                                                                                                              \
        }                                                                                                                                                      \
        QStringList iIgnoredAdvice;                                                                                                                            \
        SKGAdviceList adv = PLUGIN.advice(iIgnoredAdvice);                                                                                                     \
        for (int i = 0; i < adv.count(); ++i) {                                                                                                                \
            QVERIFY(!adv.at(i).getUUID().isEmpty());                                                                                                           \
        }                                                                                                                                                      \
    }
