#!/bin/bash
#
# File: conf.sh
# A wrapper file to run configure with the correct settings.
# Check your configuration and adjust accordingly. When you are finished
# run this script from the prompt: ~/sqsh-2.2.0$ ./conf.sh
# If configure completes successfully continue with make and make install.
#

export CC="gcc"

#
# 64bit compile flag settings
# Uncomment the LDFLAGS line with -lcrypt if you want to use the \lock command
# in combination with your Unix/Linux password to unlock the sqsh session.
#
export CFLAGS="-g -O2 -m64"
export LDFLAGS="-m64"
#export LDFLAGS="-m64 -lcrypt"
export CPPFLAGS="-DSYB_LP64"

#
# 32bit compile flag settings
# Uncomment these settings if you want to build a 32 bit version of sqsh
#
#export CFLAGS="-g -O2"
#export LDFLAGS="-lcrypt"
#export CPPFLAGS=""

#
# If you want to include X-Windows and optional Motif libraries to create
# result sets in a separate window (\go -x), then remove the # comment sign on
# the configure line.
#
./configure --with-readline # --with-x --with-motif

