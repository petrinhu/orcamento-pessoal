#!/bin/sh
EXE=skgtestimportmny3

#initialisation
. "`dirname \"$0\"`/init.sh"

chmod u-r "${IN}/skgtestimportmny1/notreadable.mny"

"${EXE}"
rc=$?

chmod u+r "${IN}/skgtestimportmny1/notreadable.mny"

if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0
