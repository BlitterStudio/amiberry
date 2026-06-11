#ifndef BASEFILE_H
#define BASEFILE_H

// file modes
#define BFFLAG_WRITE  DF_0
#define BFFLAG_CREATE DF_1
#define BFFLAG_OPEN_MODE (BFFLAG_WRITE | BFFLAG_CREATE)
#define BFFLAG_INHERIT_MODE DF_8
#define BFFLAG_INHERIT_POSITION DF_9
#define BFFLAG_INHERIT_MULTI DF_10
#define BFFLAG_CLONE_MODE (BFFLAG_INHERIT_MODE | BFFLAG_INHERIT_POSITION | BFFLAG_INHERIT_MULTI)



// generic file access
class CBaseFile
{
public:
	enum {
		Start = 0,
		Position,
		Current,
		End
	};

	CBaseFile();
	virtual ~CBaseFile();
	virtual int Select(uint32_t id);
	virtual std::string_view GetSegmentName(uint32_t id) const;
	virtual void Flush() const = 0;
	virtual std::unique_ptr<CBaseFile> Clone(unsigned int mode) const = 0;
	virtual int Clone(const CBaseFile &origin, unsigned int mode) = 0;
	virtual int Close() = 0;
	virtual size_t Read(void *buf, size_t size) = 0;
	virtual size_t Write(const void *buf, size_t size) = 0;
	virtual size_t Write(std::string_view text) = 0;
	virtual file_pos_t Seek(file_pos_t pos, int mode) = 0;
	virtual file_pos_t GetPosition() const = 0;
	virtual file_size_t GetSize() const = 0;
	virtual file_size_t GetRemainingSize() const = 0;
	int IsOpen() const;
	unsigned int GetMode() const;
	std::string_view GetName() const;

protected:
	enum {
		ftBaseFile,
		ftDiskFile,
		ftMemoryFile,
		ftMultiDiskFile,
		ftMultiMemoryFile
	};

	virtual void CloseSelected();
	void ClearBase();
	static file_size_t CalculateSeekPosition(file_size_t fsize, file_size_t fpos, file_pos_t seekpos, int mode);
	int IsSameType(const CBaseFile &other) const;

protected:
	int file_type; // class type to avoid using RTTI, double-dispatch etc just for a cloning Open function
	int file_open; // true if file is open
	unsigned int file_mode; // mode used for opening the file
	std::string file_name; // the original filename
};



// return true if file is open
inline int CBaseFile::IsOpen() const
{
	return file_open;
}

// return the mode used for opening the file
inline unsigned int CBaseFile::GetMode() const
{
	return file_mode;
}

// get the file name that was used to successfully open this file
inline std::string_view CBaseFile::GetName() const
{
	return file_name;
}


// return true if the type of the other file matches this type
inline int CBaseFile::IsSameType(const CBaseFile &other) const
{
	return file_type == other.file_type;
}

#endif
