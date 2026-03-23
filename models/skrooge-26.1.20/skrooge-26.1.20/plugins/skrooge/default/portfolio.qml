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
    property var m: report==null ? null : report.portfolio
    property var point_size: report==null ? 0 : report.point_size
    spacing: 2

    function maxValues(m, id) {
        var output = 0
        for (var i = 1; i < m.length; i++) {
            if (Math.abs(m[i][id]) > output)
                output = Math.abs(m[i][id])
        }
        return output
    }

    Repeater {
        model: 7
        ColumnLayout {
            spacing: 0
            property var modelId: modelData + 1

            // Set values
            Repeater {
                model: m
                SKGValue {
                    font.pointSize: point_size
                    Layout.fillWidth: true
                    horizontalAlignment: index == 0 ? Text.AlignHCenter :(parent.modelId <=2 ? Text.AlignLeft : Text.AlignRight)
                    font.bold: index == 0

                    value: index == 0 || parent.modelId <=2 ? null : modelData[parent.modelId]
                    max: index == 0 || parent.modelId <=3 || parent.modelId >=6 ? null : maxValues(m, parent.modelId)
                    backgroundColor: '#' + (value == null || value < 0 ? color_negativetext : color_positivetext)
                    text: index == 0 || parent.modelId <=2 ? parent.modelId==7 ? "%" : modelData[parent.modelId] : parent.modelId==7 ? document.formatPercentage(modelData[parent.modelId]) : document.formatPrimaryMoney(modelData[parent.modelId])
		    url: font.bold || parent.modelId != 5 ? "" : "skg://Skrooge_operation_plugin/?operationWhereClause=t_UNIT='"+ modelData[0] + "'&title=Transactions with unit equal to '"+ modelData[1] + "'&title_icon=view-currency-list&currentPage=-1"
                }
            }
        }
    }
}
