/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a test script.
 *
 * @author Stephane MANKOWSKI
 */
#include "skgtestmacro.h"
#include "skgtreemap.h"

#define SKGTESTLAYOUT(VARIABLE, NAME, X, Y, W, H, VALUE)                                                                                                       \
    SKGTEST(QStringLiteral("getID"), (VARIABLE).getID(), NAME)                                                                                                 \
    SKGTEST(QStringLiteral("getX"), SKGServices::doubleToString((VARIABLE).getX()), SKGServices::doubleToString(X))                                            \
    SKGTEST(QStringLiteral("getY"), SKGServices::doubleToString((VARIABLE).getY()), SKGServices::doubleToString(Y))                                            \
    SKGTEST(QStringLiteral("getW"), SKGServices::doubleToString((VARIABLE).getW()), SKGServices::doubleToString(W))                                            \
    SKGTEST(QStringLiteral("getH"), SKGServices::doubleToString((VARIABLE).getH()), SKGServices::doubleToString(H))                                            \
    SKGTEST(QStringLiteral("getValue"), SKGServices::doubleToString((VARIABLE).getValue()), SKGServices::doubleToString(VALUE))

int main(int argc, char **argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)

    // Init test
    SKGINITTEST(true)

    // A Very simple case (1 element)
    {
        // Build initial test case
        SKGTreeMap t1(QStringLiteral("Root"), 0, 0, 0, 6, 4);
        t1.addChild(SKGTreeMap(QStringLiteral("1.1"), 100));

        // Compute the layout
        t1.compute();

        // Check root
        SKGTESTLAYOUT(t1, QStringLiteral("Root"), 0.0, 0.0, 6.0, 4.0, 100.0);

        // Check the layout
        auto tiles = t1.getChildren();
        SKGTESTLAYOUT(tiles[0], QStringLiteral("1.1"), 0.0, 0.0, 6.0, 4.0, 100.0);
    }

    // A simple case (2 elements horizontal)
    {
        // Build initial test case
        SKGTreeMap t1(QStringLiteral("Root"), 0, 0, 0, 6, 4);
        t1.addChild(SKGTreeMap(QStringLiteral("1.1"), 100));
        t1.addChild(SKGTreeMap(QStringLiteral("1.2"), 300));

        // Compute the layout
        t1.compute();

        // Check root
        SKGTESTLAYOUT(t1, QStringLiteral("Root"), 0.0, 0.0, 6.0, 4.0, 400.0);

        // Check the layout
        auto tiles = t1.getChildren();
        SKGTESTLAYOUT(tiles[0], QStringLiteral("1.2"), 0.0, 0.0, 4.5, 4.0, 300.0);
        SKGTESTLAYOUT(tiles[1], QStringLiteral("1.1"), 4.5, 0.0, 1.5, 4.0, 100.0);
    }

    // A simple case (2 elements vertical)
    {
        // Build initial test case
        SKGTreeMap t1(QStringLiteral("Root"), 0, 0, 0, 4, 6);
        t1.addChild(SKGTreeMap(QStringLiteral("1.1"), 100));
        t1.addChild(SKGTreeMap(QStringLiteral("1.2"), 300));

        // Compute the layout
        t1.compute();

        // Check root
        SKGTESTLAYOUT(t1, QStringLiteral("Root"), 0.0, 0.0, 4.0, 6.0, 400.0);

        // Check the layout
        auto tiles = t1.getChildren();
        SKGTESTLAYOUT(tiles[0], QStringLiteral("1.2"), 0.0, 0.0, 4.0, 4.5, 300.0);
        SKGTESTLAYOUT(tiles[1], QStringLiteral("1.1"), 0.0, 4.5, 4.0, 1.5, 100.0);
    }

    // More complex case
    // 4 |------------|
    //   | 6    |2|2|1|
    //   |      |-----|
    // 2 |-------4  |3|
    //   | 6    |   | |
    //   |      |   | |
    // 0 |------------|
    //   0      3     6
    {
        // Build initial test case
        SKGTreeMap t1(QStringLiteral("Root"), 0, 0, 0, 6, 4);
        t1.addChild(SKGTreeMap(QStringLiteral("1.1"), 6));
        t1.addChild(SKGTreeMap(QStringLiteral("1.6"), 2));
        t1.addChild(SKGTreeMap(QStringLiteral("1.3"), 4));
        t1.addChild(SKGTreeMap(QStringLiteral("1.4"), 3));
        t1.addChild(SKGTreeMap(QStringLiteral("1.5"), 2));
        t1.addChild(SKGTreeMap(QStringLiteral("1.2"), 6));
        t1.addChild(SKGTreeMap(QStringLiteral("1.7"), 1));
        t1.addChild(SKGTreeMap(QStringLiteral("1.8"), 0));

        // Compute the layout
        t1.compute();

        // Check root
        auto result = t1.getAllTilesById();
        SKGTESTLAYOUT(result[QStringLiteral("Root")], QStringLiteral("Root"), 0.0, 0.0, 6.0, 4.0, 24.0);

        // Check the layout
        SKGTESTLAYOUT(result[QStringLiteral("1.1")], QStringLiteral("1.1"), 0.0, 0.0, 3.0, 2.0, 6.0);
        SKGTESTLAYOUT(result[QStringLiteral("1.2")], QStringLiteral("1.2"), 0.0, 2.0, 3.0, 2.0, 6.0);
        SKGTESTLAYOUT(result[QStringLiteral("1.3")], QStringLiteral("1.3"), 3.0, 0.0, 4.0 * 3.0 / 7.0, 7.0 / 3.0, 4.0);
        SKGTESTLAYOUT(result[QStringLiteral("1.4")], QStringLiteral("1.4"), 3.0 + 4.0 * 3.0 / 7.0, 0.0, 3.0 * 3.0 / 7.0, 7.0 / 3.0, 3.0);

        SKGTESTLAYOUT(result[QStringLiteral("1.5")], QStringLiteral("1.5"), 3.0, 7.0 / 3.0, 2.0 * 3.0 / 5.0, 4.0 - 7.0 / 3.0, 2.0);
        SKGTESTLAYOUT(result[QStringLiteral("1.6")], QStringLiteral("1.6"), 3.0 + 2.0 * 3.0 / 5.0, 7.0 / 3.0, 2.0 * 3.0 / 5.0, 4.0 - 7.0 / 3.0, 2.0);
        SKGTESTLAYOUT(result[QStringLiteral("1.7")], QStringLiteral("1.7"), 3.0 + 4.0 * 3.0 / 5.0, 7.0 / 3.0, 1.0 * 3.0 / 5.0, 4.0 - 7.0 / 3.0, 1.0);
        SKGTESTLAYOUT(result[QStringLiteral("1.8")], QStringLiteral("1.8"), 3.0 + 4.0 * 3.0 / 5.0, 4.0, 0.6, 0, 0);
    }

    // End test
    SKGENDTEST()
}
