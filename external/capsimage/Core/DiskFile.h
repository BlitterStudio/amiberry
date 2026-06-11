#ifndef DISKFILE_H
#define DISKFILE_H

// disk based file access
class CDiskFile : public CBaseFile
{
public:
	CDiskFile();
	virtual ~CDiskFile();
	int Open(const char *name, unsigned int mode);
	int Open(std::string_view name, unsigned int mode);
	int OpenAny(const char * const *name, unsigned int mode);
	int OpenAnyPath(const char * const *path, const char *name, unsigned int mode);
	void Flush() const override;
	std::unique_ptr<CBaseFile> Clone(unsigned int mode) const override;
	int Clone(const CBaseFile &origin, unsigned int mode) override;
	int Close() override;
	size_t Read(void *buf, size_t size) override;
	size_t Write(const void *buf, size_t size) override;
	size_t Write(std::string_view text) override;
	file_pos_t Seek(file_pos_t pos, int mode) override;
	file_pos_t GetPosition() const override;
	file_size_t GetSize() const override;
	file_size_t GetRemainingSize() const override;

protected:
	// last file operation
	enum {
		loNone, // there was no read/write on this file yet
		loRead, // last operation was Read
		loWrite // last operation was Write
	};

	void Clear();
	file_size_t GetFileSize();
	file_pos_t Seek(file_pos_t pos, int mode, file_pos_t offset);

protected:
	FILE *disk_file; // the internal file handle
	int last_op; // last operation read or write - required for safe syncing/changing read/write
	file_size_t file_count; // size of file (may be changed by multi-part functions)
	file_size_t file_pos; // file position
};



// write string to file
inline size_t CDiskFile::Write(std::string_view text)
{
	return Write(text.data(), text.size());
}

#endif
