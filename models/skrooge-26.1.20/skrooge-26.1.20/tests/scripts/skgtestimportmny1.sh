#!/bin/sh
EXE=skgtestimportmny1

#initialisation
. "`dirname \"$0\"`/init.sh"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	java -cp "${HOME}/.skrooge/sunriise.jar" com/le/sunriise/export/ExportToJSON "${IN}/skgtestimportmny1/wrongfile.mny" "" "${OUT}"
	exit $rc
fi

exit 0
