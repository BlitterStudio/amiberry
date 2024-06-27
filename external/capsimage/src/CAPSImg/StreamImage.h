#ifndef STREAMIMAGE_H
#define STREAMIMAGE_H

// KryoFlux stream image handler
class CStreamImage : public CDiskImage
{
public:
	CStreamImage();
	virtual ~CStreamImage();
	int Lock(PCAPSFILE pcf);
	int Unlock();

protected:
	int LoadTrack(PDISKTRACKINFO pti, UDWORD flag);
};

typedef CStreamImage *PCSTREAMIMAGE;

#endif
