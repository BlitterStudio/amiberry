#ifndef DISKFILE_H
#define DISKFILE_H

// disk based file access
class CDiskFile : public CBaseFile
{
public:
	CDiskFile();
	virtual ~CDiskFile();
	int Open(const char *name, unsigned int mode);
	int OpenAny(char **name, unsigned int mode);
	int OpenAnyPath(char **path, const char *name, unsigned int mode);
	int Close();
	size_t Read(void *buf, size_t size);
	size_t Write(void *buf, size_t size);
	long Seek(long pos, int mode);
	long GetSize();
	long GetPosition();
	uint8_t *GetBuffer();
	static void MakePath(const char *filename);
	static int FindFile(char *result, const char *filename, const char *filter);
	static int FileNameMatch(const char *pattern, const char *filename);

protected:
	FILE *dfile;
	int lastop;
	char tempname[MAX_FILENAMELEN]; // temporary file name
};

typedef CDiskFile *PCDISKFILE;

#endif
