#! /bin/sh
# Script to convert an Amiga executable named filesys into a series of
# dw(...) statements.
# This assumes that the first four lines only contain hunk information.
#
# Befehle auf der Amigaseite mit HD0:
# DH0:
# cd asm
# PhxAss/PhxAss filesys.asm TO fs.out OPT 0
# blink FROM fs.out TO filesys
# delete fs.out
#
# That is what you get if you assemble/link with a68k/blink
od -v -t xC -w8 filesys |tail -n +5 | sed -e "s,^.......,," -e "s,[0123456789abcdefABCDEF][0123456789abcdefABCDEF],db(0x&);,g" > filesys_bootrom.cpp
