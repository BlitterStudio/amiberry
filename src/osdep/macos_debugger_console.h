#pragma once

#include "sysconfig.h"

#if defined(AMIBERRY_MACOS)
bool macos_debugger_console_supported();
void macos_debugger_console_open();
void macos_debugger_console_close();
void macos_debugger_console_activate();
void macos_debugger_console_write(const char* text);
bool macos_debugger_console_has_input();
char macos_debugger_console_getch();
int macos_debugger_console_get(char* out, int maxlen);
#else
inline bool macos_debugger_console_supported() { return false; }
inline void macos_debugger_console_open() {}
inline void macos_debugger_console_close() {}
inline void macos_debugger_console_activate() {}
inline void macos_debugger_console_write(const char*) {}
inline bool macos_debugger_console_has_input() { return false; }
inline char macos_debugger_console_getch() { return 0; }
inline int macos_debugger_console_get(char*, int) { return -1; }
#endif
