#include <cstring>
#include <cstdlib>

#include "sysdeps.h"

#include "zfile.h"
#include "mp3decoder.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

#define MP3_BLOCK_SIZE 522
#define RAW_BLOCK_SIZE 32768

static int mp3_bitrates[] = {
	0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, -1,
	0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, -1,
	0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, -1,
	0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, -1,
	0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, -1
};
static int mp3_frequencies[] = {
	44100, 48000, 32000, 0,
	22050, 24000, 16000, 0,
	11025, 12000, 8000, 0
};
static int mp3_samplesperframe[] = {
	384, 384, 384,
	1152, 1152, 1152,
	1152, 576, 576
};

mp3decoder::~mp3decoder()
= default;

mp3decoder::mp3decoder()
= default;

uae_u8* mp3decoder::get(struct zfile* zf, uae_u8* outbuf, int maxsize)
{
	write_log(_T("MP3: decoding '%s'..\n"), zfile_getname(zf));

	auto read_proc = [](void* user, void* buffer, size_t bytes_to_read) -> size_t {
		auto* file = static_cast<zfile*>(user);
		return zfile_fread(buffer, 1, bytes_to_read, file);
	};
	auto seek_proc = [](void* user, int offset, drmp3_seek_origin origin) -> drmp3_bool32 {
		auto* file = static_cast<zfile*>(user);
		int whence = SEEK_CUR;
		if (origin == DRMP3_SEEK_SET)
			whence = SEEK_SET;
		else if (origin == DRMP3_SEEK_END)
			whence = SEEK_END;
		return zfile_fseek(file, offset, whence) == 0 ? DRMP3_TRUE : DRMP3_FALSE;
	};
	auto tell_proc = [](void* user, drmp3_int64* cursor) -> drmp3_bool32 {
		if (!cursor)
			return DRMP3_FALSE;
		auto* file = static_cast<zfile*>(user);
		*cursor = (drmp3_int64)zfile_ftell(file);
		return (*cursor >= 0) ? DRMP3_TRUE : DRMP3_FALSE;
	};

	zfile_fseek(zf, 0, SEEK_SET);
	drmp3 mp3{};
	if (!drmp3_init(&mp3, read_proc, seek_proc, tell_proc, nullptr, zf, nullptr)) {
		write_log("MP3: dr_mp3 init failed\n");
		return nullptr;
	}

	if (mp3.channels == 0) {
		drmp3_uninit(&mp3);
		return nullptr;
	}

	const size_t bytes_per_frame = mp3.channels * sizeof(drmp3_int16);
	const drmp3_uint64 max_frames = bytes_per_frame ? (maxsize / bytes_per_frame) : 0;
	const drmp3_uint64 frames_read = drmp3_read_pcm_frames_s16(&mp3, max_frames, (drmp3_int16*)outbuf);
	drmp3_uninit(&mp3);

	if (frames_read == 0)
		return nullptr;
	return outbuf;
}

