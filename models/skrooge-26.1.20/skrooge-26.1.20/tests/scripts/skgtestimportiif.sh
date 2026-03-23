#!/bin/sh
EXE=skgtestimportiif

#initialisation
. "`dirname \"$0\"`/init.sh"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

checkDiff "${OUT}/skgtestimportiif/export_all.iif" "${REF}/skgtestimportiif/export_all.iif"
checkDiff "${OUT}/skgtestimportiif/export_la.iif" "${REF}/skgtestimportiif/export_la.iif"

exit 0
