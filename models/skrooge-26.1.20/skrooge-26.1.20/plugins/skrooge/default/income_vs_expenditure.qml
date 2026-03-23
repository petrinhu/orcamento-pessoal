/***************************************************************************
 * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
 * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
 * SPDX-License-Identifier: GPL-3.0-or-later
 ***************************************************************************/
import QtQuick 2.12
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.12

GridLayout {
    property var point_size: report==null ? 0 : report.point_size
    
    id: widget
	columns: 2

	property double maxValue: Math.max(incomes1Bar.value, expenditures1Bar.value, incomes2Bar.value, expenditures2Bar.value)
        property double totrigger: 1.0
        
        Connections {
            target: report
            function onChanged() {
                totrigger = 2.0; 
                totrigger = 1.0;
            }
        }
        
	Label{
		id: label1
        text: (period1Widget ? period1Widget.text : "")
		font.bold: true
		font.pointSize: point_size
		Layout.columnSpan: 2
	}
	
	Label{
		id: label11
		text: title_incomes
        font.pointSize: point_size
	}

	SKGValue {
        font.pointSize: point_size
		id: incomes1Bar
		max: parent.maxValue
		value: totrigger*(report==null ? 0 : report.getIncomeVsExpenditure(suboperationsWidget.checked, groupedWidget.checked, transferWidget.checked, trackerWidget.checked, period1Widget.whereClause, period2Widget.whereClause)[1][3])
		text: document.formatPrimaryMoney(value)
		horizontalAlignment: Text.AlignHCenter
		backgroundColor: '#' + color_positivetext
        url: "skg://skrooge_operation_plugin/" + (suboperationsWidget && suboperationsWidget.checked ? "SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/" :"") + "?operationTable=" +
        (suboperationsWidget && suboperationsWidget.checked ? "v_suboperation_consolidated": "v_operation_display") + "&title_icon=view-financial-account-savings&currentPage=-1&title=" +
        label11.text + ' ' + label1.text + (filterWidget_txt && filterWidget_txt.length>0 ? qsTr(" and (%1)").arg(filterWidget_txt) : "") +
                "&operationWhereClause=" + (filterWidget_sql && filterWidget_sql.length>0 ? "("+filterWidget_sql+")" : "1=1") + " AND "+ (period1Widget  ? period1Widget.whereClause : "1=0") + " AND t_TYPEEXPENSE='+'" +
        (groupedWidget && groupedWidget.checked ? "" : " AND i_group_id=0") + (transferWidget && transferWidget.checked ? "" : " AND t_TRANSFER='N'") + (trackerWidget && trackerWidget.checked ? "" : (suboperationsWidget && suboperationsWidget.checked ? " AND t_REALREFUND=''" :  " AND t_REFUND=''"));
	}

	Label{
		id: label12
		text: title_expenditures
        font.pointSize: point_size
	}

	SKGValue {
                font.pointSize: point_size
		id: expenditures1Bar
		max: parent.maxValue
		value: totrigger*(report==null ? 0 : report.getIncomeVsExpenditure(suboperationsWidget.checked, groupedWidget.checked, transferWidget.checked, trackerWidget.checked, period1Widget.whereClause, period2Widget.whereClause)[2][3])
		text: document.formatPrimaryMoney(value)
		horizontalAlignment: Text.AlignHCenter
		backgroundColor: '#' + color_negativetext
        url: "skg://skrooge_operation_plugin/" + (suboperationsWidget && suboperationsWidget.checked ? "SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/" :"") + "?operationTable=" +
        (suboperationsWidget && suboperationsWidget.checked ? "v_suboperation_consolidated": "v_operation_display") + "&title_icon=view-financial-account-savings&currentPage=-1&title=" +
        label12.text + ' ' + label1.text + (filterWidget_txt && filterWidget_txt.length>0 ? qsTr(" and (%1)").arg(filterWidget_txt) : "") +
                "&operationWhereClause=" + (filterWidget_sql && filterWidget_sql.length>0 ? "("+filterWidget_sql+")" : "1=1") + " AND "+ (period1Widget  ? period1Widget.whereClause : "1=0") + " AND t_TYPEEXPENSE='-'" +
        (groupedWidget && groupedWidget.checked ? "" : " AND i_group_id=0") + (transferWidget && transferWidget.checked ? "" : " AND t_TRANSFER='N'") + (trackerWidget && trackerWidget.checked ? "" : (suboperationsWidget && suboperationsWidget.checked ? " AND t_REALREFUND=''" :  " AND t_REFUND=''"));
	}
	
	Label{
		id: label13
		text: title_savings
        font.pointSize: point_size
	}

	SKGValue {
                font.pointSize: point_size
		id: savings1Bar
		max: parent.maxValue
		value: Math.abs(incomes1Bar.value - expenditures1Bar.value)
		text: document.formatPrimaryMoney(incomes1Bar.value - expenditures1Bar.value)
		horizontalAlignment: Text.AlignHCenter
		backgroundColor: '#' + (incomes1Bar.value - expenditures1Bar.value <0 ? color_negativetext : color_positivetext)
        url: "skg://skrooge_operation_plugin/" + (suboperationsWidget && suboperationsWidget.checked ? "SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/" :"") + "?operationTable=" +
        (suboperationsWidget && suboperationsWidget.checked ? "v_suboperation_consolidated": "v_operation_display") + "&title_icon=view-financial-account-savings&currentPage=-1&title=" +
        label13.text + ' ' + label1.text + (filterWidget_txt && filterWidget_txt.length>0 ? qsTr(" and (%1)").arg(filterWidget_txt) : "") +
                "&operationWhereClause=" + (filterWidget_sql && filterWidget_sql.length>0 ? "("+filterWidget_sql+")" : "1=1") + " AND "+ (period1Widget  ? period1Widget.whereClause : "1=0") +
        (groupedWidget && groupedWidget.checked ? "" : " AND i_group_id=0") + (transferWidget && transferWidget.checked ? "" : " AND t_TRANSFER='N'") + (trackerWidget && trackerWidget.checked ? "" : (suboperationsWidget && suboperationsWidget.checked ? " AND t_REALREFUND=''" :  " AND t_REFUND=''"));
	}
	
	
	Label{
		id: label2
        text: period2Widget ? period2Widget.text : ""
		font.bold: true
		font.pointSize: point_size
		Layout.columnSpan: 2
	}
	
	Label{
		id: label21
		text: title_incomes
        font.pointSize: point_size
	}

	SKGValue {
                font.pointSize: point_size
		id: incomes2Bar
		max: parent.maxValue
		value: totrigger*(report==null ? 0 : report.getIncomeVsExpenditure(suboperationsWidget.checked, groupedWidget.checked, transferWidget.checked, trackerWidget.checked, period1Widget.whereClause, period2Widget.whereClause)[1][2])
		text: document.formatPrimaryMoney(value)
		horizontalAlignment: Text.AlignHCenter
		backgroundColor: '#' + color_positivetext
        url: "skg://skrooge_operation_plugin/" + (suboperationsWidget && suboperationsWidget.checked ? "SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/" :"") + "?operationTable=" +
        (suboperationsWidget && suboperationsWidget.checked ? "v_suboperation_consolidated": "v_operation_display") + "&title_icon=view-financial-account-savings&currentPage=-1&title=" +
        label21.text + ' ' + label2.text + (filterWidget_txt && filterWidget_txt.length>0 ? qsTr(" and (%1)").arg(filterWidget_txt) : "") +
                "&operationWhereClause=" + (filterWidget_sql && filterWidget_sql.length>0 ? "("+filterWidget_sql+")" : "1=1") + " AND "+ (period2Widget ? period2Widget.whereClause : "1=0") + " AND t_TYPEEXPENSE='+'" +
        (groupedWidget && groupedWidget.checked ? "" : " AND i_group_id=0") + (transferWidget && transferWidget.checked ? "" : " AND t_TRANSFER='N'") + (trackerWidget && trackerWidget.checked ? "" : (suboperationsWidget && suboperationsWidget.checked ? " AND t_REALREFUND=''" :  " AND t_REFUND=''"));
	}

	Label{
		id: label22
		text: title_expenditures
        font.pointSize: point_size
	}

	SKGValue {
                font.pointSize: point_size
		id: expenditures2Bar
		max: parent.maxValue
		value: totrigger*(report==null ? 0 : report.getIncomeVsExpenditure(suboperationsWidget.checked, groupedWidget.checked, transferWidget.checked, trackerWidget.checked, period1Widget.whereClause, period2Widget.whereClause)[2][2])
		text: document.formatPrimaryMoney(value)
		horizontalAlignment: Text.AlignHCenter
		backgroundColor: '#' + color_negativetext
        url: "skg://skrooge_operation_plugin/" + (suboperationsWidget && suboperationsWidget.checked ? "SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/" :"") + "?operationTable=" +
        (suboperationsWidget && suboperationsWidget.checked ? "v_suboperation_consolidated": "v_operation_display") + "&title_icon=view-financial-account-savings&currentPage=-1&title=" +
        label22.text + ' ' + label2.text + (filterWidget_txt && filterWidget_txt.length>0 ? qsTr(" and (%1)").arg(filterWidget_txt) : "") +
                "&operationWhereClause=" + (filterWidget_sql && filterWidget_sql.length>0 ? "("+filterWidget_sql+")" : "1=1") + " AND "+ (period2Widget ? period2Widget.whereClause : "1=0") + " AND t_TYPEEXPENSE='-'" +
        (groupedWidget && groupedWidget.checked ? "" : " AND i_group_id=0") + (transferWidget && transferWidget.checked ? "" : " AND t_TRANSFER='N'") + (trackerWidget && trackerWidget.checked ? "" : (suboperationsWidget && suboperationsWidget.checked ? " AND t_REALREFUND=''" :  " AND t_REFUND=''"));
	}
	
	Label{
		id: label23
		text: title_savings
        font.pointSize: point_size
	}

	SKGValue {
		id: savings2Bar
		max: parent.maxValue
		value: Math.abs(incomes2Bar.value - expenditures2Bar.value)
		text: document.formatPrimaryMoney(incomes2Bar.value - expenditures2Bar.value)
		horizontalAlignment: Text.AlignHCenter
		backgroundColor: '#' + (incomes2Bar.value - expenditures2Bar.value <0 ? color_negativetext : color_positivetext)
        url: "skg://skrooge_operation_plugin/" + (suboperationsWidget && suboperationsWidget.checked ? "SKGOPERATION_CONSOLIDATED_DEFAULT_PARAMETERS/" :"") + "?operationTable=" +
        (suboperationsWidget && suboperationsWidget.checked ? "v_suboperation_consolidated": "v_operation_display") + "&title_icon=view-financial-account-savings&currentPage=-1&title=" +
        label23.text + ' ' + label2.text + (filterWidget_txt && filterWidget_txt.length>0 ? qsTr(" and (%1)").arg(filterWidget_txt) : "") +
                "&operationWhereClause=" + (filterWidget_sql && filterWidget_sql.length>0 ? "("+filterWidget_sql+")" : "1=1") + " AND "+ (period2Widget ? period2Widget.whereClause : "1=0") +
        (groupedWidget && groupedWidget.checked ? "" : " AND i_group_id=0") + (transferWidget && transferWidget.checked ? "" : " AND t_TRANSFER='N'") + (trackerWidget && trackerWidget.checked ? "" : (suboperationsWidget && suboperationsWidget.checked ? " AND t_REALREFUND=''" :  " AND t_REFUND=''"));
	}	
}
