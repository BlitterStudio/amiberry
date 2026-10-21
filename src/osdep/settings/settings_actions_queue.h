#pragma once

// Internal action queue of the runtime actions API (amiberry_actions.h).
// It knows nothing about the emulator: amiberry_actions.cpp supplies the
// executor, and the test harness supplies a stub, which is what keeps the
// ordering, cross-thread and completion semantics testable on their own.

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <new>
#include <string>
#include <utility>

namespace amiberry_actions {

enum class kind {
	insert_floppy,        // drive, path
	eject_floppy,         // drive
	set_swapper_entry,    // drive = entry index, path
	insert_swapper_entry, // drive = entry index, number = target drive
	insert_cd,            // path
	eject_cd,
	reset,                // flag_a = hard, flag_b = keyboard
	pause,
	resume,
	save_state,           // path
	load_state,           // path
	save_state_slot,      // slot
	load_state_slot,      // slot
	screenshot,           // path
	quit
};

struct request {
	uint64_t id = 0;
	kind k = kind::pause;
	int drive = 0;         // floppy drive, or swapper entry for the swapper kinds
	int number = 0;        // target drive for insert_swapper_entry, or slot
	bool flag_a = false;   // reset: hard
	bool flag_b = false;   // reset: keyboard
	std::string path;
};

// Executes one action on the main thread. Returns false and writes *error
// (never NULL) on failure. Must not touch the queue.
using executor_fn = bool (*)(const request& r, void* context, std::string* error);

class action_queue {
public:
	static constexpr size_t max_pending = 1024;
	static constexpr size_t max_results = 1024;

	// 0 when the queue is full or memory ran out; ids start at 1.
	uint64_t post(kind k, int drive, int number, bool flag_a, bool flag_b,
		const char* path)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (pending_.size() >= max_pending)
			return 0;
		try {
			pending_.push_back(request{next_id_, k, drive, number, flag_a, flag_b,
				path ? std::string(path) : std::string()});
		} catch (const std::bad_alloc&) {
			return 0;
		}
		last_assigned_ = next_id_;
		return next_id_++;
	}

	// Main thread only. Executes everything queued when the pass starts, in
	// order, and stores one result per action. Actions posted meanwhile wait
	// for the next pass, so a front end that keeps posting cannot hold the
	// emulator thread here. Never calls the executor with the queue locked.
	size_t run_pending(executor_fn execute, void* context)
	{
		if (executing_)
			return 0;
		executing_ = true;
		uint64_t last;
		{
			std::lock_guard<std::mutex> lock(mutex_);
			last = last_assigned_;
		}
		size_t executed = 0;
		request r;
		while (pop(r, last)) {
			std::string error;
			bool ok;
			try {
				ok = execute(r, context, &error);
			} catch (const std::exception& e) {
				ok = false;
				error = e.what();
			} catch (...) {
				ok = false;
				error = "unknown error";
			}
			finish(r.id, ok, error);
			executed++;
		}
		executing_ = false;
		return executed;
	}

	// 0 pending, 1 done, 2 failed, 3 unknown. Consumes the result when it
	// is done or failed.
	int poll(uint64_t id, std::string* error)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		const auto it = results_.find(id);
		if (it != results_.end()) {
			if (consumed_.count(id) != 0)
				return 3; // already polled
			const int state = it->second.first ? 1 : 2;
			if (error)
				error->assign(it->second.second);
			consumed_.insert(id);
			return state;
		}
		if (id == 0 || id > last_assigned_ || id <= retired_below_)
			return 3;
		return 0; // queued or executing
	}

	bool has_pending() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		return !pending_.empty();
	}

private:
	bool pop(request& out, uint64_t last)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (pending_.empty() || pending_.front().id > last)
			return false;
		out = std::move(pending_.front());
		pending_.pop_front();
		return true;
	}

	void finish(uint64_t id, bool ok, const std::string& error)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		try {
			results_.emplace(id, std::make_pair(ok, error));
			done_order_.push_back(id);
			while (done_order_.size() > max_results) {
				results_.erase(done_order_.front());
				consumed_.erase(done_order_.front());
				retired_below_ = std::max(retired_below_, done_order_.front());
				done_order_.pop_front();
			}
		} catch (const std::bad_alloc&) {
			// The result cannot be stored, but the action has run: retire
			// its id so a poll reports it unknown instead of pending
			// forever. Actions finish in posting order, so every lower id
			// has finished too and keeps whatever result it stored; erase
			// and the assignment do not allocate.
			results_.erase(id);
			retired_below_ = std::max(retired_below_, id);
		}
	}

	mutable std::mutex mutex_;
	std::deque<request> pending_;
	std::map<uint64_t, std::pair<bool, std::string>> results_;
	std::set<uint64_t> consumed_;
	std::deque<uint64_t> done_order_;
	uint64_t next_id_ = 1;
	uint64_t last_assigned_ = 0;
	uint64_t retired_below_ = 0;
	bool executing_ = false;
};

} // namespace amiberry_actions
