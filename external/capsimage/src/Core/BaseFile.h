#ifndef BASEFILE_H
#define BASEFILE_H

// file modes
#define BFFLAG_WRITE  1
#define BFFLAG_CREATE 2

// generic file access
class CBaseFile
{
public:
	enum {
		Start=0,
		Position,
		Current,
		End
	};

	CBaseFile();
	virtual ~CBaseFile();
	void Clear();
	virtual int Close()=0;
	virtual size_t Read(void *buf, size_t size)=0;
	virtual size_t Write(void *buf, size_t size)=0;
	virtual long Seek(long pos, int mode)=0;
	virtual long GetSize()=0;
	virtual long GetPosition()=0;
	virtual uint8_t *GetBuffer()=0;
	int IsOpen();

protected:
	int fileopen;
	unsigned int filemode;
};

typedef CBaseFile *PCBASEFILE;



// return true if file is open
inline int CBaseFile::IsOpen()
{
	return fileopen;
}

#endif
