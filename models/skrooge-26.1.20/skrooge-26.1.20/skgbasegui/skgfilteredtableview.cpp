/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * filtered SKGTableView.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgfilteredtableview.h"

#include <qdom.h>
#include <qlineedit.h>

#include "skgmainpanel.h"
#include "skgobjectmodelbase.h"
#include "skgsortfilterproxymodel.h"

SKGFilteredTableView::SKGFilteredTableView(QWidget *iParent)
    : QWidget(iParent)
    , m_objectModel(nullptr)
    , m_refreshNeeded(true)
{
    ui.setupUi(this);
    ui.kTitle->hide();
    ui.kResetInternalFilter->hide();
    connect(ui.kResetInternalFilter, &QToolButton::clicked, this, &SKGFilteredTableView::resetFilter);
    ui.kResetInternalFilter->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-cancel")));
    ui.kConfigure->setIcon(SKGServices::fromTheme(QStringLiteral("configure")));

    ui.kConfigure->setPopupMode(QToolButton::InstantPopup);
    ui.kConfigure->setAutoRaise(true);
    ui.kConfigure->setMenu(ui.kView->getHeaderMenu());

    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, &SKGFilteredTableView::onTextFilterChanged, Qt::QueuedConnection);
    connect(ui.kFilterEdit, &QLineEdit::textChanged, this, [this]() {
        m_timer.start(300);
    });
    connect(ui.kShow, &SKGShow::stateChanged, this, &SKGFilteredTableView::onFilterChanged, Qt::QueuedConnection);
    if (SKGMainPanel::getMainPanel() != nullptr) {
        connect(SKGMainPanel::getMainPanel(), &SKGMainPanel::currentPageChanged, this, &SKGFilteredTableView::pageChanged, Qt::QueuedConnection);
    }
}

SKGFilteredTableView::~SKGFilteredTableView()
{
    m_objectModel = nullptr;
}

QString SKGFilteredTableView::getState()
{
    QDomDocument doc(QStringLiteral("SKGML"));
    QDomElement root = doc.createElement(QStringLiteral("parameters"));
    doc.appendChild(root);

    root.setAttribute(QStringLiteral("show"), ui.kShow->getState());
    root.setAttribute(QStringLiteral("filter"), getSearchField()->text());

    // Memorize table settings
    root.setAttribute(QStringLiteral("view"), ui.kView->getState());
    return doc.toString();
}

void SKGFilteredTableView::setState(const QString &iState)
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iState);
    QDomElement root = doc.documentElement();

    QString show2 = root.attribute(QStringLiteral("show"));
    QString filter = root.attribute(QStringLiteral("filter"));

    if (!show2.isEmpty()) {
        ui.kShow->setState(show2);
    }
    getSearchField()->setText(filter);

    if (m_objectModel != nullptr) {
        bool previous = m_objectModel->blockRefresh(true);
        onFilterChanged();
        m_objectModel->blockRefresh(previous);
    }

    // !!! Must be done here after onFilterChanged
    ui.kView->setState(root.attribute(QStringLiteral("view")));
}

SKGShow *SKGFilteredTableView::getShowWidget() const
{
    return ui.kShow;
}

SKGTreeView *SKGFilteredTableView::getView() const
{
    return ui.kView;
}

QLineEdit *SKGFilteredTableView::getSearchField() const
{
    return ui.kFilterEdit;
}

void SKGFilteredTableView::onFilterChanged()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    // Update model
    if ((m_objectModel != nullptr) && ui.kShow->isEnabled() && m_objectModel->setFilter(ui.kShow->getWhereClause())) {
        m_objectModel->dataModified();
    }

    QApplication::restoreOverrideCursor();
}

void SKGFilteredTableView::pageChanged()
{
    if (m_refreshNeeded) {
        dataModified(QString(), 0);
    }
}

