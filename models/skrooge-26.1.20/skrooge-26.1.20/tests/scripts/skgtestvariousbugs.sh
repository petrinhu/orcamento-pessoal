#!/bin/sh
EXE=skgtestvariousbugs

#initialisation
. "`dirname \"$0\"`/init.sh"

cp "${IN}skgtestvariousbugs/forrecovery.skg" "${OUT}skgtestvariousbugs/forrecovery.skg"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0
