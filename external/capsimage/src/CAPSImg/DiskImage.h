#ifndef DISKIMAGE_H
#define DISKIMAGE_H

#define MAX_CYLINDER 65536
#define MAX_HEAD 2
#define DEF_TRACKINFO (1024*MAX_HEAD)
#define DEF_SCANINFO 200
#define DEF_SCANEXTEND 2
#define DEF_CRCBUF 65536
#define DEF_FDALLOC 8

#define DI_COMP_MARK  DF_0
#define DI_COMP_DATA  DF_1
#define DI_COMP_RAW   DF_2
#define DI_COMP_FDATA DF_3

#define DI_COMP_CTT (DI_COMP_MARK|DI_COMP_DATA|DI_COMP_RAW|DI_COMP_FDATA)
#define DI_COMP_CTX (DI_COMP_DATA|DI_COMP_FDATA)

#define DI_LOCK_ANA  DF_30
#define DI_LOCK_COMP DF_31
#define DI_LOCK_CTA (DI_LOCK_ANA|DI_LOCK_DENVAR|DI_LOCK_DENAUTO|DI_LOCK_DENNOISE|DI_LOCK_NOISE|DI_LOCK_UPDATEFD)



// data data marker
struct DiskDataMark {
	int group; // marker group id
	uint32_t position; // data position
	int size; // data size
};

typedef DiskDataMark *PDISKDATAMARK;

// sector information collected during decoding
struct DiskSectorInfo {
	uint32_t descdatasize; // data size in bits from IPF descriptor
	uint32_t descgapsize;  // gap size in bits from IPF descriptor
	uint32_t datasize;     // data size in bits from decoder
	uint32_t gapsize;      // gap size in bits from decoder
	uint32_t datastart;    // data start position in bits from decoder
	uint32_t gapstart;     // gap start position in bits from decoder
	uint32_t gapsizews0;   // gap size before write splice
	uint32_t gapsizews1;   // gap size after write splice
	uint32_t gapws0mode;   // gap size mode before write splice
	uint32_t gapws1mode;   // gap size mode after write splice
	uint32_t celltype;     // bitcell type
	uint32_t enctype;      // encoder type
};

typedef DiskSectorInfo *PDISKSECTORINFO;

// disk image information block
struct DiskImageInfo {
	int valid;        // true if image info is valid
	int error;        // error code
	int dirty;        // true if any track modified
	int readonly;     // true if image opened in readonly mode
	int modimage;     // true if modify requires a new image
	int rawlock;      // true if locked track should be in raw
	int umincylinder; // lowest cylinder number used
	int umaxcylinder; // highest cylinder number used
	int uminhead;     // lowest head number used
	int umaxhead;     // highest head number used
	int dmpcount;     // dumped tracks
	int relcount;     // image tracks
	int didmax;       // data id maximum used in image
	int nextrev;      // next revolution for streaming
	int lastrev;      // last revolution used by streaming
	int realrev;      // last real revolution number used by streaming
	int civalid;      // true if ci is valid
	CapsInfo ci;      // CAPS image info, DO NOT MODIFY
};

typedef DiskImageInfo *PDISKIMAGEINFO;

