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
    property var m: report==null ? null : report.interests
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

		value: index == 0 ? null : modelData[2]
		max: modelData[0] ? null : maxValues(m, 2)
		backgroundColor: '#' + (value == null || value < 0 ? color_negativetext : color_positivetext)
		text: index == 0 ? modelData[2] : document.formatPrimaryMoney(modelData[2])
		url: font.bold ? "" : "skg://Skrooge_calculator_plugin/?currentPage=0&account="+ modelData[1] + "&year=" + m[0][2]
	    }
	}
    }
}
