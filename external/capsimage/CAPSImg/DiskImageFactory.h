#ifndef DISKIMAGEFACTORY_H
#define DISKIMAGEFACTORY_H



// disk image factory identifying and creating the correct image type
class CDiskImageFactory
{
public:
	CDiskImageFactory();
	virtual ~CDiskImageFactory();
	static int GetImageType(const CBaseFile &file);
	static std::unique_ptr<CDiskImage> CreateImage(int diftype);

protected:
	static int IsCAPSImage(const CBaseFile &file);
	static int IsKFStream(const CBaseFile &file);
	static int IsKFStreamCue(const CBaseFile &file);
};

typedef CDiskImageFactory *PCDISKIMAGEFACTORY;



#endif
