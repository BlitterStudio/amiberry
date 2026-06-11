#ifndef STREAMIMAGE_H
#define STREAMIMAGE_H

// KryoFlux stream image handler
class CStreamImage : public CDiskImage
{
public:
	CStreamImage();
	virtual ~CStreamImage();
	int Lock(std::unique_ptr<CBaseFile> pf) override;
	int Unlock() override;

protected:
	int LoadTrack(PDISKTRACKINFO pti, uint32_t flag) override;
};

typedef CStreamImage *PCSTREAMIMAGE;

#endif
