/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGQUERYCREATOR_H
#define SKGQUERYCREATOR_H
/** @file
 * A query creator for skrooge.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qwidget.h>

#include "skgbankgui_export.h"
#include "ui_skgquerycreator.h"

class SKGDocument;

/**
 * This file is a query creator for skrooge
 */
class SKGBANKGUI_EXPORT SKGQueryCreator : public QWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent
     * @param iDisableAdvancedMode to disable the advanced mode
     */
    explicit SKGQueryCreator(QWidget *iParent, bool iDisableAdvancedMode = false);

    /**
     * Default Destructor
     */
    ~SKGQueryCreator() override;

    /**
     * Set parameters
     * @param iDocument the document
     * @param iTable the table
     * @param iListAttribute the list of attribute (empty means all attributes supported of the table)
     * @param iModeUpdate to enable update mode
     */
    void setParameters(SKGDocument *iDocument, const QString &iTable, const QStringList &iListAttribute = QStringList(), bool iModeUpdate = false);

    /**
     * Set XML representing the query
     * @param iXML the XML representing the query
     */
    void setXMLCondition(const QString &iXML);

    /**
     * Get XML representing the query
     * @return the XML representing the query
     */
    QString getXMLCondition();

    /**
     * Get the number of line
     * @return the number of line
     */
    int getLinesCount();

    /**
     * Get the number of line
     * @return the number of line
     */
    int getColumnsCount();

    /**
     * Get the advanced search mode
     */
    bool advancedSearchMode() const;

public Q_SLOTS:
    /**
     * Clear table content
     */
    void clearContents();

    /**
     * Remove a line
     * @param iRow the index of the line (-1 for all)
     */
    void removeLine(int iRow = -1);

    /**
     * Remove a column
     * @param iColumn the index of the column
     */
    void removeColumn(int iColumn);

    /**
     * Add a new line
     */
    void addNewLine();

    /**
     * Set/unset the advanced search mode
     * @param iAdvancedMode the mode
     */
    void setAdvancedSearchMode(bool iAdvancedMode) const;

    /**
     * Switch the advanced search mode
     */
    void switchAdvancedSearchMode() const;

Q_SIGNALS:
    /**
     * Emitted when the search is triggered
     */
    void search();

private Q_SLOTS:
    void onCloseEditor();
    void onAddColumn();
    void onAddText();
    void onTextFilterChanged(const QString &iFilter);

private:
    Q_DISABLE_COPY(SKGQueryCreator)

    int getIndexQueryColumn(const QString &iAttribute, int row = -1);
    void addColumnFromAttribut(const QListWidgetItem *iListItem);
    void resizeColumns();

    Ui::skgquerycreator_base ui{};
    SKGDocument *m_document;
    QString m_table;
    bool m_updateMode;
    QStringList m_attributes;
};

#endif // SKGQUERYCREATOR_H
