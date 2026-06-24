#ifndef STREAMCUEIMAGE_H
#define STREAMCUEIMAGE_H

// KryoFlux stream ID string
#define KF_STREAM_CUE_ID "<KryoFlux_Stream_Cue/>"



// KryoFlux stream image handler
class CStreamCueImage : public CStreamImage
{
public:
	CStreamCueImage();
	virtual ~CStreamCueImage();
	int Lock(std::unique_ptr<CBaseFile> pf) override;
	int Unlock() override;
};

typedef CStreamCueImage *PCSTREAMCUEIMAGE;

#endif
