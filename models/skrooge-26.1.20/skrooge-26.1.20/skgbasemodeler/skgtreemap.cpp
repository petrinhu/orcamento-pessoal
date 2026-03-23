/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file implements classes SKGTreeMap.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtreemap.h"

#include "skgtraces.h"
#include <utility>

SKGTreeMap::SKGTreeMap(QString iID, double iValue, double iX, double iY, double iW, double iH)
    : m_id(std::move(iID))
    , m_value(iValue)
    , m_x(iX)
    , m_y(iY)
    , m_w(iW)
    , m_h(iH)
{
}

SKGTreeMap::~SKGTreeMap() = default;

QString SKGTreeMap::getID() const
{
    return m_id;
}

double SKGTreeMap::getValue() const
{
    return m_value;
}

void SKGTreeMap::setX(double iX)
{
    m_x = iX;
}

double SKGTreeMap::getX() const
{
    return m_x;
}

void SKGTreeMap::setY(double iY)
{
    m_y = iY;
}

double SKGTreeMap::getY() const
{
    return m_y;
}

void SKGTreeMap::setW(double iW)
{
    m_w = iW;
}

double SKGTreeMap::getW() const
{
    return m_w;
}

void SKGTreeMap::setH(double iH)
{
    m_h = iH;
}

double SKGTreeMap::getH() const
{
    return m_h;
}

void SKGTreeMap::addChild(const SKGTreeMap &iChildren)
{
    m_children.append(iChildren);
}

QList<SKGTreeMap> SKGTreeMap::getChildren() const
{
    return m_children;
}

void SKGTreeMap::computeValuesAndSort()
{
    // Compute the value
    if (m_children.count() != 0) {
        // Compute the value
        double sum = 0.0;
        for (auto &item : m_children) {
            item.computeValuesAndSort();
            sum += item.getValue();
        }

        // Set sum on tile
        m_value = sum;

        // Sort the children by abs(value) desc
        std::sort(m_children.begin(), m_children.end(), [](const SKGTreeMap &a, const SKGTreeMap &b) -> bool {
            if (qAbs(a.getValue() - b.getValue()) < 10e-5) {
                return a.getID() < b.getID();
            }
            return a.getValue() > b.getValue();
        });
    }
}

QMap<QString, SKGTreeMap> SKGTreeMap::getAllTilesById() const
{
    QMap<QString, SKGTreeMap> output;
    output.insert(getID(), *this);
    for (const auto &item : std::as_const(m_children)) {
        output.insert(item.getID(), item);
        auto children = item.getAllTilesById();
        for (const auto &item2 : std::as_const(children)) {
            output.insert(item2.getID(), item2);
        }
    }
    return output;
}

void SKGTreeMap::compute()
{
    // Prepare all the structure
    computeValuesAndSort();

    // If value = 0
    bool isValueZero = (getValue() < 10e-5);

    // Start the layout
    int nb = m_children.count();
    if (nb == 0) {
        // Nothing to do
    } else if (nb == 1) {
        // The child item must take the full place
        m_children[0].setX(getX());
        m_children[0].setY(getY());
        m_children[0].setW(getW());
        m_children[0].setH(getH());

        m_children[0].compute();
    } else if (nb == 2) {
        if (getW() >= getH()) {
            // Horizontal mode
            // Set the first
            m_children[0].setX(getX());
            m_children[0].setY(getY());
            m_children[0].setW(isValueZero ? 0.0 : getW() * m_children.at(0).getValue() / getValue());
            m_children[0].setH(getH());

            m_children[0].compute();

            // Set the second
            m_children[1].setX(getX() + m_children.at(0).getW());
            m_children[1].setY(getY());
            m_children[1].setW(getW() - m_children.at(0).getW());
            m_children[1].setH(getH());

            m_children[1].compute();
        } else {
            // Vertical
            // Set the first
            m_children[0].setX(getX());
            m_children[0].setY(getY());
            m_children[0].setW(getW());
            m_children[0].setH(isValueZero ? 0.0 : getH() * m_children.at(0).getValue() / getValue());

            m_children[0].compute();

            // Set the second
            m_children[1].setX(getX());
            m_children[1].setY(getY() + m_children.at(0).getH());
            m_children[1].setW(getW());
            m_children[1].setH(getH() - m_children.at(0).getH());

            m_children[1].compute();
        }
    } else {
        // Compute the number of element that can be aligned
        double sum = 0.0;
        int optimum = 0;
        double lastratio = 1000.0;
        double previous_gw = 0.0;
        double previous_gh = 0.0;
        for (int i = 0; i < nb; ++i) {
            sum += m_children.at(i).getValue();
            if (getW() >= getH()) {
                double gw = isValueZero ? 0.0 : getW() * sum / getValue();
                double ih = isValueZero ? 0.0 : m_children.at(i).getValue() * getW() * getH() / (gw * getValue());
                double ratio = qMax(ih / gw, gw / ih);
                if (ratio > lastratio) {
                    // This ratio is worst than te previous one
                    sum -= m_children.at(i).getValue();
                    optimum = i - 1;
                    break;
                }
                lastratio = ratio;
                previous_gw = gw;
            } else {
                double gh = isValueZero ? 0.0 : getH() * sum / getValue();
                double iw = isValueZero ? 0.0 : m_children.at(i).getValue() * getW() * getH() / (gh * getValue());
                double ratio = qMax(gh / iw, iw / gh);
                if (ratio > lastratio) {
                    // This ratio is worst than te previous one
                    sum -= m_children.at(i).getValue();
                    optimum = i - 1;
                    break;
                }
                lastratio = ratio;
                previous_gh = gh;
            }
        }

        // Set the layout
        double current_xy = 0.0;
        for (int i = 0; i <= optimum; ++i) {
            if (getW() >= getH()) {
                double ih = isValueZero ? 0.0 : m_children.at(i).getValue() * getW() * getH() / (previous_gw * getValue());

                m_children[i].setX(getX());
                m_children[i].setY(getY() + current_xy);
                m_children[i].setW(previous_gw);
                m_children[i].setH(ih);
                current_xy += ih;

                m_children[i].compute();
            } else {
                double iw = isValueZero ? 0.0 : m_children.at(i).getValue() * getW() * getH() / (previous_gh * getValue());

                m_children[i].setX(getX() + current_xy);
                m_children[i].setY(getY());
                m_children[i].setW(iw);
                m_children[i].setH(previous_gh);
                current_xy += iw;

                m_children[i].compute();
            }
        }

        // Treat the rest
        if (optimum == -1) {
            optimum = 1;
        }
        if (optimum < nb - 1) {
            // Create a new SKGTreeMap corresponding to the rest
            double x = getW() >= getH() ? getX() + previous_gw : getX();
            double y = getW() >= getH() ? getY() : getY() + previous_gh;
            double w = getW() >= getH() ? getW() - previous_gw : getW();
            double h = getW() >= getH() ? getH() : getH() - previous_gh;
            SKGTreeMap rest(QString(), getValue() - sum, x, y, w, h);

            // Add all items to compute
            for (int i = optimum + 1; i < nb; ++i) {
                rest.addChild(m_children.at(i));
            }

            // Compute
            rest.compute();
            auto computed = rest.getChildren();
            for (int i = optimum + 1; i < nb; ++i) {
                m_children[i] = computed[i - optimum - 1];
            }
        }
    }
}
