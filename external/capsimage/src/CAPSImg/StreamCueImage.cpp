#include "stdafx.h"



CStreamCueImage::CStreamCueImage()
{
}

CStreamCueImage::~CStreamCueImage()
{
}

// lock and scan file
int CStreamCueImage::Lock(PCAPSFILE pcf)
{
	return imgeOk;
}

// release file
int CStreamCueImage::Unlock()
{
	return imgeOk;
}
