#ifndef CAPSFILE_H
#define CAPSFILE_H



// generic file descriptor
struct CapsFile {
	LPCSTR name;
	PUBYTE memmap;
	UDWORD flag;
	int size;
};

typedef CapsFile *PCAPSFILE;

// file mode flags
#define CFF_WRITE  DF_0
#define CFF_MEMMAP DF_1
#define CFF_MEMREF DF_2
#define CFF_CREATE DF_3



// legacy CAPS file handler, bridge for CBaseFile functions
class CCapsFile
{
public:
	CCapsFile();
	virtual ~CCapsFile();
	int Open(PCAPSFILE pcf);
	int Close();
	int Read(PUBYTE buf, int size);
	int Write(PUBYTE buf, int size);
	int Seek(int pos, int mode);
	int IsOpen();
	int GetSize();
	int GetPosition();

protected:
	CBaseFile *file;
};

typedef CCapsFile *PCCAPSFILE;

#endif