void SKGFilteredTableView::dataModified(const QString &iTableName, int iIdTransaction)
{
    Q_UNUSED(iIdTransaction)

    if (((m_objectModel != nullptr) && iTableName == m_objectModel->getTable()) || iTableName.isEmpty()) {
        SKGTabPage *page = SKGTabPage::parentTabPage(this);
        if (page != nullptr && SKGMainPanel::getMainPanel() != nullptr && page != SKGMainPanel::getMainPanel()->currentPage()) {
            m_refreshNeeded = true;
            return;
        }
        m_refreshNeeded = false;

        if (getView()->isAutoResized()) {
            getView()->resizeColumnsToContentsDelayed();
        }

        getView()->onSelectionChanged();
    }
}

void SKGFilteredTableView::setModel(SKGObjectModelBase *iModel)
{
    m_objectModel = iModel;
    if (m_objectModel != nullptr) {
        auto modelProxy = new SKGSortFilterProxyModel(this);
        modelProxy->setSourceModel(m_objectModel);
        modelProxy->setSortRole(Qt::UserRole);
        modelProxy->setDynamicSortFilter(true);

        connect(modelProxy, &SKGSortFilterProxyModel::rowsInserted, ui.kView, &SKGTreeView::scroolOnSelection);
        ui.kView->setModel(modelProxy);

        onTextFilterChanged();

        ui.kView->sortByColumn(0, Qt::AscendingOrder);
        connect(m_objectModel, &SKGObjectModelBase::beforeReset, ui.kView, &SKGTreeView::saveSelection);
        connect(m_objectModel, &SKGObjectModelBase::afterReset, ui.kView, &SKGTreeView::resetSelection);
        connect(m_objectModel->getDocument(), &SKGDocument::tableModified, this, &SKGFilteredTableView::dataModified, Qt::QueuedConnection);
    }
    dataModified(QString(), 0);
}

void SKGFilteredTableView::resetFilter()
{
    getShowWidget()->setEnabled(true);
    ui.kTitle->hide();
    ui.kResetInternalFilter->hide();

    m_objectModel->setFilter(QString());
    m_objectModel->refresh();
}

void SKGFilteredTableView::setFilter(const QIcon &iIcon, const QString &iText, const QString &iWhereClause)
{
    if ((m_objectModel != nullptr) && !iWhereClause.isEmpty()) {
        getShowWidget()->setEnabled(false);

        QFontMetrics fm(fontMetrics());
        ui.kTitle->setComment("<html><body><b>" % SKGServices::stringToHtml(fm.elidedText(iText, Qt::ElideMiddle, 2000)) % "</b></body></html>");
        ui.kTitle->setToolTip(iText);
        ui.kResetInternalFilter->show();

        ui.kTitle->setIcon(iIcon, KTitleWidget::ImageLeft);

        m_objectModel->setFilter(iWhereClause);
        m_objectModel->refresh();
    }
}

void SKGFilteredTableView::onTextFilterChanged()
{
    auto filter = ui.kFilterEdit->text();
    auto modelProxy = qobject_cast<SKGSortFilterProxyModel *>(ui.kView->model());
    if (modelProxy != nullptr) {
        modelProxy->setFilterKeyColumn(-1);
        modelProxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
        modelProxy->setProperty("filter", filter);
        modelProxy->setFilterFixedString(filter);

        QStringList attributes;
        QAbstractItemModel *model = modelProxy->sourceModel();
        if (model != nullptr) {
            int nbcol = model->columnCount();
            attributes.reserve(nbcol);
            for (int j = 0; j < nbcol; ++j) {
                attributes.append(model->headerData(j, Qt::Horizontal).toString());
            }
        }
        const auto &filter_understanding =
            SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(filter), attributes, m_objectModel->getDocument(), true);
        auto tooltip =
            i18nc("Tooltip",
                  "<html><head/><body><p>Searching is case-insensitive. So table, Table, and TABLE are all the same.<br/>"
                  "If you just put a word or series of words in the search box, the application will filter the table to keep all lines having these words "
                  "(logical operator AND). <br/>"
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
                  SKGServices::stringToHtml(filter_understanding));
        ui.kFilterEdit->setToolTip(tooltip);
    }
}
