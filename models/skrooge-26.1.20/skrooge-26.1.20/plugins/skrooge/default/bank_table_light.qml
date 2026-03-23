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
    property var m: report==null ? null : report.bank_table
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

    ColumnLayout {
	spacing: 0

	// Set values
	Repeater {
	    model: m
	    SKGValue {
                font.pointSize: point_size
                Layout.fillWidth: true
		horizontalAlignment: index == 0 ? Text.AlignHCenter : Text.AlignRight
		font.bold: index == 0 || modelData[0]

		value: index == 0 ? null : modelData[6]
		max: modelData[0] ? null : maxValues(m, 6)
		backgroundColor: '#' + (value == null || value < 0 ? color_negativetext : color_positivetext)
		text: index == 0 ? modelData[6] : document.formatPrimaryMoney(modelData[6])
		url: font.bold ? "" : "skg://Skrooge_operation_plugin/?operationWhereClause=t_BANK='"+ modelData[1] + "'&title="+ modelData[1] + "&title_icon=view-bank-account"
	    }
	}
    }
}
