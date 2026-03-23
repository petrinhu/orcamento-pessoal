#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ***************************************************************************
# * SPDX-FileCopyrightText: 2024 S. MANKOWSKI stephane@mankowski.fr
# * SPDX-FileCopyrightText: 2024 G. DE BURE support@mankowski.fr
# * SPDX-License-Identifier: GPL-3.0-or-later
# ***************************************************************************

import argparse
import datetime
import os
import shutil
import subprocess
import sys
import fileinput

__VERSION__ = "1.0.0"

toolPath = os.path.dirname(os.path.realpath(__file__))
localPath = os.path.dirname(toolPath)
tempDir = "/data"


class Releasor(object):
    def __init__(self, args):
        self.versionstotreat = [
            {"v": "6", "ubuntuVersions": ["plucky", "questing"]},
        ]
        self.workdingDir = os.path.join(tempDir, "skrooge-release_" + args.version)
        print("# Working directory :" + self.workdingDir)
        if args.version.endswith(".0") or args.stable:
            self.ppa = "ppa"
            self.ppatotreat = ["beta", self.ppa]
        else:
            self.ppa = "beta"
            self.ppatotreat = [self.ppa]

        if os.path.exists(self.workdingDir):
            self.logfile = open(os.path.join(self.workdingDir, "log.txt"), "w")
        else:
            self.logfile = None

    def prepareWorkingDirectory(self, args):
        print("# Prepare the working directory")
        if os.path.exists(self.workdingDir):
            print("#   Remove " + self.workdingDir)
            shutil.rmtree(self.workdingDir)
        os.mkdir(self.workdingDir)
        os.chdir(self.workdingDir)

        self.logfile = open(os.path.join(self.workdingDir, "log.txt"), "w")
        print("#   DONE")
        return 0

    def makeTarFile(self, args):
        rc = 0
        if args.fromtar:
            print("# Make the tar file from " + args.fromtar)
            dst = os.path.join(self.workdingDir, "skrooge-" + args.version + ".tar.xz")
            shutil.copyfile(args.fromtar, dst)

            print("#   DONE")
        else:
            print("# Make the tar file")
            cmd = ["git", "clone", "https://github.com/KDE/releaseme.git"]
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
            if rc == 0:
                print("# Launch tarme.rb")
                cmd = [
                    "releaseme/tarme.rb",
                    "--version",
                    args.version,
                    "--origin",
                    "trunk",
                    "skrooge",
                ]
                self.logfile.write("### " + " ".join(cmd) + "\n")
                rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)

                shutil.rmtree(os.path.join(self.workdingDir, "skrooge-" + args.version))
                shutil.rmtree("releaseme")
                os.remove(os.path.join(self.workdingDir, "release_data"))
                sig_file = os.path.join(
                    self.workdingDir, "skrooge-" + args.version + ".tar.xz.sig"
                )
                if os.path.exists(sig_file):
                    os.remove(sig_file)
            print("#   " + ("DONE" if rc == 0 else "FAILED"))
        return rc

    def updateTarFile(self, args):
        print("# Update the tar file")
        os.chdir(self.workdingDir)
        p = "skrooge-" + args.version
        if os.path.exists(p):
            shutil.rmtree(p)
        tarfile = "skrooge-" + args.version + ".tar.xz"
        print("#   Untar " + tarfile)
        cmd = ["tar", "-xvf", tarfile]
        self.logfile.write("### " + " ".join(cmd) + "\n")
        rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
        if rc == 0:
            print("#   Change the CMakeLists.txt")
            with open(os.path.join(p, "CMakeLists.txt"), "r") as fileCMakeLists:
                CMakeListsContent = ""
                for line in fileCMakeLists:
                    if line.startswith("SET(SKG_VERSION"):
                        print('#     SKG_VERSION changes to "' + args.version + '"')
                        CMakeListsContent += 'SET(SKG_VERSION "' + args.version + '")\n'
                    else:
                        if line.startswith("SET(SKG_BETA"):
                            bb = "BETA" if self.ppa == "beta" else ""
                            print('#     SKG_BETA changes to "' + bb + '"')
                            CMakeListsContent += 'SET(SKG_BETA "' + bb + '")\n'
                        else:
                            if line.startswith("FEATURE_SUMMARY"):
                                CMakeListsContent += line
                                break
                            else:
                                CMakeListsContent += line
            with open(os.path.join(p, "CMakeLists.txt"), "w") as fileCMakeLists:
                fileCMakeLists.write(CMakeListsContent)

            # for line in fileinput.input(['skrooge/org.kde.skrooge.appdata.xml'], inplace=True):
            #    print(line.replace('<releases>', '<releases>\n<release version="'+args.version+'" date="'+datetime.datetime.today().strftime('%Y-%m-%d')+'"/>'), end='')

            print("#   Build the new splash screen")
            buildPath = os.path.join(p, "build")
            os.mkdir(buildPath)
            os.chdir(buildPath)
            cmd = [
                "cmake",
                "..",
                "-DCMAKE_INSTALL_PREFIX=/usr",
            ]
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
            if rc == 0:
                cmd = ["make", "splash"]
                self.logfile.write("### " + " ".join(cmd) + "\n")
                rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
                if rc == 0:
                    print("#   Create the tar file")
                    os.chdir(self.workdingDir)
                    shutil.rmtree(buildPath)
                    os.remove(tarfile)
                    cmd = ["tar", "-cJf", tarfile, p]
                    rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
                    self.logfile.write("### " + " ".join(cmd) + "\n")
        if rc == 0:
            shutil.rmtree(p)
        print("#   " + ("DONE" if rc == 0 else "FAILED"))
        return rc

    def generatePublicationMessage(self, args, filename):
        cmd = ["gpg2", "--armor", "--detach-sig", "-o", filename + ".sig", filename]
        self.logfile.write("### " + " ".join(cmd) + "\n")
        rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
        if rc == 0:
            print(
                "Skrooge "
                + args.version
                + " released\n\nHi,\n\nCould you publish the following files in "
                + ("stable" if args.stable else "unstable")
                + "/skrooge ?\n"
            )
            cmd = ["sha256sum", filename]
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd)
            if rc == 0:
                cmd = ["sha256sum", filename + ".sig"]
                self.logfile.write("### " + " ".join(cmd) + "\n")
                rc = subprocess.call(cmd)
            print("Thank you.\nRegards.\n")

    def buildDebian(self, args):
        print("# Get dsc")
        for v in self.versionstotreat:
            os.chdir(self.workdingDir)
            cmd = ["apt", "source", "skrooge"]
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
            if rc == 0:
                previousPackage = [
                    os.path.join(self.workdingDir, o)
                    for o in os.listdir(self.workdingDir)
                    if os.path.isdir(os.path.join(self.workdingDir, o))
                    and o.startswith("skrooge-")
                    and not o.endswith(".orig")
                ][0]
                print("#   previousPackage=" + previousPackage)
                os.chdir(previousPackage)
                if os.path.exists("debian"):
                    shutil.rmtree("debian")
                cmd = [
                    "tar",
                    "-xvf",
                    os.path.join(
                        toolPath, "skrooge-release-debian-kf" + v["v"] + ".tar.gz"
                    ),
                ]
                self.logfile.write("### " + " ".join(cmd) + "\n")
                rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
                if rc == 0:
                    cmd = ["uupdate", "-u", "skrooge-" + args.version + ".tar.xz"]
                    self.logfile.write("### " + " ".join(cmd) + "\n")
                    rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
                if rc == 0:
                    # Read the changelog
                    os.chdir("../skrooge-" + args.version)
                    if os.path.exists("debian"):
                        shutil.rmtree("debian")
                    cmd = [
                        "tar",
                        "-xvf",
                        os.path.join(
                            toolPath, "skrooge-release-debian-kf" + v["v"] + ".tar.gz"
                        ),
                    ]
                    self.logfile.write("### " + " ".join(cmd) + "\n")
                    rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
                    with open(
                        os.path.join(
                            self.workdingDir, "skrooge-" + args.version + "/CHANGELOG"
                        ),
                        "r",
                    ) as fileChangelog:
                        fileChangelog.readline()  # To pass the first line
                        changelogContent = ""
                        done = False
                        for line in fileChangelog:
                            if line.strip() == "" and done == False:
                                changelogContent += " -- Stéphane MANKOWSKI <stephane@mankowski.fr>  {}\n".format(
                                    datetime.datetime.now().strftime(
                                        "%a, %d %b %Y %H:%M:%S +0100"
                                    )
                                )
                                done = True
                            if not line.startswith(" -- "):
                                changelogContent += line

                        #
                        for ppa in self.ppatotreat:
                            for i in range(len(v["ubuntuVersions"])):
                                print(
                                    "#   {}/{}: {} - {}-kf{}".format(
                                        i + 1,
                                        len(v["ubuntuVersions"]),
                                        v["ubuntuVersions"][i],
                                        ppa,
                                        v["v"],
                                    )
                                )
                                f = open(
                                    os.path.join(
                                        self.workdingDir,
                                        "skrooge-" + args.version + "/debian/control",
                                    ),
                                    "r",
                                )
                                cf = f.read()
                                f.close()

                                # print('#   make changes for '+v["ubuntuVersions"][i])
                                # f = open(os.path.join(self.workdingDir, 'skrooge-'+args.version+'/debian/control'), 'w')
                                # f.write(cf.replace('libqjson-dev, \n', '').replace('libqjson0, ', ''))
                                # f.close()

                                with open(
                                    os.path.join(
                                        self.workdingDir,
                                        "skrooge-" + args.version + "/debian/changelog",
                                    ),
                                    "w",
                                ) as fileDebianChangelog:
                                    fileDebianChangelog.write(
                                        "skrooge ("
                                        + args.version
                                        + "-0ubuntu"
                                        + v["v"]
                                        + "~"
                                        + ppa
                                        + str(i + 1)
                                        + ") "
                                        + v["ubuntuVersions"][i]
                                        + "; urgency=medium\n"
                                    )
                                    fileDebianChangelog.write(changelogContent + "\n")

                                    cmd = ["debuild", "-S", "-sa"]
                                    self.logfile.write("### " + " ".join(cmd) + "\n")
                                    rc = subprocess.call(
                                        cmd, stdout=self.logfile, stderr=self.logfile
                                    )
                                    if rc != 0:
                                        break
                            if rc != 0:
                                break

                        os.chdir(self.workdingDir)
                        print("#   Cleaning")
                        shutil.rmtree("skrooge-" + args.version + ".orig")
                        print("#     1- skrooge-" + args.version + ".orig")
                        shutil.rmtree("skrooge-" + args.version)
                        print("#     2- skrooge-" + args.version)
                        if os.path.exists(previousPackage):
                            shutil.rmtree(previousPackage)
                            print("#     3- " + previousPackage)
                        t = previousPackage.replace(
                            os.path.basename(previousPackage),
                            os.path.basename(previousPackage).replace("-", "_"),
                        )
                        if os.path.exists(t + ".orig.tar.xz"):
                            os.remove(t + ".orig.tar.xz")
                            print("#     4- " + t + ".orig.tar.xz")
        print("#   " + ("DONE" if rc == 0 else "FAILED"))
        return rc

    def publishLaunchpad(self, args):
        print("# Publish on launchpad")
        rc = 0
        if not args.publish:
            print("#   Publication ignored")
        os.chdir(self.workdingDir)
        for v in self.versionstotreat:
            for ppa in self.ppatotreat:
                for i in range(len(v["ubuntuVersions"])):
                    print(
                        "#   {}/{}: {} {}".format(
                            i + 1, len(v["ubuntuVersions"]), v["ubuntuVersions"][i], ppa
                        )
                    )
                    cmd = [
                        "dput",
                        "-f",
                        "ppa:s-mankowski/" + ppa + "-kf" + v["v"],
                        "skrooge_"
                        + args.version
                        + "-0ubuntu"
                        + v["v"]
                        + "~"
                        + ppa
                        + str(i + 1)
                        + "_source.changes",
                    ]
                    if not args.publish:
                        print("#     " + " ".join(cmd) + "\n")
                    else:
                        self.logfile.write("### " + " ".join(cmd) + "\n")
                        rc = subprocess.call(
                            cmd, stdout=self.logfile, stderr=self.logfile
                        )
                        if rc != 0:
                            break
        print("#   " + ("DONE" if rc == 0 else "FAILED"))
        return rc

    def publishKDE(self, args):
        print("# Publish on kde")
        os.chdir(self.workdingDir)
        rc = 0
        if not args.publish:
            print("#   Publication ignored")
        os.chdir(self.workdingDir)
        filename = (
            "skrooge-" + args.version + "-x86_64.AppImage"
            if args.appimage
            else (
                "skrooge_" + args.version + "_amd64.snap"
                if args.snap
                else "skrooge-" + args.version + ".tar.xz"
            )
        )
        self.generatePublicationMessage(args, filename)

        cmd = ["kdecp5", filename, "ftp://upload.kde.org/incoming"]
        if not args.publish:
            print("#     " + " ".join(cmd) + "\n")
        else:
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
            if rc != 0:
                print("#   " + " ".join(cmd) + " FAILED. Check if already existing")

            cmd = ["kdecp5", filename + ".sig", "ftp://upload.kde.org/incoming"]
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
            if rc != 0:
                print("#   " + " ".join(cmd) + " FAILED. Check if already existing")
        print("#   " + ("DONE" if rc == 0 else "FAILED"))
        return rc

    def buildAppImage(self, args):
        print("# Generate App Image")
        os.chdir(self.workdingDir)
        targteappimage = os.path.join(
            self.workdingDir, "skrooge-" + args.version + "-x86_64.AppImage"
        )
        if os.path.exists(targteappimage):
            os.remove(targteappimage)

        cmd = [
            "wget",
            "-c",
            "-nv",
            "https://raw.githubusercontent.com/probonopd/AppImages/master/pkg2appimage",
        ]
        self.logfile.write("### " + " ".join(cmd) + "\n")
        rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
        if rc == 0:
            for line in fileinput.input(["pkg2appimage"], inplace=True):
                if not "libcurl-slim" in line:
                    print(line, end="")

            os.chmod("./pkg2appimage", 0o775)

            if args.fromlocal:
                print("#   Get appimage.yml from local path")
                cmd = ["cp", os.path.join(localPath, "appimage.yml"), "."]
            else:
                cmd = [
                    "wget",
                    "-c",
                    "-nv",
                    "https://invent.kde.org/office/skrooge/-/raw/master/appimage.yml",
                ]
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
            if rc == 0:
                if not args.stable:
                    for line in fileinput.input(["appimage.yml"], inplace=True):
                        print(line.replace("ppa-kf5", "beta-kf5"), end="")

                cmd = ["./pkg2appimage", "appimage.yml"]
                self.logfile.write("### " + " ".join(cmd) + "\n")
                rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
                if rc == 0:
                    appimage_file = [
                        os.path.join(self.workdingDir, "out/" + o)
                        for o in os.listdir("./out")
                        if o.endswith(".AppImage")
                    ][0]

                    shutil.move(appimage_file, targteappimage)
        print("#   " + ("DONE" if rc == 0 else "FAILED"))
        return rc

    def buildSnap(self, args):
        print("# Generate Snap")
        os.chdir(self.workdingDir)
        targteappimage = os.path.join(
            self.workdingDir, "skrooge_" + args.version + "_amd64.snap"
        )
        if os.path.exists(targteappimage):
            os.remove(targteappimage)

        if args.fromlocal:
            print("#   Get snapcraft.yaml from local path")
            cmd = ["cp", os.path.join(localPath, "snapcraft.yaml"), "."]
        else:
            cmd = [
                "wget",
                "-c",
                "-nv",
                "https://invent.kde.org/office/skrooge/-/raw/master/snapcraft.yaml",
            ]
        self.logfile.write("### " + " ".join(cmd) + "\n")
        rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
        if rc == 0:
            for line in fileinput.input(["snapcraft.yaml"], inplace=True):
                print(
                    line.replace(
                        'version: "X.X.X"', 'version: "' + args.version + '"'
                    ).replace(
                        "source: XXX",
                        (
                            "source: " + localPath
                            if args.fromlocal
                            else "source: https://invent.kde.org/office/skrooge.git"
                        ),
                    ),
                    end="",
                )

            cmd = ["snapcraft"]
            self.logfile.write("### " + " ".join(cmd) + "\n")
            rc = subprocess.call(cmd, stdout=self.logfile, stderr=self.logfile)
        print("#   " + ("DONE" if rc == 0 else "FAILED"))

        # TO PUBLISH https://docs.snapcraft.io/build-snaps/c
        #      snapcraft push --release=edge skrooge_*.snap
        return rc


