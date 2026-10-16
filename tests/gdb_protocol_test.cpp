#include "include/gdb_protocol.h"
#include "include/gdb_amiga.h"
#include "include/gdb_output.h"
#include "include/gdb_input.h"
#include "include/gdb_watch.h"
#include <cstring>
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
	MonitorResult monitor(const std::string& command, std::string& out) override {
		if (command == "test-query") { out = "{}"; return MonitorResult::ok; }
		if (command == "test-pending") return MonitorResult::pending;
		if (command == "test-arm") return MonitorResult::ok;
		if (command == "test-invalid") return MonitorResult::error;
		return MonitorResult::unsupported;
	}
	bool monitor_running(const std::string& command) override { return command == "test-query"; }
	std::string memory_map() override { return "<memory-map/>"; }
	std::string screenshot_path;
	bool screenshot(const std::string& path) override { screenshot_path = path; return true; }
	bool disassemble(uint32_t address, unsigned count, std::string& out) override {
		if (address != 0x1000 || count != 10) return false;
		out = "NOP\n"; return true;
	}
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
static std::string monitor(const std::string& command)
{
	std::string encoded = "qRcmd,";
	for (unsigned char c : command) {
		char pair[3]; std::snprintf(pair, sizeof(pair), "%02x", c); encoded += pair;
	}
	return encoded;
}
static std::string send(Session& session, const std::string& command)
{
	std::string packet = frame(command);
	session.receive(packet.data(), packet.size());
	return session.take_output();
}

struct GuestFixture : GuestReader {
	std::array<uint8_t, 8192> ram{};
	size_t reads = 0;
	bool read(uint32_t address, uint8_t* out, size_t length) override {
		++reads;
		if (uint64_t(address) + length > ram.size()) return false;
		std::memcpy(out, ram.data() + address, length); return true;
	}
	void put32(size_t address, uint32_t value) {
		for (int i = 3; i >= 0; --i, value >>= 8) ram[address + i] = value;
	}
	GuestFixture() {
		put32(4, 256); put32(256 + 276, 1024); ram[1024 + 8] = 13;
		put32(1024 + 10, 1600); std::memcpy(ram.data() + 1600, "Task", 5);
		put32(1024 + 172, 2048 / 4); put32(2048 + 16, 2400 / 4);
		ram[2400] = 10; std::memcpy(ram.data() + 2401, "SYS:C/Demo", 10);
		put32(2048 + 60, 4096 / 4);
	}
};
static void task_reader_tests()
{
	GuestFixture memory;
	std::vector<AmigaTask> tasks;
	require(read_tasks(memory, tasks) && tasks.size() == 1 && tasks[0].name == "Task", "current task inspection");
	memory.put32(256 + 420, 1024);
	memory.put32(1024, 256 + 424);
	memory.ram[1024 + 15] = 4; // TS_WAIT, with ThisTask retained by idle Exec.
	require(read_tasks(memory, tasks) && tasks.size() == 1 && tasks[0].state == 4,
		"waiting current task rejected or duplicated");
	memory.put32(256 + 406, 1024);
	require(!read_tasks(memory, tasks), "task in both queues accepted");
	memory.put32(256 + 420, 0);
	memory.put32(256 + 406, 1024);
	memory.put32(1024, 1024);
	require(!read_tasks(memory, tasks), "task list cycle accepted");
}

