#include "stdafx.h"



CBaseFile::CBaseFile()
{
	Clear();
}

CBaseFile::~CBaseFile()
{
}

// clear file settings
void CBaseFile::Clear()
{
	fileopen=0;
	filemode=0;
}
