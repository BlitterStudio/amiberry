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
	UBYTE name[4]; // chunk identifier
	UDWORD size;   // chunk size, including CapsID and extended chunk
	UDWORD hcrc;   // chunk CRC calculated as hcrc=0
};

typedef CapsID *PCAPSID;

// dumped memory area
struct CapsDump {
	UDWORD type; // memory type
	UDWORD size; // memory size
	UDWORD area; // memory location
	UDWORD did;  // data chunk identifier
};

typedef CapsDump *PCAPSDUMP;

// data area
struct CapsData {
	UDWORD size;  // data area size in bytes after chunk
	UDWORD bsize; // data area size in bits
	UDWORD dcrc;  // data area crc
	UDWORD did;   // data chunk identifier
};

typedef CapsData *PCAPSDATA;

// dumped track
struct CapsTrack {
	UDWORD type; // track type
	UDWORD cyl;  // cylinder
	UDWORD head; // head
	UDWORD did;  // data chunk identifier
};

typedef CapsTrack *PCAPSTRACK;

// caps packed date.time format
struct CapsDateTime {
	UDWORD date; // packed date, yyyymmdd
	UDWORD time; // packed time, hhmmssttt
};

typedef CapsDateTime *PCAPSDATETIME;

// image information
struct CapsInfo {
	UDWORD type;        // image type
	UDWORD encoder;     // image encoder ID
	UDWORD encrev;      // image encoder revision
	UDWORD release;     // release ID
	UDWORD revision;    // release revision ID
	UDWORD origin;      // original source reference
	UDWORD mincylinder; // lowest cylinder number
	UDWORD maxcylinder; // highest cylinder number
	UDWORD minhead;     // lowest head number
	UDWORD maxhead;     // highest head number
	CapsDateTime crdt;  // image creation date.time
	UDWORD platform[CAPS_MAXPLATFORM]; // intended platform(s)
	UDWORD disknum;     // disk# for release, >= 1 if multidisk
	UDWORD userid;      // user id of the image creator
	UDWORD reserved[3]; // future use
};

typedef CapsInfo *PCAPSINFO;

// track image descriptor
struct CapsImage {
	UDWORD cylinder; // cylinder#
	UDWORD head;     // head#
	UDWORD dentype;  // density type
	UDWORD sigtype;  // signal processing type
	UDWORD trksize;  // decoded track size, rounded
	UDWORD startpos; // start position, rounded
	UDWORD startbit; // start position on original data
	UDWORD databits; // decoded data size in bits
	UDWORD gapbits;  // decoded gap size in bits
	UDWORD trkbits;  // decoded track size in bits
	UDWORD blkcnt;   // number of blocks
	UDWORD process;  // encoder prcocess
	UDWORD flag;     // image flags
	UDWORD did;      // data chunk identifier
	UDWORD reserved[3]; // future use
};

typedef CapsImage *PCAPSIMAGE;

// original meaning of some CapsBlock entries for old images
struct CapsBlockExt {
	UDWORD blocksize;  // decoded block size, rounded
	UDWORD gapsize;    // decoded gap size, rounded
};

// new meaning of some CapsBlock entries for new images
struct SPSBlockExt {
	UDWORD gapoffset;  // offset of gap stream in data area
	UDWORD celltype;   // bitcell type
};

// union for old or new images
union CapsBlockType {
	CapsBlockExt caps; // access old image
	SPSBlockExt sps;   // access new image
};

// block image descriptor
struct CapsBlock {
	UDWORD blockbits;  // decoded block size in bits
	UDWORD gapbits;    // decoded gap size in bits
	CapsBlockType bt;  // content depending on image type
	UDWORD enctype;    // encoder type
	UDWORD flag;       // block flags
	UDWORD gapvalue;   // default gap value
	UDWORD dataoffset; // offset of data stream in data area
};

typedef CapsBlock *PCAPSBLOCK;



// analyser export
struct CapsExport {
	UDWORD cylinder; // cylinder#
	UDWORD head;     // head#
	UDWORD dentype;  // density type
	UDWORD anaid;    // analyser selected format
	UDWORD anafix;   // analyser fix method
	UDWORD anatrs;   // analyser track size
	UDWORD reserved[2]; // future use
};

typedef CapsExport *PCAPSEXPORT;

// analyser export info
struct CapsExportInfo {
	UDWORD releasecrc; // crc32 on release ipf
	UDWORD anarev;     // analyser revision
	UDWORD reserved[14]; // future use
};

typedef CapsExportInfo *PCAPSEXPORTINFO;



// CAPS pack/CT Raw format
struct CapsPack {
	UBYTE sign[4]; // host signature
	UDWORD usize;  // original size in bytes
	UDWORD ucrc;   // CRC on uncompressed data
	UDWORD csize;  // compressed size in bytes
	UDWORD ccrc;   // CRC on compressed data
	UDWORD hcrc;   // CRC on header calculated as hcrc=0
};

typedef CapsPack *PCAPSPACK;

// CAPS raw format
struct CapsRaw {
	UDWORD time;
	UDWORD raw;
};

typedef CapsRaw *PCAPSRAW;



// generic dump types
enum {
	cpdKICK=1, // kickstart rom
	cpdBOOT    // bootrom
};

// image type
enum {
	cpimtNA=0, // invalid image type
	cpimtFDD,  // floppy disk
	cpimtLast
};

// platform IDs, not about configuration, but intended use
enum {
	cppidNA=0, // invalid platform (dummy entry)
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
	cpdenNA=0,     // invalid density
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
	cpsigNA=0, // invalid signal type
	cpsig2us,  // 2us cells
	cpsigLast
};

// bitcell used
enum {
	cpbctNA=0, // invalid cell type
	cpbct2us,  // 2us cells
	cpbctLast
};

// encoder types
enum {
	cpencNA=0, // invalid encoder
	cpencMFM,  // MFM
	cpencRaw,  // no encoder used, test data only
	cpencLast
};

// data types
enum {
	cpdatEnd=0, // data stream end
	cpdatMark,  // mark/sync
	cpdatData,  // data
	cpdatGap,   // gap
	cpdatRaw,   // raw
	cpdatFData, // flakey data
	cpdatLast
};

// gap types
enum {
	cpgapEnd=0, // gap stream end
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
