/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
#ifndef SKGGRANTLEEFILTERS_H
#define SKGGRANTLEEFILTERS_H
/** @file
 * The grantlee's plugin to define filters.
 *
 * @author Stephane MANKOWSKI
 */
#include <KTextTemplate/TagLibraryInterface>
#include <qobject.h>

/**
 * The grantlee's plugin to define filters
 */
class SKGGrantleeFilters : public QObject, public KTextTemplate::TagLibraryInterface
{
    Q_OBJECT
    Q_INTERFACES(KTextTemplate::TagLibraryInterface)
    Q_PLUGIN_METADATA(IID "org.grantlee.TagLibraryInterface")

public:
    /**
     * Default Constructor
     */
    explicit SKGGrantleeFilters(QObject *iParent = nullptr);

    /**
     * Default Destructor
     */
    ~SKGGrantleeFilters() override;

    /**
     * Returns the Filter implementations available in this library
     * @param iName the name
     * @return the implementations
     */
    QHash<QString, KTextTemplate::Filter *> filters(const QString &iName = QString()) override;
};

#endif // SKGGRANTLEEFILTERS_H
