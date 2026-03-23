/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a query creator for skrooge
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgquerycreator.h"

#include <qdom.h>
#include <qheaderview.h>
#include <qmenu.h>

#include "skgdocument.h"
#include "skgpredicatcreator.h"
#include "skgquerydelegate.h"
#include "skgruleobject.h"
#include "skgservices.h"
#include <utility>

#define ADDOPERATOR(title, op)                                                                                                                                 \
    {                                                                                                                                                          \
        QAction *act = helpMenu->addAction(title);                                                                                                             \
        act->setData(op);                                                                                                                                      \
        connect(act, &QAction::triggered, this, &SKGQueryCreator::onAddText);                                                                                  \
    }

SKGQueryCreator::SKGQueryCreator(QWidget *iParent, bool iDisableAdvancedMode)
    : QWidget(iParent)
    , m_document(nullptr)
    , m_updateMode(false)
{
    ui.setupUi(this);

    ui.kList->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    ui.kList->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui.kList->setWordWrap(false);
    ui.kList->horizontalHeader()->setSectionsMovable(true);

    ui.kToolHelp->setIcon(SKGServices::fromTheme(QStringLiteral("dialog-information")));
    ui.kCheckMode->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-down")));
    ui.kCheckMode->setToolTip(i18nc("A tool tip", "Switch in advanced mode"));
    if (iDisableAdvancedMode) {
        ui.kCheckMode->setVisible(false);
    }

    connect(ui.kFilterEdit, &QLineEdit::textChanged, this, &SKGQueryCreator::onTextFilterChanged);
    connect(ui.kCheckMode, &QToolButton::clicked, this, &SKGQueryCreator::switchAdvancedSearchMode);
    connect(ui.kList, &SKGTableWidget::removeLine, this, &SKGQueryCreator::removeLine);
    connect(ui.kFilterEdit, &QLineEdit::returnPressed, this, &SKGQueryCreator::search);
    connect(ui.kListAtt, &QListWidget::doubleClicked, this, &SKGQueryCreator::onAddColumn);

    addNewLine();
    onTextFilterChanged(QString());
}

SKGQueryCreator::~SKGQueryCreator()
{
    m_document = nullptr;
}

void SKGQueryCreator::onTextFilterChanged(const QString &iFilter)
{
    auto tooltip = i18nc(
        "Tooltip",
        "<html><head/><body><p>Searching is case-insensitive. So table, Table, and TABLE are all the same.<br/>"
        "If you just put a word or series of words in the search box, the application will filter the table to keep all lines having these words (logical "
        "operator AND). <br/>"
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
        SKGServices::stringToHtml(SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(iFilter), m_attributes, m_document, true)));
    ui.kFilterEdit->setToolTip(tooltip);
}

bool SKGQueryCreator::advancedSearchMode() const
{
    return (!ui.kFilterEdit->isVisible());
}

void SKGQueryCreator::setAdvancedSearchMode(bool iAdvancedMode) const
{
    if (iAdvancedMode) {
        ui.kToolHelp->hide();
        ui.kFrmAdvanced->show();
        ui.kFilterEdit->hide();
        ui.kCheckMode->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-up")));
        ui.kCheckMode->setToolTip(i18nc("A tool tip", "Switch in simple mode"));
    } else {
        ui.kToolHelp->show();
        ui.kFrmAdvanced->hide();
        ui.kFilterEdit->show();
        ui.kCheckMode->setIcon(SKGServices::fromTheme(QStringLiteral("arrow-down")));
        ui.kCheckMode->setToolTip(i18nc("A tool tip", "Switch in advanced mode"));
    }
}

void SKGQueryCreator::switchAdvancedSearchMode() const
{
    setAdvancedSearchMode(!advancedSearchMode());
}

