#include "stdafx.h"



CMemoryFile::CMemoryFile()
{
	file_type = ftMemoryFile;
	Clear();
}

CMemoryFile::~CMemoryFile()
{
	Close();
}

// open file, return true on error
int CMemoryFile::Open(std::string_view name, void *buf, size_t size, unsigned int mode)
{
	// close file if it is open
	Close();

	// use storage buffer or reference user supplied buffer
	if (mode & BFFLAG_CREATE) {
		// create new storage
		if (size) {
			// allocate initial buffer with the size specified when opening the file
			AllocStorage(size);

			// copy source buffer content if specified
			if (buf) {
				std::copy_n(static_cast<uint8_t *>(buf), size, file_buf);
				file_count = size;
			}
		}

		// set buffer type
		storage_type = stLocal;
	} else {
		// reference user supplied buffer
		if (size) {
			// error if buffer size is defined, but area is not
			if (!buf)
				return true;
		} else
			buf = nullptr;

		// set user buffer reference
		file_buf = static_cast<uint8_t *>(buf);
		file_size = size;
		file_count = size;

		// set buffer type
		storage_type = stView;
	}

	// rewind the stream position
	file_pos = 0;

	// success
	file_open = true;
	file_mode = mode & BFFLAG_OPEN_MODE;
	file_name = name;

	return false;
}

// open file with read-only buffer supplied, return true on error
int CMemoryFile::Open(std::string_view name, const void *buf, size_t size, unsigned int mode)
{
	// close file if it is open - we have to call close first to make behaviour consistent with the normal Open function
	Close();

	// error if this is a valid (at least 1 byte) read/write view, see flags:
	// - - : read-only view
	// - C : read-only local copy
	// W - : read/write view
	// W C : read/write local copy
	if ((mode & BFFLAG_WRITE) && !(mode & BFFLAG_CREATE) && size)
		return true;

	// this is not a valid read/write view, so it's safe to open the file, it will never be written
	return Open(name, const_cast<void *>(buf), size, mode);
}

// open file when the buffer supplied is nullptr
int CMemoryFile::Open(std::string_view name, std::nullptr_t buf, size_t size, unsigned int mode)
{
	// just delegate to normal Open; it handles nullptr properly
	return Open(name, static_cast<void *>(buf), size, mode);
}

// flush write buffer
void CMemoryFile::Flush() const
{
}

// create a clone object of this file, return nullptr if not possible
std::unique_ptr<CBaseFile> CMemoryFile::Clone(unsigned int mode) const
{
	auto p = std::make_unique<CMemoryFile>();
	if (p->Clone(*this, mode))
		p.reset();

	return p;
}

// open file by cloning the current state of the source file
int CMemoryFile::Clone(const CBaseFile &origin, unsigned int mode)
{
	// close file if it is open in the clone
	Close();

	// the type of the source file must be the same as the type of this file and must be already open
	if (!IsSameType(origin) || !origin.IsOpen())
		return true;

	// make sure to commit the write buffer before cloning
	origin.Flush();

	// we know that the source file is the same type, so it's safe to downcast
	auto &src = static_cast<const CMemoryFile &>(origin);

	// inherit source file open mode or use the new settings
	auto open_mode = (mode & BFFLAG_INHERIT_MODE) ? src.file_mode : mode;

	// open the same file as the source file
	int res = Open(src.file_name, src.file_buf, src.file_count, open_mode);

	// inherit source file position if requested
	if (!res && (mode & BFFLAG_INHERIT_POSITION))
		Seek(src.GetPosition(), Position);

	return res;
}

// close file, return true on error
int CMemoryFile::Close()
{
	// close the active part of the multi-part file
	CloseSelected();

	Clear();

	return false;
}

// free allocated resources, close file
void CMemoryFile::Free()
{
	Close();

	file_storage.clear();
	file_storage.shrink_to_fit();
}

