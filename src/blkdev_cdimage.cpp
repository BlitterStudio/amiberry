/*
* UAE
*
* CD image file support
*
* - iso (2048/2352 block size)
* - cue/bin, cue/bin/wav, cue/bin/mp3, cue/bin/flac
* - ccd/img and ccd/img/sub
* - chd cd
*
* Copyright 2010-2013 Toni Wilen
*
*/
#include "sysconfig.h"
#include "sysdeps.h"

#include <sys/timeb.h>

#include "options.h"
#include "blkdev.h"
#include "zfile.h"
#include "gui.h"
#include "fsdb.h"
#include "threaddep/thread.h"
#include "scsidev.h"
#include "mp3decoder.h"
#include "cda_play.h"
#include "include/memory.h"
#ifdef RETROPLATFORM
#include "rp.h"
#endif

#define FLAC__NO_DLL
#include "FLAC/stream_decoder.h"

#ifdef WITH_CHD
#include "archivers/chd/chdtypes.h"
#include "archivers/chd/chd.h"
#include "archivers/chd/chdcd.h"
#endif

#define scsi_log write_log

#define CDDA_BUFFERS 12

enum audenc { AUDENC_NONE, AUDENC_PCM, AUDENC_MP3, AUDENC_FLAC, ENC_CHD };

struct cdtoc
{
	struct zfile *handle;
	uae_s64 offset;
	uae_u8 *data;
	struct zfile *subhandle;
	int suboffset;
	uae_u8 *subdata;

	uae_s64 filesize;
	TCHAR *fname;
	TCHAR *extrainfo;
	int address;
	uae_u8 adr, ctrl;
	int track;
	int size;
	int skipsize; // bytes to skip after each block
	int index1; // distance between index0 and index1
	int pregap; // sectors of silence
	int postgap; // sectors of silence
	audenc enctype;
	int writeoffset;
	int subcode;
#ifdef WITH_CHD
	const cdrom_track_info *chdtrack;
#endif
};

struct cdunit {
	bool enabled;
	bool open;
	uae_u8 buffer[2352];
	struct cdtoc toc[102];
	int tracks;
	uae_u64 cdsize;
	int blocksize;

	int cdda_play_state;
	int cdda_play;
	int cdda_paused;
	int cdda_volume[2];
	int cdda_scan;
	int cd_last_pos;
	int cdda_start, cdda_end;
	play_subchannel_callback cdda_subfunc;
	play_status_callback cdda_statusfunc;
	int cdda_delay, cdda_delay_frames;
	bool thread_active;

	TCHAR imgname[MAX_DPATH];
	uae_sem_t sub_sem;
	struct device_info di;
#ifdef WITH_CHD
	chd_file *chd_f;
	cdrom_file *chd_cdf;
#endif
};

static struct cdunit cdunits[MAX_TOTAL_SCSI_DEVICES];
static int bus_open;

static volatile int cdimage_unpack_thread, cdimage_unpack_active;
static smp_comm_pipe unpack_pipe;

static struct cdunit *unitisopen (int unitnum)
{
	struct cdunit *cdu = &cdunits[unitnum];
	if (cdu->open)
		return cdu;
	return NULL;
}


static struct cdtoc *findtoc (struct cdunit *cdu, int *sectorp)
{
	int i;
	int sector;

	if (*sectorp < 0)
		return NULL;
	sector = *sectorp;
	for (i = 0; i <= cdu->tracks; i++) {
		struct cdtoc *t = &cdu->toc[i];
		if (t->address - t->index1 > sector) {
			if (i == 0) {
				*sectorp = 0;
				return t;
			}
			t--;
			sector -= t->address - t->index1;
			if (sector < t->pregap)
				return NULL; // pregap silence
			*sectorp = sector;
			return t;
		}
	}
	return NULL;
}

static int do_read (struct cdunit *cdu, struct cdtoc *t, uae_u8 *data, int sector, int offset, int size)
{
	if (t->enctype == ENC_CHD) {
#ifdef WITH_CHD
		return read_partial_sector(cdu->chd_cdf, data, sector + t->offset, 0, offset, size) == CHDERR_NONE;
#endif
	} else if (t->handle) {
		int ssize = t->size + t->skipsize;
		zfile_fseek (t->handle, t->offset + (uae_u64)sector * ssize + offset, SEEK_SET);
		return zfile_fread (data, 1, size, t->handle) == size;
	}
	return 0;
}

// WOHOO, library that supports virtual file access functions. Perfect!
static void flac_metadata_callback (const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
	struct cdtoc *t = (struct cdtoc*)client_data;
	if (t->data)
		return;
	if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		t->filesize = metadata->data.stream_info.total_samples * (metadata->data.stream_info.bits_per_sample / 8) * metadata->data.stream_info.channels;
	}
}
static void flac_error_callback (const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
	return;
}
static FLAC__StreamDecoderWriteStatus flac_write_callback (const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
	struct cdtoc *t = (struct cdtoc*)client_data;
	uae_u16 *p = (uae_u16*)(t->data + t->writeoffset);
	int size = 4;
	for (int i = 0; i < frame->header.blocksize && t->writeoffset < t->filesize - size; i++, t->writeoffset += size) {
		*p++ = (FLAC__int16)buffer[0][i];
		*p++ = (FLAC__int16)buffer[1][i];
	}
	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}
static FLAC__StreamDecoderReadStatus file_read_callback (const FLAC__StreamDecoder *decoder, FLAC__byte buffer[], size_t *bytes, void *client_data)
{
	struct cdtoc *t = (struct cdtoc*)client_data;
	if (zfile_ftell (t->handle) >= zfile_size (t->handle))
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	return zfile_fread (buffer, *bytes, 1, t->handle) ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE : FLAC__STREAM_DECODER_READ_STATUS_ABORT;
}
static FLAC__StreamDecoderSeekStatus file_seek_callback (const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data)
{
	struct cdtoc *t = (struct cdtoc*)client_data;
	zfile_fseek (t->handle, absolute_byte_offset, SEEK_SET);
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
}
static FLAC__StreamDecoderTellStatus file_tell_callback (const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data)
{
	struct cdtoc *t = (struct cdtoc*)client_data;
	*absolute_byte_offset = zfile_ftell (t->handle);
	return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}
static FLAC__StreamDecoderLengthStatus file_len_callback (const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data)
{
	struct cdtoc *t = (struct cdtoc*)client_data;
	*stream_length = zfile_size (t->handle);
	return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}
static FLAC__bool file_eof_callback (const FLAC__StreamDecoder *decoder, void *client_data)
{
	struct cdtoc *t = (struct cdtoc*)client_data;
	return zfile_ftell (t->handle) >= zfile_size (t->handle);
}

static void flac_get_size (struct cdtoc *t)
{
	FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new ();
	if (decoder) {
		FLAC__stream_decoder_set_md5_checking (decoder, false);
		int init_status = FLAC__stream_decoder_init_stream (decoder,
			&file_read_callback, &file_seek_callback, &file_tell_callback,
			&file_len_callback, &file_eof_callback,
			&flac_write_callback, &flac_metadata_callback, &flac_error_callback, t);
		FLAC__stream_decoder_process_until_end_of_metadata (decoder);
		FLAC__stream_decoder_delete (decoder);
	}
}
static uae_u8 *flac_get_data (struct cdtoc *t)
{
	write_log (_T("FLAC: unpacking '%s'..\n"), zfile_getname (t->handle));
	t->writeoffset = 0;
	FLAC__StreamDecoder *decoder = FLAC__stream_decoder_new ();
	if (decoder) {
		FLAC__stream_decoder_set_md5_checking (decoder, false);
		int init_status = FLAC__stream_decoder_init_stream (decoder,
			&file_read_callback, &file_seek_callback, &file_tell_callback,
			&file_len_callback, &file_eof_callback,
			&flac_write_callback, &flac_metadata_callback, &flac_error_callback, t);
		FLAC__stream_decoder_process_until_end_of_stream (decoder);
		FLAC__stream_decoder_delete (decoder);
		write_log (_T("FLAC: %s unpacked\n"), zfile_getname (t->handle));
	}
	return t->data;
}

