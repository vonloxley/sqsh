#!/bin/sh
#
# $Id$
# Shell script to build .a libraries from Sybase's .lib files.
# Original provided by Helmut Ruholl.
#
# NOTE: Does NOT work with 12.5.1 libraries because their symbols
# are not "decorated". See the MS documentation on the __stdcall
# function call convention for details on symbol "decoration".
#
# In my experience the .a files in this directory can be used instead.

set -x
here=`pwd`

cd $SYBASE/OCS-12_5/lib
for i in libblk libcs libct; do
    echo "EXPORTS" >${i}.def
    nm ${i}.lib | grep 'T _' | sed 's/.* T _//' >>${i}.def
    dlltool --dllname ${i}.dll --def ${i}.def --output-lib ${i}.a -k
done
