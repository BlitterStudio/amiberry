
typedef enum {
	AUD_FMT_U8,
	AUD_FMT_S8,
	AUD_FMT_U16,
	AUD_FMT_S16,
	AUD_FMT_U32,
	AUD_FMT_S32
} audfmt_e;

struct audsettings {
	int freq;
	int nchannels;
	audfmt_e fmt;
	int endianness;
};

typedef void(*audio_callback_fn) (void *opaque, int avail);

struct SWVoiceOut
{
	int dummy;
	int event_time;
	audio_callback_fn callback;
	void *opaque;
	bool active;
	int ch_sample[2];
	int bytesperframe;
	uae_u8 samplebuf[4096];
	int samplebuf_total, samplebuf_index;
	int freq, ch, bits;
	audfmt_e fmt;
	int left_volume, right_volume;
	int streamid;
};
struct SWVoiceIn
{
	int dummy;
};
typedef struct QEMUSoundCard {
	int dummy;
} QEMUSoundCard;


void AUD_close_in(QEMUSoundCard *card, SWVoiceIn *sw);
int  AUD_read(SWVoiceIn *sw, void *pcm_buf, int size);
int  AUD_write(SWVoiceOut *sw, void *pcm_buf, int size);
void AUD_set_active_out(SWVoiceOut *sw, int on);
int  AUD_is_active_out(SWVoiceOut *sw);
void AUD_set_active_in(SWVoiceIn *sw, int on);
int  AUD_is_active_in(SWVoiceIn *sw);
void AUD_close_out(QEMUSoundCard *card, SWVoiceOut *sw);
SWVoiceIn *AUD_open_in(
	QEMUSoundCard *card,
	SWVoiceIn *sw,
	const char *name,
	void *callback_opaque,
	audio_callback_fn callback_fn,
	struct audsettings *settings
);
SWVoiceOut *AUD_open_out(
	QEMUSoundCard *card,
	SWVoiceOut *sw,
	const char *name,
	void *callback_opaque,
	audio_callback_fn callback_fn,
	struct audsettings *settings
);