// disk track information block
struct DiskTrackInfo {
	int type;       // track data type
	int linked;     // non zero if linked image
	int linkinfo;   // private link data for user
	UDWORD linkflag; // linked image flag
	int cylinder;   // cylinder number
	int head;       // head number
	int sectorcnt;  // available sectors
	int headerpos;  // pointer into image file (-1 no header)
	int datapos;    // pointer into image file (-1 no data)
	int datasize;   // data area size in image
	int trackcnt;   // track variant count
	int rawtrackcnt;   // track variant count for raw dumps
	int rawlen; // track buffer memory length for raw dumps
	int rawtimecnt; // timing information size for raw dumps
	PUDWORD rawtimebuf; // timing information for raw dumps
	int rawupdate;   // use raw dump update functionality if true (only if 1 revolution decoded is requested)
	int rawdump;     // true if this track is a raw dump
	PUBYTE trackbuf; // track buffer memory 
	int tracklen;    // track buffer memory length
	PUBYTE trackdata[CAPS_MTRS]; // track data pointer if available
	int tracksize[CAPS_MTRS];    // track data size
	int trackstart[CAPS_MTRS];   // track start position
	int timecnt;     // timing information size
	PUDWORD timebuf; // timing information
	int fixpos;      // position data for track fixing functions
	int comppos;     // position data for track compare functions
	int sdpos;       // decoder start position
	uint32_t wseed;  // weak bit generator data
	int compsblk;    // compare start block
	int compeblk;    // compare end block
	int fdpsize;     // actual usage of flakey positions
	int fdpmax;      // flakey positions size
	PDISKDATAMARK fdp; // flakey data
	int overlap;     // track overlap position bit or byte
	int overlapbit;  // track overlap bit position
	uint32_t trackbc; // size of track buffer in bits, may hold multiple revolutions
	uint32_t singletrackbc; // size of track buffer in bits, one revolution
	uint32_t startbit; // decoder start bit position
	int sipsize;         // sector info size
	PDISKSECTORINFO sip; // sector info
	CapsImage ci;    // CAPS image descriptor, DO NOT MODIFY
};

typedef DiskTrackInfo *PDISKTRACKINFO;

// track types
enum {
	dtitUndefined, // track data is invalid
	dtitError,     // track data has error
	dtitCapsDump,  // CAPS dump format
	dtitCapsImage, // CAPS image format
	dtitPlain      // decoded, simple format
};



// disk image handler
class CDiskImage  
{
public:
	CDiskImage();
	virtual ~CDiskImage();
	virtual int Lock(PCAPSFILE pcf);
	virtual int Unlock();
	virtual int LoadImage(UDWORD flag, int free=false);
	PDISKTRACKINFO GetTrack(int cylinder, int head);
	PDISKTRACKINFO LockTrack(int cylinder, int head, UDWORD flag);
	PDISKTRACKINFO LockTrackComp(int cylinder, int head, UDWORD flag, int sblk, int eblk);
	PDISKTRACKINFO UnlockTrack(int cylinder, int head, int forced=false);
	static void UnlockTrack(PDISKTRACKINFO pti, int forced=false);
	void UnlockTrack(int forced=false);
	static int LinkTrackData(PDISKTRACKINFO pti, int size);
	PDISKIMAGEINFO GetInfo();
	static void CreateDateTime(PCAPSDATETIME pcd);
	static void DecodeDateTime(PCAPSDATETIMEEXT dec, PCAPSDATETIME pcd);
	static LPCSTR GetPlatformName(int pid);
	static UDWORD CrcFile(PCAPSFILE pcf);

protected:
	void Destroy();
	int AddTrack(PDISKTRACKINFO pdti);
	virtual int LoadTrack(PDISKTRACKINFO pti, UDWORD flag);
	int LoadPlain(PDISKTRACKINFO pti);
	int AllocTrack(PDISKTRACKINFO pti, UDWORD flag);
	static void FreeTrack(PDISKTRACKINFO pti, int forced=false);
	static void FreeTrackData(PDISKTRACKINFO pti);
	static void FreeTrackTiming(PDISKTRACKINFO pti);
	static void FreeTrackFD(PDISKTRACKINFO pti);
	PDISKTRACKINFO MapTrack(int cylinder, int head);
	void UpdateImageInfo(PDISKTRACKINFO pti);
	static UDWORD ReadValue(PUBYTE buf, int cnt);
	static PDISKDATAMARK AddFD(PDISKTRACKINFO pti, PDISKDATAMARK src, int size, int units=0);
	static PDISKDATAMARK AllocFD(PDISKTRACKINFO pti, int size, int units=DEF_FDALLOC);
	static void AllocTrackSI(PDISKTRACKINFO pti);
	static void FreeTrackSI(PDISKTRACKINFO pti);

protected:
	DiskImageInfo dii;
	int dticnt;
	int dticyl;
	int dtihed;
	PDISKTRACKINFO dti;
	static LPCSTR pidname[cppidLast];
};

typedef CDiskImage *PCDISKIMAGE;



// get image information
inline PDISKIMAGEINFO CDiskImage::GetInfo()
{
	return dii.valid ? &dii : NULL;
}

#endif
