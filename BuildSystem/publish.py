#!/usr/bin/env python3

"""Program to publish a directory to an ftp target."""

import argparse
import ftplib
import logging
import os
import urllib.parse


def _parse_and_validate_url(urlstr):
    url = urllib.parse.urlparse(urlstr, allow_fragments=False)
    if url.scheme != 'ftps':
        raise RuntimeError("Only ftps is presently supported")
    if url.params != '':
        raise RuntimeError("Parameters are not presently supported")
    if ':' in url.netloc:
        raise RuntimeError("Port changing is not presently supported")
    return url

def _parse_command_line():
    parser = argparse.ArgumentParser()
    parser.add_argument('path', help='Directory to upload')
    parser.add_argument('--url',
                        required=True,
                        help='Target URL (must start with ftps:)')
    parser.add_argument('--user', help='User name for authentication')
    parser.add_argument('--password', help='Password for authentication')
    parser.add_argument('--verbose', action='store_true', help='Show debugging information')
    args = parser.parse_args()
    if not os.path.isdir(args.path):
        raise RuntimeError("The path '%s' is not a directory" % args.path)
    if args.path == '.' or args.path == '..':
        raise RuntimeError("The path must not be the current or parent directory")
    return args

def _count_files(path: str) -> int:
    num_files = 0
    num_dirs = 1
    for root, dirs, files in os.walk(path):
        if os.path.basename(root).startswith('.'):
            logging.debug("  skipping hidden directory %s", root)
            dirs[:] = []
            continue
        num_dirs += len(dirs)
        num_files += len(files)
        logging.debug("  %s: %d dirs and %d files", root, len(dirs), len(files))
    logging.debug("Found %d files in %d directories", num_files, num_dirs)
    return num_files

def _backup(ftp, dir: str) -> bool:
    backupdir = "%s.bak" % dir
    logging.debug("Backing up %s to %s", dir, backupdir)
    _remove_path(ftp, backupdir)
    try:
        ftp.rename(dir, backupdir)
    except ftplib.error_perm as ex:
        logging.debug("..could not rename, ex=%s", ex)

def _remove_path(ftp, dir: str):
    if _dir_exists(ftp, dir):
        logging.debug("Removing existing directory %s", dir)
        _remove_dir(ftp, ".", dir)

def _dir_exists(ftp, dir: str) -> bool:
    generator = ftp.mlsd()
    try:
        while True:
            (name, details) = next(generator)
            if name == dir and details['type'] == 'dir':
                return True
    except StopIteration:
        pass
    return False

def _remove_dir(ftp, path: str, dir: str):
    generator = ftp.mlsd(path + "/" + dir)
    try:
        while True:
            (name, details) = next(generator)
            if details['type'] == 'file':
                logging.debug("  %s/%s/%s" % (path, dir, name))
                ftp.delete("%s/%s/%s" % (path, dir, name))
            elif details['type'] == 'dir':
                _remove_dir(ftp, path + "/" + dir, name)
            else:
                logging.debug("  %s/%s/%s (skipping)" % (path, dir, name))
    except StopIteration:
        pass
    logging.debug("  %s/%s" % (path, dir))
    ftp.rmd("%s/%s" % (path, dir))

def _upload(ftp, file_system_root: str, ftp_server_root: str, filename: str):
    file_system_filename = '%s/%s' % (file_system_root, filename)
    ftp_server_filename = '%s/%s' % (ftp_server_root, filename)
    logging.debug("Uploading %s to %s", file_system_filename, ftp_server_filename)
    with open(file_system_filename, 'rb') as fp:
        ftp.storbinary('STOR %s' % ftp_server_filename, fp)

def _display_percent_done(num_copied: int, total_num_files: int, prev_percent_done: int) -> int:
    perc_done = int(round(num_copied / total_num_files * 100))
    if perc_done > prev_percent_done:
        prev_percent_done = perc_done
        logging.info(" ...%d%% done, (%d of %d) files", perc_done, num_copied, total_num_files)
    return perc_done

def _copy_files(ftp, path: str, total_num_files: int):
    num_copied = 0
    prev_percent_done = 0
    prefix, basedir = os.path.split(path)
    prefix += "/"
    logging.debug("Path prefix: %s", prefix)
    _backup(ftp, basedir)
    ftp.mkd(basedir)
    for root, dirs, files in os.walk(path):
        if os.path.basename(root).startswith('.'):
            dirs[:] = []
            continue
        rootdir = _remove_prefix(root, prefix)
        for subdir in dirs:
            dir = "%s/%s" % (rootdir, subdir)
            logging.debug("Creating dir %s", dir)
            ftp.mkd(dir)
        for file in files:
            _upload(ftp, root, rootdir, file)
            num_copied += 1
            prev_percent_done = _display_percent_done(num_copied, total_num_files, prev_percent_done)

def _publish(url, args):
    published_dir = os.path.basename(args.path)
    if published_dir == '':
        raise RuntimeError("Could not determine the basename of %s" % args.path)
    logging.info("Publishing %s to %s as %s" % (args.path, args.url, published_dir))
    ftp = ftplib.FTP_TLS(host=url.netloc,
                         user='' if args.user is None else args.user,
                         passwd='' if args.password is None else args.password)
    ftp.cwd(url.path)
    logging.debug("pwd (on server): %s" % ftp.pwd())
    num_files = _count_files(args.path)
    _copy_files(ftp, args.path, num_files)

def _remove_prefix(text, prefix):
    if text.startswith(prefix):
        return text[len(prefix):]
    return text

def _main():
    args = _parse_command_line()
    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO)
    url = _parse_and_validate_url(args.url)
    _publish(url, args)

if __name__ == '__main__':
    _main()
