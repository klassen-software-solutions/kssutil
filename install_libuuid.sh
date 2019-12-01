#!/bin/sh

#  install_libuuid.sh
#  kssutil
#
#  Created by Steven W. Klassen on 2019-12-01.
#  Copyright © 2019 Klassen Software Solutions. All rights reserved.

# Preparation

set -e
arch="$(uname -s)-$(uname -m)"
installPrefix=${KSS_INSTALL_PREFIX:=/opt/kss}
prereqsDir=".prereqs/$arch"

echo "arch=$arch"
echo "installPrefix=$installPrefix"
echo "prereqsDir=$prereqsDir"

cwd=$(pwd)

if [ ! -d $prereqsDir ]; then
    mkdir -p $prereqsDir
fi
cd $prereqsDir

# Clone or update the repository

if [ -d libuuid-code ]; then
    cd libuuid-code
    git pull
else
    git clone https://git.code.sf.net/p/libuuid/code libuuid-code
    cd libuuid-code
    libtoolize
    aclocal
    autoheader
    automake --add-missing
fi

# Build and install

autoreconf
./configure --prefix=$installPrefix --enable-static=no
make
make install

cd $cwd
pwd
