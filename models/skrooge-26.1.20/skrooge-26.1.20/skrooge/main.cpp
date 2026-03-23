/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file defines the main of skrooge.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgdocumentbank.h"
#include "skgmainpanel.h"
#include "skgtraces.h"

#include <kaboutdata.h>
#ifdef SKG_DBUS
#include <kdbusservice.h>
#endif
#include <klocalizedstring.h>

#include <QtQuickControls2/qquickstyle.h>
#include <qbitmap.h>
#include <qcommandlineoption.h>
#include <qcommandlineparser.h>
#include <qsplashscreen.h>

/**
 * To compute the version
 */
#define VER1_(x) #x
/**
 * To compute the version
 */
#define VER_(x) VER1_(x)
/**
 * To compute the version
 */
#define VER VER_(SKGVERSION)

void SKGMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    switch (type) {
    case QtDebugMsg:
        SKGTRACEL(1) << "DEBUG: " << msg << Qt::endl;
        break;
    case QtWarningMsg:
        SKGTRACE << "WARNING: " << msg << Qt::endl;
        break;
    case QtCriticalMsg:
        SKGTRACE << "CRITICAL: " << msg << Qt::endl;
        break;
    case QtFatalMsg:
        SKGTRACE << "FATAL: " << msg << Qt::endl;
        abort();
    default:
        SKGTRACE << "INFO: " << msg << Qt::endl;
        break;
    }
}

/**
 * The main of the application
 * @param argc number of arguments
 * @param argv arguments
 * @return return code
 */
