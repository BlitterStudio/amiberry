#include "stdafx.h"



CDiskImageFactory::CDiskImageFactory()
{
	// dummy objects to initialize static variables
	CCapsImage img;
}

CDiskImageFactory::~CDiskImageFactory()
{
}

// quickly identify the file type; opening the file may still fail if this is a false positive
int CDiskImageFactory::GetImageType(PCAPSFILE pcf)
{
	// error if file cannot be opened
	CCapsFile file;
	if (file.Open(pcf))
		return citError;

	// check IPF or CT Raw
	int type = IsCAPSImage(pcf);
	if (type != citUnknown)
		return type;

	// check KryoFlux stream cue
	type = IsKFStreamCue(pcf);
	if (type != citUnknown)
		return type;

	// check KryoFlux stream
	type = IsKFStream(pcf);
	if (type != citUnknown)
		return type;

	// at this point it's none of the supported image types
	return citUnknown;
}

// return image type if IPF or CT Raw, otherwise Unknown
int CDiskImageFactory::IsCAPSImage(PCAPSFILE pcf)
{
	// try to parse CT Raw or IPF
	CCapsLoader cload;
	int res = cload.Lock(pcf);

	// parser succeeded, check type in more detail
	if (res == CCapsLoader::ccidCaps) {
		for (int run = 1; run;) {
			// read the next chunk after the header
			int type = cload.ReadChunk();

			switch (type) {
				// end of file or error
				case CCapsLoader::ccidEof:
				case CCapsLoader::ccidErrFile:
				case CCapsLoader::ccidErrType:
				case CCapsLoader::ccidErrShort:
				case CCapsLoader::ccidErrHeader:
				case CCapsLoader::ccidErrData:
					run = 0;
					continue;

				// track header is used by CT Raw
				case CCapsLoader::ccidTrck:
					return citCTRaw;

				// track image is used by IPF
				case CCapsLoader::ccidImge:
					return citIPF;

				// ignore other types
				default:
					continue;
			}
		}
	}

	// unknown or unsupported image type
	return citUnknown;
}

// return image type if KryoFlux stream, otherwise Unknown
int CDiskImageFactory::IsKFStream(PCAPSFILE pcf)
{
	// error if file can't be opened
	CCapsFile file;
	if (file.Open(pcf))
		return citError;

	C2OOBHdr oob;
	uint8_t buf[512];

	// remaining file size
	int frem = file.GetSize();

	// process the file as long as the data read is an OOB info area
	while (true) {
		// stop if OOB header is longer than the remaining file size
		int rlen = sizeof(oob);
		if (rlen > frem)
			break;

		// read OOB header, stop on error
		if (file.Read((PUBYTE)&oob, rlen) != rlen)
			return citError;

		// update the remaining file size
		frem -= rlen;

		// check that OOB is Info
		if (oob.sign != c2eOOB || oob.type != c2otInfo)
			break;

		// read info size value as little-endian
		uint8_t *po = (uint8_t *)&oob;
		int so = offsetof(C2OOBHdr, size);
		int infosize = po[so+1] << 8 | po[so];

		// stop if info size is 0 or larger than remaining file size or larger than the buffer
		if (!infosize || infosize > frem || infosize > sizeof(buf))
			break;
		
		// read info, stop on error
		if (file.Read(buf, infosize) != infosize)
			return citError;

		// update the remaining file size
		frem -= infosize;

		// success, if KryoFlux text found in info
		if (strstr((char *)buf, "KryoFlux"))
			return citKFStream;
	}

	// file reading stopped somewhere, type is unknown
	return citUnknown;
}

// return image type if KryoFlux stream cue, otherwise Unknown
int CDiskImageFactory::IsKFStreamCue(PCAPSFILE pcf)
{
	// error if file can't be opened
	CCapsFile file;
	if (file.Open(pcf))
		return citError;

	// the ID string must be within this area from the beginning of the file
	uint8_t buf[256];

	// remaining file size
	int frem = file.GetSize();

	// buffer allowed, with room for terminating 0
	int bufmax = sizeof(buf)-1;

	int readsize = min(frem, bufmax);

	// read from the beginning of the file, stop on error
	if (file.Read(buf, readsize) != readsize)
		return citError;

	// add terminating 0
	buf[readsize] = 0;

	// success, if ID text found in info
	if (strstr((char *)buf, KF_STREAM_CUE_ID))
		return citKFStreamCue;

	// unknown or unsupported image type
	return citUnknown;
}

// create the class supporting the specified disk image type
PCDISKIMAGE CDiskImageFactory::CreateImage(int diftype)
{
	PCDISKIMAGE pdi = NULL;

	// create the image class required
	switch (diftype) {
		case citIPF:
			pdi = new CCapsImageStd;
			break;

		case citCTRaw:
			pdi = new CCapsImage;
			break;

		case citKFStream:
			pdi = new CStreamImage;
			break;

		case citKFStreamCue:
			pdi = new CStreamCueImage;
			break;
	}

	return pdi;
}

