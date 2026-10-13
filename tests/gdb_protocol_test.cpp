#include "include/gdb_protocol.h"
#include <cstdio>
#include <cstdlib>

using namespace winuae_gdb;
static void require(bool condition, const char* message)
{
	if (!condition) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
static std::string frame(const std::string& s)
{
	unsigned sum = 0;
	for (unsigned char c : s) sum += c;
	char checksum[4]; std::snprintf(checksum, sizeof(checksum), "#%02x", sum & 255);
	return "$" + s + checksum;
}
struct FakeTarget : Target {
	Registers values{};
	unsigned writes = 0, interrupts = 0, runs = 0, breaks = 0;
	bool stepped = false;
	std::string memory_map() override { return "<memory-map/>"; }
	Registers registers() override { return values; }
	bool write_registers(const Registers& v, uint32_t mask) override {
		++writes;
		for (unsigned i = 0; i < 18; ++i) if (mask & (1u << i)) values[i] = v[i];
		return true;
	}
	bool read_memory(uint32_t, size_t n, std::vector<uint8_t>& v) override { v.assign(n, 0xab); return true; }
	bool write_memory(uint32_t, const std::vector<uint8_t>&) override { ++writes; return true; }
	bool breakpoint(bool, unsigned, uint32_t, uint32_t) override { ++breaks; return true; }
	void interrupt() override { ++interrupts; }
	void resume(bool s) override { ++runs; stepped = s; }
	bool resume_range(uint32_t start, uint32_t end) override {
		++runs; values[0] = start; values[1] = end; return true;
	}
	void detach() override {}
};
static std::string send(Session& session, const std::string& command)
{
	std::string packet = frame(command);
	session.receive(packet.data(), packet.size());
	return session.take_output();
}

int main()
{
	FakeTarget target;
	Session s(target);
	require(send(s, "qXfer:memory-map:read::0,8") == "+" + frame("m<memory-"), "memory map first chunk");
	require(send(s, "qXfer:memory-map:read::8,20") == "+" + frame("lmap/>"), "memory map last chunk");
	require(send(s, "qXfer:memory-map:read::100,8") == "+" + frame("l"), "memory map end");
	require(send(s, "qXfer:memory-map:read::0,0") == "+" + frame("E01"), "memory map zero request");
	// Every split, including either checksum digit, must preserve the frame.
	const auto packet = frame("qSupported");
	for (size_t split = 1; split < packet.size(); ++split) {
		Session fragmented(target);
		fragmented.receive(packet.data(), split);
		require(fragmented.take_output().empty(), "premature response to partial packet");
		fragmented.receive(packet.data() + split, packet.size() - split);
		require(fragmented.take_output().find("PacketSize=10000") != std::string::npos, "fragmented packet lost");
	}
	std::string pair = frame("qC") + frame("qAttached");
	s.receive(pair.data(), pair.size());
	require(s.take_output() == "+" + frame("QC1") + "+" + frame("1"), "coalesced packets lost");
	s.receive("-", 1);
	require(s.take_output() == frame("1"), "negative ack must retransmit");
	s.receive("$g#00", 5);
	require(s.take_output() == "-", "bad checksum accepted");
	require(send(s, "M1000,1:ab") == "+" + frame("E16"), "running memory mutation accepted");
	require(send(s, "?") == "+", "stop query replied before CPU stopped");
	require(target.interrupts == 1, "stop query did not interrupt");
	s.stop();
	require(s.take_output() == frame("S05"), "missing delayed stop reply");
	require(send(s, "QStartNoAckMode") == "+" + frame("OK"), "no-ack transition");
	require(send(s, "p0") == frame("00000000"), "no-ack mode still acknowledges");
	const char* invalid[] = {"M1000,1000:00", "M1000,1:zz", "M1000,ffffffff:00",
		"Mffffffff,2:0000", "M1000,1:0", "M1000:00,1", "M,1:00", "M1000,0:ab",
		"P0=", "P0=xyzabcde", "P12=00000000", "P=00000000", "G", "gextra"};
	for (const char* command : invalid) {
		auto out = send(s, command);
		require(out != frame("OK"), command);
	}
	require(target.writes == 0, "invalid packet changed state");
	require(send(s, "P0=12345678") == frame("OK") && target.values[0] == 0x12345678, "single register write");
	std::string all = "G" + std::string(144, '0');
	all.back() = 'z'; send(s, all);
	require(target.writes == 1, "partial register file written before validation");
	all.back() = '0'; require(send(s, all) == frame("OK"), "register file write");
	require(send(s, "m1000,2") == frame("abab"), "memory read");
	require(send(s, "M1000,2:abcd") == frame("OK"), "memory write");
	require(send(s, "Z0,1000,2") == frame("OK") && target.breaks == 1, "breakpoint set");
	require(send(s, "Z4,ffffffff,2") == frame("E01") && target.breaks == 1, "watchpoint wrap accepted");
	require(send(s, "vCont?") == frame("vCont;c;s;r"), "range stepping not advertised");
	for (const char* bad : {"vCont;r", "vCont;r1,", "vCont;r2,1", "vCont;r0,100000000",
		"vCont;r0,2:2", "vCont;r0,2;c", "vCont;r0,2:1;s"})
		require(send(s, bad) == frame("E01"), "malformed range accepted");
	require(send(s, "vCont;r1000,1004:1").empty() && target.values[0] == 0x1000 &&
		target.values[1] == 0x1004, "range step arguments");
	require(send(s, "vCont;r1000,1004") == frame("E16"), "range accepted while running");
	s.stop(); s.take_output();
	require(send(s, "vCont;r1000,1000").empty(), "empty range must single step");
	s.stop(); s.take_output();
	require(send(s, "vCont;s").empty() && target.stepped, "step must wait for stop");
	s.stop(); require(s.take_output() == frame("S05"), "step stop reply");
	require(send(s, "vCont;c").empty() && !target.stepped, "continue");
	s.receive("\003", 1); require(target.interrupts == 2, "interrupt byte");
	s.stop("T05watch:00001000;");
	require(s.take_output() == frame("T05watch:00001000;"), "watchpoint stop reason");
	require(send(s, "qRcmd,70726f66696c65") == frame(""), "unsupported packet");
	require(send(s, "D") == frame("OK") && s.finished(), "detach");
	Session oversized(target);
	std::string huge = "$" + std::string(Session::packet_size + 5, 'x');
	oversized.receive(huge.data(), huge.size());
	require(oversized.finished(), "unbounded incomplete packet");
	std::puts("GDB protocol tests passed");
}
