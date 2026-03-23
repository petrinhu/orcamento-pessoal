# A personal finances manager, powered by KDE

![Skrooge screenshots](https://skrooge.org/images/screenies/header1.png)

Skrooge allows you to manage your personal finances, powered by [KDE](https://www.kde.org). Thanks to its many [features](https://skrooge.org/features), it is one of the most powerful way to enter, follow, and analyze your expenses.

Based on its KDE foundations, Skrooge can run on many platforms, including of course Linux, BSD, Solaris, but also on macOS, and possibly on Windows.

Learn more about [skrooge.org](https://skrooge.org).

## Features

### Import your accounts from many sources

Skrooge is able to import transactions in many formats (AFB120, QIF, CSV, MT940, OFX, QFX).
For a more rich import, Skrooge is able to import documents from many applications ([KMYMONEY](https://kmymoney.org), Microsoft Money, [GNUCASH](https://gnucash.org), [GRISBI](https://fr.grisbi.org), [HOMEBANK](https://www.gethomebank.org/en/) and [MONEY MANAGER EX](https://www.moneymanagerex.org)).

And more, Skrooge is able to directly import transactions from all your banks web sites in one click.

### Reporting

Build the graph you want to understand well how you spend your money.
Have a look at the periodic reports (monthly, annually, …) to understand the progress.
Have a quick look at the dashboard.
Skrooge is also able to give you advice based on your behavior.

### Like in a web browser

Several tabs to help you organize your work.
Bookmark your preferred reports, graphs, filters, pages, …

### Budget

Budgeting is not about restriction. It is about setting and reaching your goals. Skrooge can help you manage your budget by creating simple rules.

### Classical features

Infinite categories levels.
Scheduled transactions.
Multiple currencies.
Manage payees.

### Advanced features

Infinite undo/redo (even after the file was closed !)
Mass update of transactions.
Automatically process transactions based on search conditions.
Instant filtering on transactions and reports.
Download of quotes.
Add all properties you want on all objects (transactions, accounts, categories, …) and including files (pdf, pictures, …).

### Track refund of your expenses

Skrooge can help you check that you have received the expected refund (e.g. medical).

## How to install it

More information here: <https://skrooge.org/download>

## How to build it

### Install prerequisites

- xsltproc
- libgrantlee5-dev (>=5.0.0) libkf6texttemplate-dev
- libsqlite3-dev (>=3.7.0)
- libofx-dev
- libboost-dev
- qtscript5-dev
- qttools5-dev qt6-tools-dev
- qtwebengine5-dev qt6-webengine-dev
- libqt5svg5-dev
- libkf5coreaddons-dev libkf6coreaddons-dev
- libkf5archive-dev
- libkf5xmlgui-dev
- libkf5wallet-dev libkf6wallet-dev
- libkf5parts-dev libkf6parts-dev
- libkf5newstuff-dev libkf6newstuff-dev
- libkf5iconthemes-dev libkf6iconthemes-dev
- libkf5kdelibs4support-dev
- libkf5notifyconfig-dev libkf6notifyconfig-dev
- libkf5runner-dev
- plasma-framework-dev
- libkf5doctools-dev
- kgendesignerplugin
- libkf5activities-dev libkf6activities-dev
- kross-dev
- qtdeclarative5-dev qt6-declarative-dev qt6-base-dev
- qtquickcontrols2-5-dev
- libsqlcipher-dev
- sqlcipher
- kirigami2-dev
- libkf6notifications-dev
- kf6-kstatusnotifieritem-dev
- qt6-5compat-dev
- libkf6guiaddons-dev
- qt6-svg-dev

#### Examples

On Ubuntu qt6/kf6:

    sudo apt-get install pkg-config build-essential cmake devscripts cdbs extra-cmake-modules kross-dev sqlite3 sqlcipher libsqlcipher-dev libsqlite3-dev libofx-dev libboost-dev xsltproc libkf6texttemplate-dev qt6-tools-dev qt6-webengine-dev libkf6coreaddons-dev libkf6wallet-dev libkf6parts-dev libkf6newstuff-dev libkf6iconthemes-dev libkf6activities-dev qt6-declarative-dev qt6-base-dev libkf6notifications-dev kf6-kstatusnotifieritem-dev qt6-5compat-dev libkf6guiaddons-dev qt6-svg-dev libkf6notifyconfig-dev

### Extract the archive, and enter the "skrooge" directory. Then run

    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=[path to your KDE installation]
    make

#### Examples

On Ubuntu:

    mkdir build && cd build

    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DSKG_DESIGNER=OFF

    make

On Ubuntu (with ninja):

    mkdir build && cd build
    release -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DSKG_BUILD_TEST=OFF -DSKG_DESIGNER=OFF
    make

### Prepare to run

Skrooge uses plugins. By default, plugins are loaded from your system. It means
that, if you have a local build of Skrooge and another version of Skrooge
installed on your system the system plugins will be loaded. The easiest solution
is to install your local build like this:

    make install (as root)

#### Examples

On Ubuntu:

    sudo make install

If you don't want to install Skrooge on your system, you will have to set many Qt and KDE environment variables before running skrooge. This is not easy to do.

### Run the test

    ctest

#### Run skrooge

    /usr/bin/skrooge

## Other important things to remember

### For Static code analysis (scan-build)

    CXX=clang++ CC=clang scan-build -k cmake .. -DCMAKE_INSTALL_PREFIX=`kf5-config --prefix` -DCMAKE_BUILD_TYPE=profile
    make clean
    scan-build -k -o results make

### With sanitizer (-fno-omit-frame-pointer )

    cmake .. -DCMAKE_INSTALL_PREFIX=`kf5-config --prefix` -DCMAKE_BUILD_TYPE=profile -DECM_ENABLE_SANITIZERS='undefined,address,asan,ubsan'
    export ASAN_OPTIONS=detect_leaks=1

### With clazy

    export CLAZY_CHECKS="level0,level1,level2"
    export CLAZY_EXPORT_FIXES=ON
    export CLAZY_FIXIT="fix-qlatin1string-allocations,fix-fromLatin1_fromUtf8-allocations,fix-fromCharPtrAllocations"
    cmake .. -DCMAKE_CXX_COMPILER=clazy -DCMAKE_INSTALL_PREFIX=`kf5-config --prefix` -DCMAKE_BUILD_TYPE=Debug

    make clean
    make -i > t.txt 2>&1
    cat t.txt | grep "warning:" | grep -v "ui_" | grep -v "moc_" | grep -v "_settings" |grep -v "/usr/include" | grep -v "FixIt failed" | grep -v ".moc:" | grep -v "/build/" | sort -u > warning_all.txt
    cat warning_all.txt | grep -v "ctor-missing-parent-argument" | grep -v "clazy-copyable-polymorphic" | sort -u > warning.txt

    clang-apply-replacements ..

### With clazy-standalone

    cmake .. -DCMAKE_INSTALL_PREFIX=`kf5-config --prefix` -DCMAKE_BUILD_TYPE=release -DCMAKE_EXPORT_COMPILE_COMMANDS=1
    make
    find . -name "*cpp" | xargs clazy-standalone -checks=level0,level1,level2 -p compile_commands.json > t.txt 2>&1
    cat t.txt | grep "warning:" | grep -v "ui_" | grep -v "moc_" | grep -v "_settings" |grep -v "/usr/include" | grep -v "FixIt failed" | sort -u > warning.txt

### clang-tidy

    run-clang-tidy.py -header-filter='.*' -checks=*,-fuchsia-*,-objc-*,-android-*,-abseil-*,-google-*,-misc-unused-parameters,-clang-diagnostic-error,-cppcoreguidelines-pro-type-member-init,-llvm-header-guard,-readability-inconsistent-declaration-parameter-name,-readability-redundant-member-init -fix > traces.txt
    cat traces.txt | grep warning  | grep -v ui_ | grep  -v /build/ | grep -v "clang-tidy -header" | sort -u > clangtidy.txt

### With coverity

    export PATH=~/Telechargements/cov-analysis-linux64-2024.12.1/bin/:$PATH
    make clean
    cov-build --dir cov-int make -j 4
    tar caf skrooge.bz2 cov-int

### iwyu

    iwyu_tool.py -p .

### Docker

    sudo docker build -t skrooge .
    sudo docker run -e DISPLAY=unix$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix skrooge

### Flatpak

See more at <https://develop.kde.org/docs/packaging/flatpak/>

#### Build

    mkdir app repo
    flatpak install flathub org.kde.Platform//5.12
    flatpak install flathub org.kde.Sdk//5.12
    flatpak-builder --ccache --repo=repo --subject="Build of skrooge `date`" skrooge org.kde.skrooge.json --force-clean

#### Install

    flatpak uninstall org.kde.skrooge
    flatpak install skrooge org.kde.skrooge

#### Run

    flatpak run org.kde.skrooge

#### Debug

    flatpak run --command=sh --devel org.kde.skrooge

### Convert all logos to 100x100

    for x in /home/s/Developpements/skrooge/images/logos/*.png
    do
        convert $x -resize 100x100 $x
    done
