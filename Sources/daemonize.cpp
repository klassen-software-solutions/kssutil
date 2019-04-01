//
//  daemonize.cpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-03-29.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <system_error>

#include <pwd.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "daemonize.hpp"

using namespace std;

namespace {
    void daemonizeIt() {
        // 1. Fork and exit the parent process, this returns control to the command line or the
        //  shell invoking your program. It is required so that the new process is guaranteed
        //  not to be a process group leader.
        errno = 0;
        pid_t pid = fork();
        if (pid < 0) {
            throw system_error(errno, system_category(), "fork");
        }

        if (pid > 0) {
            exit(0);
        }

        // 2. setsid to become the process group and session group leader. This ensures that
        //  we have no controlling terminal.
        pid_t sid = setsid();
        if (sid == -1) {
            throw system_error(errno, system_category(), "setsid");
        }

        // 3. Fork again and exit the parent to ensure that we, as a non-session group leader,
        //  can never regain a controlling terminal.
        pid = fork();
        if (pid < 0) {
            throw system_error(errno, system_category(), "fork (2)");
        }

        if (pid > 0) {
            exit(0);
        }

        // 4. Change to the root directory.
        if (chdir("/") == -1) {
            throw system_error(errno, system_category(), "chdir");
        }

        // 5. Set the umask to ensure we don't inherit permissions.
        umask(0);

        // 6. Close all possible file descriptors, except for the std ones. (0, 1, and 2).
        //  Note that the (int)maxfd should be a safe cast since maxfd is the OSes maximum
        //  file descriptor and file descriptors in POSIX are ints. maxfd itself cannot be
        //  an int (or could be cast down to one sooner) as sysconf can return large values
        //  depending on what is requested in its argument.
        long maxfd = sysconf(_SC_OPEN_MAX);
        if (maxfd == -1L) {
            throw system_error(errno, system_category(), "sysconf");
        }

        for (int fd = 3; fd < (int)maxfd; ++fd) {
            close(fd);
        }

        // 7. Redirect the standard input, output, and error devices to safe locations.
        if (!freopen("/dev/null", "r", stdin)) {
            throw system_error(errno, system_category(), "freopen: stdin");
        }

        if (!freopen("/dev/null", "a", stdout)) {
            throw system_error(errno, system_category(), "freopen: stdout");
        }

        if (!freopen("/dev/console", "a", stderr)) {
            syslog(LOG_WARNING, "Could not redirect stderr to /dev/console, using /dev/null instead");
            if (!freopen("/dev/null", "a", stderr)) {
                throw system_error(errno, system_category(), "freopen: stderr");
            }
        }
    }

    void changeUser(const string& user) {
        // Determine how big a buffer we need.
        long bufsize = 0L;
        if ((bufsize = sysconf(_SC_GETPW_R_SIZE_MAX)) == -1L) {
            throw system_error(errno, system_category(), "sysconf");
        }

        // Obtain the uid and gid for the user.
        char buffer[bufsize];
        struct passwd pwd, *result = NULL;
        int err = getpwnam_r(user.c_str(), &pwd, buffer, (size_t)bufsize, &result);
        if (err) {
            throw system_error(err, system_category(), "getpwnam_r");
        }
        if (!result) {
            throw system_error(ENOENT, system_category(), user + " not found");
        }

        // Change the user.
        if (setuid(pwd.pw_uid) == -1) {
            throw system_error(errno, system_category(), "setuid");
        }
        if (setgid(pwd.pw_gid) == -1) {
            throw system_error(errno, system_category(), "setgid");
        }
    }
}

void kss::util::process::daemonize(const string& user) {
    daemonizeIt();
    if (!user.empty()) {
        changeUser(user);
    }
}
