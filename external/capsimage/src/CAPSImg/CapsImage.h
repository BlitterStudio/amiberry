#ifndef CAPSIMAGE_H
#define CAPSIMAGE_H

#define DEF_CTRAW_FDALLOC 64
#define MIN_CTRAW_WEAK_SIZE 5
#define MAX_CTRAW_WEAK_SIZE 16



// CAPS image handler with extended functionality
class CCapsImage : public CCapsImageStd
{
public:
	PCCAPSLOADER GetLoader();

protected:
	int CompareImage();
	int CompareBlock(unsigned blk);
	int DecompressDump();
	void ConvertDumpInfo(PCAPSWH wh);
	int UpdateDump();
	void FindWeakBits();
	void AddWeakBitArea(int group, int bitpos, int size);
	void InitFirstBitTables();

	static int fb_init;
	static int8_t f0b_table[8][256];
	static int8_t f1b_table[8][256];
};

typedef CCapsImage *PCCAPSIMAGE;



// get loader class
inline PCCAPSLOADER CCapsImage::GetLoader()
{
	return &loader;
}

#endif
