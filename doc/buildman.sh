#!/bin/sh

pod2man --center sqsh-2.4 --name sqsh --release 2.4 --section 1 sqsh.pod sqsh.1
pod2html --noindex --title sqsh sqsh.pod > sqsh.html