uae_u32 mp3decoder::getsize(struct zfile* zf)
{
	uae_u32 size;
	int frames, sameframes;
	int firstframe;
	int oldbitrate;
	int timelen = -1;

	firstframe = -1;
	oldbitrate = -1;
	sameframes = -1;
	frames = 0;
	size = 0;
	uae_u8 id3[10];

	if (zfile_fread(id3, sizeof id3, 1, zf) != 1)
		return 0;
	if (id3[0] == 'I' && id3[1] == 'D' && id3[2] == '3' && id3[3] == 3 && id3[4] != 0xff && id3[6] < 0x80 && id3[7] <
		0x80 && id3[8] < 0x80 && id3[9] < 0x80)
	{
		int unsync = id3[5] & 0x80;
		int exthead = id3[5] & 0x40;
		int len = (id3[9] << 0) | (id3[8] << 7) | (id3[7] << 14) | (id3[6] << 21);
		len &= 0x0fffffff;
		uae_u8* tag = xmalloc(uae_u8, len + 1);
		if (zfile_fread(tag, len, 1, zf) != 1)
		{
			xfree(tag);
			return 0;
		}
		uae_u8* p = tag;
		if (exthead)
		{
			int size = (p[4] << 21) | (p[5] << 14) | (p[6] << 7);
			size &= 0x0fffffff;
			p += size;
			len -= size;
		}
		while (len > 0)
		{
			int size = unsync
				           ? (p[4] << 21) | (p[5] << 14) | (p[6] << 7) | (p[7] << 0)
				           : (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | (p[7] << 0);
			size &= 0x0fffffff;
			if (size > len)
				break;
			int compr = p[9] & 0x80;
			int enc = p[9] & 0x40;
			if (compr == 0 && enc == 0)
			{
				if (!memcmp(p, "TLEN", 4))
				{
					uae_u8* data = p + 10;
					data[size] = 0;
					if (data[0] == 0)
						timelen = atol(reinterpret_cast<char*>(data + 1));
					else
						timelen = _tstol(reinterpret_cast<char*>(data + 1));
				}
			}
			size += 10;
			p += size;
			len -= size;
		}
		xfree(tag);
	}
	else
	{
		zfile_fseek(zf, -static_cast<int>(sizeof id3), SEEK_CUR);
	}

	for (;;)
	{
		int ver, layer, bitrate, freq, padding, bitindex, iscrc;
		int samplerate, framelen, bitrateidx, channelmode;
		int isstereo;
		uae_u8 header[4];

		if (zfile_fread(header, sizeof header, 1, zf) != 1)
			return size;
		if (header[0] != 0xff || ((header[1] & (0x80 | 0x40 | 0x20)) != (0x80 | 0x40 | 0x20)))
		{
			zfile_fseek(zf, -3, SEEK_CUR);
			continue;
		}
		if (firstframe < 0)
			firstframe = zfile_ftell32(zf);

		ver = (header[1] >> 3) & 3;
		if (ver == 1)
		{
			write_log(_T("MP3: ver==1?!\n"));
			return 0;
		}
		if (ver == 0)
			ver = 2;
		else if (ver == 2)
			ver = 1;
		else if (ver == 3)
			ver = 0;
		layer = 4 - ((header[1] >> 1) & 3);
		if (layer == 4)
		{
			write_log(_T("MP3: layer==4?!\n"));
			return 0;
		}
		iscrc = ((header[1] >> 0) & 1) ? 0 : 2;
		bitrateidx = (header[2] >> 4) & 15;
		freq = mp3_frequencies[(header[2] >> 2) & 3];
		if (!freq)
		{
			write_log(_T("MP3: reserved frequency?!\n"));
			return 0;
		}
		channelmode = (header[3] >> 6) & 3;
		isstereo = channelmode != 3;
		if (ver == 0)
		{
			bitindex = layer - 1;
		}
		else
		{
			if (layer == 1)
				bitindex = 3;
			else
				bitindex = 4;
		}
		bitrate = mp3_bitrates[bitindex * 16 + bitrateidx] * 1000;
		if (bitrate <= 0)
		{
			write_log(_T("MP3: reserved bitrate?!\n"));
			return 0;
		}
		padding = (header[2] >> 1) & 1;
		samplerate = mp3_samplesperframe[(layer - 1) * 3 + ver];
		framelen = ((samplerate / 8 * bitrate) / freq) + padding;
		if (framelen <= 4)
		{
			write_log(_T("MP3: too small frame size?!\n"));
			return 0;
		}
		zfile_fseek(zf, framelen - 4, SEEK_CUR);
		frames++;
		if (timelen > 0)
		{
			size = (static_cast<uae_u64>(timelen) * freq * 2 * (isstereo ? 2 : 1)) / 1000;
			break;
		}
		size += samplerate * 2 * (isstereo ? 2 : 1);
		if (bitrate != oldbitrate)
		{
			oldbitrate = bitrate;
			sameframes++;
		}
		if (sameframes == 0 && frames > 100)
		{
			size = samplerate * 2 * (isstereo ? 2 : 1) * ((zfile_size32(zf) - firstframe) / ((samplerate / 8 * bitrate) /
				freq));
			break;
		}
	}
	return size;
}
