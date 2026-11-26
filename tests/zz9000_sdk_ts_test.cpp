// The Transport Stream front end of the media sessions, on synthetic
// streams: the packet size search, PAT and PMT, PES reassembly and the
// Program Stream it writes.
#include "zz9000_sdk_ts.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

using zz9000_sdk::TsDemux;
using Bytes = std::vector<uint8_t>;

namespace {

uint32_t crc32_mpeg(const uint8_t *p, size_t length)
{
	uint32_t crc = 0xffffffffu;
	for (size_t i = 0; i < length; ++i) {
		crc ^= static_cast<uint32_t>(p[i]) << 24;
		for (int bit = 0; bit < 8; ++bit)
			crc = (crc & 0x80000000u) ? (crc << 1) ^ 0x04c11db7u : crc << 1;
	}
	return crc;
}

void append(Bytes &out, const Bytes &more) { out.insert(out.end(), more.begin(), more.end()); }

// One 188-byte packet; a short payload is padded with an adaptation field.
Bytes ts_packet(uint16_t pid, bool pusi, uint8_t cc, const Bytes &payload,
                bool tei = false, uint8_t scrambling = 0)
{
	assert(payload.size() <= 184);
	Bytes p(188, 0xff);
	p[0] = 0x47;
	p[1] = static_cast<uint8_t>((tei ? 0x80 : 0) | (pusi ? 0x40 : 0) | (pid >> 8));
	p[2] = static_cast<uint8_t>(pid);
	const bool stuffed = payload.size() < 184;
	p[3] = static_cast<uint8_t>((scrambling << 6) | (stuffed ? 0x30 : 0x10) | (cc & 15));
	size_t at = 4;
	if (stuffed) {
		const size_t field = 183 - payload.size();
		p[at++] = static_cast<uint8_t>(field);
		if (field) {
			p[at++] = 0; // flags; the rest is 0xff stuffing
			at += field - 1;
		}
	}
	std::copy(payload.begin(), payload.end(), p.begin() + at);
	return p;
}

Bytes section(uint8_t table, uint16_t id, const Bytes &body)
{
	Bytes s = { table, 0, 0, static_cast<uint8_t>(id >> 8), static_cast<uint8_t>(id), 0xc1, 0, 0 };
	append(s, body);
	const size_t length = s.size() - 3 + 4;
	s[1] = static_cast<uint8_t>(0xb0 | (length >> 8));
	s[2] = static_cast<uint8_t>(length);
	const uint32_t crc = crc32_mpeg(s.data(), s.size());
	for (int shift = 24; shift >= 0; shift -= 8)
		s.push_back(static_cast<uint8_t>(crc >> shift));
	return s;
}

struct Stream {
	uint8_t type;
	uint16_t pid;
};

// PAT (program 1 on pid 0x100) and the PMT naming `streams`, as two packets.
Bytes psi(const std::vector<Stream> &streams)
{
	Bytes pat_body = { 0, 1, 0xe1, 0x00 };
	Bytes payload = { 0 }; // pointer field
	append(payload, section(0, 1, pat_body));
	Bytes out = ts_packet(0, true, 0, payload);
	Bytes pmt_body = { 0xe1, 0x01, 0xf0, 0x00 };
	for (const auto &s : streams) {
		pmt_body.push_back(s.type);
		pmt_body.push_back(static_cast<uint8_t>(0xe0 | (s.pid >> 8)));
		pmt_body.push_back(static_cast<uint8_t>(s.pid));
		pmt_body.push_back(0xf0);
		pmt_body.push_back(0);
	}
	payload = { 0 };
	append(payload, section(2, 1, pmt_body));
	append(out, ts_packet(0x100, true, 0, payload));
	return out;
}

Bytes pts_field(uint8_t marker, uint64_t pts)
{
	return { static_cast<uint8_t>(marker | ((pts >> 29) & 0x0e) | 1),
		static_cast<uint8_t>(pts >> 22), static_cast<uint8_t>(((pts >> 14) & 0xfe) | 1),
		static_cast<uint8_t>(pts >> 7), static_cast<uint8_t>(((pts << 1) & 0xfe) | 1) };
}

// An MPEG-2 PES packet; length 0 (unbounded) when `bounded` is false.
Bytes pes(uint8_t sid, uint64_t pts, const Bytes &payload, bool bounded = true,
          bool with_dts = false)
{
	Bytes header_data = pts_field(with_dts ? 0x31 : 0x21, pts);
	if (with_dts)
		append(header_data, pts_field(0x11, pts - 3600));
	Bytes p = { 0, 0, 1, sid, 0, 0, 0x80, static_cast<uint8_t>(with_dts ? 0xc0 : 0x80),
		static_cast<uint8_t>(header_data.size()) };
	append(p, header_data);
	append(p, payload);
	if (bounded) {
		const size_t length = p.size() - 6;
		assert(length <= 65535);
		p[4] = static_cast<uint8_t>(length >> 8);
		p[5] = static_cast<uint8_t>(length);
	}
	return p;
}

// The packets of one PID carrying `data`, counting from `cc`.
Bytes packetize(uint16_t pid, const Bytes &data, uint8_t &cc)
{
	Bytes out;
	for (size_t at = 0; at < data.size(); at += 184) {
		const size_t take = std::min<size_t>(184, data.size() - at);
		append(out, ts_packet(pid, at == 0, cc++,
			Bytes(data.begin() + at, data.begin() + at + take)));
	}
	return out;
}

Bytes payload_of(size_t length, uint8_t seed)
{
	Bytes b(length);
	for (size_t i = 0; i < length; ++i)
		b[i] = static_cast<uint8_t>(seed + i * 7);
	return b;
}

struct Packet {
	uint8_t sid;
	bool has_pts;
	uint64_t pts;
	Bytes payload;
};

uint64_t read_pts(const uint8_t *p)
{
	return (static_cast<uint64_t>((p[0] >> 1) & 7) << 30) | (static_cast<uint64_t>(p[1]) << 22) |
		(static_cast<uint64_t>(p[2] >> 1) << 15) | (static_cast<uint64_t>(p[3]) << 7) | (p[4] >> 1);
}

// Splits a Program Stream written in the MPEG-1 form. The pack header, and
// the system header with its stream counts, come first.
std::vector<Packet> parse_mpeg1(const Bytes &ps, int &audio_streams, int &video_streams)
{
	static const uint8_t pack[] = { 0, 0, 1, 0xba, 0x21, 0x00, 0x01, 0x00, 0x01, 0x80, 0x00, 0x01 };
	assert(ps.size() >= 24 && std::memcmp(ps.data(), pack, sizeof(pack)) == 0);
	assert(ps[12] == 0 && ps[13] == 0 && ps[14] == 1 && ps[15] == 0xbb && ps[16] == 0 && ps[17] == 6);
	audio_streams = ps[21] >> 2;
	video_streams = ps[22] & 0x1f;
	std::vector<Packet> packets;
	size_t at = 24;
	while (at < ps.size()) {
		assert(at + 7 <= ps.size() && ps[at] == 0 && ps[at + 1] == 0 && ps[at + 2] == 1);
		const size_t length = (ps[at + 4] << 8) | ps[at + 5];
		assert(at + 6 + length <= ps.size());
		Packet p;
		p.sid = ps[at + 3];
		size_t body = at + 6;
		if (ps[body] == 0x0f) {
			p.has_pts = false;
			p.pts = 0;
			body += 1;
		} else {
			assert((ps[body] & 0xf1) == 0x21);
			p.has_pts = true;
			p.pts = read_pts(&ps[body]);
			body += 5;
		}
		p.payload.assign(ps.begin() + body, ps.begin() + at + 6 + length);
		packets.push_back(std::move(p));
		at += 6 + length;
	}
	return packets;
}

Bytes run(TsDemux::Form form, const Bytes &input, size_t chunk = 0)
{
	TsDemux demux(form);
	Bytes out;
	if (chunk == 0)
		chunk = input.size();
	for (size_t at = 0; at < input.size(); at += chunk)
		demux.feed(input.data() + at, std::min(chunk, input.size() - at), out);
	demux.finish(out);
	return out;
}

Bytes concat(const std::vector<Packet> &packets, uint8_t sid)
{
	Bytes all;
	for (const auto &p : packets)
		if (p.sid == sid)
			append(all, p.payload);
	return all;
}

// A short stream: PSI, then interleaved video and audio PES packets.
struct Sample {
	Bytes ts;
	Bytes video, audio;
};

Sample sample()
{
	Sample s;
	append(s.ts, psi({ { 0x81, 0x103 }, { 0x02, 0x101 }, { 0x1b, 0x104 }, { 0x03, 0x102 },
		{ 0x04, 0x105 } }));
	uint8_t video_cc = 0, audio_cc = 0, junk_cc = 0;
	for (int i = 0; i < 6; ++i) {
		const Bytes v = payload_of(300 + i * 91, static_cast<uint8_t>(i));
		const Bytes a = payload_of(40 + i * 5, static_cast<uint8_t>(0x80 + i));
		append(s.video, v);
		append(s.audio, a);
		append(s.ts, packetize(0x101, pes(0xe0, 90000 + i * 3600, v, true, true), video_cc));
		append(s.ts, packetize(0x102, pes(0xc0, 90000 + i * 2160, a), audio_cc));
		// Streams that are not followed: AC-3, an H.264 stream, the second
		// MPEG audio stream.
		append(s.ts, packetize(0x103, pes(0xbd, 1, payload_of(50, 9)), junk_cc));
		append(s.ts, packetize(0x104, pes(0xe0, 1, payload_of(50, 9)), junk_cc));
		append(s.ts, packetize(0x105, pes(0xc1, 1, payload_of(50, 9)), junk_cc));
	}
	return s;
}

void check_basic()
{
	const Sample s = sample();
	int audio_streams = 0, video_streams = 0;
	const Bytes out = run(TsDemux::Form::mpeg1, s.ts);
	const auto packets = parse_mpeg1(out, audio_streams, video_streams);
	assert(audio_streams == 1 && video_streams == 1);
	// Only the first video and the first MPEG audio stream, as MPEG-1 PES
	// packets with the PTS: video 0xe0, audio 0xc0.
	assert(packets.size() == 12);
	assert(concat(packets, 0xe0) == s.video && concat(packets, 0xc0) == s.audio);
	assert(packets[0].sid == 0xe0 && packets[0].has_pts && packets[0].pts == 90000);
	assert(packets[1].sid == 0xc0 && packets[1].pts == 90000);
	assert(packets[10].sid == 0xe0 && packets[10].pts == 90000 + 5 * 3600);

	// Any chunking of the input gives the same stream.
	for (size_t chunk : { size_t(1), size_t(7), size_t(187), size_t(188), size_t(189), size_t(1000) })
		assert(run(TsDemux::Form::mpeg1, s.ts, chunk) == out);

	// The MPEG-2 form is the pack header and the PES packets as the stream
	// carries them, DTS included, with the stream ids mapped.
	const Bytes ps2 = run(TsDemux::Form::mpeg2, s.ts);
	static const uint8_t pack2[] = { 0, 0, 1, 0xba, 0x44, 0x00, 0x04, 0x00, 0x04, 0x01, 0x01, 0x89, 0xc3, 0xf8 };
	assert(ps2.size() > 14 && std::memcmp(ps2.data(), pack2, sizeof(pack2)) == 0);
	size_t at = 14;
	int count = 0;
	Bytes video2;
	while (at < ps2.size()) {
		assert(ps2[at] == 0 && ps2[at + 1] == 0 && ps2[at + 2] == 1);
		const size_t length = (ps2[at + 4] << 8) | ps2[at + 5];
		assert((ps2[at + 6] & 0xc0) == 0x80);
		if (ps2[at + 3] == 0xe0) {
			assert(ps2[at + 7] == 0xc0 && ps2[at + 8] == 10); // PTS and DTS kept
			video2.insert(video2.end(), ps2.begin() + at + 9 + 10, ps2.begin() + at + 6 + length);
		}
		at += 6 + length;
		++count;
	}
	assert(count == 12 && video2 == s.video);
	for (size_t chunk : { size_t(1), size_t(100) })
		assert(run(TsDemux::Form::mpeg2, s.ts, chunk) == ps2);

	// A session with no audio codec follows the video stream only, and the
	// Program Stream declares no audio.
	TsDemux no_audio(TsDemux::Form::mpeg1, false);
	Bytes out3;
	no_audio.feed(s.ts.data(), s.ts.size(), out3);
	no_audio.finish(out3);
	const auto video_only = parse_mpeg1(out3, audio_streams, video_streams);
	assert(audio_streams == 0 && video_streams == 1 && !no_audio.has_audio());
	assert(video_only.size() == 6 && concat(video_only, 0xe0) == s.video);
}

void check_m2ts_and_noise()
{
	const Sample s = sample();
	const Bytes expected = run(TsDemux::Form::mpeg1, s.ts);
	// 192-byte packets: a 4-byte timestamp before each.
	Bytes m2ts;
	for (size_t at = 0; at < s.ts.size(); at += 188) {
		m2ts.insert(m2ts.end(), { 0x12, 0x34, 0x56, 0x78 });
		m2ts.insert(m2ts.end(), s.ts.begin() + at, s.ts.begin() + at + 188);
	}
	assert(run(TsDemux::Form::mpeg1, m2ts) == expected);
	assert(run(TsDemux::Form::mpeg1, m2ts, 13) == expected);
	// Junk before the first packet is skipped until three sync bytes line up.
	Bytes noisy = payload_of(301, 3);
	std::replace(noisy.begin(), noisy.end(), uint8_t(0x47), uint8_t(0x46));
	append(noisy, s.ts);
	assert(run(TsDemux::Form::mpeg1, noisy) == expected);
	TsDemux demux(TsDemux::Form::mpeg1);
	Bytes out;
	demux.feed(noisy.data(), noisy.size(), out);
	demux.finish(out);
	assert(demux.has_pmt() && demux.has_video() && demux.has_audio());
	// Damage between two packets: the stream locks again and loses at most
	// the packets it cut.
	Bytes damaged(s.ts.begin(), s.ts.begin() + 188 * 20);
	damaged.insert(damaged.end(), 11, 0x55);
	damaged.insert(damaged.end(), s.ts.begin() + 188 * 20, s.ts.end());
	TsDemux d2(TsDemux::Form::mpeg1);
	Bytes out2;
	d2.feed(damaged.data(), damaged.size(), out2);
	d2.finish(out2);
	assert(d2.sync_losses() == 1);
	int a = 0, v = 0;
	const auto packets = parse_mpeg1(out2, a, v);
	assert(packets.size() >= 10 && packets.size() <= 12);
}

void check_errors()
{
	// A lost packet cuts the PES in progress, which is dropped; the next
	// PES is whole.
	Bytes ts = psi({ { 0x02, 0x101 } });
	uint8_t cc = 0;
	const Bytes first = payload_of(600, 1), second = payload_of(500, 2), third = payload_of(400, 3);
	const Bytes p1 = packetize(0x101, pes(0xe0, 1000, first), cc);
	append(ts, p1);
	// The second PES loses its middle packet (the counter jumps).
	Bytes p2 = packetize(0x101, pes(0xe0, 2000, second), cc);
	p2.erase(p2.begin() + 188, p2.begin() + 376);
	append(ts, p2);
	append(ts, packetize(0x101, pes(0xe0, 3000, third), cc));
	TsDemux demux(TsDemux::Form::mpeg1);
	Bytes out;
	demux.feed(ts.data(), ts.size(), out);
	demux.finish(out);
	assert(demux.continuity_errors() == 1);
	int a = 0, v = 0;
	auto packets = parse_mpeg1(out, a, v);
	assert(a == 0 && v == 1);
	assert(packets.size() == 2 && packets[0].payload == first && packets[1].payload == third);

	// A transport error packet is skipped, a repeated packet ignored.
	ts = psi({ { 0x02, 0x101 } });
	cc = 0;
	Bytes good = packetize(0x101, pes(0xe0, 1000, first), cc);
	Bytes with_dup(good.begin(), good.begin() + 188);
	append(with_dup, Bytes(good.begin(), good.begin() + 188)); // the same packet again
	append(with_dup, Bytes(good.begin() + 188, good.end()));
	append(ts, with_dup);
	TsDemux d2(TsDemux::Form::mpeg1);
	out.clear();
	d2.feed(ts.data(), ts.size(), out);
	d2.finish(out);
	assert(d2.duplicate_packets() == 1);
	packets = parse_mpeg1(out, a, v);
	assert(packets.size() == 1 && packets[0].payload == first);

	// A scrambled packet and a packet without payload carry nothing.
	ts = psi({ { 0x02, 0x101 } });
	cc = 0;
	append(ts, ts_packet(0x101, true, cc++, pes(0xe0, 1, payload_of(100, 1)), false, 2));
	Bytes no_payload(188, 0xff);
	no_payload[0] = 0x47; no_payload[1] = 0x01; no_payload[2] = 0x01; no_payload[3] = 0x20 | (cc & 15);
	no_payload[4] = 183;
	no_payload[5] = 0;
	append(ts, no_payload);
	TsDemux d3(TsDemux::Form::mpeg1);
	out.clear();
	d3.feed(ts.data(), ts.size(), out);
	d3.finish(out);
	packets = parse_mpeg1(out, a, v);
	assert(packets.empty());
	// A transport-error packet is counted and dropped.
	ts = psi({ { 0x02, 0x101 } });
	cc = 0;
	Bytes erroneous = ts_packet(0x101, true, cc++, pes(0xe0, 1, payload_of(100, 1)), true);
	append(ts, erroneous);
	append(ts, ts_packet(0x101, true, cc++, pes(0xe0, 2, payload_of(100, 2))));
	TsDemux d4(TsDemux::Form::mpeg1);
	out.clear();
	d4.feed(ts.data(), ts.size(), out);
	d4.finish(out);
	assert(d4.error_packets() == 1);
	packets = parse_mpeg1(out, a, v);
	assert(packets.size() == 1 && packets[0].pts == 2);
}

void check_unbounded_and_end()
{
	// A video PES of length 0 runs to the next one, and goes out in chunks
	// whose payload adds up; only the first carries the PTS.
	Bytes ts = psi({ { 0x02, 0x101 }, { 0x03, 0x102 } });
	uint8_t video_cc = 0, audio_cc = 0;
	const Bytes big = payload_of(150000, 5), tail = payload_of(777, 6);
	append(ts, packetize(0x101, pes(0xe0, 4000, big, false), video_cc));
	append(ts, packetize(0x102, pes(0xc0, 4000, payload_of(100, 8)), audio_cc));
	append(ts, packetize(0x101, pes(0xe0, 8000, tail, false), video_cc));
	TsDemux demux(TsDemux::Form::mpeg1);
	Bytes out;
	demux.feed(ts.data(), ts.size(), out);
	demux.finish(out); // the last unbounded PES ends with the input
	int a = 0, v = 0;
	const auto packets = parse_mpeg1(out, a, v);
	assert(a == 1 && v == 1);
	size_t first_chunks = 0;
	Bytes video_payload;
	for (const auto &p : packets) {
		if (p.sid != 0xe0)
			continue;
		if (p.has_pts && p.pts == 4000)
			++first_chunks;
		else if (p.has_pts)
			assert(p.pts == 8000);
		append(video_payload, p.payload);
	}
	assert(first_chunks == 1);
	Bytes expected = big;
	append(expected, tail);
	assert(video_payload == expected);
	assert(concat(packets, 0xc0) == payload_of(100, 8));

	// A bounded PES that never reaches its length is dropped at the end.
	ts = psi({ { 0x02, 0x101 } });
	video_cc = 0;
	Bytes cut = packetize(0x101, pes(0xe0, 1, payload_of(500, 1)), video_cc);
	cut.resize(188 * 2);
	append(ts, cut);
	TsDemux d2(TsDemux::Form::mpeg1);
	out.clear();
	d2.feed(ts.data(), ts.size(), out);
	d2.finish(out);
	assert(d2.truncated_pes() == 1);
	assert(parse_mpeg1(out, a, v).empty());
}

// A PES whose first packet holds only `first` bytes of it (an adaptation
// field takes the rest of the payload): the start code, stream id, length,
// flags and PTS can each straddle two packets, and the stream is the same.
void check_split_headers()
{
	for (bool bounded : { true, false }) {
		for (uint8_t sid : { uint8_t(0xe0), uint8_t(0xc0) }) {
			const bool video = sid == 0xe0;
			const uint16_t pid = video ? 0x101 : 0x102;
			const Bytes payload = payload_of(30, 9);
			const Bytes whole = pes(sid, 123456, payload, bounded);
			auto build = [&](size_t first) {
				Bytes ts = psi({ { 0x02, 0x101 }, { 0x03, 0x102 } });
				uint8_t cc = 0;
				if (first == 0) {
					append(ts, packetize(pid, whole, cc));
				} else {
					append(ts, ts_packet(pid, true, cc++, Bytes(whole.begin(), whole.begin() + first)));
					append(ts, ts_packet(pid, false, cc++, Bytes(whole.begin() + first, whole.end())));
				}
				// The next PES ends an unbounded one.
				append(ts, ts_packet(pid, true, cc++, pes(sid, 999999, payload_of(10, 1), bounded)));
				return ts;
			};
			const Bytes reference = run(TsDemux::Form::mpeg1, build(0));
			int a = 0, v = 0;
			const auto expected = parse_mpeg1(reference, a, v);
			assert(expected.size() == 2 && expected[0].payload == payload && expected[0].pts == 123456);
			// Every split of the 14 header bytes (9 + the 5-byte PTS), and a
			// few into the payload.
			for (size_t first = 1; first <= 16; ++first) {
				for (size_t chunk : { size_t(0), size_t(1), size_t(50) }) {
					const Bytes ts = build(first);
					TsDemux demux(TsDemux::Form::mpeg1);
					Bytes out;
					if (chunk == 0) {
						demux.feed(ts.data(), ts.size(), out);
					} else {
						for (size_t at = 0; at < ts.size(); at += chunk)
							demux.feed(ts.data() + at, std::min(chunk, ts.size() - at), out);
					}
					demux.finish(out);
					assert(out == reference);
					assert(demux.truncated_pes() == 0);
				}
			}
		}
	}
	// A split header that is not a PES start code is dropped, as a whole
	// header that is not one is.
	Bytes ts = psi({ { 0x02, 0x101 } });
	uint8_t cc = 0;
	const Bytes bad = { 0, 0, 2, 0xe0, 0, 0, 0x80, 0x00, 0x00 };
	append(ts, ts_packet(0x101, true, cc++, Bytes(bad.begin(), bad.begin() + 3)));
	append(ts, ts_packet(0x101, false, cc++, Bytes(bad.begin() + 3, bad.end())));
	TsDemux demux(TsDemux::Form::mpeg1);
	Bytes out;
	demux.feed(ts.data(), ts.size(), out);
	demux.finish(out);
	int a = 0, v = 0;
	assert(parse_mpeg1(out, a, v).empty());
	// A header cut off by the end of the input counts as a truncated PES.
	ts = psi({ { 0x02, 0x101 } });
	cc = 0;
	append(ts, ts_packet(0x101, true, cc++, Bytes{ 0, 0, 1, 0xe0 }));
	TsDemux d2(TsDemux::Form::mpeg1);
	out.clear();
	d2.feed(ts.data(), ts.size(), out);
	d2.finish(out);
	assert(d2.truncated_pes() == 1 && parse_mpeg1(out, a, v).empty());
}

void check_missing_tables()
{
	// No PAT or PMT: nothing is followed; the headers still come out at
	// the end, with no streams.
	Bytes ts;
	uint8_t cc = 0;
	for (int i = 0; i < 4; ++i)
		append(ts, packetize(0x101, pes(0xe0, 1, payload_of(100, 1)), cc));
	TsDemux demux(TsDemux::Form::mpeg1);
	Bytes out;
	demux.feed(ts.data(), ts.size(), out);
	demux.finish(out);
	assert(!demux.has_pmt());
	int a = 9, v = 9;
	assert(parse_mpeg1(out, a, v).empty() && a == 0 && v == 0);
	// A PMT that names neither MPEG video nor MPEG audio.
	ts = psi({ { 0x1b, 0x101 }, { 0x81, 0x102 } });
	TsDemux d2(TsDemux::Form::mpeg1);
	out.clear();
	d2.feed(ts.data(), ts.size(), out);
	d2.finish(out);
	assert(!d2.has_pmt() && !d2.has_video() && !d2.has_audio());
	// Audio only.
	ts = psi({ { 0x04, 0x102 } });
	cc = 0;
	append(ts, packetize(0x102, pes(0xc0, 1, payload_of(100, 1)), cc));
	TsDemux d3(TsDemux::Form::mpeg1);
	out.clear();
	d3.feed(ts.data(), ts.size(), out);
	d3.finish(out);
	const auto packets = parse_mpeg1(out, a, v);
	assert(a == 1 && v == 0 && packets.size() == 1 && packets[0].sid == 0xc0);
	// Nothing at all.
	TsDemux d4(TsDemux::Form::mpeg2);
	out.clear();
	d4.finish(out);
	assert(out.size() == 14);
}
} // namespace

int main()
{
	check_basic();
	check_m2ts_and_noise();
	check_errors();
	check_unbounded_and_end();
	check_missing_tables();
	check_split_headers();
	return 0;
}
