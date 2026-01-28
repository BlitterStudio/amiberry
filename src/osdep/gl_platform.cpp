/*
 * gl_platform.cpp - OpenGL platform abstraction implementation
 *
 * Loads OpenGL extension functions via SDL_GL_GetProcAddress on desktop platforms.
 * On Android/GLES3, this file is mostly empty as functions are provided directly.
 */

#ifdef USE_OPENGL

#include "gl_platform.h"
#include "sysdeps.h"

#ifndef __ANDROID__
// Desktop: Define the function pointers
PFNGLGENVERTEXARRAYSPROC glp_GenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glp_BindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glp_DeleteVertexArrays = nullptr;
PFNGLISVERTEXARRAYPROC glp_IsVertexArray = nullptr;
PFNGLGENBUFFERSPROC glp_GenBuffers = nullptr;
PFNGLBINDBUFFERPROC glp_BindBuffer = nullptr;
PFNGLDELETEBUFFERSPROC glp_DeleteBuffers = nullptr;
PFNGLISBUFFERPROC glp_IsBuffer = nullptr;
PFNGLBUFFERDATAPROC glp_BufferData = nullptr;
PFNGLCREATESHADERPROC glp_CreateShader = nullptr;
PFNGLSHADERSOURCEPROC glp_ShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glp_CompileShader = nullptr;
PFNGLGETSHADERIVPROC glp_GetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glp_GetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glp_DeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glp_CreateProgram = nullptr;
PFNGLATTACHSHADERPROC glp_AttachShader = nullptr;
PFNGLBINDATTRIBLOCATIONPROC glp_BindAttribLocation = nullptr;
PFNGLLINKPROGRAMPROC glp_LinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glp_GetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glp_GetProgramInfoLog = nullptr;
PFNGLDELETEPROGRAMPROC glp_DeleteProgram = nullptr;
PFNGLUSEPROGRAMPROC glp_UseProgram = nullptr;
PFNGLISPROGRAMPROC glp_IsProgram = nullptr;
PFNGLGETUNIFORMLOCATIONPROC glp_GetUniformLocation = nullptr;
PFNGLUNIFORM1IPROC glp_Uniform1i = nullptr;
PFNGLUNIFORM1FPROC glp_Uniform1f = nullptr;
PFNGLUNIFORM2FPROC glp_Uniform2f = nullptr;
PFNGLUNIFORM3FPROC glp_Uniform3f = nullptr;
PFNGLUNIFORMMATRIX4FVPROC glp_UniformMatrix4fv = nullptr;
PFNGLACTIVETEXTUREPROC glp_ActiveTexture = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glp_EnableVertexAttribArray = nullptr;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glp_DisableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glp_VertexAttribPointer = nullptr;
PFNGLVERTEXATTRIB4FPROC glp_VertexAttrib4f = nullptr;
PFNGLBINDFRAMEBUFFERPROC glp_BindFramebuffer = nullptr;
PFNGLGENFRAMEBUFFERSPROC glp_GenFramebuffers = nullptr;
PFNGLDELETEFRAMEBUFFERSPROC glp_DeleteFramebuffers = nullptr;
PFNGLFRAMEBUFFERTEXTURE2DPROC glp_FramebufferTexture2D = nullptr;

bool gl_platform_init()
{
	write_log("Loading OpenGL extension functions...\n");

	// VAO functions (try OES suffix as fallback for GLES compatibility)
	glp_GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays");
	if (!glp_GenVertexArrays) glp_GenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArraysOES");

	glp_BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray");
	if (!glp_BindVertexArray) glp_BindVertexArray = (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArrayOES");

	glp_DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArrays");
	if (!glp_DeleteVertexArrays) glp_DeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArraysOES");

	glp_IsVertexArray = (PFNGLISVERTEXARRAYPROC)SDL_GL_GetProcAddress("glIsVertexArray");
	if (!glp_IsVertexArray) glp_IsVertexArray = (PFNGLISVERTEXARRAYPROC)SDL_GL_GetProcAddress("glIsVertexArrayOES");

	// Buffer functions
	glp_GenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
	glp_BindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
	glp_DeleteBuffers = (PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
	glp_IsBuffer = (PFNGLISBUFFERPROC)SDL_GL_GetProcAddress("glIsBuffer");
	glp_BufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");

	// Shader functions
	glp_CreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
	glp_ShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
	glp_CompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
	glp_GetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
	glp_GetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
	glp_DeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");

	// Program functions
	glp_CreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
	glp_AttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
	glp_BindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glBindAttribLocation");
	glp_LinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
	glp_GetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
	glp_GetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
	glp_DeleteProgram = (PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
	glp_UseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
	glp_IsProgram = (PFNGLISPROGRAMPROC)SDL_GL_GetProcAddress("glIsProgram");

	// Uniform functions
	glp_GetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
	glp_Uniform1i = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
	glp_Uniform1f = (PFNGLUNIFORM1FPROC)SDL_GL_GetProcAddress("glUniform1f");
	glp_Uniform2f = (PFNGLUNIFORM2FPROC)SDL_GL_GetProcAddress("glUniform2f");
	glp_Uniform3f = (PFNGLUNIFORM3FPROC)SDL_GL_GetProcAddress("glUniform3f");
	glp_UniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4fv");

	// Vertex attribute functions
	glp_EnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
	glp_DisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glDisableVertexAttribArray");
	glp_VertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
	glp_VertexAttrib4f = (PFNGLVERTEXATTRIB4FPROC)SDL_GL_GetProcAddress("glVertexAttrib4f");

	// Texture and framebuffer functions
	glp_ActiveTexture = (PFNGLACTIVETEXTUREPROC)SDL_GL_GetProcAddress("glActiveTexture");
	glp_BindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
	glp_GenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
	glp_DeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
	glp_FramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");

	// Verify critical functions loaded successfully
	if (!glp_CreateShader || !glp_CreateProgram || !glp_GenBuffers || !glp_GenVertexArrays) {
		write_log("WARNING: Some OpenGL extension functions failed to load\n");
		write_log("  glCreateShader: %s\n", glp_CreateShader ? "OK" : "FAILED");
		write_log("  glCreateProgram: %s\n", glp_CreateProgram ? "OK" : "FAILED");
		write_log("  glGenBuffers: %s\n", glp_GenBuffers ? "OK" : "FAILED");
		write_log("  glGenVertexArrays: %s\n", glp_GenVertexArrays ? "OK" : "FAILED");
		return false;
	}

	write_log("OpenGL extension functions loaded successfully\n");
	return true;
}

#else
// Android: Functions are provided by GLES3, nothing to load
bool gl_platform_init()
{
	write_log("OpenGL ES 3.0 - no extension loading needed\n");
	return true;
}
#endif // __ANDROID__

#endif // USE_OPENGL
