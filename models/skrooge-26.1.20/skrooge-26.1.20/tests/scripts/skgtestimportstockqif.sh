#!/bin/sh
EXE=skgtestimportstockqif

#initialisation
. "`dirname \"$0\"`/init.sh"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

#checkDiff "${OUT}/skgtestimportqif/ref.qif" "${REF}/skgtestimportqif/ref.qif"


exit 0
