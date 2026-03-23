/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a generic Skrooge plugin for html reports.set
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skghtmlboardwidget.h"

#include <qdom.h>
#include <qfileinfo.h>
#include <qqmlcontext.h>
#include <qquickitem.h>
#include <qquickwidget.h>
#include <qregularexpression.h>
#include <qtoolbutton.h>
#include <qwidgetaction.h>

#include <kcolorscheme.h>
#include <klineedit.h>
#include <klocalizedstring.h>

#include "skgdocument.h"
#include "skgmainpanel.h"
#include "skgreport.h"
#include "skgtraces.h"
#include <utility>

SKGHtmlBoardWidget::SKGHtmlBoardWidget(QWidget *iParent,
                                       SKGDocument *iDocument,
                                       const QString &iTitle,
                                       const QString &iTemplate,
                                       QStringList iTablesRefreshing,
                                       SKGSimplePeriodEdit::Modes iOptions,
                                       const QStringList &iAttributesForFilter)
    : SKGBoardWidget(iParent, iDocument, iTitle)
    , m_Quick(nullptr)
    , m_Report(iDocument->getReport())
    , m_Text(nullptr)
    , m_Template(iTemplate)
    , m_TablesRefreshing(std::move(iTablesRefreshing))
    , m_refreshNeeded(false)
    , m_period(nullptr)
    , m_filter(nullptr)
    , m_attributes(iAttributesForFilter)
{
    SKGTRACEINFUNC(10)
    // Create menu
    if (iOptions != SKGSimplePeriodEdit::NONE) {
        setContextMenuPolicy(Qt::ActionsContextMenu);

        m_period = new SKGSimplePeriodEdit(this);
        m_period->setMode(iOptions);

        QDate date = QDate::currentDate();
        QStringList list;
        // TODO(Stephane MANKOWSKI): v_operation_display must be generic
        getDocument()->getDistinctValues(QStringLiteral("v_operation_display"),
                                         QStringLiteral("MIN(d_DATEMONTH)"),
                                         QStringLiteral("d_date<=CURRENT_DATE"),
                                         list);
        if (!list.isEmpty()) {
            if (!list[0].isEmpty()) {
                date = SKGServices::periodToDate(list[0]);
            }
        }
        m_period->setFirstDate(date);

        auto periodEditWidget = new QWidgetAction(this);
        periodEditWidget->setDefaultWidget(m_period);

        addAction(periodEditWidget);
    }
    if (m_attributes.count()) {
        setContextMenuPolicy(Qt::ActionsContextMenu);

        m_filter = new KLineEdit(this);
        m_filter->setPlaceholderText(i18nc("A place holder for a filter field", "Filter"));

        auto filterEditWidget = new QWidgetAction(this);
        filterEditWidget->setDefaultWidget(m_filter);

        addAction(filterEditWidget);
    }

    // Enrich with tips of the day
    m_Report->setTipsOfDay(SKGMainPanel::getMainPanel()->getTipsOfDay());

    // Create main widget
    QString ext = QFileInfo(iTemplate).suffix().toLower();
    if (ext == QStringLiteral("qml")) {
        // Mode QML
        m_Quick = new QQuickWidget(this);
        m_Quick->setResizeMode(QQuickWidget::SizeViewToRootObject);
        m_Quick->setClearColor(Qt::transparent);
        m_Quick->setAttribute(Qt::WA_AlwaysStackOnTop);
        m_Quick->setAttribute(Qt::WA_TranslucentBackground);
        m_Quick->setObjectName(QStringLiteral("m_Quick"));
        QSizePolicy newSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        newSizePolicy.setHorizontalStretch(0);
        newSizePolicy.setVerticalStretch(0);
        m_Quick->setSizePolicy(newSizePolicy);

        // Set properties
        QVariantHash mapping = m_Report->getContextProperty();
        auto keys = mapping.keys();
        for (const auto &k : std::as_const(keys)) {
            m_Quick->rootContext()->setContextProperty(k, mapping[k]);
        }
        m_Quick->rootContext()->setContextProperty(QStringLiteral("periodWidget"), m_period);
        m_Quick->rootContext()->setContextProperty(QStringLiteral("filterWidget"), m_filter);
        m_Quick->rootContext()->setContextProperty(QStringLiteral("panel"), SKGMainPanel::getMainPanel());

        setMainWidget(m_Quick);
    } else {
        // Mode template HTML
        m_Text = new QLabel(this);
        m_Text->setObjectName(QStringLiteral("m_Text"));
        m_Text->setTextFormat(Qt::RichText);
        m_Text->setAlignment(Qt::AlignLeading | Qt::AlignLeft | Qt::AlignTop);
        m_Text->setTextInteractionFlags(Qt::TextBrowserInteraction);
        connect(m_Text, &QLabel::linkActivated, this, [](const QString &val) {
            SKGMainPanel::getMainPanel()->openPage(val);
        });

        setMainWidget(m_Text);
    }

    // Connects
    connect(getDocument(), &SKGDocument::tableModified, this, &SKGHtmlBoardWidget::dataModified, Qt::QueuedConnection);
    if (m_period != nullptr) {
        connect(m_period, static_cast<void (SKGComboBox::*)(const QString &)>(&SKGComboBox::currentTextChanged), this, [this]() {
            this->dataModified();
        });
    }
    if (m_filter != nullptr) {
        connect(m_filter, &KLineEdit::textChanged, this, &SKGHtmlBoardWidget::onTextFilterChanged);
    }
    connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGHtmlBoardWidget::pageChanged, Qt::QueuedConnection);
}

