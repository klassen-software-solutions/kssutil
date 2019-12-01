#!/usr/bin/env python3


"""Program to download, build, and install prerequisites."""

import json
import logging
import os
import subprocess
import sys
import urllib.parse

from typing import Dict, List

def _read_prereqs_file() -> List:
    try:
        with open('prereqs.json', 'r') as file:
            return json.load(file)
    except IOError:
        logging.info("Could not open prereqs.json. Assuming no dependancies.")
        return []

def _change_to_prereqs_directory():
    if not os.path.isdir(".prereqs"):
        logging.debug("Creating .prereqs")
        os.mkdir(".prereqs")
    if not os.path.isdir(PREREQS_DIR):
        logging.debug("Creating %s", PREREQS_DIR)
        os.mkdir(PREREQS_DIR)
    os.chdir(PREREQS_DIR)

def _directory_name_for_prereq(prereq: Dict) -> str:
    dirname = os.path.basename(urllib.parse.urlparse(prereq['package']).path)
    if dirname.endswith('.git'):
        dirname = dirname[:-4]
    return dirname

def _run(command: str, directory: str = None):
    logging.info("running: %s", command)
    subprocess.run("%s" % command, shell=True, check=True, cwd=directory)

def _get_run(command: str):
    res = subprocess.run("%s" % command, shell=True, check=True,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return res.stdout.decode('utf-8').strip()

def _update_repo(dirname: str):
    logging.info("Updating %s", dirname)
    _run('git pull', directory=dirname)

def _clone_repo(url: str):
    logging.info("Cloning %s", url)
    _run('git clone %s' % url)

def _rebuild_and_install(dirname: str):
    logging.info("Rebuilding %s", dirname)
    _run('./configure --prefix=%s' % INSTALL_PREFIX, directory=dirname)
    _run('make', directory=dirname)
    _run('make install', directory=dirname)

def _is_applicable(prereq: Dict) -> bool:
    if 'arch' in prereq:
        if not ARCH in prereq['arch']:
            return False
    return True

def _install_or_update(prereq: Dict):
    if not _is_applicable(prereq):
        logging.info("Skipping %s as not applicable to %s", prereq, ARCH)
        return
    if "package" in prereq:
        logging.info("Found %s", prereq['package'])
        dirname = _directory_name_for_prereq(prereq)
        if os.path.isdir(dirname):
            _update_repo(dirname)
        else:
            _clone_repo(prereq['package'])
        _rebuild_and_install(dirname)
    elif "command" in prereq:
        logging.info("Found %s", prereq['command'])
        _run(prereq['command'], directory=CWD)
    else:
        logging.error("Could not determine how to handle %s", prereq)


def _main():
    logging.basicConfig(level=logging.DEBUG)
    logging.debug("ARCH=%s", ARCH)
    logging.debug("PREREQS_DIR=%s", PREREQS_DIR)
    logging.debug("INSTALL_PREFIX=%s", INSTALL_PREFIX)
    prereqs = _read_prereqs_file()
    logging.info("Found %d prerequisites", len(prereqs))
    if len(prereqs) == 0:
        sys.exit(0)
    _change_to_prereqs_directory()
    for prereq in prereqs:
        _install_or_update(prereq)
    os.chdir(CWD)

if __name__ == '__main__':
    _OS = _get_run('uname -s')
    _MACHINE = _get_run('uname -m')
    CWD = os.getcwd()
    ARCH = "%s-%s" % (_OS, _MACHINE)
    PREREQS_DIR = ".prereqs/%s" % ARCH
    INSTALL_PREFIX = os.environ.get('KSS_INSTALL_PREFIX', '/opt/kss')
    _main()
