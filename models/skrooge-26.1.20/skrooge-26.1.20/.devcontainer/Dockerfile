FROM mcr.microsoft.com/devcontainers/cpp:1-debian-12

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.22.2"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
     && apt-get -y install --no-install-recommends pkg-config build-essential cmake devscripts cdbs extra-cmake-modules kross-dev sqlite3 sqlcipher libsqlcipher-dev libgrantlee5-dev libsqlite3-dev libofx-dev libboost-dev xsltproc qtscript5-dev qttools5-dev libqt5webkit5-dev libqt5svg5-dev libkf5coreaddons-dev libkf5archive-dev libkf5xmlgui-dev libkf5activities-dev libkf5wallet-dev libkf5parts-dev libkf5newstuff-dev libkf5iconthemes-dev libkf5kdelibs4support-dev libkf5notifyconfig-dev libkf5runner-dev libkf5doctools-dev kgendesignerplugin qtbase5-private-dev qtquickcontrols2-5-dev kirigami2-dev libkf5plasma-dev qtwebengine5-dev qtdeclarative5-dev libqt5xmlpatterns5-dev ledger default-jre
