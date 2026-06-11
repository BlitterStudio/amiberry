#ifndef MEMORYFILE_H
#define MEMORYFILE_H



// memory based file access/buffer
class CMemoryFile : public CBaseFile
{
public:
	CMemoryFile();
	virtual ~CMemoryFile();
	int Open(std::string_view name, void *buf, size_t size, unsigned int mode);
	int Open(std::string_view name, const void *buf, size_t size, unsigned int mode);
	int Open(std::string_view name, std::nullptr_t buf, size_t size, unsigned int mode);
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
	void Free();
	const uint8_t * GetBuffer() const;
	uint8_t * GetBuffer();
	size_t GetBufferSize() const;
	size_t SetSize(size_t size);

protected:
	// storage type
	enum {
		stEmpty, // there is no storage specified
		stLocal, // local storage to use
		stView   // non-owning view
	};

	void Clear();
	void AllocStorage(size_t max_size);

protected:
	int storage_type; // the current storage method used
	std::vector<uint8_t> file_storage; // the locally owned file storage (not used if this is a view)
	uint8_t *file_buf; // span/view data (but we are not using C++20)
	size_t file_size; // span/view size (but we are not using C++20)
	size_t file_count; // valid data count in buffer
	size_t file_pos; // file position in buffer
};



// write string to file
inline size_t CMemoryFile::Write(std::string_view text)
{
	return Write(text.data(), text.size());
}

#endif
