#!/bin/sh
EXE=skgtestimporturls

#initialisation
. "`dirname \"$0\"`/init.sh"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0