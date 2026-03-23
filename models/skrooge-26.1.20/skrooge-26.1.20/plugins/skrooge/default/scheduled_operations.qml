/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0

ColumnLayout {
    id: grid
    property var m: report==null ? null : report.scheduled_operations
    property var pixel_size: report==null ? 0 : report.point_size
    spacing: 0

    Repeater {
	model: m
        Row {
            SKGValue {
                font.pixelSize: pixel_size
                id: l
                text: modelData[1]
                url: modelData[2]!="" ? "skg://skrooge_scheduled_plugin/?selection="+modelData[2] : ""
                bold: modelData[0]
            }
            Button {
                text: qsTr("Skip")
                anchors.top: l.top
                anchors.bottom: l.bottom                
                onClicked: {
                    panel.openPage("skg://skip_scheduled_operations/?selection="+modelData[2])
                }
                visible: modelData[2]!=""
            }
        }
    }
}
