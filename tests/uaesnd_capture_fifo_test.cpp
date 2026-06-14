#include <cstdint>
#include <cstring>
#include <iostream>

typedef std::uint8_t uae_u8;
typedef std::uint32_t uae_u32;

#include "uaesnd_capture_fifo.h"

static int failures;

static void expect_true(bool condition, const char *message)
{
	if (!condition) {
		std::cerr << message << '\n';
		failures++;
	}
}

static uaesnd_capture_state make_capture(uae_u8 *buffer, int size)
{
	uaesnd_capture_state capture = {};
	capture.buffer = buffer;
	capture.buffer_size = size;
	uaesnd_capture_fifo_reset(&capture);
	return capture;
}

static void test_big_endian_sample_order()
{
	uae_u8 buffer[16] = {};
	uaesnd_capture_state capture = make_capture(buffer, sizeof buffer);
	const uae_u8 expected[] = { 0x12, 0x34, 0xff, 0xfe, 0x80, 0x00, 0x7f, 0xff };
	uae_u8 actual[sizeof expected] = {};

	uaesnd_capture_fifo_write_s16be(&capture, 0x1234);
	uaesnd_capture_fifo_write_s16be(&capture, -2);
	uaesnd_capture_fifo_write_s16be(&capture, -32768);
	uaesnd_capture_fifo_write_s16be(&capture, 32767);

	for (std::size_t i = 0; i < sizeof actual; i++)
		expect_true(uaesnd_capture_fifo_read_byte(&capture, &actual[i]), "expected FIFO byte");

	expect_true(std::memcmp(actual, expected, sizeof expected) == 0, "16-bit samples must be exposed as big-endian bytes");
	expect_true(capture.available == 0, "available byte count must drain to zero");
}

static void test_overrun_drops_oldest_byte()
{
	uae_u8 buffer[4] = {};
	uaesnd_capture_state capture = make_capture(buffer, sizeof buffer);
	uae_u8 actual[3] = {};

	uaesnd_capture_fifo_write_byte(&capture, 0x10);
	uaesnd_capture_fifo_write_byte(&capture, 0x11);
	uaesnd_capture_fifo_write_byte(&capture, 0x12);
	uaesnd_capture_fifo_write_byte(&capture, 0x13);

	for (uae_u8 &value : actual)
		expect_true(uaesnd_capture_fifo_read_byte(&capture, &value), "expected retained FIFO byte");

	const uae_u8 expected[] = { 0x11, 0x12, 0x13 };
	expect_true(std::memcmp(actual, expected, sizeof expected) == 0, "FIFO overrun must drop the oldest byte");
	expect_true(capture.overrun_count == 1, "overrun counter must increment once");
	expect_true(capture.dropped_byte_count == 1, "dropped byte counter must increment once");
	expect_true((capture.status & UAESND_CAPTURE_STATUS_OVERRUN) != 0, "overrun status bit must be set");
}

static void test_ack_and_status_clear_mask()
{
	uae_u8 buffer[8] = {};
	uaesnd_capture_state capture = make_capture(buffer, sizeof buffer);

	capture.intreq = 1;
	uaesnd_capture_fifo_ack_intreq(&capture, 1);
	expect_true(capture.intreq == 1, "INTREQ write-one must leave request set");
	uaesnd_capture_fifo_ack_intreq(&capture, 0);
	expect_true(capture.intreq == 0, "INTREQ write-zero must acknowledge request");

	capture.status = UAESND_CAPTURE_STATUS_ACTIVE | UAESND_CAPTURE_STATUS_OVERRUN;
	uaesnd_capture_fifo_clear_status(&capture, UAESND_CAPTURE_STATUS_ACTIVE);
	expect_true(capture.status == UAESND_CAPTURE_STATUS_ACTIVE, "status write mask must clear zero bits only");
}

struct block_writer_state {
	uae_u8 bytes[8];
};

static void block_writer(void *opaque, uae_u32 offset, uae_u8 value)
{
	block_writer_state *state = static_cast<block_writer_state *>(opaque);
	state->bytes[offset] = value;
}

static void test_block_copy_transfers_full_frames()
{
	uae_u8 buffer[16] = {};
	uaesnd_capture_state capture = make_capture(buffer, sizeof buffer);
	block_writer_state writer = {};
	const uae_u8 expected[] = { 0x12, 0x34, 0xff, 0xfe, 0x80, 0x00, 0x7f, 0xff };

	uaesnd_capture_fifo_write_s16be(&capture, 0x1234);
	uaesnd_capture_fifo_write_s16be(&capture, -2);
	uaesnd_capture_fifo_write_s16be(&capture, -32768);
	uaesnd_capture_fifo_write_s16be(&capture, 32767);

	uae_u32 copied = uaesnd_capture_fifo_copy_block(&capture, 4, &writer, block_writer);

	expect_true(copied == 2, "block copy must report full stereo frames copied");
	expect_true(std::memcmp(writer.bytes, expected, sizeof expected) == 0, "block copy must preserve FIFO byte order");
	expect_true(capture.available == 0, "block copy must consume copied bytes");
}

int main()
{
	test_big_endian_sample_order();
	test_overrun_drops_oldest_byte();
	test_ack_and_status_clear_mask();
	test_block_copy_transfers_full_frames();
	return failures == 0 ? 0 : 1;
}