int main(int argc, char **argv)
{
#ifdef SKG_WEBENGINE
    QApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
#endif

    QApplication app(argc, argv);
    QIcon appIcon = SKGServices::fromTheme(QStringLiteral("skrooge"));
    if (!appIcon.isNull()) {
        app.setWindowIcon(appIcon);
    }

    // To use CPU instead CPU for QML (needed for printing)
    qputenv("QMLSCENE_DEVICE", "softwarecontext");

#ifdef SKG_WEBENGINE
    // DrKonqi
    // https://www.dvratil.cz/2018/10/drkonqi-and-qtwebengine/
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", " --disable-in-process-stack-traces");
#endif
    KLocalizedString::setApplicationDomain("skrooge");

    KAboutData about(QStringLiteral("skrooge"),
                     i18nc("The name of the application", "Skrooge"),
                     QStringLiteral(VER),
                     i18nc("The description of the application", "Personal finances management made simple"),
                     KAboutLicense::GPL_V3,
                     i18nc("Fullname", "(c) 2007-%1 Stephane MANKOWSKI & Guillaume DE BURE", QDate::currentDate().toString(QStringLiteral("yyyy"))),
                     QString(),
                     QStringLiteral("https://skrooge.org"));

    about.addAuthor(i18nc("Fullname", "Stephane MANKOWSKI"),
                    i18nc("A job description", "Architect & Developer"),
                    QStringLiteral("stephane@mankowski.fr"),
                    QString(),
                    QStringLiteral("miraks"));
    about.addAuthor(i18nc("Fullname", "Guillaume DE BURE"),
                    i18nc("A job description", "Developer"),
                    QStringLiteral("guillaume.debure@gmail.com"),
                    QString(),
                    QStringLiteral("willy9"));
    about.addAuthor(i18nc("Fullname", "Siddharth SHARMA"),
                    i18nc("A job description", "Developer - Google Summer Of Code 2010"),
                    QStringLiteral("siddharth.kde@gmail.com"),
                    QString(),
                    QStringLiteral("h4xordood"));
    about.addAuthor(i18nc("Fullname", "Mathieu STERN"), i18nc("A job description", "Splash screen photographer"), QString(), QString(), QString());
    about.setOtherText(i18nc("The description of the application",
                             "The application name is inspired by Charles Dicken's tale <i>A Christmas Carol</i>, where the main character, Ebenezer Scrooge, "
                             "a grumpy old narrow man, gets visited by three ghosts who change the way he sees the world, in a good way."));
    about.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    about.setOrganizationDomain("kde.org");
    about.setDesktopFileName(QStringLiteral("org.kde.skrooge"));

    about.addCredit(QStringLiteral("vicnet, noidea, rbruce, JesusM, schunka, SylvaiNN, Wolf, Hizoka, neutron68, blep0, BigaAl, steffie, skierpage …"),
                    i18nc("Reason of the about/credit", "Users helping us to improve this application"));

    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    parser.addPositionalArgument(QStringLiteral("URL"), i18nc("Application argument", "Document to open"));
    QCommandLineOption envOption(QStringList() << QStringLiteral("e") << QStringLiteral("env"),
                                 i18nc("Application argument", "Display environment variables used by this application."));
    parser.addOption(envOption);

    about.setupCommandLine(&parser);
    parser.process(app);
    about.processCommandLine(&parser);

    if (parser.isSet(envOption)) {
        SKGTRACESUITE << parser.helpText() << Qt::endl;
        SKGTRACESUITE << i18nc("Help", "Environment variables:") << Qt::endl;
        SKGTRACESUITE << i18nc("Help, do not translate x",
                               "  %1: To enable traces. x is the level of traces expected. This enables the debug mode too.",
                               "export SKGTRACE=x")
                      << Qt::endl;
        SKGTRACESUITE << i18nc("Help", "  %1: To enable the profiling. This enables the debug mode too.", "export SKGTRACEPERFO=1") << Qt::endl;
        SKGTRACESUITE << i18nc("Help do not translate x", "  %1: To dump sql order taking more than x ms.", "export SKGTRACESQL=x") << Qt::endl;
        SKGTRACESUITE << i18nc("Help", "  %1: To enable the high DPI mode.", "export SKGHIGHDPI=1") << Qt::endl;
        return 0;
    }

    // Manage unicity
#ifdef SKG_DBUS
    KDBusService service(SKGServices::getEnvVariable(QStringLiteral("SKGNOTUNIQUE")).isEmpty() ? KDBusService::Unique : KDBusService::Multiple);
    QObject::connect(&service, &KDBusService::activateRequested, &service, [](const QStringList &arguments, const QString &workingDirectory) {
        Q_UNUSED(workingDirectory)
        SKGMainPanel::getMainPanel()->processArguments(arguments);
    });
#endif

    qInstallMessageHandler(SKGMessageOutput);

    // Creating a main panel on a bank document
    SKGDocumentBank doc;
    if (!SKGServices::getEnvVariable(QStringLiteral("SKGTEST")).isEmpty()) {
        QTimer::singleShot(5000, Qt::CoarseTimer, &app, &QApplication::quit);
    }

    // Build list of arguments
    QStringList argument = parser.positionalArguments();

    // Creation splash screen
    QSplashScreen *m_splash = nullptr;
    KConfigGroup pref = SKGMainPanel::getMainConfigGroup();
    if (pref.readEntry("show_splash_screen", true)) {
        QString splashPathRelativePath = KAboutData::applicationData().componentName() % QStringLiteral("/images/splash.png");
        QString splashPath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, splashPathRelativePath.toLatin1());
        if (!splashPath.isEmpty()) {
            QPixmap pix(splashPath);

            m_splash = new QSplashScreen(pix);
            if (m_splash != nullptr) {
                m_splash->setMask(pix.createMaskFromColor(Qt::blue));
                m_splash->show();
                m_splash->showMessage(i18nc("Splash screen message", "Loading …"), Qt::AlignLeft, QColor(221, 130, 8)); // krazy:exclude=qmethods
            }
        } else {
            SKGTRACE << "WARNING: Splash screen (" << splashPathRelativePath << ") not found !" << Qt::endl;
        }
    }

    // First instance
    QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings, true);
    QQuickStyle::setFallbackStyle(QStringLiteral("Material"));
    auto m_widget = new SKGMainPanel(m_splash, &doc);
    m_widget->processArguments(argument);
    m_widget->setUnifiedTitleAndToolBarOnMac(true);
    m_widget->show();

    if (m_splash != nullptr) {
        SKGTRACEINFUNC(1)

        m_splash->clearMessage();
        m_splash->finish(m_widget);
    }

    int rc = QApplication::exec(); // krazy:exclude=crashy

    delete m_splash;

    SKGTraces::dumpProfilingStatistics();
    return rc;
}
