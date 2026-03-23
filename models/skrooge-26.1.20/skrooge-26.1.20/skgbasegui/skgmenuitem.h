/***************************************************************************
 * SPDX-FileCopyrightText: 2023 N. KRUPENKO krnekit@gmail.com
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGMENUITEM_H
#define SKGMENUITEM_H
/** @file
 * This file is a widget that could be used in QWidgetAction and looks like a menu item.
 *
 * @author Nikita KRUPENKO
 */
#include <qicon.h>
#include <qwidget.h>

#include "skgbasegui_export.h"

class QStyleOptionMenuItem;

/**
 * A widget that could be used in QWidgetAction and looks like a menu item
 */
class SKGBASEGUI_EXPORT SKGMenuitem : public QWidget
{
    Q_OBJECT

public:
    /**
     * Default Constructor
     * @param iParent the parent widget
     */
    explicit SKGMenuitem(QWidget *iParent = nullptr);

    /**
     * Default Destructor
     */
    ~SKGMenuitem() override;

    /**
     * Get menu item text
     * @return menu item text.
     */
    QString getText() const;

    /**
     * Set menu item text
     * @param iText menu item text
     */
    void setText(const QString &iText);

    /**
     * Get menu item icon
     * @return menu item icon.
     */
    QIcon getIcon() const;

    /**
     * Set menu item icon
     * @param iIcon menu item icon
     */
    void setIcon(const QIcon &iIcon);

    /**
     * Get menu item color
     * @return menu item color.
     */
    QColor getColor() const;

    /**
     * Set menu item color
     * @param iColor menu item color
     */
    void setColor(const QColor &iColor);

    /**
     * Get menu item text bold state
     * @return menu item text bold state.
     */
    bool getIsBold() const;

    /**
     * Set menu item text bold state
     * @param iBold menu item text bold state
     */
    void setIsBold(bool isBold);

    /**
     * Get recommended minimum size for the widget
     * @return recommended minimum size
     */
    QSize minimumSizeHint() const override;

Q_SIGNALS:
    /**
     * This signal is launched when the menu item text is modified
     */
    void textChanged(const QString &text);

    /**
     * This signal is launched when the menu item icon is modified
     */
    void iconChanged(const QIcon &icon);

    /**
     * This signal is launched when the menu item color is modified
     */
    void colorChanged(const QColor &color);

    /**
     * This signal is launched when the menu item text bold state is modified
     */
    void isBoldChanged(bool isBold);

protected:
    /**
     * Paint event handler
     * @param event object
     */
    void paintEvent(QPaintEvent *event) override;

private:
    /**
     * Set up the style option
     * @param option style option
     */
    void initStyleOption(QStyleOptionMenuItem *option) const;

    QString m_text;
    QIcon m_icon;
    QColor m_color;
    bool m_isBold = false;
};

#endif // SKGMENUITEM_H
