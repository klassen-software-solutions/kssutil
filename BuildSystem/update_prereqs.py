#!/usr/bin/env python

import json
import os
import platform
import subprocess


def get_value(map, key, defaultValue):
    try:
        return map[key]
    except KeyError:
        return defaultValue

def get_build_commands(map, operatingSystem):
    try:
        return map["build"]
    except KeyError:
        if operatingSystem == "Linux-x86_64":
            return ["if [ ! -f configure ]; then autoreconf -fvi; fi",
                    "./configure --enable-static=no --prefix=$TARGETDIR",
                    "make",
                    "make install"]
        else:
            return ["if [ ! -f configure ]; then autoreconf -fvi; fi",
                    "./configure --enable-shared=no --prefix=$TARGETDIR",
                    "make",
                    "make install"]

if __name__ == '__main__':
    try:
        with open('prereqs.json', 'r') as file:
            deps = json.load(file)
    except IOError:
        print("Could not open prereqs.json. Assuming no dependancies.")
        exit(0)

    numChanged = 0
    cwd = os.getcwd()
    operatingSystem = platform.system() + "-" + platform.machine()
    dependancyDir = ".prereqs"
    dependancySrcDir = ".prereqs/Sources"
    dependancyOsDir = ".prereqs/" + operatingSystem
    if not os.path.exists(dependancySrcDir):
        print("Creating directory " + dependancySrcDir)
        os.makedirs(dependancySrcDir)
    if not os.path.exists(dependancyOsDir):
        print("Creating directory " + dependancyOsDir)
        os.makedirs(dependancyOsDir)
        if operatingSystem == "Darwin-x86_64":
            os.makedirs(dependancyOsDir + "/share")
            with open(dependancyOsDir + "/share/config.site", "w") as file:
                file.write("CC=clang\n")
                file.write("CXX=clang++\n")

    print("Updating prerequisites...")
    for dep in deps:
        changed = False
        name = dep["name"]
        print("  " + name + "...")

        # Download/update the sources
        if os.path.exists(dependancySrcDir + "/" + name):
            os.chdir(dependancySrcDir + "/" + name)
            cmd = get_value(dep, "update-needed", "git fetch ; git status | grep 'Your branch is up to date'")
            if subprocess.call(cmd, shell=True) == 0:
                print("    no update needed")
            else:
                print("    updating...")
                cmd = get_value(dep, "update", "git pull")
                subprocess.check_call(cmd, shell=True)
                numChanged += 1
                changed = True
            os.chdir(cwd)
        else:
            print("    downloading...")
            os.chdir(dependancySrcDir)
            cmd = dep["download"]
            subprocess.check_call(cmd, shell=True)
            os.chdir(cwd)
            numChanged += 1
            changed = True

        # Build and install
        if changed == True or not os.path.exists(dependancyOsDir + "/include/" + dep["include-test"]):
            print("    building...")
            os.chdir(dependancySrcDir + "/" + name)
            cmds = get_build_commands(dep, operatingSystem)
            tmpEnv = os.environ.copy()
            tmpEnv["TARGETDIR"] = cwd + "/" + dependancyOsDir
            for cmd in cmds:
                print("      " + cmd)
                subprocess.check_call(cmd, env=tmpEnv, shell=True)
            os.chdir(cwd)

    print("Prerequisites updated or changed: " + str(numChanged))
    exit(0)
