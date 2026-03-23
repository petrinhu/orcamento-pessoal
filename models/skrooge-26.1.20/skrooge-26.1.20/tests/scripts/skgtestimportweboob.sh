#!/bin/sh
EXE=skgtestimportweboob

#initialisation
. "`dirname \"$0\"`/init.sh"

mkdir -p "${HOME}/.local/share/kservices5"
cp "${IN}/skgtestimportbackend/bulk/"*desktop "${HOME}/.local/share/kservices5/."

unset KDEHOME
unset XDG_DATA_HOME
unset XDG_CONFIG_HOME

cd "${IN}/skgtestimportbackend/bulk"
 
"${EXE}"
rc=$?
if [ $rc != 0 ] ; then
        exit $rc
fi

exit 0