SKGHtmlBoardWidget::~SKGHtmlBoardWidget()
{
    SKGTRACEINFUNC(10)
    m_period = nullptr;
    m_filter = nullptr;
    if (m_Report != nullptr) {
        delete m_Report;
        m_Report = nullptr;
    }
    if (m_Quick != nullptr) {
        m_Quick->setParent(nullptr); // To be sure that deletion is well managed
        auto tmp = m_Quick->rootObject();
        if (tmp) {
            tmp->deleteLater();
        }
    }
}

QString SKGHtmlBoardWidget::getState()
{
    SKGTRACEINFUNC(10)
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(SKGBoardWidget::getState());
    QDomElement root = doc.documentElement();

    // Get state
    if (m_period != nullptr) {
        root.setAttribute(QStringLiteral("period"), m_period->text());
    }
    if (m_filter != nullptr) {
        root.setAttribute(QStringLiteral("filter"), m_filter->text());
    }
    return doc.toString();
}

void SKGHtmlBoardWidget::setState(const QString &iState)
{
    SKGTRACEINFUNC(10)
    SKGBoardWidget::setState(iState);

    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    // Set state
    if (m_period != nullptr) {
        QString oldMode = root.attribute(QStringLiteral("previousMonth"));
        if (oldMode.isEmpty()) {
            QString period = root.attribute(QStringLiteral("period"));
            if (!period.isEmpty() && m_period->contains(period)) {
                m_period->setText(period);
            }
        } else {
            m_period->setText(oldMode == QStringLiteral("N") ? i18nc("The current month", "Current month")
                                                             : i18nc("The month before the current month", "Last month"));
        }
    }
    if (m_filter != nullptr) {
        QString filter = root.attribute(QStringLiteral("filter"));
        m_filter->setText(filter);
    }
    dataModified(QString(), 0);
}

void SKGHtmlBoardWidget::setPointSize(int iPointSize) const
{
    if (m_Report != nullptr) {
        m_Report->setPointSize(iPointSize);
    }
}

void SKGHtmlBoardWidget::pageChanged()
{
    if (m_refreshNeeded) {
        dataModified();
    }
}

void SKGHtmlBoardWidget::onTextFilterChanged(const QString &iFilter)
{
    auto tooltip = i18nc("Tooltip",
                         "<html><head/><body><p>Searching is case-insensitive. So table, Table, and TABLE are all the same.<br/>"
                         "If you just put a word or series of words in the search box, the application will filter the table to keep all lines having these "
                         "words (logical operator AND). <br/>"
                         "If you want to add (logical operator OR) some lines, you must prefix your word by '+'.<br/>"
                         "If you want to remove (logical operator NOT) some lines, you must prefix your word by '-'.<br/>"
                         "If you want to search only on some columns, you must prefix your word by the beginning of column name like: col1:word.<br/>"
                         "If you want to search only on one column, you must prefix your word by the column name and a dot like: col1.:word.<br/>"
                         "If you want to use the character ':' in value, you must specify the column name like this: col1:value:rest.<br/>"
                         "If you want to search for a phrase or something that contains spaces, you must put it in quotes, like: 'yes, this is a phrase'.</p>"
                         "<p>You can also use operators '&lt;', '&gt;', '&lt;=', '&gt;=', '=' and '#' (for regular expression).</p>"
                         "<p><span style=\"font-weight:600; text-decoration: underline;\">Examples:</span><br/>"
                         "+val1 +val2 =&gt; Keep lines containing val1 OR val2<br/>"
                         "+val1 -val2 =&gt; Keep lines containing val1 but NOT val2<br/>"
                         "'abc def' =&gt; Keep lines containing the sentence 'abc def' <br/>"
                         "'-att:abc def' =&gt; Remove lines having a column name starting by abc and containing 'abc def' <br/>"
                         "abc:def =&gt; Keep lines having a column name starting by abc and containing def<br/>"
                         ":abc:def =&gt; Keep lines containing 'abc:def'<br/>"
                         "Date&gt;2015-03-01 =&gt; Keep lines where at least one attribute starting by Date is greater than 2015-03-01<br/>"
                         "Date.&gt;2015-03-01 =&gt; Keep lines where at the Date attribute is greater than 2015-03-01<br/>"
                         "Amount&lt;10 =&gt;Keep lines where at least one attribute starting by Amount is less than 10<br/>"
                         "Amount=10 =&gt;Keep lines where at least one attribute starting by Amount is equal to 10<br/>"
                         "Amount&lt;=10 =&gt;Keep lines where at least one attribute starting by Amount is less or equal to 10<br/>"
                         "abc#^d.*f$ =&gt; Keep lines having a column name starting by abc and matching the regular expression ^d.*f$</p>"
                         "<span style=\"font-weight:600; text-decoration: underline;\">Your filter is understood like this:</span><br/>"
                         "%1</body></html>",
                         SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(iFilter), m_attributes, getDocument(), true));
    m_filter->setToolTip(tooltip);

    SKGHtmlBoardWidget::dataModified();
}

