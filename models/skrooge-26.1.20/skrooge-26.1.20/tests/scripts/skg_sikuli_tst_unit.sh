#!/bin/sh

#initialisation
. "`dirname \"$0\"`/init.sh"

sikulirun unit
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0