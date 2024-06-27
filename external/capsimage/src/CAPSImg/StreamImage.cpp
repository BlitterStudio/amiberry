#include "stdafx.h"



CStreamImage::CStreamImage()
{
}

CStreamImage::~CStreamImage()
{
}

// lock and scan file
int CStreamImage::Lock(PCAPSFILE pcf)
{
	return imgeOk;
}

// release file
int CStreamImage::Unlock()
{
	return imgeOk;
}

// read and decode caps track format
int CStreamImage::LoadTrack(PDISKTRACKINFO pti, UDWORD flag)
{
	return imgeOk;
}