void sub_to_interleaved (const uae_u8 *s, uae_u8 *d)
{
	for (int i = 0; i < 8 * 12; i ++) {
		int dmask = 0x80;
		int smask = 1 << (7 - (i & 7));
		(*d) = 0;
		for (int j = 0; j < 8; j++) {
			(*d) |= (s[(i / 8) + j * 12] & smask) ? dmask : 0;
			dmask >>= 1;
		}
		d++;
	}
}
void sub_to_deinterleaved (const uae_u8 *s, uae_u8 *d)
{
	for (int i = 0; i < 8 * 12; i ++) {
		int dmask = 0x80;
		int smask = 1 << (7 - (i / 12));
		(*d) = 0;
		for (int j = 0; j < 8; j++) {
			(*d) |= (s[(i % 12) * 8 + j] & smask) ? dmask : 0;
			dmask >>= 1;
		}
		d++;
	}
}

static int getsub_deinterleaved (uae_u8 *dst, struct cdunit *cdu, struct cdtoc *t, int sector)
{
	int ret = 0;
	uae_sem_wait (&cdu->sub_sem);
	if (t->subcode) {
		if (t->enctype == ENC_CHD) {
#ifdef WITH_CHD
			const cdrom_track_info *cti = t->chdtrack;
			ret = do_read (cdu, t, dst, sector, cti->datasize, cti->subsize);
			if (ret)
				ret = t->subcode;
#endif
		} else if (t->subhandle) {
			int offset = 0;
			int totalsize = SUB_CHANNEL_SIZE;
			if (t->skipsize) {
				totalsize += t->size;
				offset = t->size;
			}
			zfile_fseek (t->subhandle, (uae_u64)sector * totalsize + t->suboffset + offset, SEEK_SET);
			if (zfile_fread (dst, SUB_CHANNEL_SIZE, 1, t->subhandle) > 0)
				ret = t->subcode;
		} else {
			memcpy (dst, t->subdata + sector * SUB_CHANNEL_SIZE + t->suboffset, SUB_CHANNEL_SIZE);
			ret = t->subcode;
		}
	}
	if (!ret) {
		memset (dst, 0, SUB_CHANNEL_SIZE);
		// regenerate Q-subchannel
		uae_u8 *s = dst + 12;
		s[0] = (t->ctrl << 4) | (t->adr << 0);
		s[1] = tobcd (t - &cdu->toc[0] + 1);
		s[2] = tobcd (1);
		int msf = lsn2msf (sector);
		tolongbcd (s + 7, msf);
		msf = lsn2msf (sector - t->address - 150);
		tolongbcd (s + 3, msf);
		ret = 2;
	}
	if (ret == 1) {
		uae_u8 tmp[SUB_CHANNEL_SIZE];
		memcpy (tmp, dst, SUB_CHANNEL_SIZE);
		sub_to_deinterleaved (tmp, dst);
		ret = 2;
	}
	uae_sem_post (&cdu->sub_sem);
	return ret;
}

static void dosub (struct cdunit *cdu, uae_u8 *subbuf)
{
	uae_u8 subbuf2[SUB_CHANNEL_SIZE];

	if (!cdu->cdda_subfunc)
		return;

	if (!subbuf) {
		memset (subbuf2, 0, sizeof subbuf2);
		cdu->cdda_subfunc (subbuf2, 1);
		return;
	}
	sub_to_interleaved (subbuf, subbuf2);
	cdu->cdda_subfunc (subbuf2, 1);
}

static int setstate (struct cdunit *cdu, int state)
{
	cdu->cdda_play_state = state;
	if (cdu->cdda_statusfunc)
		return cdu->cdda_statusfunc (cdu->cdda_play_state);
	return 0;
}

static void *cdda_unpack_func (void *v)
{
	cdimage_unpack_thread = 1;
	mp3decoder *mp3dec = NULL;

	for (;;) {
		uae_u32 cduidx = read_comm_pipe_u32_blocking (&unpack_pipe);
		if (cdimage_unpack_thread == 0)
			break;
		uae_u32 tocidx = read_comm_pipe_u32_blocking (&unpack_pipe);
		struct cdunit *cdu = &cdunits[cduidx];
		struct cdtoc *t = &cdu->toc[tocidx];
		if (t->handle) {
			// force unpack if handle points to delayed zipped file
			uae_s64 pos = zfile_ftell (t->handle);
			zfile_fseek (t->handle, -1, SEEK_END);
			uae_u8 b;
			zfile_fread (&b, 1, 1, t->handle);
			zfile_fseek (t->handle, pos, SEEK_SET);
			if (!t->data && (t->enctype == AUDENC_MP3 || t->enctype == AUDENC_FLAC)) {
				t->data = xcalloc (uae_u8, t->filesize + 2352);
				cdimage_unpack_active = 1;
				if (t->data) {
					if (t->enctype == AUDENC_MP3) {
						if (!mp3dec) {
							try {
								mp3dec = new mp3decoder();
							} catch (exception) { };
						}
						if (mp3dec)
							t->data = mp3dec->get (t->handle, t->data, t->filesize);
					} else if (t->enctype == AUDENC_FLAC) {
						flac_get_data (t);
					}
				}
			}
		}
		cdimage_unpack_active = 2;
	}
	delete mp3dec;
	cdimage_unpack_thread = -1;
	return 0;
}

static void audio_unpack (struct cdunit *cdu, struct cdtoc *t)
{
	// do this even if audio is not compressed, t->handle also could be
	// compressed and we want to unpack it in background too
	while (cdimage_unpack_active == 1)
		Sleep (10);
	cdimage_unpack_active = 0;
	write_comm_pipe_u32 (&unpack_pipe, cdu - &cdunits[0], 0);
	write_comm_pipe_u32 (&unpack_pipe, t - &cdu->toc[0], 1);
	while (cdimage_unpack_active == 0)
		Sleep (10);
}

