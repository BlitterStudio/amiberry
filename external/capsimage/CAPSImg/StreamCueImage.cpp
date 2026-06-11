#include "stdafx.h"



CStreamCueImage::CStreamCueImage()
{
}

CStreamCueImage::~CStreamCueImage()
{
}

// lock and scan file
int CStreamCueImage::Lock(std::unique_ptr<CBaseFile> pf)
{
	return imgeOk;
}

// release file
int CStreamCueImage::Unlock()
{
	return imgeOk;
}
