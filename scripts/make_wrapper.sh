#!/bin/sh

if [ $# -ne 2 ]; then
	echo "use: make_wrapper.sh <src_file> <dst_file>" 1>&2
	exit 1
fi

prefix=/usr/local
exec_prefix=${prefix}/bin
sybase=/opt/sybase/OCS-16_0

readline_libdir='-L/lib64'
motif_libdir=''
x_libdir='# -L/usr/X11R6/lib'

readline_libdir=`echo $readline_libdir | sed -e 's,^#.*$,,g'`
motif_libdir=`echo $motif_libdir | sed -e 's,^#.*$,,g'`
x_libdir=`echo $x_libdir | sed -e 's,^#.*$,,g'`

LD_LIBRARY_PATH=$sybase/lib
if [ "$readline_libdir" != "" ]; then
	readline_libdir=`echo $readline_libdir | sed -e 's,^-L,,g'`
	LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$readline_libdir"
fi

if [ "$motif_libdir" != "" ]; then
	readline_libdir=`echo $motif_libdir | sed -e 's,^-L,,g'`
	LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$motif_libdir"
fi

if [ "$x_libdir" != "" ]; then
	x_libdir=`echo $x_libdir | sed -e 's,^-L,,g'`
	LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$x_libdir"
fi

sed -e "s,%SYBASE%,$sybase,g" \
    -e "s,%LD_LIBRARY_PATH%,$LD_LIBRARY_PATH,g" \
    -e "s,%BIN_DIR%,$exec_prefix,g" \
    -e "s,%SQSHRC%,,g" \
	 $1 > $2
