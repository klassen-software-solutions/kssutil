#!/usr/bin/env python3

"""Program to run the Xcode static analyzer and interpret the results."""

import logging
import subprocess
import sys

def _run(command: str):
    logging.info("running: %s", command)
    subprocess.run("%s" % command, shell=True, check=True)

def _process(command: str):
    logging.info("processing: %s", command)
    res = subprocess.Popen("%s" % command, shell=True,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return res.stdout

def _main():
    logging.basicConfig(level=logging.DEBUG)
    warning_count = 0
    _run("rm -rf Build")
    for line in _process("xcodebuild -alltargets -configuration Debug -quiet analyze"):
        line = line.decode('utf-8').strip()
        if "warning:" in line:
            print("%s" % line)
            warning_count += 1

    if warning_count > 0:
        logging.info("Found %d warnings", warning_count)
        sys.exit(warning_count)

if __name__ == '__main__':
    _main()
