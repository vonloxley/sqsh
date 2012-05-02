#!/bin/bash

export CC="gcc"

#
# 64bit compile flag settings
#
export CFLAGS="-g -O2 -m64"
export LDFLAGS="-m64"
export CPPFLAGS="-DSYB_LP64"

#
# 32bit compile flag settings
#
#export CFLAGS="-g -O2"
#export LDFLAGS=""
#export CPPFLAGS=""

./configure --with-readline # --with-x --with-motif

