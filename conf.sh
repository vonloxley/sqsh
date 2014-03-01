#!/bin/bash
#
# File: conf.sh
# A wrapper file to run configure with adjusted settings.
# Check your configuration and make changes accordingly. When you are finished
# run this script from the prompt: cd ~/sqsh-2.5 && ./conf.sh
# If 'configure' completes successfully, continue with make and make install.
#

# export CC="gcc"

#
# General 64bit compile flag settings (Linux, Solaris)
# Uncomment the LDFLAGS line with -lcrypt if you want to use the \lock command
# in combination with your Unix/Linux password to unlock the sqsh session.
#
#export CPPFLAGS="-DSYB_LP64"
#export CFLAGS="-g -O2 -Wall -m64"
#export LDFLAGS="-m64"
#export LDFLAGS="-m64 -lcrypt"

#
# 64bit compile flags for IBM AIX 5.x, 6.x, 7.x
#
#export CPPFLAGS="-DSYB_LP64"
#export CFLAGS="-g -O2 -Wall -maix64"
#export LDFLAGS="-maix64"

#
# 32bit compile flag settings
# Uncomment these settings if you want to build a 32 bit version of sqsh
#
#export CPPFLAGS=""
#export CFLAGS="-g -O2 -Wall -m32"
#export LDFLAGS="-m32 -lcrypt"

#
# If you want to include X-Windows and optional Motif libraries to create
# result sets in a separate window (\go -x), then remove the # comment sign on
# the configure line. (X and Motif development packages required)
#
./configure --with-readline --with-debug # --with-x --with-motif

