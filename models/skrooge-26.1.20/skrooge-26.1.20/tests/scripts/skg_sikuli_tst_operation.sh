#!/bin/sh

#initialisation
. "`dirname \"$0\"`/init.sh"

sikulirun operation
rc=$?
if [ $rc != 0 ] ; then
	exit $rc
fi

exit 0