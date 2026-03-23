/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
/** @file
 * This file is a plugin interface default implementation.
 *
 * @author Stephane MANKOWSKI / Guillaume DE BURE
 */
#include "skgimportplugin.h"

SKGImportPlugin::SKGImportPlugin(QObject *iImporter)
    : QObject()
    , m_importer(qobject_cast<SKGImportExportManager *>(iImporter))
{
}
SKGImportPlugin::~SKGImportPlugin() = default;
