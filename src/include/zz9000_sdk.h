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
	bool poll(); // True when a visible framebuffer or PIP frame changed.
	uint16_t read_register(uint32_t offset) const;
	bool write_register(uint32_t offset, uint16_t value);
	// Offsets are within board_memory, including the 64 KiB register aperture.
	void set_framebuffer(uint32_t offset, uint32_t width, uint32_t height,
	                     uint32_t pitch, uint32_t format);
	void set_overlay(uint32_t offset, uint32_t width, uint32_t height,
	                 uint32_t pitch, uint32_t variant, bool active);
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
	struct Image;
	struct Media;
	struct Surface {
		uint32_t handle = 0;
		uint32_t width = 0, height = 0, pitch = 0, format = 0;
		std::vector<uint8_t> pixels;
	};
	struct Framebuffer {
		uint32_t offset = 0, width = 0, height = 0, pitch = 0, format = 0;
	};

	uint8_t *memory_;
	uint32_t board_size_;
	std::array<Buffer, 16> buffers_{};
	uint32_t next_handle_ = 1;
	uint16_t last_status_ = 0;
	std::unique_ptr<Audio> audio_;
	uint32_t next_audio_session_ = 1;
	std::unique_ptr<Image> image_;
	uint32_t next_image_session_ = 1;
	std::unique_ptr<Media> media_;
	uint32_t next_media_session_ = 1;
	struct Overlay {
		uint32_t offset = 0, width = 0, height = 0, pitch = 0, variant = 0;
		bool active = false;
	} overlay_{};
	std::array<Surface, 4> surfaces_{};
	Framebuffer framebuffer_{};
	uint32_t next_surface_handle_ = 1;
	Buffer *find_buffer(uint32_t handle);
	const Buffer *find_buffer(uint32_t handle) const;
	Surface *find_surface(uint32_t handle);
	uint8_t *surface_pixels(uint32_t handle);
	uint32_t surface_width(uint32_t handle) const;
	uint32_t surface_height(uint32_t handle) const;
	uint32_t surface_pitch(uint32_t handle) const;
	uint32_t surface_format(uint32_t handle) const;
	void close_audio();
	void decode_audio(const uint8_t *input, uint32_t length);
	void pump_audio();
	void audio_result(uint8_t *reply, uint16_t *reply_length);
	void close_media();
	void media_result(uint8_t *reply, uint16_t *reply_length,
	                  uint32_t bytes_written = 0, uint32_t event_flags = 0);
	void media_audio_result(uint8_t *reply, uint16_t *reply_length);
	void pump_media_audio();
	uint16_t dispatch_media(uint16_t opcode, const uint8_t *request,
	                        uint16_t request_length, uint8_t *reply,
	                        uint16_t *reply_length);
	uint32_t supported_capabilities() const;
	uint16_t dispatch(uint16_t opcode, const uint8_t *request,
	                  uint16_t request_length, uint8_t *reply,
	                  uint16_t *reply_length);
};

} // namespace zz9000_sdk

#endif
