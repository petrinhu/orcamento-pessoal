#!/bin/sh
EXE=skgtestmainpanel

#initialisation
. "`dirname \"$0\"`/init.sh"

rm "${OUT}advice.skg"
cp "${IN}advice.skg" "${OUT}advice.skg"
"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0
