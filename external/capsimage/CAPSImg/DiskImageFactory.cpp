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
int CDiskImageFactory::GetImageType(const CBaseFile &file)
{
	// error if file is not already open
	if (!file.IsOpen())
		return citError;

	// check IPF or CT Raw
	int type = IsCAPSImage(file);
	if (type != citUnknown)
		return type;

	// check KryoFlux stream cue
	type = IsKFStreamCue(file);
	if (type != citUnknown)
		return type;

	// check KryoFlux stream
	type = IsKFStream(file);
	if (type != citUnknown)
		return type;

	// at this point it's none of the supported image types
	return citUnknown;
}

// return image type if IPF or CT Raw, otherwise Unknown
int CDiskImageFactory::IsCAPSImage(const CBaseFile &file)
{
	// clone the source file as a read-only view, and set it to the same position
	auto pf = file.Clone(BFFLAG_INHERIT_POSITION);
	if (!pf)
		return citError;

	// try to parse CT Raw or IPF
	CCapsLoader cload;
	int res = cload.Lock(std::move(pf));

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
int CDiskImageFactory::IsKFStream(const CBaseFile &file)
{
	C2OOBHdr oob;
	char buf[256];

	// clone the source file as a read-only view, and set it to the same position
	auto pf = file.Clone(BFFLAG_INHERIT_POSITION);
	if (!pf)
		return citError;

	// process the file as long as the data read is an OOB info area
	while (true) {
		// stop if OOB header is longer than the remaining file size
		constexpr auto rlen = sizeof(oob);
		if (pf->GetRemainingSize() < rlen)
			break;

		// read OOB header, stop on error
		if (pf->Read(&oob, rlen) != rlen)
			return citError;

		// check that OOB is Info
		if (oob.sign != c2eOOB || oob.type != c2otInfo)
			break;

		// read Info size value
		uint32_t infosize = oob.size;

		// stop if info size is 0 or larger than remaining file size
		if (!infosize || pf->GetRemainingSize() < infosize || infosize > sizeof(buf))
			break;

		// read Info, stop on error
		if (pf->Read(buf, infosize) != infosize)
			return citError;

		// create a view on the local buffer
		std::string_view buf_view(buf, infosize);

		// success, if KryoFlux text found in Info
		if (buf_view.find("KryoFlux") != std::string_view::npos)
			return citKFStream;
	}

	// file reading stopped somewhere, type is unknown
	return citUnknown;
}

// return image type if KryoFlux stream cue, otherwise Unknown
int CDiskImageFactory::IsKFStreamCue(const CBaseFile &file)
{
	char buf[256];

	// clone the source file as a read-only view, and set it to the same position
	auto pf = file.Clone(BFFLAG_INHERIT_POSITION);
	if (!pf)
		return citError;

	// read up to the remaining file size
	auto readsize = file.GetRemainingSize();

	// limit the read size to the buffer size
	if (readsize > sizeof(buf))
		readsize = sizeof(buf);

	// read from the file, stop on error
	if (pf->Read(buf, static_cast<size_t>(readsize)) != readsize)
		return citError;

	// create a view on the local buffer
	std::string_view buf_view(buf, static_cast<size_t>(readsize));

	// success, if KryoFlux cue text found
	if (buf_view.find(KF_STREAM_CUE_ID) != std::string_view::npos)
		return citKFStreamCue;

	// unknown or unsupported image type
	return citUnknown;
}

// create the class supporting the specified disk image type
std::unique_ptr<CDiskImage> CDiskImageFactory::CreateImage(int diftype)
{
	std::unique_ptr<CDiskImage> pdi;

	// create the image class required
	switch (diftype) {
		case citIPF:
			pdi = std::make_unique<CCapsImageStd>();
			break;

		case citCTRaw:
			pdi = std::make_unique<CCapsImage>();
			break;

		case citKFStream:
			pdi = std::make_unique<CStreamImage>();
			break;

		case citKFStreamCue:
			pdi = std::make_unique<CStreamCueImage>();
			break;
	}

	return pdi;
}

