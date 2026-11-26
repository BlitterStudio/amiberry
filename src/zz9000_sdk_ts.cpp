/*
 * MPEG-2 Transport Stream front end for the SDK media sessions. See
 * zz9000_sdk_ts.h. The parsing follows the card's sdk_dvd_ts.c, one
 * 188-byte packet (192 for M2TS) at a time.
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "zz9000_sdk_ts.h"

#include <algorithm>
#include <cstring>

namespace zz9000_sdk {
namespace {

constexpr uint8_t ts_sync = 0x47;
constexpr size_t ts_packet = 188;
constexpr size_t ts_m2ts_packet = 192;
constexpr uint16_t pid_pat = 0;
constexpr uint16_t pid_null = 0x1fff;
// Sync bytes one packet apart that confirm a packet size.
constexpr size_t lock_confirm = 3;
constexpr size_t pes_header_bytes = 9;
// An unbounded PES goes out in packets of at most this size, so its length
// field always fits.
constexpr size_t pes_chunk_bytes = 32768;

uint16_t be16(const uint8_t *p)
{
	return static_cast<uint16_t>((p[0] << 8) | p[1]);
}

// CRC-32/MPEG-2 over a PSI section including its CRC field is 0.
uint32_t crc32_mpeg(const uint8_t *p, size_t length)
{
	uint32_t crc = 0xffffffffu;
	for (size_t i = 0; i < length; ++i) {
		crc ^= static_cast<uint32_t>(p[i]) << 24;
		for (unsigned bit = 0; bit < 8; ++bit)
			crc = (crc & 0x80000000u) != 0 ? (crc << 1) ^ 0x04c11db7u : crc << 1;
	}
	return crc;
}

bool stream_id_ok(bool video, uint8_t sid)
{
	return video ? sid >= 0xe0 && sid <= 0xef : sid >= 0xc0 && sid <= 0xdf;
}

// Do lock_confirm sync bytes one packet apart start at i? 1 yes, 0 no, -1
// the staged input is too short to tell. At the end of input a run cut
// short still counts when one whole packet is there.
int sync_run(const uint8_t *p, size_t avail, size_t i, size_t size,
             size_t offset, bool eof)
{
	for (size_t k = 0; k < lock_confirm; ++k) {
		const size_t at = i + k * size + offset;
		if (at >= avail) {
			if (!eof)
				return -1;
			return k != 0 && i + size <= avail;
		}
		if (p[at] != ts_sync)
			return 0;
	}
	return 1;
}

void put_pts(std::vector<uint8_t> &out, uint64_t pts)
{
	out.push_back(static_cast<uint8_t>(0x21 | ((pts >> 29) & 0x0e)));
	out.push_back(static_cast<uint8_t>(pts >> 22));
	out.push_back(static_cast<uint8_t>(((pts >> 14) & 0xfe) | 1));
	out.push_back(static_cast<uint8_t>(pts >> 7));
	out.push_back(static_cast<uint8_t>(((pts << 1) & 0xfe) | 1));
}

} // namespace

void TsDemux::stream_reset(Stream &s)
{
	s.collecting = false;
	s.continued = false;
	s.total = 0;
	s.data.clear();
}

TsDemux::Stream *TsDemux::stream_for_pid(uint16_t pid)
{
	if (video_.active && video_.pid == pid)
		return &video_;
	if (audio_.active && audio_.pid == pid)
		return &audio_;
	return nullptr;
}

// The Program Stream headers: the pack header, and for pl_mpeg the system
// header whose stream counts it reads.
void TsDemux::emit_headers(std::vector<uint8_t> &out)
{
	if (headers_written_)
		return;
	headers_written_ = true;
	static const uint8_t pack1[] = { 0, 0, 1, 0xba, 0x21, 0x00, 0x01, 0x00, 0x01,
		0x80, 0x00, 0x01 };
	static const uint8_t pack2[] = { 0, 0, 1, 0xba, 0x44, 0x00, 0x04, 0x00, 0x04,
		0x01, 0x01, 0x89, 0xc3, 0xf8 };
	if (form_ == Form::mpeg2) {
		out.insert(out.end(), pack2, pack2 + sizeof(pack2));
		return;
	}
	out.insert(out.end(), pack1, pack1 + sizeof(pack1));
	const uint8_t system[] = { 0, 0, 1, 0xbb, 0x00, 0x06, 0x80, 0x00, 0x01,
		static_cast<uint8_t>((audio_.active ? 1 : 0) << 2),
		static_cast<uint8_t>(0x20 | (video_.active ? 1 : 0)), 0xff };
	out.insert(out.end(), system, system + sizeof(system));
}

// Writes one reassembled PES packet (or chunk of an unbounded one) as a
// Program Stream packet. `pes` starts with the start code, the stream id and
// a length field that is not trusted here; continuation chunks carry a
// synthetic header without timestamps.
void TsDemux::emit_pes(const std::vector<uint8_t> &pes, bool video,
                       std::vector<uint8_t> &out)
{
	if (pes.size() < pes_header_bytes || (pes[6] & 0xc0) != 0x80)
		return;
	const size_t header = pes_header_bytes + pes[8];
	if (header > pes.size())
		return;
	const size_t payload = pes.size() - header;
	if (payload == 0)
		return;
	emit_headers(out);
	const uint8_t sid = video ? 0xe0 : 0xc0;
	if (form_ == Form::mpeg2) {
		const size_t length = pes.size() - 6;
		out.push_back(0);
		out.push_back(0);
		out.push_back(1);
		out.push_back(sid);
		out.push_back(static_cast<uint8_t>(length >> 8));
		out.push_back(static_cast<uint8_t>(length));
		out.insert(out.end(), pes.begin() + 6, pes.end());
		return;
	}
	// MPEG-1: the PTS only, in packets whose 16-bit length holds it.
	const bool has_pts = (pes[7] & 0x80) != 0 && pes[8] >= 5;
	uint64_t pts = 0;
	if (has_pts) {
		const uint8_t *p = pes.data() + 9;
		pts = (static_cast<uint64_t>((p[0] >> 1) & 7) << 30) |
			(static_cast<uint64_t>(p[1]) << 22) |
			(static_cast<uint64_t>(p[2] >> 1) << 15) |
			(static_cast<uint64_t>(p[3]) << 7) | (p[4] >> 1);
	}
	const uint8_t *data = pes.data() + header;
	size_t left = payload;
	bool first = true;
	while (left) {
		const size_t room = 65535 - ((first && has_pts) ? 5 : 1);
		const size_t take = std::min(left, room);
		out.push_back(0);
		out.push_back(0);
		out.push_back(1);
		out.push_back(sid);
		const size_t length = take + ((first && has_pts) ? 5 : 1);
		out.push_back(static_cast<uint8_t>(length >> 8));
		out.push_back(static_cast<uint8_t>(length));
		if (first && has_pts)
			put_pts(out, pts);
		else
			out.push_back(0x0f);
		out.insert(out.end(), data, data + take);
		data += take;
		left -= take;
		first = false;
	}
}

// Hands the buffered PES (or chunk) on. A chunk leaves a synthetic header
// without timestamps in the buffer, so the rest of an unbounded PES goes
// out as continuation payload.
void TsDemux::stream_flush(Stream &s, bool video, bool last,
                           std::vector<uint8_t> &out)
{
	emit_pes(s.data, video, out);
	if (last) {
		stream_reset(s);
		return;
	}
	const uint8_t sid = s.data[3];
	static const uint8_t synthetic[pes_header_bytes] = { 0, 0, 1, 0, 0, 0, 0x80, 0, 0 };
	s.data.assign(synthetic, synthetic + pes_header_bytes);
	s.data[3] = sid;
	s.continued = true;
}

// One TS payload of a followed PID.
void TsDemux::stream_packet(Stream &s, bool video, bool pusi,
                            const uint8_t *payload, size_t length,
                            std::vector<uint8_t> &out)
{
	if (pusi) {
		if (s.collecting) {
			if (s.total == 0 && !(s.continued && s.data.size() == pes_header_bytes)) {
				// An unbounded PES ends where the next one starts.
				stream_flush(s, video, true, out);
			} else {
				if (s.total != 0)
					++truncated_pes_;
				stream_reset(s);
			}
		}
		if (length < 6 || payload[0] != 0 || payload[1] != 0 || payload[2] != 1)
			return;
		if (!stream_id_ok(video, payload[3]))
			return;
		s.data.clear();
		s.continued = false;
		s.total = be16(payload + 4) != 0 ? 6u + be16(payload + 4) : 0;
		s.collecting = true;
	} else if (!s.collecting) {
		return;
	}
	if (s.total != 0) {
		if (length > s.total - s.data.size())
			length = s.total - s.data.size();
	} else if (s.data.size() + length > pes_chunk_bytes) {
		stream_flush(s, video, false, out);
	}
	s.data.insert(s.data.end(), payload, payload + length);
	if (s.total != 0 && s.data.size() >= s.total)
		stream_flush(s, video, true, out);
}

// The input ended: an unbounded PES is complete, a bounded one that did not
// reach its length can never be.
void TsDemux::stream_end(Stream &s, bool video, std::vector<uint8_t> &out)
{
	if (!s.collecting)
		return;
	if (s.total != 0) {
		++truncated_pes_;
		stream_reset(s);
		return;
	}
	if (s.continued && s.data.size() == pes_header_bytes) {
		stream_reset(s);
		return;
	}
	stream_flush(s, video, true, out);
}

// ------------------------------------------------------------------- PSI

void TsDemux::section_done(size_t total)
{
	const uint8_t *sec = section_.data();
	if ((sec[1] & 0x80) == 0 || total < 12 || (sec[5] & 1) == 0 ||
	    crc32_mpeg(sec, total) != 0)
		return;
	const size_t end = total - 4;
	if (sec[0] == 0x00 && pmt_pid_ == no_pid) {
		for (size_t at = 8; at + 4 <= end; at += 4) {
			const uint16_t pid = be16(sec + at + 2) & 0x1fff;
			if (be16(sec + at) != 0) {
				program_ = be16(sec + at);
				pmt_pid_ = pid;
				// The PAT is no longer watched.
				section_open_ = false;
				return;
			}
		}
	} else if (sec[0] == 0x02 && pmt_pid_ != no_pid && be16(sec + 3) == program_) {
		const size_t info = be16(sec + 10) & 0x0fff;
		for (size_t at = 12 + info; at + 5 <= end;) {
			const uint8_t type = sec[at];
			const uint16_t pid = be16(sec + at + 1) & 0x1fff;
			const size_t es_info = be16(sec + at + 3) & 0x0fff;
			if (at + 5 + es_info > end)
				break;
			at += 5 + es_info;
			if (pid == pid_pat || pid == pid_null || pid == pmt_pid_)
				continue;
			if (!video_.active && (type == 0x01 || type == 0x02)) {
				video_.pid = pid;
				video_.active = true;
			} else if (audio_wanted_ && !audio_.active && (type == 0x03 || type == 0x04)) {
				audio_.pid = pid;
				audio_.active = true;
			}
		}
		have_pmt_ = video_.active || audio_.active;
	}
}

// Section bytes of the PID being watched (the PAT until a program is
// chosen, then its PMT). Sections may span packets and share a packet.
void TsDemux::section_feed(const uint8_t *data, size_t length)
{
	while (length != 0) {
		if (!section_open_) {
			if (data[0] == 0xff)
				return;
			section_open_ = true;
			section_fill_ = 0;
		}
		size_t total = section_fill_ >= 3 ?
			3 + (be16(section_.data() + 1) & 0x0fffu) : 3;
		size_t take = std::min(total - section_fill_, length);
		if (section_fill_ + take > section_max) {
			section_open_ = false;
			return;
		}
		std::memcpy(section_.data() + section_fill_, data, take);
		section_fill_ += take;
		data += take;
		length -= take;
		if (section_fill_ < 3)
			continue;
		total = 3 + (be16(section_.data() + 1) & 0x0fffu);
		if (total > section_max) {
			section_open_ = false;
			return;
		}
		if (section_fill_ == total) {
			section_open_ = false;
			section_done(total);
			if (have_pmt_)
				return;
		}
	}
}

void TsDemux::psi_packet(bool pusi, const uint8_t *payload, size_t length)
{
	if (pusi) {
		const size_t pointer = payload[0];
		if (1 + pointer > length) {
			section_open_ = false;
			return;
		}
		// The bytes before the pointer finish the previous section.
		if (section_open_)
			section_feed(payload + 1, pointer);
		section_open_ = false;
		section_feed(payload + 1 + pointer, length - 1 - pointer);
	} else if (section_open_) {
		section_feed(payload, length);
	}
}

// --------------------------------------------------------------- packets

void TsDemux::packet(const uint8_t *t, std::vector<uint8_t> &out)
{
	const uint16_t pid = be16(t + 1) & 0x1fff;
	const bool pusi = (t[1] & 0x40) != 0;
	const uint32_t control = (t[3] >> 4) & 3;
	size_t at = 4;
	bool discontinuity = false;
	if ((t[1] & 0x80) != 0) {
		// Transport error: nothing in it can be trusted, and the counter
		// jump after it drops a PES it cut.
		++tei_packets_;
		return;
	}
	if ((control & 2) != 0) {
		const size_t field = t[4];
		if (field > 183)
			return;
		discontinuity = field != 0 && (t[5] & 0x80) != 0;
		at = 5 + field;
	}
	Stream *s = have_pmt_ ? stream_for_pid(pid) : nullptr;
	if ((control & 1) == 0) {
		if (discontinuity && s)
			s->cc_valid = false;
		return;
	}
	// Scrambled payload, or no room for one.
	if ((t[3] & 0xc0) != 0 || at >= ts_packet)
		return;
	if (!have_pmt_) {
		const uint16_t watched = pmt_pid_ == no_pid ? pid_pat : pmt_pid_;
		if (pid == watched)
			psi_packet(pusi, t + at, ts_packet - at);
		return;
	}
	if (!s)
		return;
	const uint8_t cc = t[3] & 0x0f;
	if (discontinuity)
		s->cc_valid = false;
	if (s->cc_valid) {
		if (cc == s->cc) {
			++duplicate_packets_;
			return;
		}
		if (cc != ((s->cc + 1u) & 0x0f)) {
			// A packet of this PID was lost: the PES in progress is cut
			// and cannot be used.
			++continuity_errors_;
			stream_reset(*s);
		}
	}
	s->cc = cc;
	s->cc_valid = true;
	stream_packet(*s, s == &video_, pusi, t + at, ts_packet - at, out);
}

// Finds the first packet start at or after `from`, dropping what precedes
// it. true when locked; false when more input is needed or the input holds
// no packet start (everything was dropped).
bool TsDemux::lock(std::vector<uint8_t> &, size_t from, bool eof, bool &more)
{
	const uint8_t *p = staging_.data() + head_;
	const size_t avail = staging_.size() - head_;
	size_t i = from;
	more = false;
	for (; i < avail; ++i) {
		const int r188 = sync_run(p, avail, i, ts_packet, 0, eof);
		if (r188 == 1) {
			packet_bytes_ = ts_packet;
			sync_offset_ = 0;
			break;
		}
		const int r192 = sync_run(p, avail, i, ts_m2ts_packet, 4, eof);
		if (r192 == 1) {
			packet_bytes_ = ts_m2ts_packet;
			sync_offset_ = 4;
			break;
		}
		if (r188 < 0 || r192 < 0) {
			head_ += i;
			more = true;
			return false;
		}
	}
	if (i >= avail) {
		head_ += avail;
		return false;
	}
	head_ += i;
	return true;
}

void TsDemux::run(std::vector<uint8_t> &out, bool eof)
{
	size_t from = 0;
	for (;;) {
		const size_t avail = staging_.size() - head_;
		if (packet_bytes_ != 0) {
			if (avail < packet_bytes_)
				break;
			if (staging_[head_ + sync_offset_] == ts_sync) {
				packet(staging_.data() + head_ + sync_offset_, out);
				head_ += packet_bytes_;
				from = 0;
				continue;
			}
			packet_bytes_ = 0;
			++sync_losses_;
			from = 1;
		}
		if (avail == 0)
			break;
		bool more = false;
		if (!lock(out, from, eof, more))
			break;
		from = 0;
	}
	staging_.erase(staging_.begin(), staging_.begin() + head_);
	head_ = 0;
}

void TsDemux::feed(const uint8_t *data, size_t length, std::vector<uint8_t> &out)
{
	staging_.insert(staging_.end(), data, data + length);
	run(out, false);
}

void TsDemux::finish(std::vector<uint8_t> &out)
{
	run(out, true);
	stream_end(video_, true, out);
	stream_end(audio_, false, out);
	// Bytes short of a packet at the end of input are dropped.
	staging_.clear();
	head_ = 0;
	emit_headers(out);
}

} // namespace zz9000_sdk
