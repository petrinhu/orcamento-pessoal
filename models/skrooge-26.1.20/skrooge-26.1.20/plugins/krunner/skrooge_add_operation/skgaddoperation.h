/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGADDOPERATION_H
#define SKGADDOPERATION_H

#include <krunner/abstractrunner.h>

/**
 * @brief An KRunner addon to create a new transaction
 *
 */
class SKGAddOperation : public KRunner::AbstractRunner
{
    Q_OBJECT

public:
    /**
     * @brief The constructor
     *
     * @param iParent The parent object
     * @param pluginMetaData Plugin metadata from metadata.json
     * @param args The list of arguments
     */
    SKGAddOperation(QObject *iParent, const KPluginMetaData &pluginMetaData, const QVariantList &args);

    /**
     * @brief Check if the user input match
     *
     * @param iContext The KRunner context
     * @return void
     */
    void match(KRunner::RunnerContext &iContext) override;

    /**
     * @brief Execute the creation of operation
     *
     * @param iContext The KRunner context
     * @param iMatch The query match
     * @return void
     */
    void run(const KRunner::RunnerContext &iContext, const KRunner::QueryMatch &iMatch) override;

    /**
     * @brief Reload the configuration
     *
     * @return void
     */
    void reloadConfiguration() override;

private:
    QString m_triggerWord;
};

#endif