static void process_reader_tests()
{
	GuestFixture memory;
	AmigaProcess process;
	require(read_process(memory, 0, process) && process.address == 1024 &&
		process.name == "Task" && process.command == "SYS:C/Demo" && process.segments == 4096,
		"CLI process metadata");
	require(process_name_matches(process.command, "demo"), "process basename matching");
	require(!process_name_matches(process.command, "dem"), "process prefix must not match");
	std::vector<AmigaSegment> segments;
	memory.put32(4092, 24); memory.put32(4096, 5000 / 4);
	memory.put32(4996, 16); memory.put32(5000, 0);
	require(read_segments(memory, process.segments, segments) && segments.size() == 2 &&
		segments[0].address == 4100 && segments[0].size == 16 && segments[1].address == 5004,
		"segment load addresses and sizes");
	memory.put32(5000, 4096 / 4);
	require(!read_segments(memory, process.segments, segments), "cyclic segment list accepted");
	memory.put32(5000, 0); memory.put32(4996, 0xfffffffc);
	require(!read_segments(memory, process.segments, segments), "segment size overflow accepted");
	memory.put32(4996, 4);
	require(!read_segments(memory, process.segments, segments), "short segment header accepted");
	memory.put32(1024 + 172, 0); memory.put32(1024 + 128, 2048 / 4);
	memory.put32(2048 + 12, 4096 / 4);
	require(read_process(memory, 1024, process) && process.segments == 4096 && process.command.empty(), "Workbench process metadata");
	memory.put32(2048 + 12, 0x40000000);
	require(!read_process(memory, 1024, process), "BPTR overflow accepted");
	memory.put32(4, 0xfffffffc);
	require(!read_process(memory, 0, process), "ExecBase overflow accepted");
	memory.put32(1024 + 10, 1700); std::memset(memory.ram.data() + 1700, 'x', 255);
	memory.reads = 0;
	require(!read_process(memory, 1024, process) && memory.reads < 270, "unterminated guest name not bounded");
}