static void *cdda_play_func (void *v)
{
	int cdda_pos;
	int num_sectors = CDDA_BUFFERS;
	int bufnum;
	int bufon[2];
	int oldplay;
	int idleframes = 0;
	int silentframes = 0;
	bool foundsub;
	struct cdunit *cdu = (struct cdunit*)v;
	int oldtrack = -1;

	cdu->thread_active = true;

	while (cdu->cdda_play == 0)
		Sleep (10);
	oldplay = -1;

	bufon[0] = bufon[1] = 0;
	bufnum = 0;

	cda_audio *cda = new cda_audio (num_sectors);

	while (cdu->cdda_play > 0) {

		if (oldplay != cdu->cdda_play) {
			struct cdtoc *t;
			int sector, diff;
			struct timeb tb1, tb2;

			idleframes = 0;
			silentframes = 0;
			foundsub = false;
			_ftime (&tb1);
			cdda_pos = cdu->cdda_start;
			oldplay = cdu->cdda_play;
			sector = cdu->cd_last_pos = cdda_pos;
			t = findtoc (cdu, &sector);
			if (!t) {
				sector = cdu->cd_last_pos = cdda_pos + 2 * 75;
				t = findtoc (cdu, &sector);
				if (!t) {
				  write_log (_T("IMAGE CDDA: illegal sector number %d\n"), cdu->cdda_start);
				  setstate (cdu, AUDIO_STATUS_PLAY_ERROR);
			  } else {
					audio_unpack (cdu, t);
				}
			} else {
				write_log (_T("IMAGE CDDA: playing from %d to %d, track %d ('%s', offset %lld, secoffset %d (%d))\n"),
					cdu->cdda_start, cdu->cdda_end, t->track, t->fname, t->offset, sector, t->index1);
				oldtrack = t->track;
				audio_unpack (cdu, t);
			}
			idleframes = cdu->cdda_delay_frames;
			while (cdu->cdda_paused && cdu->cdda_play > 0) {
				Sleep (10);
				idleframes = -1;
			}

			if (cdu->cdda_scan == 0) {
				// find possible P-subchannel=1 and fudge starting point so that
				// buggy CD32/CDTV software CD+G handling does not miss any frames
				bool seenindex = false;
				for (sector = cdda_pos - 200; sector < cdda_pos; sector++) {
					int sec = sector;
					t = findtoc (cdu, &sec);
					if (t) {
						uae_u8 subbuf[SUB_CHANNEL_SIZE];
						getsub_deinterleaved (subbuf, cdu, t, sector);
						if (seenindex) {
							for (int i = 2 * SUB_ENTRY_SIZE; i < SUB_CHANNEL_SIZE; i++) {
								if (subbuf[i]) { // non-zero R-W subchannels
									int diff = cdda_pos - sector + 2;
									write_log (_T("-> CD+G start pos fudge -> %d (%d)\n"), sector, -diff);
									idleframes -= diff;
									cdda_pos = sector;
									break;
								}
							}
						} else if (subbuf[0] == 0xff) { // P == 1?
							seenindex = true;
						}
					}
				}
			}
			cdda_pos -= idleframes;

			_ftime (&tb2);
			diff = (tb2.time * (uae_s64)1000 + tb2.millitm) - (tb1.time * (uae_s64)1000 + tb1.millitm);
			diff -= cdu->cdda_delay;
			if (idleframes >= 0 && diff < 0 && cdu->cdda_play > 0)
				Sleep (-diff);
			setstate (cdu, AUDIO_STATUS_IN_PROGRESS);

			sector = cdda_pos;
			struct cdtoc *t1 = findtoc (cdu, &sector);
			int tsector = cdda_pos + 2 * 75;
			struct cdtoc *t2 = findtoc (cdu, &tsector);
			if (t1 != t2) {
				for (sector = cdda_pos; sector < cdda_pos + 2 * 75; sector++) {
					int sec = sector;
					t = findtoc (cdu, &sec);
					if (t == t2)
						break;
					silentframes++;
				}
			}
		}

		cda->wait(bufnum);
		bufon[bufnum] = 0;
		if (!cdu->cdda_play)
			goto end;

		if (idleframes <= 0 && cdda_pos >= cdu->cdda_start && !isaudiotrack (&cdu->di.toc, cdda_pos)) {
			setstate (cdu, AUDIO_STATUS_PLAY_ERROR);
			write_log (_T("IMAGE CDDA: attempted to play data track %d\n"), cdda_pos);
			goto end; // data track?
		}

		if ((cdda_pos < cdu->cdda_end || cdu->cdda_end == 0xffffffff) && !cdu->cdda_paused && cdu->cdda_play > 0) {
			struct cdtoc *t;
			int sector, cnt;
			int dofinish = 0;

			gui_flicker_led (LED_CD, cdu->di.unitnum - 1, LED_CD_AUDIO);

			memset (cda->buffers[bufnum], 0, num_sectors * 2352);

			for (cnt = 0; cnt < num_sectors && cdu->cdda_play > 0; cnt++) {
				uae_u8 *dst = cda->buffers[bufnum] + cnt * 2352;
				uae_u8 subbuf[SUB_CHANNEL_SIZE];
				sector = cdda_pos;

				memset (subbuf, 0, SUB_CHANNEL_SIZE);

				t = findtoc (cdu, &sector);
				if (t) {
					if (t->track != oldtrack) {
						oldtrack = t->track;
						write_log (_T("IMAGE CDDA: track %d ('%s', offset %lld, secoffset %d (%d))\n"),
							t->track, t->fname, t->offset, sector, t->index1);
						audio_unpack (cdu, t);
					}
					if (!(t->ctrl & 4)) {
						if (t->enctype == ENC_CHD) {
#ifdef WITH_CHD
							do_read (cdu, t, dst, sector, 0, t->size);
							for (int i = 0; i < 2352; i+=2) {
								uae_u8 p;
								p = dst[i + 0];
								dst[i + 0] = dst[i + 1];
								dst[i +1] = p;
							}
#endif
						} else if (t->handle) {
						  int totalsize = t->size + t->skipsize;
							int offset = t->offset;
							if (offset >= 0) {
  						  if ((t->enctype == AUDENC_MP3 || t->enctype == AUDENC_FLAC) && t->data) {
									if (t->filesize >= sector * totalsize + offset + t->size)
										memcpy (dst, t->data + sector * totalsize + offset, t->size);
						    } else if (t->enctype == AUDENC_PCM) {
									if (sector * totalsize + offset + totalsize < t->filesize) {
										zfile_fseek (t->handle, (uae_u64)sector * totalsize + offset, SEEK_SET);
								    zfile_fread (dst, t->size, 1, t->handle);
									}
							  }
						  }
            }
					}
					getsub_deinterleaved (subbuf, cdu, t, cdda_pos);
				}

				if (idleframes > 0 || silentframes > 0) {
				  if (idleframes > 0) {
					  idleframes--;
					  memset (subbuf, 0, SUB_CHANNEL_SIZE);
					}
					if (silentframes > 0)
						silentframes--;
					memset (dst, 0, 2352);
				}

				if (cdda_pos < cdu->cdda_start && cdu->cdda_scan == 0)
					memset (dst, 0, 2352);

				dosub (cdu, subbuf);

				if (cdu->cdda_scan) {
					cdda_pos += cdu->cdda_scan;
					if (cdda_pos < 0)
						cdda_pos = 0;
				} else  {
					cdda_pos++;
				}

				if (cdda_pos - num_sectors < cdu->cdda_end && cdda_pos >= cdu->cdda_end)
					dofinish = 1;

			}
	
			if (idleframes <= 0)
				cdu->cd_last_pos = cdda_pos;

			bufon[bufnum] = 1;
			cda->setvolume (currprefs.sound_volume_cd, cdu->cdda_volume[0], cdu->cdda_volume[1]);
			if (!cda->play (bufnum)) {
				if (cdu->cdda_play > 0)
				  setstate (cdu, AUDIO_STATUS_PLAY_ERROR);
				goto end;
			}

			if (dofinish) {
				if (cdu->cdda_play >= 0)
				  setstate (cdu, AUDIO_STATUS_PLAY_COMPLETE);
				cdu->cdda_play = -1;
				cdda_pos = cdu->cdda_end + 1;
			}

		}

		if (bufon[0] == 0 && bufon[1] == 0) {
			while (cdu->cdda_paused && cdu->cdda_play == oldplay)
				Sleep (10);
		}

		bufnum = 1 - bufnum;
	}

end:
	cda->wait (0);
	cda->wait (1);

	while (cdimage_unpack_active == 1)
		Sleep (10);

	delete cda;

	cdu->cdda_play = 0;
	write_log (_T("IMAGE CDDA: thread killed\n"));
	cdu->thread_active = false;
	return NULL;
}


static void cdda_stop (struct cdunit *cdu)
{
	if (cdu->cdda_play != 0) {
		cdu->cdda_play = -1;
		while (cdu->cdda_play && cdu->thread_active) {
			Sleep (10);
		}
		cdu->cdda_play = 0;
	}
	cdu->cdda_paused = 0;
	cdu->cdda_play_state = 0;
}


static int command_pause (int unitnum, int paused)
{
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return -1;
	int old = cdu->cdda_paused;
	if ((paused && cdu->cdda_play) || !paused)
	  cdu->cdda_paused = paused;
	return old;
}

static int command_stop (int unitnum)
{
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return 0;
	cdda_stop (cdu);
	return 1;
}

static int command_play (int unitnum, int startlsn, int endlsn, int scan, play_status_callback statusfunc, play_subchannel_callback subfunc)
{
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return 0;
	if (cdu->cdda_play) {
		cdu->cdda_play = -1;
		while (cdu->thread_active)
			Sleep (10);
		cdu->cdda_play = 0;
	}
	cdu->cd_last_pos = startlsn;
	cdu->cdda_start = startlsn;
	cdu->cdda_end = endlsn;
	cdu->cdda_subfunc = subfunc;
	cdu->cdda_statusfunc = statusfunc;
	cdu->cdda_scan = scan > 0 ? 10 : (scan < 0 ? 10 : 0);
	cdu->cdda_delay = setstate (cdu, -1);
	cdu->cdda_delay_frames = setstate (cdu, -2);
	setstate (cdu, AUDIO_STATUS_NOT_SUPPORTED);
	if (!isaudiotrack (&cdu->di.toc, startlsn)) {
		setstate (cdu, AUDIO_STATUS_PLAY_ERROR);
		return 0;
	}
	if (!cdu->thread_active) {
		uae_start_thread (_T("cdimage_cdda_play"), cdda_play_func, cdu, NULL);
		while (!cdu->thread_active)
			Sleep (10);
	}
	cdu->cdda_play++;
	return 1;
}

static int command_qcode (int unitnum, uae_u8 *buf, int sector)
{
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return 0;

	uae_u8 subbuf[SUB_CHANNEL_SIZE];
	uae_u8 *p;
	int trk;
	int pos;
	int status;

	memset (buf, 0, SUBQ_SIZE);
	p = buf;

	status = cdu->cdda_play_state;
	if (cdu->cdda_play > 0 && cdu->cdda_paused)
		status = AUDIO_STATUS_PAUSED;

	if (sector < 0)
		pos = cdu->cd_last_pos;
	else
		pos = sector;

	p[1] = status;
	p[3] = 12;

	p = buf + 4;

	struct cdtoc *td = NULL;
	for (trk = 0; trk <= cdu->tracks; trk++) {
		td = &cdu->toc[trk];
		if (pos < td->address) {
			if (trk > 0)
				td--;
			break;
		}
		if (pos >= td->address && pos < td[1].address)
			break;
	}
	if (!td)
		return 0;
	getsub_deinterleaved (subbuf, cdu, td, pos);
	memcpy (p, subbuf + 12, 12);
//	write_log (_T("%6d %02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x.%02x\n"),
//		pos, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11]);
	return 1;
}

