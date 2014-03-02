#!/bin/sh

pod2man --center sqsh-2.5 --name sqsh --release 2.5 --section 1 sqsh.pod sqsh.1
pod2html --noindex --title sqsh sqsh.pod > sqsh.html
