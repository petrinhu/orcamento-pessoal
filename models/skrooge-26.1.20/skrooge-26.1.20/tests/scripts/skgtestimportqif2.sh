#!/bin/sh
EXE=skgtestimportqif2

#initialisation
. "`dirname \"$0\"`/init.sh"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

checkDiff "${OUT}/skgtestimportqif2/export_all.qif" "${REF}/skgtestimportqif2/export_all.qif"
checkDiff "${OUT}/skgtestimportqif2/export_la.qif" "${REF}/skgtestimportqif2/export_la.qif"

exit 0