static uae_u32 command_volume (int unitnum, uae_u16 volume_left, uae_u16 volume_right)
{
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return -1;
	uae_u32 old = (cdu->cdda_volume[1] << 16) | (cdu->cdda_volume[0] << 0);
	cdu->cdda_volume[0] = volume_left;
	cdu->cdda_volume[1] = volume_right;
	return old;
}

extern void encode_l2 (uae_u8 *p, int address);

static int command_rawread (int unitnum, uae_u8 *data, int sector, int size, int sectorsize, uae_u32 extra)
{
	int ret = 0;
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return 0;
	int asector = sector;
	struct cdtoc *t = findtoc (cdu, &sector);
	int ssize = t->size + t->skipsize;

	if (!t)
		goto end;

	cdda_stop (cdu);
	if (sectorsize > 0) {
		if (sectorsize == 2352 && t->size == 2048) {
			// 2048 -> 2352
			while (size-- > 0) {
				memset (data, 0, 16);
				do_read (cdu, t, data + 16, sector, 0, 2048);
				encode_l2 (data, sector + 150);
				sector++;
				asector++;
				data += sectorsize;
				ret += sectorsize;
			}
		} else if (sectorsize == 2048 && t->size == 2352) {
			// 2352 -> 2048
			while (size-- > 0) {
				uae_u8 b = 0;
				do_read (cdu, t, &b, sector, 15, 1);
				do_read (cdu, t, data, sector, b == 2 ? 24 : 16, sectorsize);
				sector++;
				asector++;
				data += sectorsize;
				ret += sectorsize;
			}
		} else if (sectorsize == 2336 && t->size == 2352) {
			// 2352 -> 2336
			while (size-- > 0) {
				uae_u8 b = 0;
				do_read (cdu, t, &b, sector, 15, 1);
				if (b != 2 && b != 0) // MODE0 or MODE2 only allowed
					return 0; 
				do_read (cdu, t, data, sector, 16, sectorsize);
				sector++;
				asector++;
				data += sectorsize;
				ret += sectorsize;
			}
		} else if (sectorsize == t->size) {
			// no change
			while (size -- > 0) {
				do_read (cdu, t, data, sector, 0, sectorsize);
				sector++;
				asector++;
				data += sectorsize;
				ret++;
			}
		}
		cdu->cd_last_pos = asector;

	} else {

		uae_u8 sectortype = extra >> 16;
		uae_u8 cmd9 = extra >> 8;
		int sync = (cmd9 >> 7) & 1;
		int headercodes = (cmd9 >> 5) & 3;
		int userdata = (cmd9 >> 4) & 1;
		int edcecc = (cmd9 >> 3) & 1;
		int errorfield = (cmd9 >> 1) & 3;
		uae_u8 subs = extra & 7;
		if (subs != 0 && subs != 1 && subs != 2 && subs != 4) {
			ret = -1;
			goto end;
		}

		if (isaudiotrack (&cdu->di.toc, sector)) {
			if (sectortype != 0 && sectortype != 1) {
				ret = -2;
				goto end;
			}
			if (t->size != 2352) {
				ret = -1;
				goto end;
			}
			for (int i = 0; i < size; i++) {
				do_read (cdu, t, data, sector, 0, t->size);
				uae_u8 *p = data + t->size;
				if (subs) {
					uae_u8 subdata[SUB_CHANNEL_SIZE];
					getsub_deinterleaved (subdata, cdu, t, sector);
					if (subs == 4) { // all, de-interleaved
						memcpy (p, subdata, SUB_CHANNEL_SIZE);
						p += SUB_CHANNEL_SIZE;
					} else if (subs == 2) { // q-only
						memcpy (p, subdata + SUB_ENTRY_SIZE, SUB_ENTRY_SIZE);
						p += SUB_ENTRY_SIZE;
					} else if (subs == 1) { // all, interleaved
						sub_to_interleaved (subdata, p);
						p += SUB_CHANNEL_SIZE;
					}
				}
				ret += p - data;
				data = p;
				sector++;
			}
		}
	}
end:
	return ret;
}

// this only supports 2048 byte sectors
static int command_read (int unitnum, uae_u8 *data, int sector, int numsectors)
{
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return 0;
	struct cdtoc *t = findtoc (cdu, &sector);
	if (!t)
		return 0;
	cdda_stop (cdu);
	if (t->size == 2048) {
		while (numsectors-- > 0) {
			do_read (cdu, t, data, sector, 0, 2048);
			data += 2048;
			sector++;
		}
	} else {
		while (numsectors-- > 0) {
			if (t->size == 2352) {
				uae_u8 b = 0;
				do_read (cdu, t, &b, sector, 15, 1);
				// 2 = MODE2
				do_read (cdu, t, data, sector, b == 2 ? 24 : 16, 2048);
			} else {
				do_read (cdu, t, data, sector, 16, 2048);
			}
			data += 2048;
			sector++;
		}
	}
	cdu->cd_last_pos = sector;
	return 1;
}

static int command_toc (int unitnum, struct cd_toc_head *th)
{
	struct cdunit *cdu = unitisopen (unitnum);
	if (!cdu)
		return 0;

	int i;

	memset (&cdu->di.toc, 0, sizeof (struct cd_toc_head));
	if (!cdu->tracks)
		return 0;

	memset (th, 0, sizeof (struct cd_toc_head));
	struct cd_toc *toc = &th->toc[0];
	th->first_track = 1;
	th->last_track = cdu->tracks;
	th->points = cdu->tracks + 3;
	th->tracks = cdu->tracks;
	th->firstaddress = 0;
	th->lastaddress = cdu->toc[cdu->tracks].address;

	toc->adr = 1;
	toc->point = 0xa0;
	toc->track = th->first_track;
	toc++;

	th->first_track_offset = 1;
	for (i = 0; i < cdu->tracks; i++) {
		toc->adr = cdu->toc[i].adr;
		toc->control = cdu->toc[i].ctrl;
		toc->track = i + 1;
		toc->point = i + 1;
		toc->paddress = cdu->toc[i].address;
		toc++;
	}

	th->last_track_offset = cdu->tracks;
	toc->adr = 1;
	toc->point = 0xa1;
	toc->track = th->last_track;
	toc++;

	toc->adr = 1;
	toc->point = 0xa2;
	toc->paddress = th->lastaddress;
	toc++;

	memcpy (&cdu->di.toc, th, sizeof (struct cd_toc_head));
	return 1;
}

static void skipspace (TCHAR **s)
{
	while (_istspace (**s))
		(*s)++;
}
static void skipnspace (TCHAR **s)
{
	while (!_istspace (**s))
		(*s)++;
}

static TCHAR *nextstring (TCHAR **sp)
{
	TCHAR *s;
	TCHAR *out = NULL;

	skipspace (sp);
	s = *sp;
	if (*s == '\"') {
		s++;
		out = s;
		while (*s && *s != '\"')
			s++;
		*s++ = 0;
	} else if (*s) {
		out = s;
		skipnspace (&s);
		*s++ = 0;
	}
	*sp = s;
	return out;
}

static int readval (const TCHAR *s)
{
	int base = 10;
	TCHAR *endptr;
	if (s[0] == '0' && _totupper (s[1]) == 'X')
		s += 2, base = 16;
	return _tcstol (s, &endptr, base);
}

#define MEDIA_DESCRIPTOR "MEDIA DESCRIPTOR"

/* MDS spec structures from cdemu */

#define MDS_MEDIUM_CD           0x00 /* CD-ROM */
#define MDS_MEDIUM_CD_R         0x01 /* CD-R */
#define MDS_MEDIUM_CD_RW        0x02 /* CD-RW */
#define MDS_MEDIUM_DVD          0x10 /* DVD-ROM */
#define MDS_MEDIUM_DVD_MINUS_R  0x12 /* DVD-R */
 
#define MDS_TRACKMODE_UNKNOWN       0x00
#define MDS_TRACKMODE_AUDIO         0xA9 /* sector size = 2352 */
#define MDS_TRACKMODE_MODE1         0xAA /* sector size = 2048 */
#define MDS_TRACKMODE_MODE2         0xAB /* sector size = 2336 */
#define MDS_TRACKMODE_MODE2_FORM1   0xAC /* sector size = 2048 */
#define MDS_TRACKMODE_MODE2_FORM2   0xAD /* sector size = 2324 (+4) */