void SKGHtmlBoardWidget::dataModified(const QString &iTableName, int iIdTransaction)
{
    SKGTRACEINFUNC(10)
    Q_UNUSED(iIdTransaction)

    // Set title
    QString period = (m_period != nullptr ? m_period->period() : QString());
    QString title = getOriginalTitle();
    if (title.contains(QStringLiteral("%1"))) {
        setMainTitle(title.arg(period));
    }
    QString filterSQL;
    QString filterTXT;
    if (m_Report != nullptr) {
        m_Report->setPeriod((m_period != nullptr ? m_period->period() : SKGServices::dateToPeriod(QDate::currentDate(), QStringLiteral("M"))));
        if (m_filter) {
            filterSQL = SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(m_filter->text()), m_attributes, getDocument(), false);
            filterTXT = SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(m_filter->text()), m_attributes, getDocument(), true);
            m_Report->setSqlFilter(filterSQL);
        }
    }

    if (m_TablesRefreshing.isEmpty() || m_TablesRefreshing.contains(iTableName) || iTableName.isEmpty()) {
        // Is this the current page
        SKGTabPage *page = SKGTabPage::parentTabPage(this);
        if (page != nullptr && page != SKGMainPanel::getMainPanel()->currentPage()) {
            // No, we memorize that we have to compute later the report
            m_refreshNeeded = true;
            return;
        }

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        if (m_Quick != nullptr) {
            m_Quick->rootContext()->setContextProperty(QStringLiteral("filterWidget_sql"), filterSQL);
            m_Quick->rootContext()->setContextProperty(QStringLiteral("filterWidget_txt"), filterTXT);

            // Set the source
            if (!m_Quick->source().isValid()) {
                m_Quick->setSource(QUrl::fromLocalFile(m_Template));

                QQuickItem *root = m_Quick->rootObject();
                if (root != nullptr) {
                    connect(root, &QQuickItem::widthChanged, this, [this, root]() {
                        m_Quick->setMinimumSize(QSize(root->width(), root->height()));
                    });
                    connect(root, &QQuickItem::heightChanged, this, [this, root]() {
                        m_Quick->setMinimumSize(QSize(root->width(), root->height()));
                    });
                    m_Quick->setMinimumSize(QSize(root->width(), root->height()));
                    m_Quick->setMinimumSize(QSize(root->width(), root->height()));
                }
            } else {
                // Clean the cache to trigger the modification
                m_Report->cleanCache();
            }

            m_refreshNeeded = false;
        }

        if (m_Text != nullptr) {
            // Clean the cache to trigger the modification
            m_Report->cleanCache();

            QString stream;
            SKGError err = SKGReport::getReportFromTemplate(m_Report, m_Template, stream);
            IFKO(err) stream = err.getFullMessage();
            static QRegularExpression reg(QStringLiteral("<img[^>]*/>"));
            stream = stream.remove(reg);

            // Remove color of hyperlinks
            KColorScheme scheme(QPalette::Normal, KColorScheme::Window);
            auto color = scheme.foreground(KColorScheme::NormalText).color().name().right(6);
            stream = stream.replace(QStringLiteral("<a href"), QStringLiteral("<a style=\"color: #") + color + ";\" href");

            m_Text->setText(stream);

            m_refreshNeeded = false;
        }
        QApplication::restoreOverrideCursor();
    }

    // TODO(Stephane MANKOWSKI): No widget if no account (must not be hardcoded)
    bool exist = false;
    getDocument()->existObjects(QStringLiteral("account"), QString(), exist);
    if (parentWidget() != nullptr) {
        setVisible(exist);
    }
}
#include "skghtmlboardwidget.h"
