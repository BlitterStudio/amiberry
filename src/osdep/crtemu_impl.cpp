/*
 * crtemu_impl.cpp - Compiles the crtemu and crt_frame single-header implementations
 *
 * This file exists solely to compile the CRTEMU_IMPLEMENTATION and CRT_FRAME_IMPLEMENTATION
 * sections of crtemu.h and crt_frame.h as a separate translation unit. Previously these were
 * compiled inline in amiberry_gfx.cpp via #define *_IMPLEMENTATION, causing 3400+ extra lines
 * to recompile on every edit to that file.
 *
 * Copyright 2025-2026 Dimitris Panokostas
 */

#ifdef USE_OPENGL

#include "gl_platform.h"

// Error reporting macros (must be defined before CRTEMU_IMPLEMENTATION)
#define CRTEMU_REPORT_SHADER_ERRORS
// Forward-declare write_log so crtemu can use it for error reporting
extern void write_log(const char* format, ...);
#define CRTEMU_REPORT_ERROR( str ) write_log( "%s\n", str )

#define CRTEMU_IMPLEMENTATION
#include "crtemu.h"

#define CRT_FRAME_IMPLEMENTATION
#include "crt_frame.h"

#endif // USE_OPENGL
