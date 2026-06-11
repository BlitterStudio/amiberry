#ifndef CAPSDEFINITIONS_H
#define CAPSDEFINITIONS_H

#pragma pack(push, 1)

#define CAPS_ENCODER 1
#define CAPS_ENCODER_REV 1
#define SPS_ENCODER 2
#define SPS_ENCODER_REV 1

#define CAPS_IDFILE "CAPS"
#define CAPS_IDDUMP "DUMP"
#define CAPS_IDDATA "DATA"
#define CAPS_IDTRCK "TRCK"
#define CAPS_IDINFO "INFO"
#define CAPS_IDIMGE "IMGE"
#define CAPS_IDCTEX "CTEX"
#define CAPS_IDCTEI "CTEI"
#define CAPS_IDPACK "PACK"

#define CAPS_PHOLD 24576

#define CAPS_DATAMASK 0x1f
#define CAPS_SIZE_S 5

// CapsImage flags
#define CAPS_IF_FLAKEY DF_0

// CapsBlock flags
#define CAPS_BF_GP0 DF_0
#define CAPS_BF_GP1 DF_1
#define CAPS_BF_DMB DF_2

#define CAPS_EF_RESAMPLE DF_0



// generic CAPS chunk
struct CapsID {
	uint8_t name[4]; // chunk identifier
	uint32_t size;   // chunk size, including CapsID and extended chunk
	uint32_t hcrc;   // chunk CRC calculated as hcrc=0
};

typedef CapsID *PCAPSID;

// dumped memory area
struct CapsDump {
	uint32_t type; // memory type
	uint32_t size; // memory size
	uint32_t area; // memory location
	uint32_t did;  // data chunk identifier
};

typedef CapsDump *PCAPSDUMP;

// data area
struct CapsData {
	uint32_t size;  // data area size in bytes after chunk
	uint32_t bsize; // data area size in bits
	uint32_t dcrc;  // data area crc
	uint32_t did;   // data chunk identifier
};

typedef CapsData *PCAPSDATA;

// dumped track
struct CapsTrack {
	uint32_t type; // track type
	uint32_t cyl;  // cylinder
	uint32_t head; // head
	uint32_t did;  // data chunk identifier
};

typedef CapsTrack *PCAPSTRACK;

// caps packed date.time format
struct CapsDateTime {
	uint32_t date; // packed date, yyyymmdd
	uint32_t time; // packed time, hhmmssttt
};

typedef CapsDateTime *PCAPSDATETIME;

// image information
struct CapsInfo {
	uint32_t type;        // image type
	uint32_t encoder;     // image encoder ID
	uint32_t encrev;      // image encoder revision
	uint32_t release;     // release ID
	uint32_t revision;    // release revision ID
	uint32_t origin;      // original source reference
	uint32_t mincylinder; // lowest cylinder number
	uint32_t maxcylinder; // highest cylinder number
	uint32_t minhead;     // lowest head number
	uint32_t maxhead;     // highest head number
	CapsDateTime crdt;  // image creation date.time
	uint32_t platform[CAPS_MAXPLATFORM]; // intended platform(s)
	uint32_t disknum;     // disk# for release, >= 1 if multidisk
	uint32_t userid;      // user id of the image creator
	uint32_t reserved[3]; // future use
};

typedef CapsInfo *PCAPSINFO;

// track image descriptor
struct CapsImage {
	uint32_t cylinder; // cylinder#
	uint32_t head;     // head#
	uint32_t dentype;  // density type
	uint32_t sigtype;  // signal processing type
	uint32_t trksize;  // decoded track size, rounded
	uint32_t startpos; // start position, rounded
	uint32_t startbit; // start position on original data
	uint32_t databits; // decoded data size in bits
	uint32_t gapbits;  // decoded gap size in bits
	uint32_t trkbits;  // decoded track size in bits
	uint32_t blkcnt;   // number of blocks
	uint32_t process;  // encoder prcocess
	uint32_t flag;     // image flags
	uint32_t did;      // data chunk identifier
	uint32_t reserved[3]; // future use
};

typedef CapsImage *PCAPSIMAGE;

// original meaning of some CapsBlock entries for old images
struct CapsBlockExt {
	uint32_t blocksize;  // decoded block size, rounded
	uint32_t gapsize;    // decoded gap size, rounded
};

// new meaning of some CapsBlock entries for new images
struct SPSBlockExt {
	uint32_t gapoffset;  // offset of gap stream in data area
	uint32_t celltype;   // bitcell type
};

// union for old or new images
union CapsBlockType {
	CapsBlockExt caps; // access old image
	SPSBlockExt sps;   // access new image
};

