#include "stdafx.h"



CMemoryFile::CMemoryFile()
{
	Clear();
}

CMemoryFile::~CMemoryFile()
{
	Free();
}

// open file, return true on error
int CMemoryFile::Open(void *buf, size_t size, unsigned int mode)
{
	// reset buffers
	Clear(0);

	// create new buffer or reference user supplied buffer
	if (mode & BFFLAG_CREATE) {
		// create new buffer
		if (size) {
			// allocate initial buffer with the size specified when opening the file
			AllocBuffer(size);

			// copy buffer content if specified
			if (buf) {
				memcpy(filebuf[mtAlloc], buf, size*sizeof(uint8_t));
				filecount=size;
			}
		}

		// set class owned buffer
		filemt=mtAlloc;
	} else {
		// reference user supplied buffer
		if (size) {
			// error if buffer size is defined, but area is not
			if (!buf)
				return 1;
		} else
			buf=NULL;

		// set user buffer reference
		filemt=mtUser;
		filebuf[mtUser]=(uint8_t *)buf;
		filesize[mtUser]=size;
		filecount=size;
	}

	// rewind stream position
	filepos=0;

	// success
	fileopen=1;
	filemode=mode;

	return 0;
}

// close file, return true on error; class owned buffer is not freed to prevent re-allocation if class is re-used
int CMemoryFile::Close()
{
	// reset buffers, free resources if already closed
	if (filemt == mtLast)
		Free();
	else
		Clear(0);

	return 0;
}

// free allocated resources, close file
void CMemoryFile::Free()
{
	FreeBuffer();
	Clear();
}

// allocate class owned file buffer if needed, keep existing content
void CMemoryFile::AllocBuffer(size_t maxsize)
{
	if (filesize[mtAlloc] < maxsize) {
		maxsize+=DEF_MEMORYFILEALLOC;
		uint8_t *newfile=new uint8_t[maxsize];
		size_t oldcount=filecount;
		size_t oldpos=filepos;

		if (oldcount)
			memcpy(newfile, filebuf[mtAlloc], oldcount*sizeof(uint8_t));

		FreeBuffer();

		filebuf[mtAlloc]=newfile;
		filesize[mtAlloc]=maxsize;
		filecount=oldcount;
		filepos=oldpos;
	}
}

// free class owned file buffer
void CMemoryFile::FreeBuffer()
{
	filesize[mtAlloc]=0;
	filecount=0;
	filepos=0;

	delete [] filebuf[mtAlloc];
	filebuf[mtAlloc]=NULL;
}

// clear all data
void CMemoryFile::Clear(int clbuf)
{
	if (clbuf) {
		filebuf[mtAlloc]=NULL;
		filesize[mtAlloc]=0;

		filebuf[mtUser]=NULL;
		filesize[mtUser]=0;
	}

	filecount=0;
	filepos=0;

	filemt=mtLast;

	CBaseFile::Clear();
}

// read file, return number of bytes read
size_t CMemoryFile::Read(void *buf, size_t size)
{
	// stop if: nothing to read; buffer is invalid; file is not open
	if (!buf || !size || filemt == mtLast)
		return 0;

	// limit readable size to remaining valid content
	if (filecount-filepos < size)
		size=filecount-filepos;

	// read from file, update stream position
	if (size) {
		memcpy(buf, filebuf[filemt]+filepos, size);
		filepos+=size;
	}

	return size;
}

// write file, return number of bytes written
size_t CMemoryFile::Write(void *buf, size_t size)
{
	// stop if: nothing to write; buffer is invalid; file is not open; file is read-only
	if (!buf || !size || filemt == mtLast || !(filemode & BFFLAG_WRITE))
		return 0;

	// grow class owned file buffer if needed
	if (filemt == mtAlloc)
		AllocBuffer(filepos+size);

	// limit writeable size to buffer size
	if (filesize[filemt]-filepos < size)
		size=filesize[filemt]-filepos;

	// write into file, update stream position
	if (size) {
		memcpy(filebuf[filemt]+filepos, buf, size);
		filepos+=size;

		// update valid content count if wrote past the current file size
		if (filepos > filecount)
			filecount=filepos;
	}

	return size;
}

// seek in file, return the current position
long CMemoryFile::Seek(long pos, int mode)
{
	// default result 
	size_t res=0, tmppos;

	// return default result if file is not open
	if (filemt == mtLast)
		return (long)res;

	// seek in file
	switch (mode) {
		case Start:
			filepos=0;
			break;

		case Position:
			if (pos >= 0 && pos <= (long)filecount)
				filepos=pos;
			break;

		case Current:
			tmppos=filepos+pos;
			if (tmppos <= filecount)
				filepos = tmppos;
			break;

		case End:
			filepos=filecount;
			break;

		default:
			return (long)res;
	}

	return (long)filepos;
}

// get the file size
long CMemoryFile::GetSize()
{
	return (long)filecount;
}

// get file position
long CMemoryFile::GetPosition()
{
	return (long)filepos;
}

// get the file buffer
uint8_t *CMemoryFile::GetBuffer()
{
	return (filemt == mtLast) ? NULL : filebuf[filemt];
}
