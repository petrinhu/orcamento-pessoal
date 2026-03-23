#!/bin/sh
EXE=skgtestimportcsv

#initialisation
. "`dirname \"$0\"`/init.sh"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

checkDiff "${OUT}/skgtestimportcsv/bankperfect.csv" "${REF}/skgtestimportcsv/bankperfect.csv"

checkDiff "${OUT}/skgtestimportcsv/export_all.csv" "${REF}/skgtestimportcsv/export_all.csv"
checkDiff "${OUT}/skgtestimportcsv/export_la.csv" "${REF}/skgtestimportcsv/export_la.csv"

exit 0