def main():
    parser = argparse.ArgumentParser(
        prog="skrooge-release", description="skrooge release maker"
    )

    # Global arguments
    parser.add_argument("--version", required=True, help="The release version")
    parser.add_argument(
        "--fromtar",
        required=False,
        help="To copy an existing tar file instead of building it from git",
    )
    parser.add_argument("--pwd", required=False, help="The password")
    parser.add_argument(
        "--stable",
        action="store_true",
        help="To define this version as a master version",
    )
    parser.add_argument(
        "--publish", action="store_true", help="To publish on launchpad and KDE"
    )
    parser.add_argument(
        "--appimage", action="store_true", help="To generate the appimage only"
    )
    parser.add_argument("--snap", action="store_true", help="To generate the snap only")
    parser.add_argument(
        "--fromlocal",
        action="store_true",
        help="To generate The snap and the appimage from local path ("
        + localPath
        + ")",
    )
    args = parser.parse_args()

    print("#####################")
    print("# Launching release #")
    print("#####################")
    print("# Version         :" + args.version)
    print("# Stable          :" + ("Y" if args.stable else "N"))
    print("# Publish         :" + ("Y" if args.publish else "N"))
    print("# Appimage        :" + ("Y" if args.appimage else "N"))
    print("# Snap            :" + ("Y" if args.snap else "N"))
    if args.fromlocal:
        print("# From local path :" + localPath)
    # Launch the release
    r = Releasor(args)
    rc = 0
    rc = r.prepareWorkingDirectory(args)
    if rc == 0 and not (args.appimage or args.snap):
        rc = r.makeTarFile(args)
    if rc == 0 and not (args.appimage or args.snap):
        rc = r.updateTarFile(args)
    if rc == 0 and not (args.appimage or args.snap):
        rc = r.buildDebian(args)
    if rc == 0 and args.appimage:
        rc = r.buildAppImage(args)
    if rc == 0 and args.snap:
        rc = r.buildSnap(args)
    if rc == 0 and not (args.appimage or args.snap):
        rc = r.publishLaunchpad(args)
    if rc == 0 and not (args.snap):
        rc = r.publishKDE(args)
    print("#####################")
    print("# End of release    #" if rc == 0 else "# FAILURE         #")
    print("#####################")
    return rc


if __name__ == "__main__":
    sys.exit(main())
