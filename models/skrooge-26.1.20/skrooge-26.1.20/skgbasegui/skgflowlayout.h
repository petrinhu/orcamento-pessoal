/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGFLOWLAYOUT_H
#define SKGFLOWLAYOUT_H
/** @file
 * This file defines classes SKGFlowLayout.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include <qlayout.h>
#include <qstyle.h>

#include "skgbasegui_export.h"

/**
 * This file is a flow layout
 */
class SKGBASEGUI_EXPORT SKGFlowLayout : public QLayout
{
    Q_OBJECT

public:
    /**
     * Constructor
     * @param iParent the parent
     * @param iMargin margin
     * @param hSpacing horizontal spacing
     * @param vSpacing vertical spacing
     */
    explicit SKGFlowLayout(QWidget *iParent, int iMargin = -1, int hSpacing = -1, int vSpacing = -1);

    /**
     * Constructor
     * @param iMargin margin
     * @param hSpacing horizontal spacing
     * @param vSpacing vertical spacing
     */
    explicit SKGFlowLayout(int iMargin = -1, int hSpacing = -1, int vSpacing = -1);

    /**
     * Default Destructor
     */
    ~SKGFlowLayout() override;

    /**
     * Add an item
     * @param item the item to add
     */
    void addItem(QLayoutItem *item) override;

    /**
     * Set horizontal and vertical spacing
     * @param space the space
     */
    virtual void setSpacing(int space) override;

    /**
     * Get horizontal spacing
     * @return horizontal spacing
     */
    virtual int horizontalSpacing() const;

    /**
     * Get vertical spacing
     * @return vertical spacing
     */
    virtual int verticalSpacing() const;

    /**
     * Get expanding directions
     * @return expanding directions
     */
    Qt::Orientations expandingDirections() const override;

    /**
     * Returns true if this layout's preferred height depends on its width; otherwise returns false.
     * The default implementation returns false.
     * Reimplement this function in layout managers that support height for width.
     * @return true of false
     */
    bool hasHeightForWidth() const override;

    /**
     * Returns the preferred height for this layout item, given the width w.
     * The default implementation returns -1, indicating that the preferred height is independent of the width of the item. Using the function
     * hasHeightForWidth() will typically be much faster than calling this function and testing for -1. Reimplement this function in layout managers that
     * support height for width
     * @return height
     */
    int heightForWidth(int /*width*/ /*unused*/) const override;

    /**
     * Returns the number of items
     * @return number of items
     */
    int count() const override;

    /**
     * Returns the item for an index
     * @param index the index
     * @return the item
     */
    QLayoutItem *itemAt(int index) const override;

    /**
     * Returns the minimum size
     * @return the minimum size
     */
    QSize minimumSize() const override;

    /**
     * Set the geometry
     * @param rect the geometry
     */
    void setGeometry(const QRect &rect) override;

    /**
     * Returns the preferred size
     * @return the preferred size
     */
    QSize sizeHint() const override;

    /**
     * Takes the item for an index
     * @param index the index
     * @return the item
     */
    QLayoutItem *takeAt(int index) override;

private:
    int doLayout(QRect rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> m_itemList;
    int m_hSpace;
    int m_vSpace;
};

#endif
