#!/bin/sh
EXE=skgtestimportqif1

#initialisation
. "`dirname \"$0\"`/init.sh"

"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

checkDiff "${OUT}/skgtestimportqif1/ref.qif" "${REF}/skgtestimportqif1/ref.qif"
checkDiff "${OUT}/skgtestimportqif1/E0269787.qif" "${REF}/skgtestimportqif1/E0269787.qif"
checkDiff "${OUT}/skgtestimportqif1/E0269787_bis.qif" "${OUT}/skgtestimportqif1/E0269787.qif"
checkDiff "${OUT}/skgtestimportqif1/E0269787_ref.qif" "${REF}/skgtestimportqif1/E0269787_ref.qif"
checkDiff "${OUT}/skgtestimportqif1/ing.qif" "${REF}/skgtestimportqif1/ing.qif"
checkDiff "${IN}/skgtestimportqif1/exp_inv.qif" "${OUT}/skgtestimportqif1/exp_inv.qif"
checkDiff "${OUT}/skgtestimportqif1/267996.csv" "${OUT}/skgtestimportqif1/267996.csv"

exit 0
