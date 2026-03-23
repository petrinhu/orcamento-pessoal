#!/bin/sh
EXE=skgtestunit

#initialisation
. "`dirname \"$0\"`/init.sh"

export PATH=${IN}/../../skgbankmodeler:${PATH}

skrooge-boursorama.py AIRBUS 2024-09-01 1d
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0
