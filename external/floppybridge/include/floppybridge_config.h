#ifndef FLOPPY_BRIDGE_CONFIG_H
#define FLOPPY_BRIDGE_CONFIG_H
/* floppybridge_config
*
* Copyright (C) 2021-2022 Robert Smith (@RobSmithDev)
* https://amiga.robsmithdev.co.uk
*
* This library defines which interfaces are enabled within *UAE
*
* This file, along with currently active and supported interfaces
* are maintained from by GitHub repo at
* https://github.com/RobSmithDev/FloppyDriveBridge
*
* This is free and unencumbered released into the public domain.
* See the file COPYING for more details, or visit <http://unlicense.org>.
*
*/

// DrawBridge aka Arduino floppy reader/writer
#define ROMTYPE_ARDUINOREADER_WRITER			ROMTYPE_FLOPYBRDGE0
#include "ArduinoFloppyBridge.h"

// GreaseWeazle floppy reader/writer
#define ROMTYPE_GREASEWEAZLEREADER_WRITER		ROMTYPE_FLOPYBRDGE1
#include "GreaseWeazleBridge.h"

// Supercard Pro floppy reader/writer
#define ROMTYPE_SUPERCARDPRO_WRITER				ROMTYPE_FLOPYBRDGE2
#include "SuperCardProBridge.h"

#endif
