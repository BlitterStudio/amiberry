#ifndef CAPSFDC_H
#define CAPSFDC_H

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
	uint32_t type;      // structure size
	uint32_t rpm;       // drive rpm
	int32_t maxtrack;  // track with hard stop (head can't move)
	int32_t track;     // actual track
	int32_t buftrack;  // track# in buffer
	int32_t side;      // actual side used for processing
	int32_t bufside;   // side# in buffer
	int32_t newside;   // side to select after processing
	uint32_t diskattr;  // disk attributes
	uint32_t idistance; // distance from index in clock cycles
	uint32_t clockrev;  // clock cycles per revolution
	int32_t clockip;   // clock cycles for index pulse hold
	int32_t ipcnt;     // index pulse clock counter, <0 init, 0 stopped
	uint32_t ttype;     // track type
	uint8_t *trackbuf;  // track buffer memory
	uint32_t *timebuf;  // timing buffer
	uint32_t tracklen;  // track buffer memory length
	int32_t overlap;   // overlap position
	int32_t trackbits; // used track size
	int32_t ovlmin;    // overlap first bit position
	int32_t ovlmax;    // overlap last bit position
	int32_t ovlcnt;    // overlay bit count
	int32_t ovlact;    // active overlay phase
	int32_t nact;      // active noise phase
	uint32_t nseed;     // noise generator seed
	void *userptr;    // free to use pointer for the host application
	uint32_t userdata;  // free to use data for the host application
};

typedef struct CapsDrive *PCAPSDRIVE;

typedef struct CapsFdc *PCAPSFDC;
typedef void(__cdecl *CAPSFDCHOOK)(PCAPSFDC pfdc, uint32_t state);

// fdc state
struct CapsFdc {
	uint32_t type;         // structure size
	uint32_t model;        // fdc type
	uint32_t endrequest;   // non-zero value ends command
	uint32_t clockact;     // clock cycles completed
	uint32_t clockreq;     // requested clock cycles to complete
	uint32_t clockfrq;     // clock frequency
	uint32_t addressmask;  // valid address lines
	uint32_t dataline;     // data bus
	uint32_t datamask;     // valid data lines
	uint32_t lineout;      // output lines
	uint32_t runmode;      // run mode
	uint32_t runstate;     // local run state in a command
	uint32_t r_st0;        // status0 register
	uint32_t r_st1;        // status1 register
	uint32_t r_stm;        // status mask register (1 bits select st1)
	uint32_t r_command;    // command register
	uint32_t r_track;      // track register
	uint32_t r_sector;     // sector register
	uint32_t r_data;       // data register
	uint32_t seclenmask;   // sector length mask
	uint32_t seclen;       // sector length
	uint32_t crc;          // crc holder
	uint32_t crccnt;       // crc bit counter
	uint32_t amdecode;     // am detector decoder/shifter
	uint32_t aminfo;       // am info
	uint32_t amisigmask;   // enabled am info signal bits
	int32_t amdatadelay;  // am data delay clock
	int32_t amdataskip;   // am data skip clock
	int32_t ammarkdist;   // am invalid distance from last mark in bitcells or 0 if valid
	int32_t ammarktype;   // am last mark type
	uint32_t dsr;          // data shift register
	int32_t dsrcnt;       // dsr bit counter
	int32_t datalock;     // data lock bit position, <0 not locked
	uint32_t datamode;     // data access mode
	uint32_t datacycle;    // clock cycle remainder of actual bit
	uint32_t dataphase;    // data access phase
	uint32_t datapcnt;     // data phase counter
	int32_t indexcount;   // index pulse counter
	int32_t indexlimit;   // index pulse abort point
	int32_t readlimit;    // read abort point
	int32_t verifylimit;  // verify abort point
	int32_t spinupcnt;    // counter for spin-up status
	int32_t spinuplimit;  // spin-up point
	int32_t idlecnt;      // counter for idle
	int32_t idlelimit;    // idle point
	uint32_t clockcnt;     // clock counter
	uint32_t steptime[4];  // stepping rates us
	uint32_t clockstep[4]; // clock cycles for stepping rates
	uint32_t hstime;       // head settling delay us
	uint32_t clockhs;      // clock cycles for head settling
	uint32_t iptime;       // index pulse hold us
	uint32_t updatetime;   // update delay between short operations us
	uint32_t clockupdate;  // clock cycles for update delay
	int32_t drivecnt;     // number of drives, 0: no drive attached
	int32_t drivemax;     // number of active drives, 0: no drive attached
	int32_t drivenew;     // drive to select after processing, <0 invalid
	int32_t drivesel;     // drive selected for processing, <0 invalid
	int32_t driveact;     // drive used for processing, <0 invalid
	PCAPSDRIVE driveprc; // drive processed, helper
	PCAPSDRIVE drive;    // available drives
	CAPSFDCHOOK cbirq;   // irq line change callback
	CAPSFDCHOOK cbdrq;   // drq line change callback
	CAPSFDCHOOK cbtrk;   // track change callback
	void *userptr;       // free to use pointer for the host application
	uint32_t userdata;     // free to use data for the host application
};

#pragma pack(pop)

// emulator info
enum {
	cfdciNA = 0,       // invalid
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
	cfdcmNA = 0,   // invalid fdc model
	cfdcmWD1772  // WD1772
};

// run modes
enum {
	cfdcrmNop = 0,  // no-operation
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
	cfdcdmNoline = 0, // no line input
	cfdcdmNoise,    // noise input
	cfdcdmData,     // data input
	cfdcdmDMap      // data with density map
};

// reset states
enum {
	cfdcrs_Cold = 0, // cold reset
	cfdcrs_Warm, // warm reset
};

#endif
