/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGTREEMAP_H
#define SKGTREEMAP_H
/** @file
 * This file defines classes SKGTreeMap.
 *
 * @author Stephane MANKOWSKI
 */
#include <qlist.h>
#include <qmap.h>
#include <qstring.h>

#include "skgbasemodeler_export.h"

/**
 * This class allows to compute a tree map
 */
class SKGBASEMODELER_EXPORT SKGTreeMap
{
public:
    /**
     * Constructor
     * @param iID The ID of this tile
     * @param iValue The value (this attribute is only needed for leaves)
     * @param iX The x of the tile
     * @param iY The y of the tile
     * @param iW The width of the tile
     * @param iH The height of the tile
     */
    explicit SKGTreeMap(QString iID = QString(), double iValue = 0.0, double iX = 0.0, double iY = 0.0, double iW = 100.0, double iH = 100.0);

    /**
     * Destructor
     */
    ~SKGTreeMap();

    /**
     * Add a child tile
     * @param iChildren The child tile
     */
    void addChild(const SKGTreeMap &iChildren);

    /**
     * Compute the tiles layout
     */
    void compute();

    /**
     * Get children tile
     * @return children tile
     */
    QList<SKGTreeMap> getChildren() const;

    /**
     * Get the ID of the tile
     * @return the ID of the tile
     */
    QString getID() const;

    /**
     * Get the value of the tile
     * @return value x of the tile
     */
    double getValue() const;

    /**
     * Set the x of the tile
     * @param iX The x of the tile
     */
    void setX(double iX);

    /**
     * Get the x of the tile
     * @return the x of the tile
     */
    double getX() const;

    /**
     * Set the y of the tile
     * @param iY The y of the tile
     */
    void setY(double iY);

    /**
     * Get the y of the tile
     * @return the y of the tile
     */
    double getY() const;

    /**
     * Set the width of the tile
     * @param iW The width of the tile
     */
    void setW(double iW);

    /**
     * Get the width of the tile
     * @return the width of the tile
     */
    double getW() const;

    /**
     * Set the height of the tile
     * @param iH The height of the tile
     */
    void setH(double iH);

    /**
     * Get the height of the tile
     * @return the height of the tile
     */
    double getH() const;

    /**
     * Get all tiles by ID
     * @return all tiles
     */
    QMap<QString, SKGTreeMap> getAllTilesById() const;

private:
    QString m_id;
    double m_value;
    double m_x;
    double m_y;
    double m_w;
    double m_h;

    QList<SKGTreeMap> m_children;

    void computeValuesAndSort();
};
#endif // SKGTREEMAP_H