// block image descriptor
struct CapsBlock {
	uint32_t blockbits;  // decoded block size in bits
	uint32_t gapbits;    // decoded gap size in bits
	CapsBlockType bt;  // content depending on image type
	uint32_t enctype;    // encoder type
	uint32_t flag;       // block flags
	uint32_t gapvalue;   // default gap value
	uint32_t dataoffset; // offset of data stream in data area
};

typedef CapsBlock *PCAPSBLOCK;



// analyser export
struct CapsExport {
	uint32_t cylinder; // cylinder#
	uint32_t head;     // head#
	uint32_t dentype;  // density type
	uint32_t anaid;    // analyser selected format
	uint32_t anafix;   // analyser fix method
	uint32_t anatrs;   // analyser track size
	uint32_t reserved[2]; // future use
};

typedef CapsExport *PCAPSEXPORT;

// analyser export info
struct CapsExportInfo {
	uint32_t releasecrc; // crc32 on release ipf
	uint32_t anarev;     // analyser revision
	uint32_t reserved[14]; // future use
};

typedef CapsExportInfo *PCAPSEXPORTINFO;



// CAPS pack/CT Raw format
struct CapsPack {
	uint8_t sign[4]; // host signature
	uint32_t usize;  // original size in bytes
	uint32_t ucrc;   // CRC on uncompressed data
	uint32_t csize;  // compressed size in bytes
	uint32_t ccrc;   // CRC on compressed data
	uint32_t hcrc;   // CRC on header calculated as hcrc=0
};

typedef CapsPack *PCAPSPACK;

// CAPS raw format
struct CapsRaw {
	uint32_t time;
	uint32_t raw;
};

typedef CapsRaw *PCAPSRAW;



// generic dump types
enum {
	cpdKICK = 1, // kickstart rom
	cpdBOOT    // bootrom
};

// image type
enum {
	cpimtNA = 0, // invalid image type
	cpimtFDD,  // floppy disk
	cpimtLast
};

// platform IDs, not about configuration, but intended use
enum {
	cppidNA = 0, // invalid platform (dummy entry)
	cppidAmiga,
	cppidAtariST,
	cppidPC,
	cppidAmstradCPC,
	cppidSpectrum,
	cppidSamCoupe,
	cppidArchimedes,
	cppidC64,
	cppidAtari8,
	cppidLast
};

// density types
enum {
	cpdenNA = 0,     // invalid density
	cpdenNoise,    // cells are unformatted (random size)
	cpdenAuto,     // automatic cell size, according to track size
	cpdenCLAmiga,  // Copylock Amiga
	cpdenCLAmiga2, // Copylock Amiga, new
	cpdenCLST,     // Copylock ST
	cpdenSLAmiga,  // Speedlock Amiga
	cpdenSLAmiga2, // Speedlock Amiga, old
	cpdenABAmiga,  // Adam Brierley Amiga
	cpdenABAmiga2, // Adam Brierley, density key Amiga
	cpdenLast
};

// signal processing used
enum {
	cpsigNA = 0, // invalid signal type
	cpsig2us,  // 2us cells
	cpsigLast
};

// bitcell used
enum {
	cpbctNA = 0, // invalid cell type
	cpbct2us,  // 2us cells
	cpbctLast
};

// encoder types
enum {
	cpencNA = 0, // invalid encoder
	cpencMFM,  // MFM
	cpencRaw,  // no encoder used, test data only
	cpencLast
};

// data types
enum {
	cpdatEnd = 0, // data stream end
	cpdatMark,  // mark/sync
	cpdatData,  // data
	cpdatGap,   // gap
	cpdatRaw,   // raw
	cpdatFData, // flakey data
	cpdatLast
};

// gap types
enum {
	cpgapEnd = 0, // gap stream end
	cpgapCount, // gap counter
	cpgapData,  // gap data pattern
	cpgapLast
};



// chunk modifiers
union CapsMod {
	CapsDump dump;
	CapsData data;
	CapsTrack trck;
	CapsInfo info;
	CapsImage imge;
	CapsExport ctex;
	CapsExportInfo ctei;
};

// generic chunk
struct CapsGeneric {
	CapsID file; // generic part always available
	CapsMod mod; // modifier part
};

typedef CapsGeneric *PCAPSGENERIC;

// convenience easy type handling
struct CapsChunk {
	int type;       // type identified for generic part
	CapsGeneric cg; // generic caps placeholder
};

typedef CapsChunk *PCAPSCHUNK;

#pragma pack(pop)

#endif
