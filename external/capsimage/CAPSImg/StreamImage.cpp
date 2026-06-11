#include "stdafx.h"



CStreamImage::CStreamImage()
{
}

CStreamImage::~CStreamImage()
{
}

// lock and scan file
int CStreamImage::Lock(std::unique_ptr<CBaseFile> pf)
{
	return imgeOk;
}

// release file
int CStreamImage::Unlock()
{
	return imgeOk;
}

// read and decode caps track format
int CStreamImage::LoadTrack(PDISKTRACKINFO pti, uint32_t flag)
{
	return imgeOk;
}
