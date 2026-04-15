#pragma once

#include "sysconfig.h"

#if !defined(AMIBERRY_MACOS) && !defined(__ANDROID__) && !defined(AMIBERRY_IOS)
bool imgui_debugger_console_supported();
void imgui_debugger_console_open();
void imgui_debugger_console_close();
void imgui_debugger_console_activate();
void imgui_debugger_console_write(const char* text);
bool imgui_debugger_console_has_input();
char imgui_debugger_console_getch();
int imgui_debugger_console_get(char* out, int maxlen);
#else
inline bool imgui_debugger_console_supported() { return false; }
inline void imgui_debugger_console_open() {}
inline void imgui_debugger_console_close() {}
inline void imgui_debugger_console_activate() {}
inline void imgui_debugger_console_write(const char*) {}
inline bool imgui_debugger_console_has_input() { return false; }
inline char imgui_debugger_console_getch() { return 0; }
inline int imgui_debugger_console_get(char*, int) { return -1; }
#endif
