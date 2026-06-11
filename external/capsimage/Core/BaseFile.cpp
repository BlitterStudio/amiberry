#include "stdafx.h"



CBaseFile::CBaseFile()
{
	file_type = ftBaseFile;
	ClearBase();
}

CBaseFile::~CBaseFile()
{
}

// select/open the active part of a multi-part file - this always succeeds and does nothing on normal files
int CBaseFile::Select(uint32_t id)
{
	return false;
}

// get the file name associated with the id of a multi-part file or the current file name for normal files
std::string_view CBaseFile::GetSegmentName(uint32_t id) const
{
	return GetName();
}

// close the active part of a multi-part file - nothing to do on normal files
void CBaseFile::CloseSelected()
{
}

// clear file settings
void CBaseFile::ClearBase()
{
	file_open = false;
	file_mode = 0;
	file_name.clear();
}

// this function must be used to calculate the new file position in Seek functions
file_size_t CBaseFile::CalculateSeekPosition(file_size_t fsize, file_size_t fpos, file_pos_t seekpos, int mode)
{
	// return the current file position by default
	file_size_t nextpos = fpos;

	// seek in file
	switch (mode) {
		case Start:
			nextpos = 0;
			break;

		case Position:
			if (seekpos >= 0) {
				std::make_unsigned_t<decltype(seekpos)> upos = seekpos;
				if (upos <= fsize)
					nextpos = static_cast<decltype(nextpos)>(upos);
			}
			break;

		case Current:
		{
			uintmax_t tpos = fpos + seekpos;
			if (tpos <= fsize)
				nextpos = static_cast<decltype(nextpos)>(tpos);
		}
		break;

		case End:
			nextpos = fsize;
			break;
	}

	return nextpos;
}
