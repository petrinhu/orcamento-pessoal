/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGWIDGETSELECTOR_H
#define SKGWIDGETSELECTOR_H
/** @file
 * A widget selector.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */

#include "skgbasegui_export.h"
#include "ui_skgwidgetselector.h"

class QToolButton;

/**
 * This file is a color selector box with more features.
 */
class SKGBASEGUI_EXPORT SKGWidgetSelector : public QWidget
{
    Q_OBJECT
    /**
     * The selected mode
     */
    Q_PROPERTY(int selectedMode READ getSelectedMode WRITE setSelectedMode NOTIFY selectedModeChanged USER true)

    /**
     * The default mode
     */
    Q_PROPERTY(bool alwaysOneOpen READ getAlwaysOneOpen WRITE setAlwaysOneOpen NOTIFY alwaysOneOpenChanged USER true)

public:
    /**
     * A list of QWidget* ==> SKGListQWidget
     */
    using SKGListQWidget = QList<QWidget *>;

    /**
     * Default Constructor
     * @param iParent the parent
     */
    explicit SKGWidgetSelector(QWidget *iParent);

    /**
     * Default Destructor
     */
    ~SKGWidgetSelector() override;

    /**
     * Add a button to the selector
     * @param iIcon the icon
     * @param iTitle the text
     * @param iToolTip the tooltip
     * @param iListOfShownWidgets the list of widget to display
     */
    virtual void addButton(const QIcon &iIcon, const QString &iTitle, const QString &iToolTip, const SKGWidgetSelector::SKGListQWidget &iListOfShownWidgets);

    /**
     * Add a button to the selector
     * @param iIcon the icon
     * @param iTitle the text
     * @param iToolTip the tooltip
     * @param iWidgets the widget to display
     */
    virtual void addButton(const QIcon &iIcon, const QString &iTitle, const QString &iToolTip, QWidget *iWidgets);

    /**
     * Get the current selected mode
     * @return the current selected mode
     */
    virtual int getSelectedMode() const;

    /**
     * Set the selected mode
     * @param iMode the selected mode
     */
    virtual void setSelectedMode(int iMode);
    /**
     * Enable/disable a mode
     * @param iMode the mode
     * @param iEnabled the state
     */
    virtual void setEnabledMode(int iMode, bool iEnabled);

    /**
     * Get the "Always one open" mode
     * @return the mode
     */
    virtual bool getAlwaysOneOpen() const;

    /**
     * Set the "Always one open" mode.
     * @param iMode the mode
     */
    virtual void setAlwaysOneOpen(bool iMode);

Q_SIGNALS:
    /**
     * Emitted when the selected mode changed
     * @param iMode the new selected mode
     */
    void selectedModeChanged(int iMode);

    /**
     * Emitted when the property is modified
     */
    void alwaysOneOpenChanged();

private Q_SLOTS:

    void onButtonClicked();

private:
    Ui::skgwidgetselector_base ui{};

    QList<QToolButton *> m_listButton;
    QList<SKGListQWidget> m_listWidgets;
    // QHash<QWidget*, QRect> m_originGeometries;
    // QHash<QWidget*, QRect> m_endGeometries;
    int m_currentMode;
    bool m_alwaysOneOpen;
};

#endif // SKGWIDGETSELECTOR_H
