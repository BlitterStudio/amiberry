#!/bin/bash
g++ `sdl2-config --cflags` -I./ -I./include -I./osdep -I./threaddep -I./archivers -DAMIBERRY -D_FILE_OFFSET_BITS=64 -lSDL2 -fuse-ld=gold -o gencpu cpudefs.cpp gencpu.cpp readcpu.cpp ./osdep/charset.cpp
