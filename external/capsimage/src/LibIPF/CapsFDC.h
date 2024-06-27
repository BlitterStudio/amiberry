#ifndef CAPSFDC_H
#define CAPSFDC_H

#include "ComLib.h"

// drive defaults
#define CAPSDRIVE_35DD_RPM 300
#define CAPSDRIVE_35DD_HST 83

// disk & drive attributes
// 0: true disk inserted (if not inserted it is write protected)
// 1: true disk write protected
// 2: true motor on
// 3: true single sided drive
#define CAPSDRIVE_DA_IN DF_0
#define CAPSDRIVE_DA_WP DF_1
#define CAPSDRIVE_DA_MO DF_2
#define CAPSDRIVE_DA_SS DF_3

// index pulse only available if disk is inserted and motor is running
#define CAPSDRIVE_DA_IPMASK (CAPSDRIVE_DA_IN|CAPSDRIVE_DA_MO)

// fdc output lines
// 0: drq line state
// 1: intrq line state
// 2: forced interrupt, internal
// 3: motor line state
// 4: direction line state
// 5: interrupt on index pulse
// 6: set drq, internal
#define CAPSFDC_LO_DRQ    DF_0
#define CAPSFDC_LO_INTRQ  DF_1
#define CAPSFDC_LO_INTFRC DF_2
#define CAPSFDC_LO_MO     DF_3
#define CAPSFDC_LO_DIRC   DF_4
#define CAPSFDC_LO_INTIP  DF_5
#define CAPSFDC_LO_DRQSET DF_6

// fdc status lines
// 0: busy
// 1: index pulse/drq
// 2: track0/lost data
// 3: crc error
// 4: record not found
// 5: spin-up/record type
// 6: write protect
// 7: motor on
#define CAPSFDC_SR_BUSY   DF_0
#define CAPSFDC_SR_IP_DRQ DF_1
#define CAPSFDC_SR_TR0_LD DF_2
#define CAPSFDC_SR_CRCERR DF_3
#define CAPSFDC_SR_RNF    DF_4
#define CAPSFDC_SR_SU_RT  DF_5
#define CAPSFDC_SR_WP     DF_6
#define CAPSFDC_SR_MO     DF_7

// flags to clear before new command
#define CAPSFDC_SR_NCCLR (CAPSFDC_SR_SU_RT|CAPSFDC_SR_RNF|CAPSFDC_SR_CRCERR|CAPSFDC_SR_BUSY)

// flags to set before new command
#define CAPSFDC_SR_NCSET CAPSFDC_SR_BUSY

// type1 mask, index/track0/spin-up
// type2 read mask, drq/lost data/record type/wp=0
// type2 write mask, drq/lost data/record type
#define CAPSFDC_SM_TYPE1 0x00
#define CAPSFDC_SM_TYPE2R (CAPSFDC_SR_IP_DRQ|CAPSFDC_SR_TR0_LD|CAPSFDC_SR_SU_RT|CAPSFDC_SR_WP)
#define CAPSFDC_SM_TYPE2W (CAPSFDC_SR_IP_DRQ|CAPSFDC_SR_TR0_LD|CAPSFDC_SR_SU_RT)

// end request flags
// 0: command complete
// 1: request to stop execution at current cycle; can be bitwise set in a callback
#define CAPSFDC_ER_COMEND DF_0
#define CAPSFDC_ER_REQEND DF_1

// am info flags
//  0: AM detector enabled
//  1: CRC logic enabled
//  2: CRC logic active after first mark found
//  3: data being assembled in DSR is the last byte of AM
//  4: data being assembled in DSR is an A1 mark
//  5: AM in decoder, valid for 1 cell
//  6: A1 mark in decoder, valid for 1 cell
//  7: C2 mark in decoder, valid for 1 cell
//  8: DSR holds a complete byte, valid until DSR has been changed
//  9: DSR synced to AM, valid until DSR has been changed, next DSRREADY is a data byte
// 10: DSR synced to A1 mark
#define CAPSFDC_AI_AMDETENABLE DF_0
#define CAPSFDC_AI_CRCENABLE   DF_1
#define CAPSFDC_AI_CRCACTIVE   DF_2
#define CAPSFDC_AI_AMACTIVE    DF_3
#define CAPSFDC_AI_MA1ACTIVE   DF_4
#define CAPSFDC_AI_AMFOUND     DF_5
#define CAPSFDC_AI_MARKA1      DF_6
#define CAPSFDC_AI_MARKC2      DF_7
#define CAPSFDC_AI_DSRREADY    DF_8
#define CAPSFDC_AI_DSRAM       DF_9
#define CAPSFDC_AI_DSRMA1      DF_10

#pragma pack(push, 1)

// drive state
struct CapsDrive {
	UDWORD type;      // structure size
	UDWORD rpm;       // drive rpm
	SDWORD maxtrack;  // track with hard stop (head can't move)
	SDWORD track;     // actual track
	SDWORD buftrack;  // track# in buffer
	SDWORD side;      // actual side used for processing
	SDWORD bufside;   // side# in buffer
	SDWORD newside;   // side to select after processing
	UDWORD diskattr;  // disk attributes
	UDWORD idistance; // distance from index in clock cycles
	UDWORD clockrev;  // clock cycles per revolution
	SDWORD clockip;   // clock cycles for index pulse hold
	SDWORD ipcnt;     // index pulse clock counter, <0 init, 0 stopped
	UDWORD ttype;     // track type
	PUBYTE trackbuf;  // track buffer memory
	PUDWORD timebuf;  // timing buffer
	UDWORD tracklen;  // track buffer memory length
	SDWORD overlap;   // overlap position
	SDWORD trackbits; // used track size
	SDWORD ovlmin;    // overlap first bit position
	SDWORD ovlmax;    // overlap last bit position
	SDWORD ovlcnt;    // overlay bit count
	SDWORD ovlact;    // active overlay phase
	SDWORD nact;      // active noise phase
	UDWORD nseed;     // noise generator seed
	PVOID userptr;    // free to use pointer for the host application
	UDWORD userdata;  // free to use data for the host application
};

