#!/bin/sh
EXE=skgtestimportmny2

#initialisation
. "`dirname \"$0\"`/init.sh"
export SKGTRACESQL=300
"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0