void SKGQueryCreator::setParameters(SKGDocument *iDocument, const QString &iTable, const QStringList &iListAttribute, bool iModeUpdate)
{
    m_document = iDocument;
    m_table = iTable;
    m_updateMode = iModeUpdate;
    m_attributes = iListAttribute;

    setAdvancedSearchMode(m_updateMode);
    if (m_updateMode)
        ui.kCheckMode->setVisible(false);

    //     QString txt=(updateMode ? i18nc("Description", "Double click on a field name to add it to your modification definition.") : i18nc("Description",
    //     "Double click on a field name to add it to your search definition."))+'\n'+i18nc("Description", "Double click on a cell to modify it.");
    //     ui.kLabel->setComment ( "<html><body><b>"+SKGServices::stringToHtml ( txt ) +"</b></body></html>" );
    //     ui.kLabel->setIcon ( SKGServices::fromTheme( updateMode ? "view-refresh" :"system-search" ), KTitleWidget::ImageLeft );

    // Build list of attributes
    if (m_document != nullptr) {
        auto delegate = new SKGQueryDelegate(ui.kList, m_document, m_updateMode, iListAttribute);
        connect(delegate, &SKGQueryDelegate::commitData, this, &SKGQueryCreator::onCloseEditor, Qt::QueuedConnection);

        ui.kList->setItemDelegate(delegate);

        // Keep only existing attribute
        SKGServices::SKGAttributesList listAtts;
        int nb = iListAttribute.count();

        SKGServices::SKGAttributesList attributes;
        m_document->getAttributesDescription(m_table, attributes);
        listAtts.reserve(nb + attributes.count());
        for (const auto &att : std::as_const(attributes)) {
            if (iListAttribute.isEmpty() || iListAttribute.contains(att.name)) {
                listAtts.push_back(att);
            }
        }

        // Adding properties
        for (int i = 0; i < nb; ++i) {
            const QString &att = iListAttribute.at(i);
            if (att.startsWith(QStringLiteral("p_"))) {
                SKGServices::SKGAttributeInfo info;
                info.name = att;
                info.display = att.right(att.length() - 2);
                info.type = SKGServices::TEXT;
                info.icon = iDocument->getIcon(att);
                listAtts.push_back(info);
            }
        }

        ui.kList->setRowCount(0);

        // Build list of attributes
        QMenu *helpMenu = nullptr;
        if (!iModeUpdate) {
            helpMenu = new QMenu(this);
            {
                QAction *act = helpMenu->addAction(i18nc("Operator contains", "or"));
                act->setData(QStringLiteral(" +"));
                connect(act, &QAction::triggered, this, &SKGQueryCreator::onAddText);
            }
            {
                QAction *act = helpMenu->addAction(i18nc("Operator contains", "but not"));
                act->setData(QStringLiteral(" -"));
                connect(act, &QAction::triggered, this, &SKGQueryCreator::onAddText);
            }
            helpMenu->addSeparator();
        }
        int nbCol = listAtts.count();
        for (int i = 0; i < nbCol; ++i) {
            auto listItem = new QListWidgetItem(listAtts.at(i).icon, listAtts.at(i).display);
            ui.kListAtt->addItem(listItem);
            listItem->setData(Qt::UserRole, listAtts.at(i).name);

            if (helpMenu != nullptr) {
                QAction *act = helpMenu->addAction(listAtts.at(i).icon, listAtts.at(i).display);
                act->setData(listAtts.at(i).display);
                connect(act, &QAction::triggered, this, &SKGQueryCreator::onAddText);
            }
        }
        ui.kListAtt->sortItems();
        ui.kListAtt->setModelColumn(nbCol);
        if (helpMenu != nullptr) {
            helpMenu->addSeparator();
            {
                QAction *act = helpMenu->addAction(i18nc("Operator contains", "Contains"));
                act->setData(QStringLiteral(":"));
                connect(act, &QAction::triggered, this, &SKGQueryCreator::onAddText);
            }
            ADDOPERATOR(QStringLiteral("="), QStringLiteral("="))
            ADDOPERATOR(i18nc("Noun", "Regular expression"), QStringLiteral("#"))
            ADDOPERATOR(QStringLiteral(">"), QStringLiteral(">"))
            ADDOPERATOR(QStringLiteral("<"), QStringLiteral("<"))
            ADDOPERATOR(QStringLiteral(">="), QStringLiteral(">="))
            ADDOPERATOR(QStringLiteral("<="), QStringLiteral("<="))
            ui.kToolHelp->setMenu(helpMenu);
        }

        connect(ui.kList->verticalHeader(), &QHeaderView::sectionClicked, this, &SKGQueryCreator::removeLine);
        connect(ui.kList->horizontalHeader(), &QHeaderView::sectionClicked, this, &SKGQueryCreator::removeColumn);

        addNewLine();
    }
}

int SKGQueryCreator::getIndexQueryColumn(const QString &iAttribute, int row)
{
    // Search column for this attribute
    int output = -1;
    int nbCol = ui.kList->columnCount();
    for (int i = 0; i < nbCol && output == -1; ++i) {
        QTableWidgetItem *it_h = ui.kList->horizontalHeaderItem(i);
        if ((it_h != nullptr) && iAttribute == it_h->data(Qt::UserRole).toString()) {
            if (row >= 0) {
                // Check if the cell is empty
                QTableWidgetItem *it = ui.kList->item(row, i);
                if (it != nullptr) {
                    if (it->text().isEmpty()) {
                        output = i;
                    }
                }
            } else {
                output = i;
            }
        }
    }

    // If not existing, we have to create it
    if (output == -1) {
        int nb = ui.kListAtt->count();
        for (int i = 0; i < nb; ++i) {
            QListWidgetItem *it = ui.kListAtt->item(i);
            if ((it != nullptr) && iAttribute == it->data(Qt::UserRole).toString()) {
                addColumnFromAttribut(it);
                output = nbCol;
                break;
            }
        }
    }

    return output;
}

