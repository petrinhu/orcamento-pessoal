/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
import QtQuick 2.12
import QtQuick.Controls 2.12

Column {
    property var point_size: report==null ? 0 : report.point_size
    
    Repeater {
        model: report==null ? null : report.categories_variations_issues
        Label {
            text: modelData
            font.pointSize: point_size
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
    }
}