// clear the object state
void CMemoryFile::Clear()
{
	storage_type = stEmpty;
	file_buf = nullptr;
	file_size = 0;
	file_count = 0;
	file_pos = 0;

	ClearBase();
}

// allocate class owned file storage if needed
void CMemoryFile::AllocStorage(size_t max_size)
{
	// the storage can only grow
	if (max_size > file_storage.size())
		file_storage.resize(max_size);

	file_buf = file_storage.data();
	file_size = file_storage.size();
}

// read file, return number of bytes read
size_t CMemoryFile::Read(void *buf, size_t size)
{
	// 0 bytes read if the file is not open or the buffer is invalid
	if (storage_type == stEmpty || !buf || !size)
		return 0;

	// limit readable size to remaining valid content
	if (file_count - file_pos < size)
		size = file_count - file_pos;

	// read from the file and update the stream position
	if (size) {
		std::copy_n(file_buf + file_pos, size, static_cast<uint8_t *>(buf));
		file_pos += size;
	}

	return size;
}

// write file, return number of bytes written
size_t CMemoryFile::Write(const void *buf, size_t size)
{
	// 0 bytes written if file is not open or read-only or the buffer is invalid
	if (storage_type == stEmpty || !(file_mode & BFFLAG_WRITE) || !buf || !size)
		return 0;

	// grow class owned storage if needed
	if (storage_type == stLocal)
		AllocStorage(file_pos + size);

	// limit writeable size to buffer size
	if (file_size - file_pos < size)
		size = file_size - file_pos;

	// write into file, update stream position
	if (size) {
		std::copy_n(static_cast<const uint8_t *>(buf), size, file_buf + file_pos);
		file_pos += size;

		// update valid content count if wrote past the current file size
		if (file_pos > file_count)
			file_count = file_pos;
	}

	return size;
}

// seek in file, return the current position
file_pos_t CMemoryFile::Seek(file_pos_t pos, int mode)
{
	// return 0 if file is not open
	if (storage_type == stEmpty)
		return 0;

	// set the new file position
	file_pos = static_cast<decltype(file_pos)>(CalculateSeekPosition(file_count, file_pos, pos, mode));

	return file_pos;
}

// get file position
file_pos_t CMemoryFile::GetPosition() const
{
	return (storage_type == stEmpty) ? 0 : file_pos;
}

// get the file size
file_size_t CMemoryFile::GetSize() const
{
	return (storage_type == stEmpty) ? 0 : file_count;
}

// get the remaining file size from the current file position
file_size_t CMemoryFile::GetRemainingSize() const
{
	// return 0 if the file is not valid
	if (storage_type == stEmpty)
		return 0;

	// remaining valid content
	return file_count - file_pos;
}

// get the file buffer, read-only
const uint8_t * CMemoryFile::GetBuffer() const
{
	return (storage_type == stEmpty) ? nullptr : file_buf;
}

// get the file buffer if the file is write enabled
uint8_t * CMemoryFile::GetBuffer()
{
	// return nullptr if the file is not open or read-only
	if (storage_type == stEmpty || !(file_mode & BFFLAG_WRITE))
		return nullptr;

	return file_buf;
}

// get the file buffer size
size_t CMemoryFile::GetBufferSize() const
{
	return (storage_type == stEmpty) ? 0 : file_size;
}

// set the valid content size
size_t CMemoryFile::SetSize(size_t size)
{
	// return 0 if the file is not valid
	if (storage_type == stEmpty)
		return 0;

	// grow class owned storage if needed and writing is allowed
	if (storage_type == stLocal && (file_mode & BFFLAG_WRITE))
		AllocStorage(size);

	// limit content size to buffer size
	if (size > file_size)
		size = file_size;

	// set the valid content size
	file_count = size;

	// make sure the current position is within the new content boundaries
	if (file_pos > file_count)
		file_pos = file_count;

	return size;
}