void SKGQueryCreator::clearContents()
{
    ui.kList->clearContents();
    ui.kList->setRowCount(0);

    addNewLine();
}

void SKGQueryCreator::setXMLCondition(const QString &iXML)
{
    QDomDocument doc(QStringLiteral("SKGML"));
    doc.setContent(iXML);
    QDomElement root = doc.documentElement();

    ui.kList->clearContents();
    ui.kList->setRowCount(0);
    ui.kList->setColumnCount(0);

    if (root.tagName() == QStringLiteral("element")) {
        // Mode advanced
        setAdvancedSearchMode(true);
        int row = -1;
        QDomNode l = root.firstChild();
        while (!l.isNull()) {
            QDomElement elementl = l.toElement();
            if (!elementl.isNull()) {
                // Add new line
                addNewLine();
                ++row;

                QDomNode n = elementl.firstChild();
                while (!n.isNull()) {
                    QDomElement element = n.toElement();
                    if (!element.isNull()) {
                        QString attribute = element.attribute(QStringLiteral("attribute"));
                        int idx = getIndexQueryColumn(attribute, row);
                        if (idx >= 0) {
                            QDomDocument doc2(QStringLiteral("SKGML"));
                            QDomElement root2 = doc2.createElement(QStringLiteral("element"));
                            doc2.appendChild(root2);
                            root2.setAttribute(QStringLiteral("operator"), element.attribute(QStringLiteral("operator")));
                            root2.setAttribute(QStringLiteral("value"), element.attribute(QStringLiteral("value")));
                            root2.setAttribute(QStringLiteral("value2"), element.attribute(QStringLiteral("value2")));
                            root2.setAttribute(QStringLiteral("att2"), element.attribute(QStringLiteral("att2")));
                            root2.setAttribute(QStringLiteral("att2s"), element.attribute(QStringLiteral("att2s")));
                            QTableWidgetItem *it = ui.kList->item(row, idx);
                            if (it != nullptr) {
                                QString xml = doc2.toString();

                                it->setText(SKGPredicatCreator::getTextFromXml(xml));
                                it->setData(Qt::UserRole, xml);
                            }
                        }
                    }
                    n = n.nextSibling();
                }
            }
            l = l.nextSibling();
        }

        addNewLine();
    } else {
        // Mode simple
        setAdvancedSearchMode(false);
        ui.kFilterEdit->setText(root.attribute(QStringLiteral("query")));
    }
}

QString SKGQueryCreator::getXMLCondition()
{
    QString output;
    if (advancedSearchMode()) {
        // Mode advanced
        QHeaderView *hHeader = ui.kList->horizontalHeader();
        if (hHeader != nullptr) {
            QDomDocument doc(QStringLiteral("SKGML"));
            QDomElement element = doc.createElement(QStringLiteral("element"));
            doc.appendChild(element);

            element.appendChild(doc.createComment(QStringLiteral("OR")));

            bool empty = true;
            int nbRow = ui.kList->rowCount();
            int nbCol = hHeader->count();
            for (int j = 0; j < nbRow; ++j) {
                QDomElement elementLine = doc.createElement(QStringLiteral("element"));
                element.appendChild(elementLine);

                elementLine.appendChild(doc.createComment(QStringLiteral("AND")));

                bool atLeastOne = false;
                for (int i = 0; i < nbCol; ++i) {
                    int iRealPos = hHeader->logicalIndex(i);
                    QTableWidgetItem *it = ui.kList->item(j, iRealPos);
                    if (it != nullptr) {
                        QString co = it->data(Qt::UserRole).toString();
                        if (!co.isEmpty()) {
                            QDomElement elementElement = doc.createElement(QStringLiteral("element"));
                            elementLine.appendChild(elementElement);

                            QTableWidgetItem *it_h = ui.kList->horizontalHeaderItem(iRealPos);
                            QString attname = it_h->data(Qt::UserRole).toString();

                            QDomDocument doc2(QStringLiteral("SKGML"));
                            doc2.setContent(co);
                            QDomElement root2 = doc2.documentElement();

                            elementElement.setAttribute(QStringLiteral("attribute"), attname);
                            elementElement.setAttribute(QStringLiteral("operator"), root2.attribute(QStringLiteral("operator")));
                            if (root2.hasAttribute(QStringLiteral("value"))) {
                                elementElement.setAttribute(QStringLiteral("value"), root2.attribute(QStringLiteral("value")));
                            }
                            if (root2.hasAttribute(QStringLiteral("value2"))) {
                                elementElement.setAttribute(QStringLiteral("value2"), root2.attribute(QStringLiteral("value2")));
                            }
                            if (root2.hasAttribute(QStringLiteral("att2"))) {
                                elementElement.setAttribute(QStringLiteral("att2"), root2.attribute(QStringLiteral("att2")));
                            }
                            if (root2.hasAttribute(QStringLiteral("att2s"))) {
                                elementElement.setAttribute(QStringLiteral("att2s"), root2.attribute(QStringLiteral("att2s")));
                            }

                            atLeastOne = true;
                            empty = false;
                        }
                    }
                }

                if (!atLeastOne) {
                    element.removeChild(elementLine);
                }
            }
            if (!empty) {
                output = doc.toString();
            }
        }
    } else {
        // Mode simple
        QDomDocument doc(QStringLiteral("SKGML"));
        QDomElement element = doc.createElement(QStringLiteral("simple"));
        element.setAttribute(QStringLiteral("query"), ui.kFilterEdit->text());
        element.setAttribute(QStringLiteral("sql"),
                             SKGServices::searchCriteriasToWhereClause(SKGServices::stringToSearchCriterias(ui.kFilterEdit->text()), m_attributes, m_document));
        doc.appendChild(element);
        output = doc.toString();
    }

    return output;
}

