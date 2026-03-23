#! /bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg | grep -v _CPack_ | grep -v '/templates/' | sort -u` >> rc.cpp || exit 11
$XGETTEXT `find . -name \*.h -o -name \*.cpp -o -name \*.qml | grep -v '/templates/' | grep -v '/tests/'` -o $podir/skrooge.pot
rm -f rc.cpp