#define MDS_SUBCHAN_NONE            0x00 /* no subchannel */
#define MDS_SUBCHAN_PW_INTERLEAVED  0x08 /* 96-byte PW subchannel, interleaved */

#define MDS_POINT_TRACK_FIRST   0xA0 /* info about first track */
#define MDS_POINT_TRACK_LAST    0xA1 /* info about last track  */
#define MDS_POINT_TRACK_LEADOUT 0xA2 /* info about lead-out    */

#pragma pack(1)

typedef struct {
    uae_u8 signature[16]; /* "MEDIA DESCRIPTOR" */
    uae_u8 version[2]; /* Version ? */
    uae_u16 medium_type; /* Medium type */
    uae_u16 num_sessions; /* Number of sessions */
    uae_u16 __dummy1__[2]; /* Wish I knew... */
    uae_u16 bca_len; /* Length of BCA data (DVD-ROM) */
    uae_u32 __dummy2__[2];
    uae_u32 bca_data_offset; /* Offset to BCA data (DVD-ROM) */
    uae_u32 __dummy3__[6]; /* Probably more offsets */
    uae_u32 disc_structures_offset; /* Offset to disc structures */
    uae_u32 __dummy4__[3]; /* Probably more offsets */
    uae_u32 sessions_blocks_offset; /* Offset to session blocks */
    uae_u32 dpm_blocks_offset; /* offset to DPM data blocks */
} MDS_Header; /* length: 88 bytes */

typedef struct {
    uae_s32 session_start; /* Session's start address */
    uae_s32 session_end; /* Session's end address */
    uae_u16 session_number; /* (Unknown) */
    uae_u8 num_all_blocks; /* Number of all data blocks. */
    uae_u8 num_nontrack_blocks; /* Number of lead-in data blocks */
    uae_u16 first_track; /* Total number of sessions in image? */
    uae_u16 last_track; /* Number of regular track data blocks. */
    uae_u32 __dummy2__; /* (unknown) */
    uae_u32 tracks_blocks_offset; /* Offset of lead-in+regular track data blocks. */
} MDS_SessionBlock; /* length: 24 bytes */

typedef struct {
    uae_u8 mode; /* Track mode */
    uae_u8 subchannel; /* Subchannel mode */
    uae_u8 adr_ctl; /* Adr/Ctl */
    uae_u8 __dummy2__; /* Track flags? */
    uae_u8 point; /* Track number. (>0x99 is lead-in track) */
    
    uae_u32 __dummy3__;
    uae_u8 min; /* Min */
    uae_u8 sec; /* Sec */
    uae_u8 frame; /* Frame */
    uae_u32 extra_offset; /* Start offset of this track's extra block. */
    uae_u16 sector_size; /* Sector size. */
    
    uae_u8 __dummy4__[18];
    uae_u32 start_sector; /* Track start sector (PLBA). */
    uae_u64 start_offset; /* Track start offset. */
    uae_u8 session; /* Session or index? */
    uae_u8 __dummy5__[3];
    uae_u32 footer_offset; /* Start offset of footer. */
    uae_u8 __dummy6__[24];
} MDS_TrackBlock; /* length: 80 bytes */

typedef struct {
    uae_u32 pregap; /* Number of sectors in pregap. */
    uae_u32 length; /* Number of sectors in track. */
} MDS_TrackExtraBlock; /* length: 8 bytes */

typedef struct {
    uae_u32 filename_offset; /* Start offset of image filename. */
    uae_u32 widechar_filename; /* Seems to be set to 1 if widechar filename is used */
    uae_u32 __dummy1__;
    uae_u32 __dummy2__;
} MDS_Footer; /* length: 16 bytes */

#pragma pack()

static int parsemds (struct cdunit *cdu, struct zfile *zmds, const TCHAR *img)
{
	MDS_Header *head;
	struct cdtoc *t;
	uae_u8 *mds = NULL;
	uae_u64 size;
	MDS_SessionBlock *sb;
  
	write_log (_T("MDS TOC: '%s'\n"), img);
	size = zfile_size (zmds);
	mds = xmalloc (uae_u8, size);
	if (!mds)
		goto end;
	if (zfile_fread (mds, size, 1, zmds) != 1)
		goto end;

	head = (MDS_Header*)mds;
	if (!memcmp (&head, MEDIA_DESCRIPTOR, strlen (MEDIA_DESCRIPTOR)))
		goto end;
	if (head->version[0] != 1) {
		write_log (_T("unsupported MDS version %d, only v.1 supported\n"), head->version[0]);
		goto end;
	}

	sb = (MDS_SessionBlock*)(mds + head->sessions_blocks_offset);
	cdu->tracks = sb->last_track - sb->first_track + 1;
	for (int i = 0; i < sb->num_all_blocks; i++) {
		MDS_TrackBlock *tb = (MDS_TrackBlock*)(mds + sb->tracks_blocks_offset + i * sizeof (MDS_TrackBlock));
		int point = tb->point;
		int tracknum = -1;
		if (point == 0xa2)
			tracknum = cdu->tracks;
		else if (point >= 1 && point <= 99)
			tracknum = point - 1;
		if (tracknum >= 0) {
			MDS_Footer *footer = tb->footer_offset == 0 ? NULL : (MDS_Footer*)(mds + tb->footer_offset);
			MDS_TrackExtraBlock *teb = tb->extra_offset == 0 ? NULL : (MDS_TrackExtraBlock*)(mds + tb->extra_offset);
			t = &cdu->toc[tracknum];
			t->adr = tb->adr_ctl >> 4;
			t->ctrl = tb->adr_ctl & 15;
			if (point == 0xa2)
				t->address = sb->session_end;
			else
				t->address = tb->start_sector;
			t->track = point;
			t->offset = tb->start_offset;
			t->size = tb->sector_size;

			if (point >= 100)
				continue;

			if (footer) {
				TCHAR *fname = NULL;
				if (footer->widechar_filename == 0)
				  fname = au ((char*)(mds + footer->filename_offset));
				else
					fname = my_strdup ((TCHAR*)(mds + footer->filename_offset));
				if (fname[0] == '*' && fname[1] == '.') {
					TCHAR newname[MAX_DPATH];
					_tcscpy (newname, img);
					TCHAR *ext = _tcsrchr (newname, '.');
					if (ext)
						_tcscpy (ext, fname + 1);
					xfree (fname);
					fname = my_strdup (newname);
				}

				t->handle = zfile_fopen (fname, _T("rb"), ZFD_NORMAL);
				t->fname = my_strdup (fname);
				if (t->handle)
					t->filesize = zfile_size (t->handle);
			}

			if (tb->subchannel && t->handle) {
				t->suboffset = t->size;
				t->subcode = 1; // interleaved
				t->subhandle = zfile_dup (t->handle);
				t->skipsize = SUB_CHANNEL_SIZE;
				t->size -= SUB_CHANNEL_SIZE;
			}
			if ((t->ctrl & 0x0c) != 4)
				t->enctype = AUDENC_PCM;
		}
	}

end:
	xfree (mds);

	return cdu->tracks;
}

