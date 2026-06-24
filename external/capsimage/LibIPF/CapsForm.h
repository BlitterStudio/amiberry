#ifndef CAPSFORM_H
#define CAPSFORM_H

#pragma pack(push, 1)

// block data descriptor for converter
struct CapsFormatBlock {
	uint32_t gapacnt;   // gap before first am count
	uint32_t gapavalue; // gap before first am value
	uint32_t gapbcnt;   // gap after first am count
	uint32_t gapbvalue; // gap after first am value
	uint32_t gapccnt;   // gap before second am count
	uint32_t gapcvalue; // gap before second am value
	uint32_t gapdcnt;   // gap after second am count
	uint32_t gapdvalue; // gap after second am value
	uint32_t blocktype; // type of block
	uint32_t track;     // track#
	uint32_t side;      // side#
	uint32_t sector;    // sector#
	int32_t sectorlen; // sector length in bytes
	uint8_t *databuf;   // source data buffer
	uint32_t datavalue; // source data value if buffer is nullptr
};

typedef struct CapsFormatBlock *PCAPSFORMATBLOCK;

// track data descriptor for converter
struct CapsFormatTrack {
	uint32_t type;      // structure type
	uint32_t gapacnt;   // gap after index count
	uint32_t gapavalue; // gap after index value
	uint32_t gapbvalue; // gap before index value
	uint8_t *trackbuf;  // track buffer memory
	uint32_t tracklen;  // track buffer memory length
	uint32_t buflen;    // track buffer allocation size
	uint32_t bufreq;    // track buffer requested size
	uint32_t startpos;  // start position in buffer
	int32_t blockcnt;  // number of blocks
	PCAPSFORMATBLOCK block; // block data
	uint32_t size;      // internal counter
};

typedef struct CapsFormatTrack *PCAPSFORMATTRACK;

#pragma pack(pop)

// block types
enum {
	cfrmbtNA = 0,  // invalid type
	cfrmbtIndex, // index 
	cfrmbtData   // data
};

#endif
