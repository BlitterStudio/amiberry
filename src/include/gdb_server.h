/* Optional WinUAE remote debugger. Guest output capture also accepts trap threads. */
#ifndef WINUAE_GDB_SERVER_H
#define WINUAE_GDB_SERVER_H

#define DEBUG_FEATURE_GDBSERVER (1 << 2)

#if defined(DEBUGGER) && defined(GDBSERVER)
namespace winuae_gdb { class Target; }
winuae_gdb::Target& debug_gdb_target();
void gdb_server_poll();
void gdb_server_input_frame();
void gdb_server_guest_output(const char*, bool truncated = false);
void gdb_server_close();
void gdb_server_reset();
void gdb_server_begin_restore();
void gdb_server_restore_complete(bool);
void gdb_server_monitor_complete(bool);
bool gdb_server_connected();
bool gdb_server_halted();
bool gdb_server_stop(const char* reason);
// CPU debugger hooks, implemented by the remote target.
bool gdb_target_check_stop(uaecptr pc, char* reason, size_t size);
void gdb_target_stopped(char* reason, size_t size);
void gdb_target_exception(int nr);
void gdb_target_watch_hit(int slot, uaecptr addr, int size, int rwi, uae_u32 source, uae_u32 reg,
	uae_u32 value, uae_u32 old_value, bool old_valid);
#else
static inline void gdb_server_poll() {}
static inline void gdb_server_input_frame() {}
static inline void gdb_server_guest_output(const char*, bool = false) {}
static inline void gdb_server_close() {}
static inline void gdb_server_reset() {}
static inline void gdb_server_begin_restore() {}
static inline void gdb_server_restore_complete(bool) {}
static inline void gdb_server_monitor_complete(bool) {}
static inline bool gdb_server_connected() { return false; }
static inline bool gdb_server_halted() { return false; }
static inline bool gdb_server_stop(const char*) { return false; }
static inline bool gdb_target_check_stop(uaecptr, char*, size_t) { return false; }
static inline void gdb_target_stopped(char*, size_t) {}
static inline void gdb_target_exception(int) {}
static inline void gdb_target_watch_hit(int, uaecptr, int, int, uae_u32, uae_u32, uae_u32, uae_u32, bool) {}
#endif
#endif
