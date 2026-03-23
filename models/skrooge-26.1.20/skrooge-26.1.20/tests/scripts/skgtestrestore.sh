#!/bin/sh
EXE=skgtestrestore

#initialisation
. "`dirname \"$0\"`/init.sh"

cp -f "${IN}/skgtestrestore/"*.skg "${OUT}/skgtestrestore/."
cp -f "${IN}/skgtestrestore/nopwd.skg.wrk" "${OUT}/skgtestrestore/.nopwd.skg.wrk"
cp -f "${IN}/skgtestrestore/pwd.skg.wrk" "${OUT}/skgtestrestore/.pwd.skg.wrk"
cp -f "${IN}/skgtestrestore/recovery.skg" "${OUT}/skgtestrestore/recovery.skg"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0
