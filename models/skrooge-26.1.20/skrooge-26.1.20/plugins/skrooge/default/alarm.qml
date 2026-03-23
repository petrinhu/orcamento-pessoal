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
    property var m: report==null ? null : report.alarms
    property var point_size: report==null ? 0 : report.point_size
    spacing: 2

    ColumnLayout {
	spacing: 0

	// Set values
	Repeater {
	    model: m
	    SKGValue {
                font.pointSize: point_size
		horizontalAlignment: Text.AlignHCenter
		value: modelData[1]
		max: modelData[2]
		backgroundColor: '#' + (value == null || value < 0.7 * max  ? color_positivetext : value < 0.9 * max  ? color_neutraltext : color_negativetext)
		text: modelData[0]
	    }
	}
    }
}
