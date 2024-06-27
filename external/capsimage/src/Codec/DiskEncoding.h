#ifndef DISKENCODING_H
#define DISKENCODING_H

// gcr mode id
enum {
	gcridNone,
	gcridCBM,
	gcridBigFive
};

// vmax mode id
enum {
	vmaxidNone,
	vmaxidNormal,
	vmaxidOld
};



// disk encoding functions
class CDiskEncoding
{
public:
	CDiskEncoding();
	virtual ~CDiskEncoding();
	static void InitFM();
	static void InitMFM(uint32_t mfmsize);
	static void InitGCRCBM(uint32_t *gcrtable, int gcrid);
	static void InitGCRCBM_S(uint32_t *gcrtable, int gcrid);
	static void InitGCRAppleH();
	static void InitGCRApple5(uint32_t *gcrtable);
	static void InitGCRApple6(uint32_t *gcrtable);
	static void InitGCRVorpal(uint32_t *gcrtable);
	static void InitGCRVorpal2(uint32_t *gcrtable);
	static void InitGCRVMax(uint32_t *gcrtable, int vmaxid);
	static void InitGCR4Bit(uint32_t *gcrtable);
	static int FindViolation(uint8_t *buffer, int bitpos, int bitcnt, int max0, int max1, int mode);

protected:
	void Clear();

public:
	static uint32_t fminit; // fm table initialized
	static uint32_t *fmcode; // fm code table
	static uint32_t *fmdecode; // fm decode table
	static uint32_t mfminit; // mfm table initialized
	static uint32_t mfmcodebit; // mfm table number of bits to index the code table
	static uint32_t *mfmcode; // mfm code table
	static uint32_t *mfmdecode; // mfm decode table
	static int gcrinit; // gcr table initialized
	static uint32_t *gcrcode; // gcr code table
	static uint32_t *gcrdecode; // gcr decode table
	static int gcrinit_s; // gcr table initialized
	static uint32_t *gcrcode_s; // gcr code table
	static uint32_t *gcrdecode_s; // gcr decode table
	static int gcrahinit; // gcr apple header table initialized
	static uint32_t *gcrahcode; // gcr apple header code table
	static uint32_t *gcrahdecode; // gcr apple header decode table
	static int gcra5init; // gcr apple 5 bit table initialized
	static uint32_t *gcra5code; // gcr apple 5 bit code table
	static uint32_t *gcra5decode; // gcr apple 5 bit decode table
	static int gcra6init; // gcr apple 6 bit table initialized
	static uint32_t *gcra6code; // gcr apple 6 bit code table
	static uint32_t *gcra6decode; // gcr apple 6 bit decode table
	static int gcrvorpalinit; // gcr vorpal 6 bit table initialized
	static uint32_t *gcrvorpalcode; // gcr vorpal 6 bit code table
	static uint32_t *gcrvorpaldecode; // gcr vorpal 6 bit decode table
	static int gcrvorpal2init; // gcr vorpal 2, 5 bit table initialized
	static uint32_t *gcrvorpal2code; // gcr vorpal 2, 5 bit code table
	static uint32_t *gcrvorpal2decode; // gcr vorpal 2, 5 bit decode table
	static int gcrvmaxinit; // gcr vmax, 6 bit table initialized
	static uint32_t *gcrvmaxcode; // gcr vmax, 6 bit code table
	static uint32_t *gcrvmaxdecode; // gcr vmax, 6 bit decode table
	static int gcr4bitinit; // gcr 4 bit table initialized
	static uint32_t *gcr4bitcode; // gcr 4 bit code table
	static uint32_t *gcr4bitdecode; // gcr 4 bit decode table

	static uint32_t gcr_cbm[]; // cbm gcr table
	static uint32_t gcr_bigfive[]; // cbm big five gcr table
	static uint32_t gcr_apple5[]; // apple 5 bit gcr table
	static uint32_t gcr_apple6[]; // apple 6 bit gcr table
	static uint32_t gcr_vorpal[]; // cbm vorpal 6 bit gcr table
	static uint32_t gcr_vorpal2[]; // cbm vorpal 2, 5 bit gcr table
	static uint32_t gcr_vmax[]; // cbm vmax 6 bit gcr table, normal
	static uint32_t gcr_vmaxold[]; // cbm vmax 6 bit gcr table, old version
	static uint32_t gcr_teque[]; // cbm teque 4 bit gcr table
	static uint32_t gcr_ozisoft[]; // cbm ozisoft 4 bit gcr table
};

typedef CDiskEncoding *PCDISKENCODING;

#endif
