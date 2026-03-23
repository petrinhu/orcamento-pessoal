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
    property var m: report==null ? null : report.account_table
    property var point_size: report==null ? 0 : report.point_size
    spacing: 2

    function maxValues(m, id) {
        var output = 0
        for (var i = 1; i < m.length; i++) {
            if (!m[i][0] && Math.abs(m[i][id]) > output)
                output = Math.abs(m[i][id])
        }
        return output
    }

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
        model: 6
        ColumnLayout {
            spacing: 0
            property var modelId: modelData + 2

            // Set values
            Repeater {
                model: m
                SKGValue {
                    font.pointSize: point_size
                    Layout.fillWidth: true
                    horizontalAlignment: index == 0 ? Text.AlignHCenter : Text.AlignRight
                    font.bold: index == 0 || modelData[0]

                    value: index == 0 || parent.modelId == 4 || parent.modelId == 7 ? null : modelData[parent.modelId]
                    max: parent.modelId == 4 || parent.modelId == 7 || modelData[0] ? null : maxValues(m, parent.modelId)
                    backgroundColor: '#' + (value == null || value < 0 ? color_negativetext : color_positivetext)
                    text: index == 0 ? modelData[parent.modelId] : parent.modelId == 4 || parent.modelId == 7 ? document.formatPercentage(modelData[parent.modelId]) : document.formatPrimaryMoney(modelData[parent.modelId])
		    url: font.bold || parent.modelId != 6 ? "" : "skg://Skrooge_operation_plugin/?operationWhereClause=t_ACCOUNT='"+ modelData[1] + "'&title="+ modelData[1] + "&title_icon=view-financial-account"
                }
            }
        }
    }
}
