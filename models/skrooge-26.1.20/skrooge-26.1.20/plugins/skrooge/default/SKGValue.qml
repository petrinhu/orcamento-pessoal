/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0

Label {
    property var value: null
    property var max: null
    property var bold: false
    property var url: ""
    property var backgroundColor: "#FF0000"
    
    color: '#' + (value == null || max != null ? color_normaltext : (value < 0 ? color_negativetext : color_positivetext))
    
    Layout.fillWidth: true
    Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    font.pointSize: point_size
    font.bold: bold

    MouseArea {
        anchors.fill: parent
        cursorShape: url.length ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: {
            if (url.length) panel.openPage(url)
        }
    }
        
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        color: parent.backgroundColor
        width: parent.max == null || parent.max == 0 ? 0 : Math.abs(parent.width * parent.value / parent.max)
        z: -1
        visible: parent.value != null && parent.max != null
        
        Behavior on width {
            NumberAnimation {
                duration: 300
                easing.type: Easing.InOutQuad
            }
        }
    }
}
