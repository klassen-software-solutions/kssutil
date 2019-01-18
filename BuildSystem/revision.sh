#!/bin/sh

newver=`git describe --tags --dirty=M 2>/dev/null`
if [ $? -ne 0 ]; then
    newver="0.0.0"
fi

# allow override by environment variable
if [ x"$REVISION" != "x" ]; then
	newver=$REVISION
fi

if [ -f REVISION ]; then
    oldver=`cat REVISION`
    if [ "$oldver" != "$newver" ]; then
        echo "$newver" > REVISION
    fi
else
    echo "$newver" > REVISION
fi

echo "$newver"
