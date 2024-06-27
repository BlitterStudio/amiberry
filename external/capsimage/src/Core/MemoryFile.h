#ifndef MEMORYFILE_H
#define MEMORYFILE_H

#define DEF_MEMORYFILEALLOC 512



// memory based file access/buffer
class CMemoryFile : public CBaseFile
{
public:
	CMemoryFile();
	virtual ~CMemoryFile();
	int Open(void *buf, size_t size, unsigned int mode);
	int Close();
	void Free();
	size_t Read(void *buf, size_t size);
	size_t Write(void *buf, size_t size);
	long Seek(long pos, int mode);
	long GetSize();
	long GetPosition();
	uint8_t *GetBuffer();

protected:
	// memory type
	enum {
		mtAlloc,
		mtUser,
		mtLast
	};

	void Clear(int clbuf=1);
	void AllocBuffer(size_t maxsize);
	void FreeBuffer();

protected:
	int filemt; // file memory type
	uint8_t *filebuf[mtLast]; // file buffer
	size_t filesize[mtLast]; // buffer size
	size_t filecount; // valid data count in buffer
	size_t filepos; // file position in buffer
};

typedef CMemoryFile *PCMEMORYFILE;

#endif
