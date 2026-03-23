#!/bin/sh
EXE=skgtestactions

#initialisation
. "`dirname \"$0\"`/init.sh"

export SKGTEST=1

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0