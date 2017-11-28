#!/bin/sh


LOCAL_PATH=`dirname $0`
LOCAL_PATH=`cd $LOCAL_PATH && pwd`

JOBS=4

ln -sf libsdl_ttf.so $LOCAL_PATH/../../../obj/local/$1/libSDL_ttf.so
ln -sf libguichan.so $LOCAL_PATH/../../../obj/local/$1/libguichan_sdl.so

../setEnvironment-$1.sh sh -c "make -j$JOBS" && mv -f uae4arm libapplication-$1.so

