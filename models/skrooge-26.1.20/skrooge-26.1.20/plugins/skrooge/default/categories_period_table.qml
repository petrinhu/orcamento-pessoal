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
    property var m: report==null ? null : report.categories_period
    property var point_size: report==null ? 0 : report.point_size
    property var max: maxValues(m)
    spacing: 2

    function maxValues(m) {
        var output = -1
        for (var i = 1; i < (m==null ? 0 : m.length); i++) {
            if (m[i][2] > output)
                output = m[i][2]
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
                font.bold: index == 0
                font.pointSize: point_size
                text: index == 0 ? modelData[1] : index + ": " + modelData[1]
                horizontalAlignment: index == 0 ? Text.AlignHCenter : Text.AlignLeft
            }
        }
    }

    ColumnLayout {
        spacing: 0

        // Set values
        Repeater {
            model: m
            SKGValue {
                font.pointSize: point_size
                Layout.fillWidth: true
                horizontalAlignment: index == 0 ? Text.AlignHCenter : Text.AlignRight
                font.bold: index == 0

                value: index == 0 ? null : modelData[2]
                max: grid.max
                text: index == 0 ? modelData[2] : document.formatPrimaryMoney(modelData[2])
                backgroundColor: '#' + color_negativetext
                url: index == 0 ? "" : "skg://Skrooge_operation_plugin/SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/?operationTable=v_suboperation_consolidated&operationWhereClause="
                                  + modelData[3] + "&title=" + modelData[1] + "/"
                                  + m[0][2] + "&title_icon=applications-all-symbolic"
            }
        }
    }
}
