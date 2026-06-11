#include "stdafx.h"



CDiskFile::CDiskFile()
{
	file_type = ftDiskFile;
	Clear();
}

CDiskFile::~CDiskFile()
{
	Close();
}

// open file, return true on error
int CDiskFile::Open(const char *name, unsigned int mode)
{
	// close file if it is open
	Close();

	// error if name is empty
	if (!name || !strlen(name))
		return true;

	// select open mode
	const char *om;

	if (mode & BFFLAG_WRITE) {
		if (mode & BFFLAG_CREATE)
			om = "w+b";
		else
			om = "r+b";
	} else
		om = "rb";

	// open file, stop on error
	if (!(disk_file = fopen64(name, om)))
		return true;

	// rewind the stream position
	file_count = GetFileSize();
	file_pos = 0;

	// success
	file_open = true;
	file_mode = mode & BFFLAG_OPEN_MODE;
	file_name = name;

	return false;
}

// open file, return true on error
int CDiskFile::Open(std::string_view name, unsigned int mode)
{
	// close file if it is open
	Close();

	// convert the string_view to a string, so it is guaranteed to end with 0 required for fopen
	std::string strname(name);

	// delegate to normal open
	return Open(strname.c_str(), mode);
}

// open the first accessible file from the name list
// return the name index 0...n on the first successful attempt, -1 if all failed
int CDiskFile::OpenAny(const char * const *name, unsigned int mode)
{
	// close file if it is open
	Close();

	// if name list is valid
	if (name) {
		// try each name in order
		for (int pos = 0; name[pos]; pos++) {
			// open the file, return the name index position on success
			if (!Open(name[pos], mode))
				return pos;
		}
	}

	// all attempts failed
	return -1;
}

// open file from any of the path entries listed
// return the path index 0...n on the first successful attempt, -1 if all failed
int CDiskFile::OpenAnyPath(const char * const *path, const char *name, unsigned int mode)
{
	// close file if it is open
	Close();

	// if name and path list are valid
	if (name && path) {
		std::string full_name;

		// try each path entry in order
		for (int pos = 0; path[pos]; pos++) {
			// append name to current path entry
			full_name = path[pos];
			full_name += name;

			// open the file, return the name index position on success
			if (!Open(full_name.c_str(), mode))
				return pos;
		}
	}

	// all attempts failed
	return -1;
}

// flush write buffer
void CDiskFile::Flush() const
{
	// flush if the last operation was write
	if (disk_file && last_op == loWrite)
		fflush(disk_file);
}

// create a clone object of this file, return nullptr if not possible
std::unique_ptr<CBaseFile> CDiskFile::Clone(unsigned int mode) const
{
	auto p = std::make_unique<CDiskFile>();
	if (p->Clone(*this, mode))
		p.reset();

	return p;
}

// open file by cloning the current state of the source file
int CDiskFile::Clone(const CBaseFile &origin, unsigned int mode)
{
	// close file if it is open in the clone
	Close();

	// the type of the source file must be the same as the type of this file and must be already open
	if (!IsSameType(origin) || !origin.IsOpen())
		return true;

	// make sure to commit the write buffer before cloning
	origin.Flush();

	// we know that the source file is the same type, so it's safe to downcast
	auto &src = static_cast<const CDiskFile &>(origin);

	// inherit source file open mode or use the new settings
	auto open_mode = (mode & BFFLAG_INHERIT_MODE) ? src.file_mode : mode;

	// open the same file as the source file
	int res = Open(src.file_name.c_str(), open_mode);

	// inherit source file position if requested
	if (!res && (mode & BFFLAG_INHERIT_POSITION))
		Seek(src.GetPosition(), Position);

	return res;
}

// close file, return true on error
int CDiskFile::Close()
{
	// close the active part of the multi-part file
	CloseSelected();

	int res = false;

	// if file is open, close it and set error
	if (disk_file)
		res = fclose(disk_file) ? true : false;

	// clear settings
	Clear();

	return res;
}

