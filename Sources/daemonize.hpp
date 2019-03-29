//
//  daemonize.hpp
//  kssutil
//
//  Created by Steven W. Klassen on 2013-03-29.
//  Copyright (c) 2013 Klassen Software Solutions. All rights reserved.
//  Licensing follows the MIT License.
//

#ifndef kssutil_daemonize_hpp
#define kssutil_daemonize_hpp

#include <string>

namespace kss { namespace util { namespace process {

    /*!
     "Daemonize" a process. The steps involved in this are based on discussions
     found at stackoverflow.com. It will perform the following:

     1. fork the process and exit the parent to ensure new process is not a group leader
     2. setsid to become a process group leader
     3. fork again to ensure we can never regain a controlling terminal
     4. change directory to "/"
     5. umask(0) to ensure we don't inherit any permissions
     6. ensure all file descriptors except for stdin, stdout, and stderr are closed
     7. redirect the standard file descriptors as follows
         stdin -> /dev/null
         stdout -> /dev/null
         stderr -> /dev/console (or /dev/null if the user cannot write to /dev/console)

     The function will return 0 if all of this works and a standard error code
     (i.e. one that may be found in errno) if anything fails. It also calls "syslog"
     along the way with details of anything that goes wrong.

     The second form of this function will finish by setting the uid and gid of the process
     to that of the given user. This is useful if you need to start as root but wish to
     hand the process over to another user. In addition to the errors that may be generated
     by the underlying system calls, this may return ENOENT if the given username does
     not exist.

     @param user if set will also set the uid and gid to that of the given user. This is
        useful if you need to start as root but wish to hand the process over to
        another user.
     @throws system_error if there is a problem with an underlying system call
     @throws system_error with a code of ENOENT if the username does not exist.
     */
    void daemonize(const std::string& user = "");

} } }

#endif
