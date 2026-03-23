/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
import QtQuick 2.12
import QtQuick.Controls 2.12

Row {
    spacing: 5

    Rectangle {
        id: rect1
        width: 50
        height: width
        color: "#" + (report==null ? "red" : report.personal_finance_score_details.color)
        radius: width / 10

        Label {
            id: pfstext
            text: report==null ? null : report.personal_finance_score_details.value.toFixed(2)
            fontSizeMode: Text.Fit
            minimumPixelSize: 10
            font.pixelSize: 72
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            anchors.fill: rect1
        }
    }

    Column {
        spacing: 2
        Label {
            id: t1
            width: rect1.width * 4
            height: rect1.height / 2
            text: '=' + title_networth + ' / ' + title_annual_spending
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
        Label {
            id: t2
            width: t1.width
            height: t1.height
            text: '=' + document.formatPrimaryMoney(report==null ? 0 : report.networth) + ' / ' + document.formatPrimaryMoney(report==null ? 0 : report.annual_spending)
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
        Label {
            id: msg
            width: t1.width
            height: t1.height
            text: report==null ? null : report.personal_finance_score_details.message
            fontSizeMode: Text.Fit
            minimumPixelSize: 10
            font.pixelSize: 72
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.Wrap
        }
    }
}
