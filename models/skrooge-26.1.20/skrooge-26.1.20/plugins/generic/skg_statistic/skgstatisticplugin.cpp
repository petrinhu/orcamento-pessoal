/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A plugin to generate statistic
 *
 * @author Stephane MANKOWSKI
 */
#include "skgstatisticplugin.h"

#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kpluginfactory.h>
#include <kstandardaction.h>

#include <qapplication.h>
#include <qcryptographichash.h>
#include <qdir.h>
#include <qjsondocument.h>
#include <qscreen.h>

#include "skgmainpanel.h"
#include "skgtraces.h"
#include <utility>

/**
 * This plugin factory.
 */
K_PLUGIN_CLASS_WITH_JSON(SKGStatisticPlugin, "metadata.json")

SKGStatisticPlugin::SKGStatisticPlugin(QWidget *iWidget, QObject *iParent, const KPluginMetaData & /*metaData*/, const QVariantList & /*iArg*/)
    : SKGInterfacePlugin(iParent)
    , m_currentDocument(nullptr)
{
    Q_UNUSED(iWidget)
    m_timeInit = QDateTime::currentDateTime();
    SKGTRACEINFUNC(10)

    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGStatisticPlugin::pageChanged);
    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::pageOpened, this, &SKGStatisticPlugin::pageOpened);
}

SKGStatisticPlugin::~SKGStatisticPlugin()
{
    SKGTRACEINFUNC(10)
    // Set duration
    m_stats[QStringLiteral("avg.exec_time_sec")] =
        (m_stats.value(QStringLiteral("avg.exec_time_sec")).toDouble() * (m_stats.value(QStringLiteral("nb_launch")).toInt() - 1)
         + m_timeInit.secsTo(QDateTime::currentDateTime()))
        / m_stats.value(QStringLiteral("nb_launch")).toInt();

    // Write stat file
    writeStats();

    m_currentDocument = nullptr;
}

bool SKGStatisticPlugin::setupActions(SKGDocument *iDocument)
{
    SKGTRACEINFUNC(10)

    m_currentDocument = iDocument;

    setComponentName(QStringLiteral("skg_statistic"), title());
    setXMLFile(QStringLiteral("skg_statistic.rc"));
    return true;
}

void SKGStatisticPlugin::refresh()
{
    SKGTRACEINFUNC(10)
    if (m_currentDocument != nullptr) {
        if (m_currentDocument->getMainDatabase() != nullptr) {
            static bool initialisationDone = false;
            if (!initialisationDone) {
                // Connect actions
                QMap<QString, QPointer<QAction>> actions = SKGMainPanel::getMainPanel()->getGlobalActions();
                QStringList keys = actions.keys();
                for (const auto &k : std::as_const(keys)) {
                    QPointer<QAction> act = actions[k];
                    connect(act.data(), &QAction::triggered, this, &SKGStatisticPlugin::triggerAction);
                }
                initialisationDone = true;
            }

            QString doc_id = m_currentDocument->getUniqueIdentifier();
            if (m_docUniqueIdentifier != doc_id) {
                m_docUniqueIdentifier = std::move(doc_id);

                // Initialize
                QString appname = KAboutData::applicationData().componentName();
                auto dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
                QDir(dir).mkdir(QStringLiteral("statistics"));
                QString docUUID = QString(QCryptographicHash::hash(m_currentDocument->getCurrentFileName().toLatin1(), QCryptographicHash::Md5).toHex());
                m_file = dir % "/statistics/" % docUUID % ".stats.txt";

                // Read previous stat file
                readStats();

                // Initial values
                if (!m_stats.contains(QStringLiteral("init.date"))) {
                    m_stats[QStringLiteral("init.date")] = QDate::currentDate();
                    m_stats[QStringLiteral("init.qt_version")] = qVersion();
                    m_stats[QStringLiteral("init.app_name")] = appname;
                    m_stats[QStringLiteral("init.app_version")] = KAboutData::applicationData().version();
                }
                m_stats[QStringLiteral("nb_launch")] = m_stats.value(QStringLiteral("nb_launch")).toInt() + 1;

                // Current values
                m_stats[QStringLiteral("current.date")] = QDate::currentDate();
                m_stats[QStringLiteral("current.qt_version")] = qVersion();
                m_stats[QStringLiteral("current.app_version")] = KAboutData::applicationData().version();
                m_stats[QStringLiteral("current.language")] = QLocale::languageToString(QLocale().language());
                m_stats[QStringLiteral("current.country")] = QLocale::territoryToString(QLocale().territory());
                m_stats[QStringLiteral("current.country.code")] = QLocale().name().split(QStringLiteral("_")).at(0);
                m_stats[QStringLiteral("current.locale")] = QLocale().name();

                // OS
#ifdef Q_OS_WIN32
                QString os = QStringLiteral("Windows");
#elif defined(Q_OS_FREEBSD)
                QString os = QStringLiteral("FreeBSD");
#elif defined(Q_OS_NETBSD)
                QString os = QStringLiteral("NetBSD");
#elif defined(Q_OS_OPENBSD)
                QString os = QStringLiteral("OpenBSD");
#elif defined(Q_OS_LINUX)
                QString os = QStringLiteral("Linux");
#elif defined(Q_OS_MAC)
                QString os = QStringLiteral("Mac OS");
#else
                QString os = QStringLiteral("Unknown");
#endif
                m_stats[QStringLiteral("current.os")] = os;
                QRect scr = QGuiApplication::primaryScreen()->geometry();
                m_stats[QStringLiteral("current.screen")] = QString(SKGServices::intToString(scr.width()) % u'x' % SKGServices::intToString(scr.height()));

#ifdef HAVE_UNAME
                struct utsname buf {
                };
                if (uname(&buf) != -1) {
                    m_stats[QStringLiteral("current.os.machine")] = QString::fromLocal8Bit(buf.machine);
                    m_stats[QStringLiteral("current.os.version")] = QString::fromLocal8Bit(buf.version);
                }
#endif

                // Nb calls
                QMap<QString, QPointer<QAction>> actions = SKGMainPanel::getMainPanel()->getGlobalActions();
                QStringList keys = actions.keys();
                for (const auto &k : std::as_const(keys)) {
                    QPointer<QAction> act = actions[k];
                    if (act != nullptr) {
                        QString id = "nb_call." % act->objectName();
                        if (!m_stats.contains(id)) {
                            m_stats[id] = 0;
                        }
                    }
                }
                m_stats[QStringLiteral("document.uuid")] = docUUID;

                // Set tables sizes
                QStringList tables;
                m_currentDocument->getTablesList(tables);
                for (const auto &t : std::as_const(tables)) {
                    QString r;
                    m_currentDocument->executeSingleSelectSqliteOrder("SELECT COUNT(1) FROM " % t, r);
                    m_stats["count." % t] = SKGServices::stringToInt(r);
                }
            }
        }
    }
}