typedef struct CapsDrive *PCAPSDRIVE;

typedef struct CapsFdc *PCAPSFDC;
typedef void (__cdecl *CAPSFDCHOOK)(PCAPSFDC pfdc, UDWORD state);

// fdc state
struct CapsFdc {
	UDWORD type;         // structure size
	UDWORD model;        // fdc type
	UDWORD endrequest;   // non-zero value ends command
	UDWORD clockact;     // clock cycles completed
	UDWORD clockreq;     // requested clock cycles to complete
	UDWORD clockfrq;     // clock frequency
	UDWORD addressmask;  // valid address lines
	UDWORD dataline;     // data bus
	UDWORD datamask;     // valid data lines
	UDWORD lineout;      // output lines
	UDWORD runmode;      // run mode
	UDWORD runstate;     // local run state in a command
	UDWORD r_st0;        // status0 register
	UDWORD r_st1;        // status1 register
	UDWORD r_stm;        // status mask register (1 bits select st1)
	UDWORD r_command;    // command register
	UDWORD r_track;      // track register
	UDWORD r_sector;     // sector register
	UDWORD r_data;       // data register
	UDWORD seclenmask;   // sector length mask
	UDWORD seclen;       // sector length
	UDWORD crc;          // crc holder
	UDWORD crccnt;       // crc bit counter
	UDWORD amdecode;     // am detector decoder/shifter
	UDWORD aminfo;       // am info
	UDWORD amisigmask;   // enabled am info signal bits
	SDWORD amdatadelay;  // am data delay clock
	SDWORD amdataskip;   // am data skip clock
	SDWORD ammarkdist;   // am invalid distance from last mark in bitcells or 0 if valid
	SDWORD ammarktype;   // am last mark type
	UDWORD dsr;          // data shift register
	SDWORD dsrcnt;       // dsr bit counter
	SDWORD datalock;     // data lock bit position, <0 not locked
	UDWORD datamode;     // data access mode
	UDWORD datacycle;    // clock cycle remainder of actual bit
	UDWORD dataphase;    // data access phase
	UDWORD datapcnt;     // data phase counter
	SDWORD indexcount;   // index pulse counter
	SDWORD indexlimit;   // index pulse abort point
	SDWORD readlimit;    // read abort point
	SDWORD verifylimit;  // verify abort point
	SDWORD spinupcnt;    // counter for spin-up status
	SDWORD spinuplimit;  // spin-up point
	SDWORD idlecnt;      // counter for idle
	SDWORD idlelimit;    // idle point
	UDWORD clockcnt;     // clock counter
	UDWORD steptime[4];  // stepping rates us
	UDWORD clockstep[4]; // clock cycles for stepping rates
	UDWORD hstime;       // head settling delay us
	UDWORD clockhs;      // clock cycles for head settling
	UDWORD iptime;       // index pulse hold us
	UDWORD updatetime;   // update delay between short operations us
	UDWORD clockupdate;  // clock cycles for update delay
	SDWORD drivecnt;     // number of drives, 0: no drive attached
	SDWORD drivemax;     // number of active drives, 0: no drive attached
	SDWORD drivenew;     // drive to select after processing, <0 invalid
	SDWORD drivesel;     // drive selected for processing, <0 invalid
	SDWORD driveact;     // drive used for processing, <0 invalid
	PCAPSDRIVE driveprc; // drive processed, helper
	PCAPSDRIVE drive;    // available drives
	CAPSFDCHOOK cbirq;   // irq line change callback
	CAPSFDCHOOK cbdrq;   // drq line change callback
	CAPSFDCHOOK cbtrk;   // track change callback
	PVOID userptr;       // free to use pointer for the host application
	UDWORD userdata;     // free to use data for the host application
};

#pragma pack(pop)

// emulator info
enum {
	cfdciNA=0,       // invalid
	cfdciSize_Fdc,   // size required for FDC structure
	cfdciSize_Drive, // size required for Drive structure
	cfdciR_Command,  // command register
	cfdciR_ST,       // status register
	cfdciR_Track,    // track register
	cfdciR_Sector,   // sector register
	cfdciR_Data,     // data register
};

// fdc models
enum {
	cfdcmNA=0,   // invalid fdc model
	cfdcmWD1772  // WD1772
};

// run modes
enum {
	cfdcrmNop=0,  // no-operation
	cfdcrmIdle,   // idle/wait loop
	cfdcrmType1,  // type 1 loop
	cfdcrmType2R, // type 2 read loop
	cfdcrmType2W, // type 2 write loop
	cfdcrmType3R, // type 3 read loop
	cfdcrmType3W, // type 3 write loop
	cfdcrmType3A, // type 3 address loop
	cfdcrmType4   // type 4 loop
};

// data modes
enum {
	cfdcdmNoline=0, // no line input
	cfdcdmNoise,    // noise input
	cfdcdmData,     // data input
	cfdcdmDMap      // data with density map
};

#endif
