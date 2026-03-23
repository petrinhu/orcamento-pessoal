#!/bin/sh
EXE=skgtestcalculatoredit

#initialisation
. "`dirname \"$0\"`/init.sh"

unset SKGTRACE #does not work if traces activated

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0