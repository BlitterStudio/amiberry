#ifndef UAESND_CAPTURE_FIFO_H
#define UAESND_CAPTURE_FIFO_H

#define UAESND_CAPTURE_STATUS_UNAVAILABLE 1
#define UAESND_CAPTURE_STATUS_ACTIVE 2
#define UAESND_CAPTURE_STATUS_OVERRUN 4

struct uaesnd_capture_state
{
	uae_u32 control;
	uae_u32 status;
	uae_u32 frequency;
	uae_u32 available;
	uae_u32 overrun_count;
	uae_u32 intreq;
	uae_u32 threshold;
	uae_u32 frame_count;
	uae_u32 dropped_byte_count;
	uae_u32 block_address;
	uae_u32 block_frames;
	uae_u32 block_done;
	int buffer_size;
	int read_index;
	int write_index;
	uae_u8 *buffer;
};

typedef void (*uaesnd_capture_fifo_write_func)(void *opaque, uae_u32 offset, uae_u8 value);

static inline uae_u32 uaesnd_capture_fifo_available(struct uaesnd_capture_state *capture)
{
	if (!capture->buffer || capture->buffer_size <= 0)
		return 0;
	if (capture->write_index >= capture->read_index)
		return capture->write_index - capture->read_index;
	return capture->buffer_size - capture->read_index + capture->write_index;
}

static inline void uaesnd_capture_fifo_reset(struct uaesnd_capture_state *capture)
{
	capture->read_index = 0;
	capture->write_index = 0;
	capture->available = 0;
}

static inline void uaesnd_capture_fifo_write_byte(struct uaesnd_capture_state *capture, uae_u8 value)
{
	if (!capture->buffer || capture->buffer_size <= 1)
		return;
	int next = (capture->write_index + 1) % capture->buffer_size;
	if (next == capture->read_index) {
		capture->read_index = (capture->read_index + 1) % capture->buffer_size;
		capture->overrun_count++;
		capture->dropped_byte_count++;
		capture->status |= UAESND_CAPTURE_STATUS_OVERRUN;
	}
	capture->buffer[capture->write_index] = value;
	capture->write_index = next;
	capture->available = uaesnd_capture_fifo_available(capture);
}

static inline void uaesnd_capture_fifo_write_s16be(struct uaesnd_capture_state *capture, int sample)
{
	unsigned int value = (unsigned int)sample & 0xffff;
	uaesnd_capture_fifo_write_byte(capture, (uae_u8)(value >> 8));
	uaesnd_capture_fifo_write_byte(capture, (uae_u8)value);
}

static inline bool uaesnd_capture_fifo_read_byte(struct uaesnd_capture_state *capture, uae_u8 *value)
{
	if (!capture->buffer || capture->read_index == capture->write_index) {
		capture->available = 0;
		return false;
	}
	*value = capture->buffer[capture->read_index];
	capture->read_index = (capture->read_index + 1) % capture->buffer_size;
	capture->available = uaesnd_capture_fifo_available(capture);
	return true;
}

static inline void uaesnd_capture_fifo_ack_intreq(struct uaesnd_capture_state *capture, uae_u32 value)
{
	capture->intreq &= value;
}

static inline void uaesnd_capture_fifo_clear_status(struct uaesnd_capture_state *capture, uae_u32 value)
{
	capture->status &= value;
}

static inline uae_u32 uaesnd_capture_fifo_copy_block(struct uaesnd_capture_state *capture, uae_u32 frames,
	void *opaque, uaesnd_capture_fifo_write_func write_byte)
{
	uae_u32 available_frames = capture->available / 4;
	if (frames > available_frames)
		frames = available_frames;
	if (frames == 0 || !write_byte)
		return 0;
	uae_u32 bytes = frames * 4;
	for (uae_u32 i = 0; i < bytes; i++) {
		uae_u8 value = 0;
		if (!uaesnd_capture_fifo_read_byte(capture, &value))
			return i / 4;
		write_byte(opaque, i, value);
	}
	return frames;
}

#endif
