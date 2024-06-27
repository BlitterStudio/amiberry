#include "stdafx.h"



CCapsFile::CCapsFile()
{
	file = NULL;
}

CCapsFile::~CCapsFile()
{
	Close();
}

// open file
int CCapsFile::Open(PCAPSFILE pcf)
{
	int res=true;

	Close();

	if (!pcf)
		return res;

	if (pcf->flag & CFF_MEMMAP) {
		PCMEMORYFILE f = new CMemoryFile();
		file = f;
		int mode = (pcf->flag & CFF_MEMREF) ? 0 : BFFLAG_CREATE;
		res=f->Open(pcf->memmap, pcf->size, mode);
	}	else {
		PCDISKFILE f = new CDiskFile();
		file = f;
		int fm = (pcf->flag & CFF_WRITE) ? BFFLAG_WRITE : 0;
		if (pcf->flag & CFF_CREATE)
			fm |= BFFLAG_CREATE;
		res = f->Open(pcf->name, fm);
	}

	return res;
}

// close file
int CCapsFile::Close()
{
	int res=0;

	if (file) {
		res = file->Close();
		delete file;
		file = NULL;
	}

	return res;
}

// read file chunk into memory
int CCapsFile::Read(PUBYTE buf, int size)
{
	return (int)file->Read(buf, size);
}

// write file
int CCapsFile::Write(PUBYTE buf, int size)
{
	return (int)file->Write(buf, size);
}

// seek in file
int CCapsFile::Seek(int pos, int mode)
{
	if (!mode)
		mode = CBaseFile::Current;
	else
		mode = CBaseFile::Position;

	return file->Seek(pos, mode);
}

// check whether file is accessible
int CCapsFile::IsOpen()
{
	return file->IsOpen();
}

// get file size
int CCapsFile::GetSize()
{
	return file->GetSize();
}

// get current file position
int CCapsFile::GetPosition()
{
	return file->GetPosition();
}

