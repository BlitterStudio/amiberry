/*
 * MPEG-2 Transport Stream front end for the SDK media sessions.
 *
 * It follows the card's sdk_dvd_ts.c: the input is 188-byte packets, or
 * 192-byte M2TS packets (a 4-byte timestamp before each); the packet size
 * is found from the sync bytes, and found again after damaged bytes. PAT
 * and PMT select the first program, its first MPEG video stream (stream
 * type 0x01 or 0x02) and its first MPEG audio stream (0x03 or 0x04). The
 * PES packets of those two PIDs are reassembled and written as a Program
 * Stream, which the Program Stream decoders (pl_mpeg for MPEG-1, FFmpeg's
 * mpeg demuxer for MPEG-2) read as they read container 1.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef AMIBERRY_ZZ9000_SDK_TS_H
#define AMIBERRY_ZZ9000_SDK_TS_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace zz9000_sdk {

class TsDemux {
public:
	// The Program Stream dialect written. pl_mpeg reads only MPEG-1
	// packets (pack and system header, PES with a PTS and no extension);
	// FFmpeg's mpeg demuxer wants the MPEG-2 form, which keeps the PES
	// header of the transport stream, DTS included.
	enum class Form { mpeg1, mpeg2 };

	// `audio` false follows the video stream only, as a session with no
	// audio codec: its audio packets are dropped, and the Program Stream
	// it writes declares none.
	explicit TsDemux(Form form, bool audio = true) : form_(form), audio_wanted_(audio) {}
	// Parses `length` more bytes and appends the Program Stream bytes they
	// complete to `out`. Bytes short of a packet wait for the next call.
	void feed(const uint8_t *data, size_t length, std::vector<uint8_t> &out);
	// The input ended: completes the unbounded PES packets still open,
	// drops what is short of a packet, and writes the Program Stream
	// headers if no picture or sound ever produced them.
	void finish(std::vector<uint8_t> &out);

	// Counters, as the card's demux keeps them.
	uint32_t sync_losses() const { return sync_losses_; }
	uint32_t continuity_errors() const { return continuity_errors_; }
	uint32_t error_packets() const { return tei_packets_; }
	uint32_t duplicate_packets() const { return duplicate_packets_; }
	uint32_t truncated_pes() const { return truncated_pes_; }
	bool has_pmt() const { return have_pmt_; }
	bool has_video() const { return video_.active; }
	bool has_audio() const { return audio_.active; }

private:
	struct Stream {
		uint16_t pid = 0;
		bool active = false;
		uint8_t cc = 0;
		bool cc_valid = false;
		// A PES is being collected in data. total is its length from the
		// PES header (start code included), 0 when unbounded.
		bool collecting = false;
		uint32_t total = 0;
		// An earlier chunk of this unbounded PES already went out with
		// the timestamps: data starts with a synthetic header.
		bool continued = false;
		std::vector<uint8_t> data;
	};

	void run(std::vector<uint8_t> &out, bool eof);
	bool lock(std::vector<uint8_t> &out, size_t from, bool eof, bool &more);
	void packet(const uint8_t *t, std::vector<uint8_t> &out);
	Stream *stream_for_pid(uint16_t pid);
	void stream_packet(Stream &s, bool video, bool pusi, const uint8_t *payload,
	                   size_t length, std::vector<uint8_t> &out);
	void stream_flush(Stream &s, bool video, bool last, std::vector<uint8_t> &out);
	void stream_end(Stream &s, bool video, std::vector<uint8_t> &out);
	void stream_reset(Stream &s);
	void emit_pes(const std::vector<uint8_t> &pes, bool video,
	              std::vector<uint8_t> &out);
	void emit_headers(std::vector<uint8_t> &out);
	void psi_packet(bool pusi, const uint8_t *payload, size_t length);
	void section_feed(const uint8_t *data, size_t length);
	void section_done(size_t total);

	static constexpr uint16_t no_pid = 0xffff;
	static constexpr size_t section_max = 1024;

	Form form_;
	bool audio_wanted_;
	std::vector<uint8_t> staging_;
	size_t head_ = 0;
	// 188, or 192 for M2TS; 0 until the sync byte pattern is found (again).
	// sync_offset_ is where the 0x47 sits inside a packet.
	size_t packet_bytes_ = 0, sync_offset_ = 0;
	bool headers_written_ = false;
	Stream video_, audio_;
	bool have_pmt_ = false;
	uint16_t pmt_pid_ = no_pid;
	uint16_t program_ = 0;
	bool section_open_ = false;
	size_t section_fill_ = 0;
	std::array<uint8_t, section_max> section_{};
	uint32_t sync_losses_ = 0, continuity_errors_ = 0, tei_packets_ = 0;
	uint32_t duplicate_packets_ = 0, truncated_pes_ = 0;
};

} // namespace zz9000_sdk

#endif
