/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * A report class for document
 *
 * @author Stephane MANKOWSKI
 */
#include "skgreport.h"

#include "ktexttemplate/metatype.h"
#include <KTextTemplate/Context>
#include <KTextTemplate/Engine>
#include <KTextTemplate/QtLocalizer>
#include <KTextTemplate/Template>

#include <kaboutdata.h>
#include <kcolorscheme.h>

#include "qregularexpression.h"
#include <QRandomGenerator>
#include <qdir.h>
#include <qfile.h>
#include <qfont.h>
#include <qfontdatabase.h>
#include <qstandardpaths.h>
#include <qurl.h>

#include "skgdocument.h"
#include "skgobjectbase.h"
#include "skgtraces.h"

KTEXTTEMPLATE_BEGIN_LOOKUP(SKGObjectBase)
Q_UNUSED(object)
Q_UNUSED(property)
KTEXTTEMPLATE_END_LOOKUP

SKGReport::SKGReport(SKGDocument *iDocument)
    : m_document(iDocument)
    , m_previous(nullptr)
    , m_pointSize(10)
{
    SKGTRACEINFUNC(1)

    // KTextTemplate initialization
    KTextTemplate::registerMetaType<SKGObjectBase>();
}

SKGReport::~SKGReport()
{
    SKGTRACEINFUNC(1)
    if (m_previous != nullptr) {
        delete m_previous;
        m_previous = nullptr;
    }
}

SKGDocument *SKGReport::getDocument() const
{
    return m_document;
}

void SKGReport::setPeriod(const QString &iPeriod)
{
    if (iPeriod != m_cache[QStringLiteral("period")]) {
        cleanCache(false);
        if (m_previous != nullptr) {
            delete m_previous;
            m_previous = nullptr;
        }
        m_cache[QStringLiteral("period")] = iPeriod;
        Q_EMIT changed();
    }
}

QString SKGReport::getPeriod()
{
    QString month = m_cache.value(QStringLiteral("period")).toString();
    if (month.isEmpty()) {
        month = QDate::currentDate().toString(QStringLiteral("yyyy-MM"));
        m_cache[QStringLiteral("period")] = month;
    }
    return month;
}

void SKGReport::setSqlFilter(const QString &iFilter)
{
    if (iFilter != m_cache[QStringLiteral("filter")]) {
        cleanCache(false);
        m_cache[QStringLiteral("filter")] = iFilter;
        Q_EMIT changed();
    }
}

QString SKGReport::getSqlFilter()
{
    return m_cache.value(QStringLiteral("filter")).toString();
    ;
}

QString SKGReport::getPreviousPeriod()
{
    QString previousmonth = m_cache.value(QStringLiteral("previousperiod")).toString();
    if (previousmonth.isEmpty()) {
        QString period = getPeriod();
        if (!period.isEmpty()) {
            previousmonth = SKGServices::getNeighboringPeriod(period);
        }
        m_cache[QStringLiteral("previousperiod")] = previousmonth;
    }
    return previousmonth;
}

SKGReport *SKGReport::getPrevious()
{
    if (m_previous == nullptr) {
        m_previous = m_document->getReport();
        m_previous->setPeriod(getPreviousPeriod());
    }
    return m_previous;
}

void SKGReport::setTipsOfDay(const QStringList &iTipsOfDays)
{
    m_tipsOfTheDay = iTipsOfDays;

    Q_EMIT changed();
}

QString SKGReport::getTipOfDay() const
{
    auto tips = getTipsOfDay();
    auto tip = tips.count() > 0 ? SKGServices::htmlToString(tips.at(QRandomGenerator::global()->bounded(tips.size()))) : QString();
    return tip;
}

QStringList SKGReport::getTipsOfDay() const
{
    return m_tipsOfTheDay;
}

QVariantHash SKGReport::getContextProperty()
{
    QVariantHash mapping;
    addItemsInMapping(mapping);

    if (m_document != nullptr) {
        mapping.insert(QStringLiteral("document"), QVariant::fromValue<QObject *>(m_document));
    }
    return mapping;
}

