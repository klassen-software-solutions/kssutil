#!/usr/bin/env python3


"""Scan the dependancies for licenses and produce a report.

   Note that this program assumes that ninka has been installed and will be used to
   identify the licenses.
"""

import bisect
import json
import logging
import os
import subprocess

from typing import Dict


_PREREQS_DIRECTORY = '.prereqs/Darwin-x86_64'
_PREREQS_LICENSE_FILE = 'Dependancies/prereqs-licenses.json'
_SPDX_LICENSES = 'BuildSystem/spdx-licenses.json'

def _get_run(command: str, directory: str = None):
    res = subprocess.run("%s" % command, shell=True, check=True, cwd=directory,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    return res.stdout.decode('utf-8').strip()

def _get_prereqs():
    prereqs = []
    if not os.path.isdir(_PREREQS_DIRECTORY):
        logging.warning("No prerequisites directory found, ensure they have been built.")
    else:
        for entry in os.listdir(_PREREQS_DIRECTORY):
            if os.path.isdir(_PREREQS_DIRECTORY + '/' + entry):
                prereqs.append(entry)
    return prereqs

def _get_license_filename(dir: str):
    for entry in os.listdir(dir):
        if entry.upper().startswith('LICENSE'):
            return entry
    return None

def _guess_license_type(filename: str):
    return _get_run("ninka %s | cut -d ';' -f2" % filename)

def _read_spdx():
    with open(_SPDX_LICENSES) as infile:
        data = json.load(infile)
        spdx = {}
        for lic in data['licenses']:
            spdx[lic['licenseId']] = lic
        return spdx

def _examine_license_file(filename: str, spdx: Dict) -> Dict:
    logging.debug("   examining license file %s" % filename)
    licensetype = _guess_license_type(filename)
    license = { 'moduleLicense': licensetype }
    if licensetype.startswith('spdx'):
        key = licensetype[4:]
        entry = spdx.get(key, None)
        if entry is not None:
            license['moduleLicense'] = entry['name']
            license['x-spdxId'] = key
            license['x-isOsiApproved'] = entry['isOsiApproved']
            urls = entry['seeAlso']
            if urls:
                license['moduleLicenseUrl'] = urls[0]
    logging.debug("   identified license as %s" % license['moduleLicense'])
    return license

def _add_module_details(prereq: str, license: Dict):
    license['moduleName'] = prereq
    moduledir = _PREREQS_DIRECTORY + "/" + prereq
    if os.path.isfile(moduledir + "/.git/config"):
        license['moduleUrl'] = _get_run("git config --get remote.origin.url",
                                        directory=moduledir)

def _get_module_name() -> str:
    return os.path.basename(os.getcwd())

def _write_licenses(licenses: Dict):
    data = {'dependencies': sorted(licenses.values(), key=lambda x: x['moduleName'])}
    with open(_PREREQS_LICENSE_FILE, 'w') as outfile:
        json.dump(data, outfile, indent=4, sort_keys=True)

def _create_license_entry(prereq: str, spdx) -> Dict:
    dir = _PREREQS_DIRECTORY + '/' + prereq
    logging.debug("   searching in %s" % dir)
    license = { 'moduleLicense': 'UNKNOWN' }
    filename = _get_license_filename(dir)
    if filename is None:
        logging.warning("Could not find a license file in %s, assuming 'UNKNOWN'" % dir)
    else:
        fn = dir + "/" + filename
        license = _examine_license_file(fn, spdx)
    _add_module_details(prereq, license)
    return license

def _merge_prereq_dependancies(licenses: Dict, prereq: str):
    filename = _PREREQS_DIRECTORY + '/' + prereq + '/' + _PREREQS_LICENSE_FILE
    if os.path.isfile(filename):
        logging.debug("Adding prereqs from %s", prereq)
        with open(filename) as infile:
            data = json.load(infile)
            for entry in data['dependencies']:
                key = entry['moduleName']
                if key in licenses:
                    bisect.insort(licenses[key]['x-usedBy'], key)
                else:
                    licenses[key] = entry

def _add_manually_edited_licenses(licenses: Dict):
    if os.path.isfile(_PREREQS_LICENSE_FILE):
        logging.debug("Keeping manually edited licenses")
        with open(_PREREQS_LICENSE_FILE) as infile:
            data = json.load(infile)
            for entry in data['dependencies']:
                key = entry['moduleName']
                ismanual = entry.get('x-manuallyEdited', False)
                if ismanual:
                    licenses[key] = entry

def _main():
    logging.basicConfig(level=logging.DEBUG)
    spdx = _read_spdx()
    modulename = _get_module_name()
    licenses = {}
    _add_manually_edited_licenses(licenses)
    prereqs = _get_prereqs()
    for prereq in prereqs:
        _merge_prereq_dependancies(licenses, prereq)
    for prereq in prereqs:
        logging.debug("Examining %s...", prereq)
        if prereq in licenses:
            logging.debug("   adding %s to usedBy of %s", modulename, prereq)
            bisect.insort(licenses[prereq]['x-usedBy'], modulename)
        else:
            license = _create_license_entry(prereq, spdx)
            license['x-usedBy'] = [ modulename ]
            licenses[license['moduleName']] = license
    _write_licenses(licenses)

if __name__ == '__main__':
    _main()
