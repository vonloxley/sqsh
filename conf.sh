#!/bin/bash
#
# File: conf.sh
# A wrapper file to run configure with appropriate settings.
# Check your configuration and make changes accordingly. When you are finished
# run this script from the prompt: cd ~/sqsh-2.5 && ./conf.sh
# If 'configure' completes successfully, continue with
#
# make clean
# make
# make install
# make install.man
#

# Simple 64 bits compilation setup.
# Note that it is not needed anymore in sqsh-2.5 to explicitly set the -DSYB_LP64
# precompiler flag for 64 bit compilations. "configure" will set this flag for
# 64 bit compilations automatically in the src/Makefile.
#export CC="gcc"
#export CPPFLAGS=""
# export CPPFLAGS="-DSYB_LP64"
#export CFLAGS="-g -O2 -Wall -m64"
#export LDFLAGS="-s -m64"
#./configure

#
# Below are some additional examples
#
# General 32 bit compile flag settings
#
# export CPPFLAGS=""
# export CFLAGS="-g -O2 -Wall -m32"
# export LDFLAGS="-s -m32"
# ./configure

#
# If you want to include X-Windows and optional Motif libraries to create
# result sets in a separate window (\go -x), then remove the # comment sign on
# the configure line. (X and Motif development packages required)
#
# ./configure --with-readline --with-debug # --with-x --with-motif

#
# 64 bit compile flags for IBM AIX 5.x, 6.x, 7.x
#
# export CPPFLAGS="-DSYB_LP64" # No need to set this explicitly in sqsh-2.5
# export CFLAGS="-g -O2 -Wall -maix64"
# export LDFLAGS="-maix64"

#
# An example with some paranoia compiler settings
#
CC="gcc" \
CFLAGS="-g -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fomit-frame-pointer \
-fexceptions -fstack-protector --param=ssp-buffer-size=4 \
-mtune=generic -fasynchronous-unwind-tables" \
LDFLAGS="" \
./configure --prefix=/usr/local --sysconfdir=/usr/local/etc --with-readline \
 --with-debug --without-devlib --without-static --with-gcc --with-x --with-motif

