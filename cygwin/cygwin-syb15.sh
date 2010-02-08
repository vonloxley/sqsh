#!/bin/sh
#
# $Id: cygwin.sh,v 1.1 2004/11/24 12:33:00 mpeppler Exp $
# Shell script to build .a libraries from Sybase's .lib files.
# Original provided by Helmut Ruholl.
#

set +x
export SYBASE=/cygdrive/c/Sybase
export OCSLIB=${SYBASE}/${SYBASE_OCS}/lib
for i in libsybblk libsybcs libsybct ; do
    echo "EXPORTS" >${i}.def
    nm ${OCSLIB}/${i}.lib | grep 'T _' | sed 's/.* T _//' >>${i}.def
    dlltool --dllname ${i}.dll --def ${i}.def --output-lib ${i}.a -k
done
rm -f *.def

