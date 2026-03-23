#!/bin/sh
#initialisation
. "`dirname \"$0\"`/init.sh"

export SKGTEST=1

skrooge
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

skrooge "${IN}/advice.skg"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

skrooge "${IN}/notfound.skg"
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

skrooge
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

skrooge
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0
