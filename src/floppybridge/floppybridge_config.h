#ifndef FLOPPY_BRIDGE_CONFIG_H
#define FLOPPY_BRIDGE_CONFIG_H
/* floppybridge_config
*
* Copyright 2021 Robert Smith (@RobSmithDev)
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

/* rommgr.h has the following ROM TYPES reserved:  This is so that file does not need to be changed to support upto 16 new interfaces
    ROMTYPE_FLOPYBRDGE0 
    ROMTYPE_FLOPYBRDGE1 
    ROMTYPE_FLOPYBRDGE2 
    ROMTYPE_FLOPYBRDGE3 
    ROMTYPE_FLOPYBRDGE4 
    ROMTYPE_FLOPYBRDGE5 
    ROMTYPE_FLOPYBRDGE6 
    ROMTYPE_FLOPYBRDGE7 
    ROMTYPE_FLOPYBRDGE8 
    ROMTYPE_FLOPYBRDGE9 
    ROMTYPE_FLOPYBRDGEA 
    ROMTYPE_FLOPYBRDGEB 
    ROMTYPE_FLOPYBRDGEC 
    ROMTYPE_FLOPYBRDGED 
    ROMTYPE_FLOPYBRDGEE 
    ROMTYPE_FLOPYBRDGEF 
*/


// DrawBridge aka Arduino floppy reader/writer
#define ROMTYPE_ARDUINOREADER_WRITER			ROMTYPE_FLOPYBRDGE0
#include "ArduinoFloppyBridge.h"

// GreaseWeazle floppy reader/writer
#define ROMTYPE_GREASEWEAZLEREADER_WRITER		ROMTYPE_FLOPYBRDGE1
#include "GreaseWeazleBridge.h"

// Supercard Pro floppy reader/writer
#define ROMTYPE_SUPERCARDPRO_WRITER				ROMTYPE_FLOPYBRDGE2
#include "SupercardProBridge.h"


// Not the nicest way to do this, but here's how they get installed, for now
#define FACTOR_BUILDER(romtype, classname, bridgemode, densitymode, settings) case romtype: bridge = new classname(bridgemode,densitymode,settings); break;


// Build a list of what can be installed
// Options are: setting:  The first bit-field relating to options below
//				canstall: If true the reader is allowed to stall WinUAE to get the data in as required
//				useindex: If true the reader shoudl use the index marker to sync revolutions rather than guessing by other means
//              useturbo: Accelerates how fast the OS reads the data, this will break some copy protections
//              enablehd: Enable HD floppy disk support
#define BRIDGE_FACTORY(settings, bridgemode, densitymode)																									\
		FACTOR_BUILDER(ROMTYPE_ARDUINOREADER_WRITER,ArduinoFloppyDiskBridge,bridgemode,densitymode,settings)												\
		FACTOR_BUILDER(ROMTYPE_GREASEWEAZLEREADER_WRITER,GreaseWeazleDiskBridge,bridgemode,densitymode,settings)											\
		FACTOR_BUILDER(ROMTYPE_SUPERCARDPRO_WRITER,SupercardProDiskBridge,bridgemode,densitymode,settings)													\



// This builds up the config options shown in in WinUAE.  
#define FLOPPY_BRIDGE_CONFIG																																	\
	{	_T("arduinoreaderwriter"), _T("DrawBridge (Arduino R/W)"), _T("RobSmithDev"),																			\
		NULL, NULL, NULL, NULL, ROMTYPE_ARDUINOREADER_WRITER | ROMTYPE_NOT, 0, 0, BOARD_IGNORE, true,															\
		bridge_drive_selection_config, 0, false, EXPANSIONTYPE_FLOPPY,																							\
		0, 0, 0, false, NULL, false, 0, arduino_reader_writer_options },																						\
	{	_T("greaseweazlewriter"), _T("Greaseweazle"), _T("Keir Fraser"),																						\
		NULL, NULL, NULL, NULL, ROMTYPE_GREASEWEAZLEREADER_WRITER | ROMTYPE_NOT, 0, 0, BOARD_IGNORE, true,														\
		bridge_drive_selection_config, 0, false, EXPANSIONTYPE_FLOPPY,																							\
		0, 0, 0, false, NULL, false, 0, greaseweazle_reader_writer_options },																					\
	{	_T("supercardprowriter"), _T("SuperCard Pro"), _T("Jim Drew"),																							\
		NULL, NULL, NULL, NULL, ROMTYPE_SUPERCARDPRO_WRITER | ROMTYPE_NOT, 0, 0, BOARD_IGNORE, true,															\
		bridge_drive_selection_config, 0, false, EXPANSIONTYPE_FLOPPY,																							\
		0, 0, 0, false, NULL, false, 0, supercardpro_reader_writer_options },																					\


#ifdef _WIN32
// And the options to add to the hardware extensions
#define FLOPPY_BRIDGE_CONFIG_OPTIONS																															\
	static const struct expansionboardsettings arduino_reader_writer_options[] = {{																				\
		_T("COM Port\0")  _T("Auto Detect\0")																													\
		_T("COM 1\0") _T("COM 2\0") _T("COM 3\0") _T("COM 4\0") _T("COM 5\0") _T("COM 6\0") _T("COM 7\0") _T("COM 8\0") _T("COM 9\0") _T("COM 10\0") _T("COM 11\0") _T("COM 12\0") _T("COM 13\0") _T("COM 14\0") _T("COM 15\0"), \
		_T("port\0") _T("AUTO\0") _T("COM1\0") _T("COM2\0") _T("COM3\0") _T("COM4\0") _T("COM5\0") _T("COM6\0") _T("COM7\0") _T("COM8\0") _T("COM9\0")  _T("COMA\0") _T("COMB\0") _T("COMC\0") _T("COMD\0") _T("COME\0") _T("COMF\0"), 	\
		true,false,0 }, { NULL }};																																\
	static const struct expansionboardsettings greaseweazle_reader_writer_options[] = {{																		\
		_T("Drive on Cable\0") _T("Drive A\0") _T("Drive B\0") ,																								\
		_T("drive\0") _T("drva\0") _T("drvb\0"),																												\
		true,false,0 }, { NULL }};																																\
	static const struct expansionboardsettings supercardpro_reader_writer_options[] = {{																		\
		_T("Drive on Cable\0") _T("Drive A\0") _T("Drive B\0") ,																								\
		_T("drive\0") _T("drva\0") _T("drvb\0"),																												\
		true,false,0 }, { NULL }};																																\

#else
// And the options to add to the hardware extensions
#define FLOPPY_BRIDGE_CONFIG_OPTIONS																															\
	static const struct expansionboardsettings arduino_reader_writer_options[] = {{																				\
		_T("COM Port\0")  _T("Auto Detect\0"),																													\
		_T("port\0") _T("AUTO\0"),																																\
		true,false,0 }, { NULL }};																																\
	static const struct expansionboardsettings greaseweazle_reader_writer_options[] = {{																		\
		_T("Drive on Cable\0") _T("Drive A\0") _T("Drive B\0") ,																								\
		_T("drive\0") _T("drva\0") _T("drvb\0"),																												\
		true,false,0 }, { NULL }};																																\
	static const struct expansionboardsettings supercardpro_reader_writer_options[] = {{																		\
		_T("Drive on Cable\0") _T("Drive A\0") _T("Drive B\0") ,																								\
		_T("drive\0") _T("drva\0") _T("drvb\0"),																												\
		true,false,0 }, { NULL }};																																\

#endif


#endif