#ifdef WITH_CHD
static int parsechd (struct cdunit *cdu, struct zfile *zcue, const TCHAR *img)
{
	chd_error err;
	struct cdrom_file *cdf;
	struct zfile *f = zfile_dup (zcue);
	if (!f)
		return 0;
	chd_file *cf = new chd_file();
	err = cf->open(f, false, NULL);
	if (err != CHDERR_NONE) {
		write_log (_T("CHD '%s' err=%d\n"), zfile_getname (zcue), err);
		zfile_fclose (f);
		return 0;
	}
	if (!(cdf = cdrom_open (cf))) {
		write_log (_T("Couldn't open CHD '%s' as CD\n"), zfile_getname (zcue));
		cf->close ();
		zfile_fclose (f);
		return 0;
	}
	cdu->chd_f = cf;
	cdu->chd_cdf = cdf;
	
	const cdrom_toc *stoc = cdrom_get_toc (cdf);
	cdu->tracks = stoc->numtrks;
	uae_u32 hunkcnt = cf->hunk_count ();
	uae_u32 hunksize = cf->hunk_bytes ();
	uae_u32 cbytes;
	chd_codec_type compr;

	for (int i = 0; i <cdu->tracks; i++) {
		int size;
		const cdrom_track_info *strack = &stoc->tracks[i];
		struct cdtoc *dtrack = &cdu->toc[i];
		dtrack->address = strack->physframeofs;
		dtrack->offset = strack->chdframeofs;
		dtrack->adr = cdrom_get_adr_control (cdf, i) >> 4;
		dtrack->ctrl = cdrom_get_adr_control (cdf, i) & 15;
		switch (strack->trktype)
		{
			case CD_TRACK_MODE1:
			case CD_TRACK_MODE2_FORM1:
				size = 2048;
			break;
			case CD_TRACK_MODE1_RAW:
			case CD_TRACK_MODE2_RAW:
			case CD_TRACK_AUDIO:
			default:
				size = 2352;
			break;
			case CD_TRACK_MODE2:
			case CD_TRACK_MODE2_FORM_MIX:
				size = 2336;
			break;
			case CD_TRACK_MODE2_FORM2:
				size = 2324;
			break;
		}
		dtrack->suboffset = size;
		dtrack->subcode = strack->subtype == CD_SUB_NONE ? 0 : strack->subtype == CD_SUB_RAW ? 1 : 2;
		dtrack->chdtrack = strack;
		dtrack->size = size;
		dtrack->enctype = ENC_CHD;
		dtrack->fname = my_strdup (zfile_getname (zcue));
		dtrack->filesize = cf->logical_bytes ();
		dtrack->track = i + 1;
		dtrack[1].address = dtrack->address + strack->frames;
		if (cf->hunk_info(dtrack->offset * CD_FRAME_SIZE / hunksize, compr, cbytes) == CHDERR_NONE) {
			TCHAR tmp[100];
			uae_u32 c = (uae_u32)compr;
			for (int j = 0; j < 4; j++) {
				uae_u8 b = c >> ((3 - j) * 8);
				if (c < 10) {
					b += '0';
				}
				if (b < ' ' || b >= 127)
						b = '.';
				tmp[j] = b;
			}
			tmp[4] = 0;
			dtrack->extrainfo = my_strdup (tmp);
		}

	}
	return cdu->tracks;
}
#endif

static int parseccd (struct cdunit *cdu, struct zfile *zcue, const TCHAR *img)
{
	int mode;
	int num, tracknum, trackmode;
	int adr, control, lba;
	bool gotlba;
	struct cdtoc *t;
	struct zfile *zimg, *zsub;
	TCHAR fname[MAX_DPATH];
	
	write_log (_T("CCD TOC: '%s'\n"), img);
	_tcscpy (fname, zfile_getname(zcue));
	TCHAR *ext = _tcsrchr (fname, '.');
	if (ext)
		*ext = 0;
	_tcscat (fname, _T(".img"));
	zimg = zfile_fopen (fname, _T("rb"), ZFD_NORMAL);
	if (!zimg) {
		write_log (_T("CCD: can't open '%s'\n"), fname);
		return 0;
	}
	ext = _tcsrchr (fname, '.');
	if (ext)
		*ext = 0;
	_tcscat (fname, _T(".sub"));
	zsub = zfile_fopen (fname, _T("rb"), ZFD_NORMAL);
	if (zsub)
		write_log (_T("CCD: '%s' detected\n"), fname);

	num = -1;
	mode = -1;
	for (;;) {
		TCHAR buf[MAX_DPATH], *p;
		if (!zfile_fgets (buf, sizeof buf / sizeof (TCHAR), zcue))
			break;
		p = buf;
		skipspace (&p);
		if (!_tcsnicmp (p, _T("[DISC]"), 6)) {
			mode = 1;
		} else if (!_tcsnicmp (p, _T("[ENTRY "), 7)) {
			t = NULL;
			mode = 2;
			num = readval (p + 7);
			if (num < 0)
				break;
			adr = control = -1;
			gotlba = false;
		} else if (!_tcsnicmp (p, _T("[TRACK "), 7)) {
			mode = 3;
			tracknum = readval (p + 7);
			trackmode = -1;
			if (tracknum <= 0 || tracknum > 99)
				break;
			t = &cdu->toc[tracknum - 1];
		}
		if (mode < 0)
			continue;
		if (mode == 1) {
			if (!_tcsnicmp (p, _T("TocEntries="), 11)) {
				cdu->tracks = readval (p + 11) - 3;
				if (cdu->tracks <= 0 || cdu->tracks > 99)
					break;
			}
			continue;
		}
		if (cdu->tracks <= 0)
			break;
		
		if (mode == 2) {

			if (!_tcsnicmp (p, _T("SESSION="), 8)) {
				if (readval (p + 8) != 1)
					mode = -1;
				continue;
			} else if (!_tcsnicmp (p, _T("POINT="), 6)) {
				tracknum = readval (p + 6);
				if (tracknum <= 0)
					break;
				if (tracknum >= 0xa0 && tracknum != 0xa2) {
					mode = -1;
					continue;
				}
				if (tracknum == 0xa2)
					tracknum = cdu->tracks + 1;
				t = &cdu->toc[tracknum - 1];
				continue;
			}
			if (!_tcsnicmp (p, _T("ADR="), 4))
				adr = readval (p + 4);
			if (!_tcsnicmp (p, _T("CONTROL="), 8))
				control = readval (p + 8);
			if (!_tcsnicmp (p, _T("PLBA="), 5)) {
				lba = readval (p + 5);
				gotlba = true;
			}
			if (gotlba && adr >= 0 && control >= 0) {
				t->adr = adr;
				t->ctrl = control;
				t->address = lba;
				t->offset = 0;
				t->size = 2352;
				t->offset = lba * t->size;
				t->track = tracknum;
				if ((control & 0x0c) != 4)
					t->enctype = AUDENC_PCM;
				if (zsub) {
					t->subcode = 2;
					t->subhandle = zfile_dup (zsub);
					t->suboffset = 0;
				}
				if (zimg) {
					t->handle = zfile_dup (zimg);
					t->fname = my_strdup (zfile_getname (zimg));
				}
				mode = -1;
			}

		} else if (mode == 3) {

			if (!_tcsnicmp (p, _T("MODE="), 5))
				trackmode = _tstol (p + 5);
			if (trackmode < 0 || trackmode > 2)
				continue;
			
		}

	}
	zfile_fclose (zimg);
	zfile_fclose (zsub);
	return cdu->tracks;
}

