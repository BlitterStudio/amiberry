/*
 * ZZ9000 SDK v2 mailbox subset used by installed AmigaOS clients.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_H
#define AMIBERRY_ZZ9000_SDK_H

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace zz9000_sdk {

constexpr uint32_t mailbox_offset = 0xd000;
constexpr uint32_t mailbox_size = 128 + 2 * 64 * 64;
constexpr uint32_t mailbox_arm_address = 0x3fe43000;

class Engine {
public:
	Engine(uint8_t *board_memory, uint32_t board_size);
	~Engine();
	void reset();
	void poll();
	uint16_t read_register(uint32_t offset) const;
	void write_register(uint32_t offset, uint16_t value);
	uint8_t *buffer_data(uint32_t handle);
	uint32_t buffer_length(uint32_t handle) const;

private:
	struct Buffer {
		uint32_t handle = 0;
		uint32_t board_offset = 0;
		uint32_t length = 0;
		uint32_t flags = 0;
		std::vector<uint8_t> card_only;
	};
	struct Audio;

	uint8_t *memory_;
	uint32_t board_size_;
	std::array<Buffer, 16> buffers_{};
	uint32_t next_handle_ = 1;
	uint16_t last_status_ = 0;
	std::unique_ptr<Audio> audio_;
	uint32_t next_audio_session_ = 1;
	Buffer *find_buffer(uint32_t handle);
	const Buffer *find_buffer(uint32_t handle) const;
	void close_audio();
	void decode_audio(const uint8_t *input, uint32_t length);
	void pump_audio();
	void audio_result(uint8_t *reply, uint16_t *reply_length);
	uint16_t dispatch(uint16_t opcode, const uint8_t *request,
	                  uint16_t request_length, uint8_t *reply,
	                  uint16_t *reply_length);
};

} // namespace zz9000_sdk

#endif