void SKGStatisticPlugin::readStats()
{
    m_stats.clear();

    // Read file
    QFile data(m_file);
    if (data.open(QFile::ReadOnly)) {
        // Parse json
        m_stats = QJsonDocument::fromJson(data.readAll()).toVariant().toMap();
        data.close();
    }
}

void SKGStatisticPlugin::writeStats()
{
    // Write it in file
    QFile data(m_file);
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        // serialize json
        QJsonDocument serializer = QJsonDocument::fromVariant(m_stats);
        QByteArray doc = serializer.toJson(QJsonDocument::Indented);

        data.write(doc);
        data.close();
    } else {
        SKGTRACE << "ERROR: Impossible to write " << m_file << Qt::endl;
    }
}

void SKGStatisticPlugin::triggerAction()
{
    SKGTRACEINFUNC(10)
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        QString id = "nb_call." % act->objectName();
        SKGTRACEL(10) << "SKGStatisticPlugin::triggerAction " << id << "++" << Qt::endl;
        m_stats[id] = m_stats[id].toInt() + 1;
    }
}

void SKGStatisticPlugin::pageChanged()
{
    SKGTabPage::SKGPageHistoryItem currentPage = SKGMainPanel::getMainPanel()->currentPageHistoryItem();
    if (!currentPage.plugin.isEmpty()) {
        QString id =
            "nb_activated_" % QString(currentPage.bookmarkID.isEmpty() ? QStringLiteral("page") : QStringLiteral("bookmark")) % "." % currentPage.plugin;
        m_stats[id] = m_stats[id].toInt() + 1;
    }
}

void SKGStatisticPlugin::pageOpened()
{
    SKGTabPage::SKGPageHistoryItem currentPage = SKGMainPanel::getMainPanel()->currentPageHistoryItem();
    if (!currentPage.plugin.isEmpty()) {
        QString id = "nb_opened_" % QString(currentPage.bookmarkID.isEmpty() ? QStringLiteral("page") : QStringLiteral("bookmark")) % "." % currentPage.plugin;
        m_stats[id] = m_stats[id].toInt() + 1;
    }
}

QString SKGStatisticPlugin::title() const
{
    return i18nc("The title", "Statistic");
}

QString SKGStatisticPlugin::icon() const
{
    return QStringLiteral("dialog-information");
}

QString SKGStatisticPlugin::toolTip() const
{
    return title();
}

int SKGStatisticPlugin::getOrder() const
{
    return 9999;
}

bool SKGStatisticPlugin::isInPagesChooser() const
{
    return false;
}

#include <skgstatisticplugin.moc>