static int parsecue (struct cdunit *cdu, struct zfile *zcue, const TCHAR *img)
{
	int tracknum, pregap, postgap, lastpregap, lastpostgap;
	int newfile, secoffset;
	uae_s64 fileoffset;
	int index0;
	TCHAR *fname, *fnametype;
	audenc fnametypeid;
	int ctrl;
	mp3decoder *mp3dec = NULL;

	fname = NULL;
	fnametype = NULL;
	tracknum = 0;
	fileoffset = 0;
	secoffset = 0;
	newfile = 0;
	ctrl = 0;
	index0 = -1;
	pregap = 0;
	postgap = 0;
	lastpregap = 0;
	lastpostgap = 0;
	fnametypeid = AUDENC_NONE;

	write_log (_T("CUE TOC: '%s'\n"), img);
	for (;;) {
		TCHAR buf[MAX_DPATH], *p;
		if (!zfile_fgets (buf, sizeof buf / sizeof (TCHAR), zcue))
			break;

		p = buf;
		skipspace (&p);

		if (!_tcsnicmp (p, _T("FILE"), 4)) {
			p += 4;
			xfree (fname);
			fname = my_strdup (nextstring (&p));
			fnametype = nextstring (&p);
			fnametypeid = AUDENC_NONE;
			if (!fnametype)
				break;
			if (_tcsicmp (fnametype, _T("BINARY")) && _tcsicmp (fnametype, _T("WAVE")) && _tcsicmp (fnametype, _T("MP3")) && _tcsicmp (fnametype, _T("FLAC"))) {
				write_log (_T("CUE: unknown file type '%s' ('%s')\n"), fnametype, fname);
			}
			fnametypeid = AUDENC_PCM;
			if (!_tcsicmp (fnametype, _T("MP3")))
				fnametypeid = AUDENC_MP3;
			else if (!_tcsicmp (fnametype, _T("FLAC")))
				fnametypeid = AUDENC_FLAC;
			fileoffset = 0;
			newfile = 1;
			ctrl = 0;
		} else if (!_tcsnicmp (p, _T("FLAGS"), 5)) {
			ctrl &= ~(1 | 2 | 8);
			for (;;) {
				TCHAR *f = nextstring (&p);
				if (!f)
					break;
				if (!_tcsicmp (f, _T("PRE")))
					ctrl |= 1;
				if (!_tcsicmp (f, _T("DCP")))
					ctrl |= 2;
				if (!_tcsicmp (f, _T("4CH")))
					ctrl |= 8;
			}
		} else if (!_tcsnicmp (p, _T("TRACK"), 5)) {
			int size;
			TCHAR *tracktype;
			
			p += 5;
			index0 = -1;
			lastpregap = 0;
			lastpostgap = 0;
			tracknum = _tstoi (nextstring (&p));
			tracktype = nextstring (&p);
			if (!tracktype)
				break;
			size = 2352;
			if (!_tcsicmp (tracktype, _T("AUDIO"))) {
				ctrl &= ~4;
			} else {
				ctrl |= 4;
				if (!_tcsicmp (tracktype, _T("MODE1/2048")))
					size = 2048;
				else if (!_tcsicmp (tracktype, _T("MODE1/2352")))
					size = 2352;
				else if (!_tcsicmp (tracktype, _T("MODE2/2336")) || !_tcsicmp (tracktype, _T("CDI/2336")))
					size = 2336;
				else if (!_tcsicmp (tracktype, _T("MODE2/2352")) || !_tcsicmp (tracktype, _T("CDI/2352")))
					size = 2352;
				else {
					write_log (_T("CUE: unknown tracktype '%s' ('%s')\n"), tracktype, fname);
				}
			}
			if (tracknum >= 1 && tracknum <= 99) {
				struct cdtoc *t = &cdu->toc[tracknum - 1];
				struct zfile *ztrack;

				if (tracknum > 1 && newfile) {
					t--;
					secoffset += (int)(t->filesize / t->size);
					t++;
				}

				newfile = 0;
				ztrack = zfile_fopen (fname, _T("rb"), ZFD_ARCHIVE | ZFD_DELAYEDOPEN);
				if (!ztrack) {
					TCHAR tmp[MAX_DPATH];
					_tcscpy (tmp, fname);
					p = tmp + _tcslen (tmp);
					while (p > tmp) {
						if (*p == '/' || *p == '\\') {
							ztrack = zfile_fopen (p + 1, _T("rb"), ZFD_ARCHIVE | ZFD_DELAYEDOPEN);
							if (ztrack) {
								xfree (fname);
								fname = my_strdup (p + 1);
							}
							break;
						}
						p--;
					}
				}
				if (!ztrack) {
					TCHAR tmp[MAX_DPATH];
					TCHAR *s2;
					_tcscpy (tmp, zfile_getname (zcue));
					s2 = _tcsrchr (tmp, '\\');
					if (!s2)
						s2 = _tcsrchr (tmp, '/');
					if (s2) {
						s2[0] = 0;
						_tcscat (tmp, FSDB_DIR_SEPARATOR_S);
						_tcscat (tmp, fname);
						ztrack = zfile_fopen (tmp, _T("rb"), ZFD_ARCHIVE | ZFD_DELAYEDOPEN);
					}
				}
				t->track = tracknum;
				t->ctrl = ctrl;
				t->adr = 1;
				t->handle = ztrack;
				t->size = size;
				t->fname = my_strdup (fname);
				if (tracknum > cdu->tracks)
					cdu->tracks = tracknum;
				if (t->handle)
					t->filesize = zfile_size (t->handle);
			}
		} else if (!_tcsnicmp (p, _T("PREGAP"), 6)) {
			TCHAR *tt;
			int tn;
			p += 6;
			tt = nextstring (&p);
			tn = _tstoi (tt) * 60 * 75;
			tn += _tstoi (tt + 3) * 75;
			tn += _tstoi (tt + 6);
			pregap += tn;
			lastpregap = tn;
		} else if (!_tcsnicmp (p, _T("POSTGAP"), 7)) {
			struct cdtoc *t = &cdu->toc[tracknum - 1];
			TCHAR *tt;
			int tn;
			p += 7;
			tt = nextstring (&p);
			tn = _tstoi (tt) * 60 * 75;
			tn += _tstoi (tt + 3) * 75;
			tn += _tstoi (tt + 6);
			postgap += tn;
			lastpostgap = tn;
		} else if (!_tcsnicmp (p, _T("INDEX"), 5)) {
			int idxnum;
			int tn = 0;
			TCHAR *tt;
			p += 5;
			idxnum = _tstoi (nextstring (&p));
			tt = nextstring (&p);
			tn = _tstoi (tt) * 60 * 75;
			tn += _tstoi (tt + 3) * 75;
			tn += _tstoi (tt + 6);
			if (idxnum == 0) {
				index0 = tn;
			} else if (idxnum == 1 && tracknum >= 1 && tracknum <= 99) {
				struct cdtoc *t = &cdu->toc[tracknum - 1];
				if (!t->address) {
					t->address = tn + secoffset;
					t->address += pregap;
					t->pregap = lastpregap;
					t->postgap = lastpostgap;
					if (index0 >= 0) {
						t->index1 = tn - index0;
					}
					if (lastpregap && !secoffset) {
						t->index1 = lastpregap;
					}
					int blockoffset = t->address - t->index1;
					if (tracknum > 1)
						blockoffset -= t[-1].address - t[-1].index1;
					fileoffset += blockoffset * t[-1].size;
					if (!secoffset) {
						// secoffset == 0: same file contained also previous track
						t->offset = fileoffset - pregap * t->size;
					}
					t->address += postgap;
					if (fnametypeid == AUDENC_PCM && t->handle) {
						struct zfile *zf = t->handle;
						uae_u8 buf[16] = { 0 };
						zfile_fread (buf, 12, 1, zf);
						if (!memcmp (buf, "RIFF", 4) && !memcmp (buf + 8, "WAVE", 4)) {
							int size;
							for (;;) {
								memset (buf, 0, sizeof buf);
								if (zfile_fread (buf, 8, 1, zf) != 1)
									break;
								size = (buf[4] << 0) | (buf[5] << 8) | (buf[6] << 16) | (buf[7] << 24);
								if (!memcmp (buf, "data", 4))
									break;
								if (size <= 0)
									break;
								zfile_fseek (zf, size, SEEK_CUR);
							}
							t->offset += zfile_ftell (zf);
							t->filesize = size;
						}
						t->enctype = fnametypeid;
					} else if (fnametypeid == AUDENC_MP3 && t->handle) {
						if (!mp3dec) {
							try {
								mp3dec = new mp3decoder();
							} catch (exception) { }
						}
						if (mp3dec) {
							t->offset = 0;
							t->filesize = mp3dec->getsize (t->handle);
							if (t->filesize)
								t->enctype = fnametypeid;
						}
					} else if (fnametypeid == AUDENC_FLAC && t->handle) {
						flac_get_size (t);
						if (t->filesize)
							t->enctype = fnametypeid;
					}
				}
			}
		}
	}

	struct cdtoc *t = &cdu->toc[cdu->tracks - 1];
	uae_s64 size = t->filesize;
	if (!secoffset)
		size -= fileoffset;
	if (size < 0)
		size = 0;
	size /= t->size;
	cdu->toc[cdu->tracks].address = t->address + (int)size;

	xfree (fname);

	delete mp3dec;

	return cdu->tracks;
}

