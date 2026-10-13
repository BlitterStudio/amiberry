/* Optional WinUAE remote debugger. All calls run on the emulation thread. */
#ifndef WINUAE_GDB_SERVER_H
#define WINUAE_GDB_SERVER_H

#define DEBUG_FEATURE_GDBSERVER (1 << 2)

#ifdef DEBUGGER
namespace winuae_gdb { class Target; }
winuae_gdb::Target& debug_gdb_target();
void gdb_server_poll();
void gdb_server_close();
bool gdb_server_connected();
bool gdb_server_stop(const char* reason);
// CPU debugger hooks, implemented by the remote target.
bool gdb_target_check_stop(uaecptr pc, char* reason, size_t size);
void gdb_target_stopped(char* reason, size_t size);
#else
static inline void gdb_server_poll() {}
static inline void gdb_server_close() {}
static inline bool gdb_server_connected() { return false; }
static inline bool gdb_server_stop(const char*) { return false; }
static inline bool gdb_target_check_stop(uaecptr, char*, size_t) { return false; }
static inline void gdb_target_stopped(char*, size_t) {}
#endif
#endif
