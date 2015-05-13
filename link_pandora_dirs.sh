#!/bin/bash

cd ./src

# Link to our config.h
if ! [ -e "config.h" ]; then
  ln -s od-pandora/config.h config.h
fi

# Link to our sysconfig.h
if ! [ -e "sysconfig.h" ]; then
  ln -s od-pandora/sysconfig.h sysconfig.h
fi

# Link to our target.h
if ! [ -e "target.h" ]; then
  ln -s od-pandora/target.h target.h
fi

# Link od-pandora to osdep
if ! [ -d "osdep" ]; then
  ln -s od-pandora osdep
fi

# We use SDL-threads, so link td-sdl to threaddep
if ! [ -d "threaddep" ]; then
  ln -s td-sdl threaddep
fi

# Link md-pandora to machdep
if ! [ -d "machdep" ]; then
  ln -s md-pandora machdep
fi

# Link od-sound to sounddep
if ! [ -d "sounddep" ]; then
  ln -s sd-pandora sounddep
fi

cd ..
