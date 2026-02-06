/*
 * gl_platform.h - OpenGL platform abstraction
 *
 * This header provides OpenGL function pointers for platforms that don't have
 * them available directly (i.e., desktop OpenGL without GLEW).
 *
 * On Android/GLES3: Functions are provided by <GLES3/gl3.h>
 * On Desktop: Functions are loaded via SDL_GL_GetProcAddress and accessed via macros
 */

#ifndef GL_PLATFORM_H
#define GL_PLATFORM_H

#ifdef USE_OPENGL

#ifdef __ANDROID__
// Android: GLES3 provides all functions directly
#include <GLES3/gl3.h>
#include <SDL_opengles2.h>

#else
// Desktop: Need to load extension functions dynamically
#include <SDL_opengl.h>

// Declare external function pointers (defined in gl_platform.cpp)
extern PFNGLGENVERTEXARRAYSPROC glp_GenVertexArrays;
extern PFNGLBINDVERTEXARRAYPROC glp_BindVertexArray;
extern PFNGLDELETEVERTEXARRAYSPROC glp_DeleteVertexArrays;
extern PFNGLISVERTEXARRAYPROC glp_IsVertexArray;
extern PFNGLGENBUFFERSPROC glp_GenBuffers;
extern PFNGLBINDBUFFERPROC glp_BindBuffer;
extern PFNGLDELETEBUFFERSPROC glp_DeleteBuffers;
extern PFNGLISBUFFERPROC glp_IsBuffer;
extern PFNGLBUFFERDATAPROC glp_BufferData;
extern PFNGLCREATESHADERPROC glp_CreateShader;
extern PFNGLSHADERSOURCEPROC glp_ShaderSource;
extern PFNGLCOMPILESHADERPROC glp_CompileShader;
extern PFNGLGETSHADERIVPROC glp_GetShaderiv;
extern PFNGLGETSHADERINFOLOGPROC glp_GetShaderInfoLog;
extern PFNGLDELETESHADERPROC glp_DeleteShader;
extern PFNGLCREATEPROGRAMPROC glp_CreateProgram;
extern PFNGLATTACHSHADERPROC glp_AttachShader;
extern PFNGLBINDATTRIBLOCATIONPROC glp_BindAttribLocation;
extern PFNGLLINKPROGRAMPROC glp_LinkProgram;
extern PFNGLGETPROGRAMIVPROC glp_GetProgramiv;
extern PFNGLGETPROGRAMINFOLOGPROC glp_GetProgramInfoLog;
extern PFNGLDELETEPROGRAMPROC glp_DeleteProgram;
extern PFNGLUSEPROGRAMPROC glp_UseProgram;
extern PFNGLISPROGRAMPROC glp_IsProgram;
extern PFNGLGETUNIFORMLOCATIONPROC glp_GetUniformLocation;
extern PFNGLUNIFORM1IPROC glp_Uniform1i;
extern PFNGLUNIFORM1FPROC glp_Uniform1f;
extern PFNGLUNIFORM2FPROC glp_Uniform2f;
extern PFNGLUNIFORM3FPROC glp_Uniform3f;
extern PFNGLUNIFORMMATRIX4FVPROC glp_UniformMatrix4fv;
extern PFNGLACTIVETEXTUREPROC glp_ActiveTexture;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glp_EnableVertexAttribArray;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glp_DisableVertexAttribArray;
extern PFNGLVERTEXATTRIBPOINTERPROC glp_VertexAttribPointer;
extern PFNGLVERTEXATTRIB4FPROC glp_VertexAttrib4f;
extern PFNGLBINDFRAMEBUFFERPROC glp_BindFramebuffer;
extern PFNGLGENFRAMEBUFFERSPROC glp_GenFramebuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC glp_DeleteFramebuffers;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glp_FramebufferTexture2D;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glp_CheckFramebufferStatus;
extern PFNGLGENERATEMIPMAPPROC glp_GenerateMipmap;

// Define macros to redirect standard GL calls to our function pointers
#define glGenVertexArrays glp_GenVertexArrays
#define glBindVertexArray glp_BindVertexArray
#define glDeleteVertexArrays glp_DeleteVertexArrays
#define glIsVertexArray glp_IsVertexArray
#define glGenBuffers glp_GenBuffers
#define glBindBuffer glp_BindBuffer
#define glDeleteBuffers glp_DeleteBuffers
#define glIsBuffer glp_IsBuffer
#define glBufferData glp_BufferData
#define glCreateShader glp_CreateShader
#define glShaderSource glp_ShaderSource
#define glCompileShader glp_CompileShader
#define glGetShaderiv glp_GetShaderiv
#define glGetShaderInfoLog glp_GetShaderInfoLog
#define glDeleteShader glp_DeleteShader
#define glCreateProgram glp_CreateProgram
#define glAttachShader glp_AttachShader
#define glBindAttribLocation glp_BindAttribLocation
#define glLinkProgram glp_LinkProgram
#define glGetProgramiv glp_GetProgramiv
#define glGetProgramInfoLog glp_GetProgramInfoLog
#define glDeleteProgram glp_DeleteProgram
#define glUseProgram glp_UseProgram
#define glIsProgram glp_IsProgram
#define glGetUniformLocation glp_GetUniformLocation
#define glUniform1i glp_Uniform1i
#define glUniform1f glp_Uniform1f
#define glUniform2f glp_Uniform2f
#define glUniform3f glp_Uniform3f
#define glUniformMatrix4fv glp_UniformMatrix4fv
#define glActiveTexture glp_ActiveTexture
#define glEnableVertexAttribArray glp_EnableVertexAttribArray
#define glDisableVertexAttribArray glp_DisableVertexAttribArray
#define glVertexAttribPointer glp_VertexAttribPointer
#define glVertexAttrib4f glp_VertexAttrib4f
#define glBindFramebuffer glp_BindFramebuffer
#define glGenFramebuffers glp_GenFramebuffers
#define glDeleteFramebuffers glp_DeleteFramebuffers
#define glFramebufferTexture2D glp_FramebufferTexture2D
#define glCheckFramebufferStatus glp_CheckFramebufferStatus
#define glGenerateMipmap glp_GenerateMipmap

#endif // __ANDROID__

// GL format constants that may not be defined on all platforms
#ifndef GL_RGBA16F
#define GL_RGBA16F 0x881A
#endif
#ifndef GL_SRGB8_ALPHA8
#define GL_SRGB8_ALPHA8 0x8C43
#endif
#ifndef GL_FRAMEBUFFER_COMPLETE
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_COLOR_ATTACHMENT0
#define GL_COLOR_ATTACHMENT0 0x8CE0
#endif
#ifndef GL_FRAMEBUFFER
#define GL_FRAMEBUFFER 0x8D40
#endif

// Initialize OpenGL function pointers (call after creating GL context)
// Returns true on success, false if critical functions failed to load
bool gl_platform_init();

#endif // USE_OPENGL

#endif // GL_PLATFORM_H