void SKGQueryCreator::onCloseEditor()
{
    // If all lignes have at least one value then add a new line
    bool lineEmpty = false;
    int nbCol = ui.kList->columnCount();
    int nbRow = ui.kList->rowCount();
    for (int j = 0; !lineEmpty && j < nbRow; ++j) {
        lineEmpty = true;
        for (int i = 0; lineEmpty && i < nbCol; ++i) {
            QTableWidgetItem *it = ui.kList->item(j, i);
            if ((it != nullptr) && !it->text().isEmpty()) {
                lineEmpty = false;
            }
        }
    }

    if (!lineEmpty) {
        addNewLine();
    }

    resizeColumns();
}

void SKGQueryCreator::onAddText()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (act != nullptr) {
        ui.kFilterEdit->insert(act->data().toString());
    }
}

void SKGQueryCreator::onAddColumn()
{
    QList<QListWidgetItem *> selection = ui.kListAtt->selectedItems();
    if (selection.count() == 1) {
        QListWidgetItem *listItem = selection.at(0);
        addColumnFromAttribut(listItem);
    }
}

void SKGQueryCreator::addColumnFromAttribut(const QListWidgetItem *iListItem)
{
    if (iListItem != nullptr) {
        bool previous = ui.kList->blockSignals(true);

        int nb = ui.kList->columnCount();
        ui.kList->setColumnCount(nb + 1);

        // Create header
        auto item = new QTableWidgetItem(iListItem->icon(), iListItem->text());
        item->setData(Qt::UserRole, iListItem->data(Qt::UserRole));
        ui.kList->setHorizontalHeaderItem(nb, item);

        // Create items
        int nbRows = ui.kList->rowCount();
        for (int i = 0; i < nbRows; ++i) {
            ui.kList->setItem(i, nb, new QTableWidgetItem());
        }
        ui.kList->blockSignals(previous);

        resizeColumns();
    }
}

void SKGQueryCreator::addNewLine()
{
    // add line is only for
    if (!m_updateMode || ui.kList->rowCount() < 1) {
        bool previous = ui.kList->blockSignals(true);

        int nbCol = ui.kList->columnCount();
        int row = ui.kList->rowCount();
        ui.kList->insertRow(row);

        // Add a delete icon
        if (!m_updateMode) {
            ui.kList->setVerticalHeaderItem(row, new QTableWidgetItem(SKGServices::fromTheme(QStringLiteral("edit-delete")), QString()));
        }

        for (int i = 0; i < nbCol; ++i) {
            auto item = new QTableWidgetItem();
            ui.kList->setItem(row, i, item);
        }
        ui.kList->blockSignals(previous);

        resizeColumns();
    }
}

int SKGQueryCreator::getColumnsCount()
{
    return ui.kList->horizontalHeader()->count();
}

int SKGQueryCreator::getLinesCount()
{
    return ui.kList->rowCount();
}

void SKGQueryCreator::removeColumn(int iColumn)
{
    ui.kList->removeColumn(iColumn);

    // To be sure that we have at least an empty line
    onCloseEditor();
}

void SKGQueryCreator::removeLine(int iRow)
{
    ui.kList->removeRow(iRow);

    // To be sure that we have at least an empty line
    onCloseEditor();
}

void SKGQueryCreator::resizeColumns()
{
    ui.kList->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    qApp->processEvents(QEventLoop::AllEvents, 300);
    ui.kList->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
}
