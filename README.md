# kssutil
C++ General Utility Library

## Description

This library provides general C++ utilities that are either unrelated or loosely related to each other.
It includes code for dealing with containers, for creating custom iterators, for creating and parsing
program options (command lines), for obtaining run time type information, handling strings,
dealing with time, and many other things.

You should note that these utilities do not compete with or replace the standard C++ library. Rather
they built on it, adding things that seemed to be missing, or making things easier to work with.


[API Documentation](http://www.kss.cc/apis/kssutil/docs/index.html) 

## What has changed in V14?

Version 14 is a complete, non-back-compatible rewrite of the library. It drops all the older "C" code,
as well as many of the things that are now covered by the standard C++ libraries. It has also moved out 
things related to io and threading into their own, separate, libraries, and it has been rewritten to follow a 
consistent coding standard across the entire library.

## Prerequisites

This library has the following prerequisites. They will be installed/updated by running `make prereqs`.
By default they will be installed into `/opt/kss` but you can change that by setting the environment
variable `KSS_INSTALL_PREFIX`. Note that your user must have permission to create/update items
in that directory. Alternately you may install each manually. Note that the Xcode project assumes they
are installed in `/opt/kss`.

* ksscontract - https://github.com/klassen-software-solutions/ksscontract.git
* libuuid (Linux) - `sudo apt install uuid-dev`


## Installing the Library

To build and install this library, run the following commands. Of course you can set the prefix to
whatever you want for your installation. But `/opt/kss` is what the Xcode projects are configured
to use. (If you leave out `--prefix`, it will default to `/usr/local` to remain consistent with most
"tarball" installations.)

```
./configure --prefix=/opt/kss
make
make check
make install
```


## Contributing

If you wish to make changes to this library that you believe will be useful to others, you can
contribute to the project. If you do, there are a number of policies you should follow:

* Check the issues to see if there are already similar bug reports or enhancements.
* Feel free to add bug reports and enhancements at any time.
* Don't work on things that are not part of an approved project.
* Don't create projects - they are only created the by owner.
* Projects are created based on conversations on the wiki.
* Feel free to initiate or join conversations on the wiki.
* Follow our [C++ Coding Standards](https://www.kss.cc/standards/c-.html).