void SKGReport::addItemsInMapping(QVariantHash &iMapping)
{
    iMapping.insert(QStringLiteral("report"), QVariant::fromValue<QObject *>(this));
    iMapping.insert(QStringLiteral("current_date"), QDate::currentDate());
    KColorScheme scheme(QPalette::Normal, KColorScheme::Window);
    iMapping.insert(QStringLiteral("color_negativetext"), scheme.foreground(KColorScheme::NegativeText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_positivetext"), scheme.foreground(KColorScheme::PositiveText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_neutraltext"), scheme.foreground(KColorScheme::NeutralText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_normaltext"), scheme.foreground(KColorScheme::NormalText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_inactivetext"), scheme.foreground(KColorScheme::InactiveText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_activetext"), scheme.foreground(KColorScheme::ActiveText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_linktext"), scheme.foreground(KColorScheme::LinkText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_visitedtext"), scheme.foreground(KColorScheme::VisitedText).color().name().right(6));
    iMapping.insert(QStringLiteral("color_normalbackground"), scheme.background(KColorScheme::NormalBackground).color().name().right(6));
    iMapping.insert(QStringLiteral("color_activebackground"), scheme.background(KColorScheme::ActiveBackground).color().name().right(6));

    QFont generalFont = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    iMapping.insert(QStringLiteral("font_family"), generalFont.family());

    QString dir = "file://"
        % QFileInfo(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf5/infopage/kde_infopage.css"))).dir().absolutePath() % u'/';
    {
        QFile file(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kf5/infopage/kde_infopage.css")));
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            iMapping.insert(QStringLiteral("kde_infopage_css"), QString(file.readAll()).replace(QStringLiteral("url("), "url(" % dir));
        }
    }

    KAboutData about = KAboutData::applicationData();
    iMapping.insert(QStringLiteral("about_welcome"), i18nc("Welcome message", "Welcome to %1", about.displayName()));
    iMapping.insert(QStringLiteral("about_programname"), about.displayName());
    iMapping.insert(QStringLiteral("about_version"), about.version());
    iMapping.insert(QStringLiteral("about_bugaddress"), about.bugAddress());
    iMapping.insert(QStringLiteral("about_copyrightstatement"), about.copyrightStatement());
    iMapping.insert(QStringLiteral("about_homepage"), about.homepage());
    iMapping.insert(QStringLiteral("about_shortdescription"), about.shortDescription());
    iMapping.insert(QStringLiteral("about_othertext"), about.otherText());
    iMapping.insert(QStringLiteral("about_did_you_know"), i18nc("Title for tips of the day", "Did you know …?"));
}

SKGError SKGReport::getReportFromTemplate(SKGReport *iReport, const QString &iFile, QString &oHtml)
{
    SKGError err;
    SKGTRACEINFUNCRC(10, err)

    // Prepare KTextTemplate engine
    KTextTemplate::Engine gEngine;
    gEngine.addDefaultLibrary(QStringLiteral("grantlee_skgfilters"));

    QSharedPointer<KTextTemplate::FileSystemTemplateLoader> gLoader =
        QSharedPointer<KTextTemplate::FileSystemTemplateLoader>(new KTextTemplate::FileSystemTemplateLoader());
    gLoader->setTemplateDirs(QStringList(QFileInfo(iFile).dir().absolutePath()));
    gEngine.addTemplateLoader(gLoader);

    KTextTemplate::Template gTemplate = gEngine.loadByName(QFileInfo(iFile).fileName());
    if (gTemplate->error() != 0u) {
        err = SKGError(gTemplate->error(), gTemplate->errorString());
    } else {
        QVariantHash mapping;
        if (iReport != nullptr) {
            mapping = iReport->getContextProperty();
        }
        KTextTemplate::Context gContext(mapping);

        // Generation
        {
            SKGTRACEINFUNCRC(10, err)
            oHtml = gTemplate->render(&gContext);
            static QRegularExpression rx(QStringLiteral("\\n\\s*\\n"));
            oHtml = oHtml.replace(rx, QStringLiteral("\n"));
            if (gTemplate->error() != 0u) {
                err = SKGError(gTemplate->error(), gTemplate->errorString());
            }
        }
    }
    return err;
}

void SKGReport::cleanCache(bool iEmitSignal)
{
    QString month = m_cache.value(QStringLiteral("period")).toString();
    QString filter = m_cache.value(QStringLiteral("filter")).toString();
    m_cache.clear();
    if (!month.isEmpty()) {
        m_cache[QStringLiteral("period")] = month;
    }
    if (!filter.isEmpty()) {
        m_cache[QStringLiteral("filter")] = filter;
    }
    if (iEmitSignal) {
        Q_EMIT changed();
    }
}

void SKGReport::addParameter(const QString &iName, const QVariant &ivalue)
{
    m_parameters[iName] = ivalue;
}

void SKGReport::setPointSize(int iPointSize)
{
    m_pointSize = iPointSize;
    Q_EMIT changed();
}

int SKGReport::getPointSize() const
{
    return m_pointSize;
}