static int parse_image (struct cdunit *cdu, const TCHAR *img)
{
	struct zfile *zcue;
	int i;
	const TCHAR *ext;
	int secoffset;

	secoffset = 0;
	cdu->tracks = 0;
	if (!img)
		return 0;
	zcue = zfile_fopen (img, _T("rb"), ZFD_ARCHIVE | ZFD_CD | ZFD_DELAYEDOPEN);
	if (!zcue)
		return 0;

	ext = _tcsrchr (zfile_getname (zcue), '.');
	if (ext) {
		TCHAR curdir[MAX_DPATH];
		TCHAR oldcurdir[MAX_DPATH], *p;

		ext++;
		oldcurdir[0] = 0;
		_tcscpy (curdir, img);
		p = curdir + _tcslen (curdir);
		while (p > curdir) {
			if (*p == '/' || *p == '\\')
				break;
			p--;
		}
		*p = 0;
		if (p > curdir)
			my_setcurrentdir (curdir, oldcurdir);

		if (!_tcsicmp (ext, _T("cue")))
			parsecue (cdu, zcue, img);
		else if (!_tcsicmp (ext, _T("ccd")))
			parseccd (cdu, zcue, img);
		else if (!_tcsicmp (ext, _T("mds")))
			parsemds (cdu, zcue, img);
#ifdef WITH_CHD
		else if (!_tcsicmp (ext, _T("chd")))
			parsechd (cdu, zcue, img);
#endif

		if (oldcurdir[0])
			my_setcurrentdir (oldcurdir, NULL);
	}
	if (!cdu->tracks) {
		uae_u64 siz = zfile_size (zcue);
		if (siz >= 16384 && ((siz % 2048) == 0 || (siz % 2352) == 0)) {
			struct cdtoc *t = &cdu->toc[0];
			cdu->tracks = 1;
			t->ctrl = 4;
			t->adr = 1;
			t->fname = my_strdup (img);
			t->handle = zcue;
			t->size = (siz % 2048) == 0 ? 2048 : 2352;
			t->filesize = siz;
			t->track = 1;
			write_log (_T("CD: plain CD image mounted!\n"));
			cdu->toc[1].address = t->address + (int)(t->filesize / t->size);
			zcue = NULL;
		}
	}

	if (!cdu->tracks)
		write_log (_T("CD: couldn't mount '%s'!\n"), img);

	for (i = 0; i <= cdu->tracks; i++) {
		struct cdtoc *t = &cdu->toc[i];
		uae_u32 msf;
		if (t->pregap) {
			msf = lsn2msf (t->pregap - 150);
			write_log (_T("   PREGAP : %02d:%02d:%02d\n"), (msf >> 16) & 0x7fff, (msf >> 8) & 0xff, (msf >> 0) & 0xff);
		}
		if (t->index1) {
			msf = lsn2msf (t->index1 - 150);
			write_log (_T("   INDEX1 : %02d:%02d:%02d\n"), (msf >> 16) & 0x7fff, (msf >> 8) & 0xff, (msf >> 0) & 0xff);
		}
		if (i < cdu->tracks)
			write_log (_T("%2d: "), i + 1);
		else
			write_log (_T("    "));
		msf = lsn2msf (t->address);
		write_log (_T("%7d %02d:%02d:%02d"),
			t->address, (msf >> 16) & 0x7fff, (msf >> 8) & 0xff, (msf >> 0) & 0xff);
		if (i < cdu->tracks) {
			write_log (_T(" %s %x %10lld %10lld %s%s"),
        (t->ctrl & 4) ? _T("DATA    ") : (t->subcode ? _T("CDA+SUB") : _T("CDA     ")),
			  t->ctrl, t->offset, t->filesize, 
				t->extrainfo ? t->extrainfo : _T(""),
				t->handle == NULL && t->enctype != ENC_CHD ? _T("[FILE ERROR]") : _T(""));
    }
		write_log (_T("\n"));
		if (i < cdu->tracks)
			write_log (_T(" - %s\n"), t->fname);
		if (t->handle && !t->filesize)
			t->filesize = zfile_size (t->handle);
		if (t->postgap) {
			msf = lsn2msf (t->postgap - 150);
			write_log (_T("   POSTGAP: %02d:%02d:%02d\n"), (msf >> 16) & 0x7fff, (msf >> 8) & 0xff, (msf >> 0) & 0xff);
		}
	}

	cdu->blocksize = 2048;
	cdu->cdsize = (uae_u64)cdu->toc[cdu->tracks].address * cdu->blocksize;
	

	zfile_fclose (zcue);
	return 1;
}

static int ismedia (int unitnum, int quick)
{
	struct cdunit *cdu = &cdunits[unitnum];
	if (!cdu->enabled)
		return -1;
	return cdu->tracks > 0 ? 1 : 0;
}

static struct device_info *info_device (int unitnum, struct device_info *di, int quick, int session)
{
	struct cdunit *cdu = &cdunits[unitnum];
	memset (di, 0, sizeof (struct device_info));
	if (!cdu->enabled)
		return NULL;
	di->open = cdu->open;
	di->removable = 1;
	di->bus = unitnum;
	di->target = 0;
	di->lun = 0;
	di->media_inserted = 0;
	di->bytespersector = 2048;
	di->mediapath[0] = 0;
	di->cylinders = 1;
	di->trackspercylinder = 1;
	di->sectorspertrack = (int)(cdu->cdsize / di->bytespersector);
	if (ismedia (unitnum, 1)) {
		di->media_inserted = 1;
		_tcscpy (di->mediapath, cdu->imgname);
	}
	memset (&di->toc, 0, sizeof (struct cd_toc_head));
	command_toc (unitnum, &di->toc);
	di->write_protected = 1;
	di->type = INQ_ROMD;
	di->unitnum = unitnum + 1;
	if (di->mediapath[0]) {
		_tcscpy (di->label, _T("IMG:"));
		_tcscat (di->label, di->mediapath);
	} else {
		_tcscpy (di->label, _T("IMG:<EMPTY>"));
	}
	_tcscpy (di->vendorid, _T("UAE"));
	_stprintf (di->productid, _T("SCSICD%d"), unitnum);
	_tcscpy (di->revision, _T("1.0"));
	di->backend = _T("IMAGE");
	return di;
}

static void unload_image (struct cdunit *cdu)
{
	int i;

	for (i = 0; i < sizeof cdu->toc / sizeof (struct cdtoc); i++) {
		struct cdtoc *t = &cdu->toc[i];
		zfile_fclose (t->handle);
		if (t->handle != t->subhandle)
			zfile_fclose (t->subhandle);
		xfree (t->fname);
		xfree (t->data);
		xfree (t->subdata);
		xfree (t->extrainfo);
	}
#ifdef WITH_CHD
	cdrom_close (cdu->chd_cdf);
	cdu->chd_cdf = NULL;
	if (cdu->chd_f)
		cdu->chd_f->close();
	cdu->chd_f = NULL;
#endif
	memset (cdu->toc, 0, sizeof cdu->toc);
	cdu->tracks = 0;
	cdu->cdsize = 0;
}


static int open_device (int unitnum, const TCHAR *ident, int flags)
{
	struct cdunit *cdu = &cdunits[unitnum];
	int ret = 0;

	if (!cdu->open) {
		uae_sem_init (&cdu->sub_sem, 0, 1);
		cdu->imgname[0] = 0;
		if (ident)
			_tcscpy (cdu->imgname, ident);
		parse_image (cdu, ident);
		cdu->open = true;
		cdu->enabled = true;
		cdu->cdda_volume[0] = 0x7fff;
		cdu->cdda_volume[1] = 0x7fff;
		if (cdimage_unpack_thread == 0) {
			init_comm_pipe (&unpack_pipe, 10, 1);
			uae_start_thread (_T("cdimage_unpack"), cdda_unpack_func, NULL, NULL);
			while (cdimage_unpack_thread == 0)
				Sleep (10);
		}
		ret = 1;
	}
	blkdev_cd_change (unitnum, cdu->imgname);
	return ret;
}

static void close_device (int unitnum)
{
	struct cdunit *cdu = &cdunits[unitnum];
	if (cdu->open) {
		cdda_stop (cdu);
		cdu->open = false;
		if (cdimage_unpack_thread) {
			cdimage_unpack_thread = 0;
			write_comm_pipe_u32 (&unpack_pipe, -1, 0);
			write_comm_pipe_u32 (&unpack_pipe, -1, 1);
			while (cdimage_unpack_thread == 0)
				Sleep (10);
			cdimage_unpack_thread = 0;
			destroy_comm_pipe (&unpack_pipe);
		}
		unload_image (cdu);
		uae_sem_destroy (&cdu->sub_sem);
	}
	blkdev_cd_change (unitnum, cdu->imgname);
}

static void close_bus (void)
{
	if (!bus_open) {
		write_log (_T("IMAGE close_bus() when already closed!\n"));
		return;
	}
	for (int i = 0; i < MAX_TOTAL_SCSI_DEVICES; i++) {
		struct cdunit *cdu = &cdunits[i];
		if (cdu->open)
			close_device (i);
		cdu->enabled = false;
	}
	bus_open = 0;
	write_log (_T("IMAGE driver closed.\n"));
}

static int open_bus (int flags)
{
	if (bus_open) {
		write_log (_T("IOCTL open_bus() more than once!\n"));
		return 1;
	}
	bus_open = 1;
	write_log (_T("Image driver open.\n"));
	return 1;
}

struct device_functions devicefunc_cdimage = {
	_T("IMAGE"),
	open_bus, close_bus, open_device, close_device, info_device,
	0, 0, 0,
	command_pause, command_stop, command_play, command_volume, command_qcode,
	command_toc, command_read, command_rawread, 0,
	0, ismedia, 0
};
