/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.0

RowLayout {
    id: grid
    property var m: report==null ? null : report.budget_table
    property var point_size: report==null ? 0 : report.point_size
    spacing: 2

    ColumnLayout {
        spacing: 0

        // Set titles
        Repeater {
            model: m
            Label {
                Layout.fillWidth: true
                font.bold: index == 0 || modelData[0]
                font.pointSize: point_size
                text: modelData[1]
                horizontalAlignment: index == 0 ? Text.AlignHCenter : Text.AlignLeft
            }
        }
    }

    Repeater {
        model: [2, 3, 4]
        ColumnLayout {
            spacing: 0
            property var modelId: modelData

            // Set values
            Repeater {
                model: m
                SKGValue {
                    font.pointSize: point_size
                    Layout.fillWidth: true
                    horizontalAlignment: index == 0 ? Text.AlignHCenter : Text.AlignRight
                    font.bold: index == 0 || modelData[0]

                    value: index == 0  ? null : modelData[parent.modelId]
                    backgroundColor: '#' + (value == null || value < 0 ? color_negativetext : color_positivetext)
                    text: index == 0 ? modelData[parent.modelId] : parent.modelId!=2 ? document.formatPrimaryMoney(modelData[parent.modelId]) : document.formatPrimaryMoney(modelData[5])

                    Timer {
                        interval: 2000; running: index != 0 && parent.modelId==2 && document.formatPrimaryMoney(modelData[5])!=document.formatPrimaryMoney(modelData[2]); repeat: true
                        onTriggered: {

                            if(parent.text == document.formatPrimaryMoney(modelData[5])) {
                                parent.text = document.formatPrimaryMoney(modelData[2])
                                parent.font.strikeout = true
                            }else {
                                parent.text = document.formatPrimaryMoney(modelData[5])
                                parent.font.strikeout = false
                            }
                        }
                    }
                }
            }
        }
    }
}
