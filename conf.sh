#!/bin/bash
#
# File: conf.sh
# A wrapper file to run configure with adjusted settings.
# Check your configuration and make changes accordingly. When you are finished
# run this script from the prompt: cd ~/sqsh-2.5 && ./conf.sh
# If 'configure' completes successfully, continue with
# "make clean && make && make install".
#

# export CC="gcc"

#
# General 32 bit compile flag settings
# Uncomment these settings if you want to build a 32 bit version of sqsh
#
# export CPPFLAGS=""
# export CFLAGS="-g -O2 -Wall -m32"
# export LDFLAGS="-m32"

#
# If you want to include X-Windows and optional Motif libraries to create
# result sets in a separate window (\go -x), then remove the # comment sign on
# the configure line. (X and Motif development packages required)
#
#./configure --with-readline --with-debug # --with-x --with-motif

# CC="gcc" \
# CFLAGS="-g -O2 -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fomit-frame-pointer \
# -fexceptions -fstack-protector --param=ssp-buffer-size=4 -m32 \
# -march=i686 -mtune=generic -fasynchronous-unwind-tables" \
# LDFLAGS="-s -m32" \
# ./configure --prefix=/usr/local --sysconfdir=/usr/local/etc --with-readline \
#   --with-debug --with-x --with-motif

#
# General 64 bit compile flag settings (Linux, Solaris)
#
# export CPPFLAGS="-DSYB_LP64" # Not needed anymore in sqsh-2.5
# export CFLAGS="-g -O2 -Wall -m64"
# export LDFLAGS="-m64"

#
# 64 bit compile flags for IBM AIX 5.x, 6.x, 7.x
#
# export CPPFLAGS="-DSYB_LP64"
# export CFLAGS="-g -O2 -Wall -maix64"
# export LDFLAGS="-maix64"

CC="gcc" \
CFLAGS="-g -O2 -pipe -Wall -Wp,-D_FORTIFY_SOURCE=2 -fomit-frame-pointer \
-fexceptions -fstack-protector --param=ssp-buffer-size=4 -m64 \
-mtune=generic -fasynchronous-unwind-tables" \
LDFLAGS="-s -m64" \
./configure --prefix=/usr/local --sysconfdir=/usr/local/etc --with-readline \
  --without-debug --without-devlib --without-static --with-gcc # --with-x --with-motif

