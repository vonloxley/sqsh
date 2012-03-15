#!/bin/sh

pod2man --center sqsh-2.1.8 --name sqsh --release 2.1.8 --section 1 sqsh.pod sqsh.1

#pod2man sqsh.pod > sqsh.1

pod2html --noindex --title sqsh sqsh.pod > sqsh.html