// clear the object state
void CDiskFile::Clear()
{
	disk_file = nullptr;
	last_op = loNone;
	file_count = 0;
	file_pos = 0;

	ClearBase();
}

// internal file size report
file_size_t CDiskFile::GetFileSize()
{
	// default result 
	file_size_t res = 0;

	// return default result if file is not open
	if (!disk_file)
		return res;

	// get the current file position, stop on error
	file_pos_t act_pos = ftello64(disk_file);
	if (act_pos < 0)
		return res;

	// seek to the end of the file, stop on error
	if (fseeko64(disk_file, 0, SEEK_END))
		return res;

	// get the current file position, stop on error
	file_pos_t end_pos = ftello64(disk_file);
	if (end_pos < 0)
		return res;

	// restore file position, stop on error
	if (fseeko64(disk_file, act_pos, SEEK_SET))
		return res;

	return end_pos;
}

// read file, return number of bytes read
size_t CDiskFile::Read(void *buf, size_t size)
{
	// 0 bytes read if the file is not open or the buffer is invalid
	if (!disk_file || !buf || !size)
		return 0;

	// limit readable size to remaining valid content
	if (file_count - file_pos < size)
		size = static_cast<decltype(size)>(file_count - file_pos);

	// read from the file and update the stream position
	if (size) {
		// sync read/write change
		if (last_op != loRead) {
			fseeko64(disk_file, 0, SEEK_CUR);
			last_op = loRead;
		}

		size = fread(buf, 1, size, disk_file);
		file_pos += size;
	}

	return size;
}

// write file, return number of bytes written
size_t CDiskFile::Write(const void *buf, size_t size)
{
	// 0 bytes written if file is not open or read-only or the buffer is invalid
	if (!disk_file || !(file_mode & BFFLAG_WRITE) || !buf || !size)
		return 0;

	// write into file, update stream position
	if (size) {
		// sync read/write change
		if (last_op != loWrite) {
			fseeko64(disk_file, 0, SEEK_CUR);
			last_op = loWrite;
		}

		size = fwrite(buf, 1, size, disk_file);
		file_pos += size;

		// update valid content count if wrote past the current file size
		if (file_pos > file_count)
			file_count = file_pos;
	}

	return size;
}

// seek in file, return the current position
file_pos_t CDiskFile::Seek(file_pos_t pos, int mode)
{
	// delegate to seek with offset 0
	return Seek(pos, mode, 0);
}

// seek in file using offset, return the current position
file_pos_t CDiskFile::Seek(file_pos_t pos, int mode, file_pos_t offset)
{
	// the offset should be at least 0 (can't seek before the start of the file)
	if (offset < 0)
		offset = 0;

	// return 0 with offset if file is not open
	if (!disk_file)
		return offset;

	// get the new realitve file position with the offset (so the lower and upper file boundaries are correct)
	auto next_pos = CalculateSeekPosition(file_count - offset, file_pos - offset, pos, mode);

	// now convert the relative file position to absolute file position
	next_pos += offset;

	// try to seek if the new position is not the same as the current one
	if (next_pos != file_pos) {
		if (fseeko64(disk_file, next_pos, SEEK_SET)) {
			// restore the current file position if the seek failed
			fseeko64(disk_file, file_pos, SEEK_SET);
		}

		// get the current file position, return 0 with offset on error
		pos = ftello64(disk_file);
		if (pos < 0)
			return offset;

		// update the file position
		file_pos = pos;
	}

	return file_pos;
}

// get file position
file_pos_t CDiskFile::GetPosition() const
{
	return (!disk_file) ? 0 : file_pos;
}

// get the file size
file_size_t CDiskFile::GetSize() const
{
	return (!disk_file) ? 0 : file_count;
}

// get the remaining file size from the current file position
file_size_t CDiskFile::GetRemainingSize() const
{
	// return 0 if the file is not valid
	if (!disk_file)
		return 0;

	// remaining valid content
	return file_count - file_pos;
}

