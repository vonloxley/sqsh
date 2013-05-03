#!/bin/sh

pod2man --center sqsh-2.2.0 --name sqsh --release 2.2.0 --section 1 sqsh.pod sqsh.1
pod2html --noindex --title sqsh sqsh.pod > sqsh.html
