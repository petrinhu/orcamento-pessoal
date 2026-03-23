/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGDATEEDIT_H
#define SKGDATEEDIT_H
/** @file
 * A date edit with more features.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "kdateedit.h"

#include "skgbasegui_export.h"
/**
 * This file is a tab widget used by plugins
 * based on KDateEdit of PIM
 */
class SKGBASEGUI_EXPORT SKGDateEdit : public KPIM::KDateEdit
{
    Q_OBJECT
    /**
     * Mode of the editor
     */
    Q_PROPERTY(SKGDateEdit::Mode mode READ mode WRITE setMode NOTIFY modeChanged)
public:
    /**
     * Mode of the editor
     */
    enum Mode {
        PREVIOUS, /**< if date is incompleted, the previous one is selected */
        CURRENT, /**< if date is incomplete, the current month is selected */
        NEXT /**< if date is incomplete, the next one is selected */
    };
    /**
     * Mode of the editor
     */
    Q_ENUM(Mode)

    /**
     * Constructor
     * @param iParent the parent
     * @param name name
     */
    explicit SKGDateEdit(QWidget *iParent, const char *name = nullptr);

    /**
     * Destructor
     */
    ~SKGDateEdit() override;

    /**
     * Get the mode
     * @return the mode
     */
    Mode mode() const;

    /**
     * Set the mode
     * @param iMode the mode
     */
    void setMode(Mode iMode);

Q_SIGNALS:
    /**
     * Emitted when the mode changed
     */
    void modeChanged();

private:
    Mode m_mode;
};

#endif
