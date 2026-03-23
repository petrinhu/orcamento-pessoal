#!/bin/sh
EXE=skroogeconvert

#initialisation
. "`dirname \"$0\"`/init.sh"

cd "${OUT}skroogeconvert/"
"${EXE}" --in "${IN}/skgtestimportgnucash/228904.gnucash" --out 228904.skg
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

"${EXE}" --in 228904.skg --out 228904.csv
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

# Must return an error because pwd is missing
"${EXE}" --in "${IN}/skgtestrestore/sqlcipher_pwd_ABC.skg" --out "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlite"
rc=$?
if [ $rc = 0 ] ; then
	exit 99
fi

"${EXE}" --in "${IN}/skgtestrestore/sqlcipher_pwd_ABC.skg" --out "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlite" --param password --value ABC
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

"${EXE}" --in "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlite" --out "${OUT}skroogeconvert/sqlcipher_pwd_ABC.skg"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

echo "sqlite3 ${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlite"
sqlite3 -bail -echo "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlite" "SELECT COUNT(1) FROM operation"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

# Must return an error because pwd is missing
"${EXE}" --in "${IN}/skgtestrestore/sqlcipher_pwd_ABC.skg" --out "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlcipher"
rc=$?
if [ $rc = 0 ] ; then
	exit 99
fi


"${EXE}" --in "${IN}/skgtestrestore/sqlcipher_pwd_ABC.skg" --out "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlcipher" --param password --value ABC
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

"${EXE}" --in "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlcipher" --out "${OUT}skroogeconvert/sqlcipher_pwd_ABC.skg" --param password --value ABC
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

# Must return an error because encrypted
echo "sqlite3 ${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlcipher"
sqlite3 -bail -echo "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlcipher" "SELECT COUNT(1) FROM operation"
rc=$?
if [ $rc != 26 ] ; then
	exit 99
fi

echo "sqlcipher ${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlcipher"
sqlcipher -bail -echo "${OUT}skroogeconvert/sqlcipher_pwd_ABC.sqlcipher" "PRAGMA KEY='ABC';SELECT COUNT(1) FROM operation"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

export LANG="fr_FR.ISO8859-15"
export MM_CHARSET="ISO-8859-15"
export LC_ALL="fr_FR.ISO8859-1"

SPECIALDIR="${OUT}skroogeconvert/téléchargement"
mkdir "${SPECIALDIR}"
"${EXE}" --in 228904.csv --out "${SPECIALDIR}/228904.csv" --param header_position --value 1
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

#For coverage
"${EXE}" --in 228904.csv

#"${EXE}" --in ftp://skrooge.org/files/skgtestimporturl/test.skg --out t.csv
#rc=$?
#if [ $rc != 0 ] ; then
#	exit $rc
#fi

exit 0