int main()
{
	WatchLog log;
	for (unsigned i = 0; i < 70; ++i) log.capture({0,1,i,2,2,1,0,0x10000,i,0,true});
	require(log.snapshot().find("\"dropped\":6") != std::string::npos &&
		log.snapshot(true).find("\"address\":69") != std::string::npos, "watch log bounds/order");
	log.clear(); require(log.snapshot(true) == "null", "stale watch evidence after clear");
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
	require(send(s, "p12") == frame(std::string(24, 'x')), "fp0 read must be 12 unavailable bytes");
	require(send(s, "p1a") == frame("xxxxxxxx"), "fpcontrol read must be 4 unavailable bytes");
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
	require(send(s, monitor("test-query")) == frame("7b7d") && !s.stopped(), "live query stopped CPU");
	require(send(s, monitor("test-arm")) == frame("E16"), "unsafe live monitor accepted");
	s.receive("\003", 1); require(target.interrupts == 2, "interrupt byte");
	s.stop("T05watch:00001000;");
	require(s.take_output() == frame("T05watch:00001000;"), "watchpoint stop reason");
	require(send(s, "qRcmd,70726f66696c65") == frame(""), "unsupported monitor command");
	require(send(s, monitor("screenshot /tmp/screen shot.png")) == frame("OK") &&
		target.screenshot_path == "/tmp/screen shot.png", "screenshot path with spaces");
	require(send(s, "qRcmd,00") == frame("E01"), "monitor NUL accepted");
	require(send(s, monitor("disasm 1000 10")) == frame("4e4f500a"), "decimal disassembly count");
	for (const char* command : {"disasm 1000 0", "disasm 1000 101", "disasm z 1", "disasm 1000 1x", "disasm 1000 "})
		require(send(s, monitor(command)) == frame("E01"), command);
	require(send(s, monitor("test-query")) == frame("7b7d"), "monitor response encoding");
	require(send(s, monitor("test-arm")) == frame("OK"), "monitor acknowledgement");
	require(send(s, monitor("test-invalid")) == frame("E01"), "monitor rejection");
	require(send(s, monitor("test-pending")).empty() && !s.stopped(), "deferred monitor acknowledged early");
	require(send(s, "P0=00000000") == frame("E16"), "state changed during deferred monitor");
	s.complete_monitor(true); s.stop();
	require(s.take_output() == frame("OK") && s.stopped(), "deferred monitor completion");
	s.complete_monitor(false); require(s.take_output().empty(), "duplicate monitor completion");
	require(send(s, monitor("test-pending")).empty(), "second deferred monitor");
	s.complete_monitor(false); require(s.take_output() == frame("E01"), "deferred failure hidden");
	struct Sink : InputSink { std::vector<InputEvent> events; void input(const InputEvent& e) override { events.push_back(e); } } sink;
	GuestInput input(sink);
	require(!input.event({0, 0, 0x80, 1}) && !input.event({2, 2, 0, 1}) &&
		!input.event({1, 0, 0, 128}) && sink.events.empty(), "invalid input delivered");
	require(input.event({0, 0, 0x45, 1}) && input.event({0, 0, 0x45, 1}) &&
		sink.events.size() == 1, "duplicate key press delivered");
	require(input.event({1, 1, 0, 0xffffff81}) && input.event({2, 1, 4, 1}), "valid input rejected");
	input.release();
	require(sink.events.size() == 5 && input.status().find("\"held\":[]") != std::string::npos, "held inputs not released");
	input.release(); require(sink.events.size() == 5, "release was not idempotent");
	require(!input.start({{3601, {0,0,0,1}}}) && !input.start({{0,{0,0,128,1}}}), "invalid sequence accepted");
	require(input.start({{0,{0,0,0x45,1}}, {3,{0,0,0x45,0}}}), "sequence rejected");
	require(!input.event({0,0,0x20,1}) && !input.start({{0,{3,0,0,0}}}), "active sequence overwritten");
	input.frame(); require(sink.events.size() == 6 && input.active(), "first frame input");
	input.frame(); input.frame(); require(sink.events.size() == 6, "release before frame deadline");
	input.frame(); require(sink.events.size() == 7 && !input.active(), "sequence completion");
	require(input.start({{0,{2,1,4,1}}, {20,{3,0,0,0}}}), "second sequence rejected");
	input.frame(); input.cancel(); require(!input.active() && sink.events.size() == 9, "cancel did not release");
	input.frame(); require(sink.events.size() == 9, "cancelled sequence advanced");
	struct CancellingSink : InputSink {
		GuestInput* guest = nullptr;
		std::vector<InputEvent> events;
		void input(const InputEvent& e) override {
			events.push_back(e);
			if (e.kind == 0 && e.code == 0x45 && e.value)
				guest->cancel();
		}
	} cancelling_sink;
	GuestInput cancelled_input(cancelling_sink);
	cancelling_sink.guest = &cancelled_input;
	require(cancelled_input.start({{0,{0,0,0x45,1}}, {0,{0,0,0x20,1}}}), "cancelling sequence rejected");
	cancelled_input.frame();
	require(!cancelled_input.active() && cancelling_sink.events.size() == 2 &&
		cancelling_sink.events[1].code == 0x45 && !cancelling_sink.events[1].value,
		"cancelled sequence dispatched another input");
	process_reader_tests();
	task_reader_tests();
	GuestOutput output;
	output.capture("disabled");
	require(output.snapshot().find("disabled") == std::string::npos, "guest capture enabled by default");
	output.enable(true);
	output.capture("hello\n");
	require(output.snapshot().find("hello\\u000a") != std::string::npos, "guest output escaping");
	GuestOutput boundary_output;
	boundary_output.enable(true);
	std::string boundary(1024, 'x');
	boundary_output.capture(boundary.c_str());
	require(boundary_output.snapshot().find("\"truncated\":false") != std::string::npos,
		"exact guest output boundary marked truncated");
	std::string flood(2048, char(255));
	for (int i = 0; i < 100; ++i) output.capture(flood.c_str());
	auto snapshot = output.snapshot();
	require(snapshot.size() < Session::packet_size / 2 && snapshot.find("\"truncated\":true") != std::string::npos &&
		snapshot.find("\"dropped\":97") != std::string::npos, "guest output flood not bounded");
	output.clear(true); output.capture("detached");
	require(output.snapshot() == "{\"enabled\":false,\"dropped\":0,\"records\":[]}", "guest output teardown");
	require(json_string("a\n\"") == "\"a\\u000a\\\"\"", "JSON escaping");
	require(send(s, "D") == frame("OK") && s.finished(), "detach");
	Session oversized(target);
	std::string huge = "$" + std::string(Session::packet_size + 5, 'x');
	oversized.receive(huge.data(), huge.size());
	require(oversized.finished(), "unbounded incomplete packet");
	std::puts("GDB protocol tests passed");
}
