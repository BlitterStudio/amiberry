/*
------------------------------------------------------------------------------
          Licensing information can be found at the end of the file.
------------------------------------------------------------------------------

crtemu.h - v0.1 - Cathode ray tube emulation shader for C/C++.

Do this:
	#define CRTEMU_IMPLEMENTATION
before you include this file in *one* C/C++ file to create the implementation.
*/


#ifndef crtemu_h
#define crtemu_h

#ifndef CRTEMU_U32
	#define CRTEMU_U32 unsigned int
#endif
#ifndef CRTEMU_U64
	#define CRTEMU_U64 unsigned long long
#endif

typedef struct crtemu_t crtemu_t;

crtemu_t* crtemu_create( void* memctx );

void crtemu_destroy( crtemu_t* crtemu );

void crtemu_frame( crtemu_t* crtemu, CRTEMU_U32* frame_abgr, int frame_width, int frame_height );

void crtemu_present( crtemu_t* crtemu, CRTEMU_U64 time_us, CRTEMU_U32 const* pixels_xbgr, int width, int height, 
    CRTEMU_U32 mod_xbgr, CRTEMU_U32 border_xbgr );

#endif /* crtemu_h */

/*
----------------------
	IMPLEMENTATION
----------------------
*/
#ifdef CRTEMU_IMPLEMENTATION
#undef CRTEMU_IMPLEMENTATION

#define _CRT_NONSTDC_NO_DEPRECATE 
#define _CRT_SECURE_NO_WARNINGS
#include <stddef.h>
#include <string.h>
#include <math.h>

#ifndef CRTEMU_MALLOC
	#include <stdlib.h>
	#if defined(__cplusplus)
		#define CRTEMU_MALLOC( ctx, size ) ( ::malloc( size ) )
		#define CRTEMU_FREE( ctx, ptr ) ( ::free( ptr ) )
	#else
		#define CRTEMU_MALLOC( ctx, size ) ( malloc( size ) )
		#define CRTEMU_FREE( ctx, ptr ) ( free( ptr ) )
	#endif
#endif

#ifdef CRTEMU_REPORT_SHADER_ERRORS
    #ifndef CRTEMU_REPORT_ERROR
        #define _CRT_NONSTDC_NO_DEPRECATE 
        #define _CRT_SECURE_NO_WARNINGS
        #include <stdio.h>
        #define CRTEMU_REPORT_ERROR( str ) printf( "%s", str )
    #endif
#endif

extern "C"
    {
    __declspec(dllimport) struct HINSTANCE__* __stdcall LoadLibraryA( char const* lpLibFileName );
    __declspec(dllimport) int __stdcall FreeLibrary( struct HINSTANCE__* hModule );
    #if defined(_WIN64)
        typedef __int64 (__stdcall* CRTEMU_PROC)();
        __declspec(dllimport) CRTEMU_PROC __stdcall GetProcAddress( struct HINSTANCE__* hModule, char const* lpLibFileName );
    #else
        typedef __int32 (__stdcall* CRTEMU_PROC)();
        __declspec(dllimport) CRTEMU_PROC __stdcall GetProcAddress( struct HINSTANCE__* hModule, char const* lpLibFileName );
    #endif
    };

#define CRTEMU_GLCALLTYPE __stdcall
typedef unsigned int CRTEMU_GLuint;
typedef int CRTEMU_GLsizei;
typedef unsigned int CRTEMU_GLenum;
typedef int CRTEMU_GLint;
typedef float CRTEMU_GLfloat;
typedef char CRTEMU_GLchar;
typedef unsigned char CRTEMU_GLboolean;
typedef size_t CRTEMU_GLsizeiptr;
typedef unsigned int CRTEMU_GLbitfield;

#define CRTEMU_GL_FLOAT 0x1406
#define CRTEMU_GL_FALSE 0
#define CRTEMU_GL_FRAGMENT_SHADER 0x8b30
#define CRTEMU_GL_VERTEX_SHADER 0x8b31
#define CRTEMU_GL_COMPILE_STATUS 0x8b81
#define CRTEMU_GL_LINK_STATUS 0x8b82
#define CRTEMU_GL_INFO_LOG_LENGTH 0x8b84
#define CRTEMU_GL_ARRAY_BUFFER 0x8892
#define CRTEMU_GL_TEXTURE_2D 0x0de1
#define CRTEMU_GL_TEXTURE0 0x84c0
#define CRTEMU_GL_TEXTURE1 0x84c1
#define CRTEMU_GL_TEXTURE2 0x84c2
#define CRTEMU_GL_TEXTURE3 0x84c3
#define CRTEMU_GL_TEXTURE_MIN_FILTER 0x2801
#define CRTEMU_GL_TEXTURE_MAG_FILTER 0x2800
#define CRTEMU_GL_NEAREST 0x2600
#define CRTEMU_GL_LINEAR 0x2601
#define CRTEMU_GL_STATIC_DRAW 0x88e4
#define CRTEMU_GL_RGBA 0x1908
#define CRTEMU_GL_UNSIGNED_BYTE 0x1401
#define CRTEMU_GL_COLOR_BUFFER_BIT 0x00004000
#define CRTEMU_GL_TRIANGLE_FAN 0x0006
#define CRTEMU_GL_FRAMEBUFFER 0x8d40
#define CRTEMU_GL_VIEWPORT 0x0ba2
#define CRTEMU_GL_RGB 0x1907
#define CRTEMU_GL_COLOR_ATTACHMENT0 0x8ce0
#define CRTEMU_GL_TEXTURE_WRAP_S 0x2802
#define CRTEMU_GL_TEXTURE_WRAP_T 0x2803
#define CRTEMU_GL_CLAMP_TO_BORDER 0x812D
#define CRTEMU_GL_TEXTURE_BORDER_COLOR 0x1004


struct crtemu_t
    {
    void* memctx;

	CRTEMU_GLuint vertexbuffer;
    CRTEMU_GLuint backbuffer; 

	CRTEMU_GLuint accumulatetexture_a; 
	CRTEMU_GLuint accumulatetexture_b; 
    CRTEMU_GLuint accumulatebuffer_a; 
	CRTEMU_GLuint accumulatebuffer_b; 

    CRTEMU_GLuint blurtexture_a; 
	CRTEMU_GLuint blurtexture_b; 
    CRTEMU_GLuint blurbuffer_a; 
	CRTEMU_GLuint blurbuffer_b; 

	CRTEMU_GLuint frametexture; 
    float use_frame;

	CRTEMU_GLuint crt_shader;	
    CRTEMU_GLuint blur_shader;	
    CRTEMU_GLuint accumulate_shader;	
    CRTEMU_GLuint blend_shader;	
    CRTEMU_GLuint copy_shader;	

    int last_present_width;
    int last_present_height;


	struct HINSTANCE__* gl_dll;
    int (CRTEMU_GLCALLTYPE *wglGetProcAddress) (char const* );
    BOOL (CRTEMU_GLCALLTYPE *wglSwapIntervalEXT) (int);
    void (CRTEMU_GLCALLTYPE* glTexParameterfv) (CRTEMU_GLenum target, CRTEMU_GLenum pname, CRTEMU_GLfloat const* params);
    void (CRTEMU_GLCALLTYPE* glDeleteFramebuffers) (CRTEMU_GLsizei n, CRTEMU_GLuint const* framebuffers);
    void (CRTEMU_GLCALLTYPE* glGetIntegerv) (CRTEMU_GLenum pname, CRTEMU_GLint *data);
    void (CRTEMU_GLCALLTYPE* glGenFramebuffers) (CRTEMU_GLsizei n, CRTEMU_GLuint *framebuffers);
    void (CRTEMU_GLCALLTYPE* glBindFramebuffer) (CRTEMU_GLenum target, CRTEMU_GLuint framebuffer);    
    void (CRTEMU_GLCALLTYPE* glUniform1f) (CRTEMU_GLint location, CRTEMU_GLfloat v0);
    void (CRTEMU_GLCALLTYPE* glUniform2f) (CRTEMU_GLint location, CRTEMU_GLfloat v0, CRTEMU_GLfloat v1);
    void (CRTEMU_GLCALLTYPE* glFramebufferTexture2D) (CRTEMU_GLenum target, CRTEMU_GLenum attachment, CRTEMU_GLenum textarget, CRTEMU_GLuint texture, CRTEMU_GLint level);
    CRTEMU_GLuint (CRTEMU_GLCALLTYPE* glCreateShader) (CRTEMU_GLenum type);
    void (CRTEMU_GLCALLTYPE* glShaderSource) (CRTEMU_GLuint shader, CRTEMU_GLsizei count, CRTEMU_GLchar const* const* string, CRTEMU_GLint const* length);
    void (CRTEMU_GLCALLTYPE* glCompileShader) (CRTEMU_GLuint shader);
    void (CRTEMU_GLCALLTYPE* glGetShaderiv) (CRTEMU_GLuint shader, CRTEMU_GLenum pname, CRTEMU_GLint *params);
    CRTEMU_GLuint (CRTEMU_GLCALLTYPE* glCreateProgram) (void);
    void (CRTEMU_GLCALLTYPE* glAttachShader) (CRTEMU_GLuint program, CRTEMU_GLuint shader);
    void (CRTEMU_GLCALLTYPE* glBindAttribLocation) (CRTEMU_GLuint program, CRTEMU_GLuint index, CRTEMU_GLchar const* name);
    void (CRTEMU_GLCALLTYPE* glLinkProgram) (CRTEMU_GLuint program);
    void (CRTEMU_GLCALLTYPE* glGetProgramiv) (CRTEMU_GLuint program, CRTEMU_GLenum pname, CRTEMU_GLint *params);
    void (CRTEMU_GLCALLTYPE* glGenBuffers) (CRTEMU_GLsizei n, CRTEMU_GLuint *buffers);
    void (CRTEMU_GLCALLTYPE* glBindBuffer) (CRTEMU_GLenum target, CRTEMU_GLuint buffer);
    void (CRTEMU_GLCALLTYPE* glEnableVertexAttribArray) (CRTEMU_GLuint index);
    void (CRTEMU_GLCALLTYPE* glVertexAttribPointer) (CRTEMU_GLuint index, CRTEMU_GLint size, CRTEMU_GLenum type, CRTEMU_GLboolean normalized, CRTEMU_GLsizei stride, void const* pointer);
    void (CRTEMU_GLCALLTYPE* glGenTextures) (CRTEMU_GLsizei n, CRTEMU_GLuint* textures);
    void (CRTEMU_GLCALLTYPE* glEnable) (CRTEMU_GLenum cap);
    void (CRTEMU_GLCALLTYPE* glActiveTexture) (CRTEMU_GLenum texture);
    void (CRTEMU_GLCALLTYPE* glBindTexture) (CRTEMU_GLenum target, CRTEMU_GLuint texture);
    void (CRTEMU_GLCALLTYPE* glTexParameteri) (CRTEMU_GLenum target, CRTEMU_GLenum pname, CRTEMU_GLint param);
    void (CRTEMU_GLCALLTYPE* glDeleteBuffers) (CRTEMU_GLsizei n, CRTEMU_GLuint const* buffers);
    void (CRTEMU_GLCALLTYPE* glDeleteTextures) (CRTEMU_GLsizei n, CRTEMU_GLuint const* textures);
    void (CRTEMU_GLCALLTYPE* glBufferData) (CRTEMU_GLenum target, CRTEMU_GLsizeiptr size, void const *data, CRTEMU_GLenum usage);
    void (CRTEMU_GLCALLTYPE* glUseProgram) (CRTEMU_GLuint program);
    void (CRTEMU_GLCALLTYPE* glUniform1i) (CRTEMU_GLint location, CRTEMU_GLint v0);
    void (CRTEMU_GLCALLTYPE* glUniform3f) (CRTEMU_GLint location, CRTEMU_GLfloat v0, CRTEMU_GLfloat v1, CRTEMU_GLfloat v2);
    CRTEMU_GLint (CRTEMU_GLCALLTYPE* glGetUniformLocation) (CRTEMU_GLuint program, CRTEMU_GLchar const* name);
    void (CRTEMU_GLCALLTYPE* glTexImage2D) (CRTEMU_GLenum target, CRTEMU_GLint level, CRTEMU_GLint internalformat, CRTEMU_GLsizei width, CRTEMU_GLsizei height, CRTEMU_GLint border, CRTEMU_GLenum format, CRTEMU_GLenum type, void const* pixels);
    void (CRTEMU_GLCALLTYPE* glClearColor) (CRTEMU_GLfloat red, CRTEMU_GLfloat green, CRTEMU_GLfloat blue, CRTEMU_GLfloat alpha);
    void (CRTEMU_GLCALLTYPE* glClear) (CRTEMU_GLbitfield mask);
    void (CRTEMU_GLCALLTYPE* glDrawArrays) (CRTEMU_GLenum mode, CRTEMU_GLint first, CRTEMU_GLsizei count);
    void (CRTEMU_GLCALLTYPE* glViewport) (CRTEMU_GLint x, CRTEMU_GLint y, CRTEMU_GLsizei width, CRTEMU_GLsizei height);
    void (CRTEMU_GLCALLTYPE* glDeleteShader) (CRTEMU_GLuint shader);
    void (CRTEMU_GLCALLTYPE* glDeleteProgram) (CRTEMU_GLuint program);
    #ifdef CRTEMU_REPORT_SHADER_ERRORS
        void (CRTEMU_GLCALLTYPE* glGetShaderInfoLog) (CRTEMU_GLuint shader, CRTEMU_GLsizei bufSize, CRTEMU_GLsizei *length, CRTEMU_GLchar *infoLog);
    #endif
    };


static CRTEMU_GLuint crtemu_internal_build_shader( crtemu_t* crtemu, char const* vs_source, char const* fs_source )
    {
	#ifdef CRTEMU_REPORT_SHADER_ERRORS
		char error_message[ 1024 ]; 
	#endif

    CRTEMU_GLuint vs = crtemu->glCreateShader( CRTEMU_GL_VERTEX_SHADER );
	crtemu->glShaderSource( vs, 1, (char const**) &vs_source, NULL );
	crtemu->glCompileShader( vs );
	CRTEMU_GLint vs_compiled;
	crtemu->glGetShaderiv( vs, CRTEMU_GL_COMPILE_STATUS, &vs_compiled );
	if( !vs_compiled )
		{
		#ifdef CRTEMU_REPORT_SHADER_ERRORS
			char const* prefix = "Vertex Shader Error: ";
			strcpy( error_message, prefix );
			int len = 0, written = 0;
			crtemu->glGetShaderiv( vs, CRTEMU_GL_INFO_LOG_LENGTH, &len );
			crtemu->glGetShaderInfoLog( vs, (CRTEMU_GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written, 
				error_message + strlen( prefix ) );		
			CRTEMU_REPORT_ERROR( error_message );
		#endif
		return 0;
		}
	
	CRTEMU_GLuint fs = crtemu->glCreateShader( CRTEMU_GL_FRAGMENT_SHADER );
	crtemu->glShaderSource( fs, 1, (char const**) &fs_source, NULL );
	crtemu->glCompileShader( fs );
	CRTEMU_GLint fs_compiled;
	crtemu->glGetShaderiv( fs, CRTEMU_GL_COMPILE_STATUS, &fs_compiled );
	if( !fs_compiled )
		{
		#ifdef CRTEMU_REPORT_SHADER_ERRORS
			char const* prefix = "Fragment Shader Error: ";
			strcpy( error_message, prefix );
			int len = 0, written = 0;
			crtemu->glGetShaderiv( vs, CRTEMU_GL_INFO_LOG_LENGTH, &len );
			crtemu->glGetShaderInfoLog( fs, (CRTEMU_GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written, 
				error_message + strlen( prefix ) );		
			CRTEMU_REPORT_ERROR( error_message );
		#endif
		return 0;
		}


	CRTEMU_GLuint prg = crtemu->glCreateProgram();
	crtemu->glAttachShader( prg, fs );
	crtemu->glAttachShader( prg, vs );
	crtemu->glBindAttribLocation( prg, 0, "pos" );
	crtemu->glLinkProgram( prg );

	CRTEMU_GLint linked;
	crtemu->glGetProgramiv( prg, CRTEMU_GL_LINK_STATUS, &linked );
	if( !linked )
		{
		#ifdef CRTEMU_REPORT_SHADER_ERRORS
			char const* prefix = "Shader Link Error: ";
			strcpy( error_message, prefix );
			int len = 0, written = 0;
			crtemu->glGetShaderiv( vs, CRTEMU_GL_INFO_LOG_LENGTH, &len );
			crtemu->glGetShaderInfoLog( prg, (CRTEMU_GLsizei)( sizeof( error_message ) - strlen( prefix ) ), &written, 
				error_message + strlen( prefix ) );		
			CRTEMU_REPORT_ERROR( error_message );
		#endif
		return 0;
		}

    return prg;
    }


crtemu_t* crtemu_create( void* memctx )
    {
    crtemu_t* crtemu = (crtemu_t*) CRTEMU_MALLOC( memctx, sizeof( crtemu_t ) );
    memset( crtemu, 0, sizeof( crtemu_t ) );
    crtemu->memctx = memctx;

    crtemu->use_frame = 0.0f;
	
    crtemu->last_present_width = 0;
    crtemu->last_present_height = 0;
    
	crtemu->gl_dll = LoadLibraryA( "opengl32.dll" );
    if( !crtemu->gl_dll ) goto failed;

    crtemu->wglGetProcAddress = (int (CRTEMU_GLCALLTYPE*)(char const*)) (uintptr_t) GetProcAddress( crtemu->gl_dll, "wglGetProcAddress" );
    if( !crtemu->gl_dll ) goto failed;

	crtemu->wglSwapIntervalEXT = (BOOL (CRTEMU_GLCALLTYPE*)(int)) (uintptr_t) crtemu->wglGetProcAddress( "wglSwapIntervalEXT" );
	if( crtemu->wglSwapIntervalEXT ) crtemu->wglSwapIntervalEXT( 1 );


    // Attempt to bind opengl functions using GetProcAddress
    crtemu->glTexParameterfv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLfloat const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glTexParameterfv" );
    crtemu->glDeleteFramebuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glDeleteFramebuffers" );
    crtemu->glGetIntegerv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLint *) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGetIntegerv" );
    crtemu->glGenFramebuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint *) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGenFramebuffers" );
    crtemu->glBindFramebuffer = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glBindFramebuffer" );    
    crtemu->glUniform1f = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLfloat) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glUniform1f" );
    crtemu->glUniform2f = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLfloat, CRTEMU_GLfloat) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glUniform2f" );
    crtemu->glFramebufferTexture2D = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLuint, CRTEMU_GLint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glFramebufferTexture2D" );    
    crtemu->glCreateShader = ( CRTEMU_GLuint (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glCreateShader" );
    crtemu->glShaderSource = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLsizei, CRTEMU_GLchar const* const*, CRTEMU_GLint const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glShaderSource" );
    crtemu->glCompileShader = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glCompileShader" );
    crtemu->glGetShaderiv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLenum, CRTEMU_GLint*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGetShaderiv" );
    crtemu->glCreateProgram = ( CRTEMU_GLuint (CRTEMU_GLCALLTYPE*) (void) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glCreateProgram" );
    crtemu->glAttachShader = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glAttachShader" );
    crtemu->glBindAttribLocation = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLuint, CRTEMU_GLchar const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glBindAttribLocation" );
    crtemu->glLinkProgram = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glLinkProgram" );
    crtemu->glGetProgramiv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLenum, CRTEMU_GLint*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGetProgramiv" );
    crtemu->glGenBuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGenBuffers" );
    crtemu->glBindBuffer = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glBindBuffer" );
    crtemu->glEnableVertexAttribArray = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glEnableVertexAttribArray" );
    crtemu->glVertexAttribPointer = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLint, CRTEMU_GLenum, CRTEMU_GLboolean, CRTEMU_GLsizei, void const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glVertexAttribPointer" );
    crtemu->glGenTextures = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGenTextures" );
    crtemu->glEnable = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glEnable" );
    crtemu->glActiveTexture = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glActiveTexture" );
    crtemu->glBindTexture = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glBindTexture" );
    crtemu->glTexParameteri = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glTexParameteri" );
    crtemu->glDeleteBuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glDeleteBuffers" );
    crtemu->glDeleteTextures = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glDeleteTextures" );
    crtemu->glBufferData = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLsizeiptr, void const *, CRTEMU_GLenum) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glBufferData" );
    crtemu->glUseProgram = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glUseProgram" );
    crtemu->glUniform1i = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glUniform1i" );
    crtemu->glUniform3f = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLfloat, CRTEMU_GLfloat, CRTEMU_GLfloat) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glUniform3f" );
    crtemu->glGetUniformLocation = ( CRTEMU_GLint (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLchar const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGetUniformLocation" );
    crtemu->glTexImage2D = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLint, CRTEMU_GLint, CRTEMU_GLsizei, CRTEMU_GLsizei, CRTEMU_GLint, CRTEMU_GLenum, CRTEMU_GLenum, void const*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glTexImage2D" );
    crtemu->glClearColor = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLfloat, CRTEMU_GLfloat, CRTEMU_GLfloat, CRTEMU_GLfloat) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glClearColor" );
    crtemu->glClear = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLbitfield) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glClear" );
    crtemu->glDrawArrays = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLint, CRTEMU_GLsizei) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glDrawArrays" );
    crtemu->glViewport = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLint, CRTEMU_GLsizei, CRTEMU_GLsizei) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glViewport" );
    crtemu->glDeleteShader = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glDeleteShader" );
    crtemu->glDeleteProgram = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glDeleteProgram" );
    #ifdef CRTEMU_REPORT_SHADER_ERRORS
        crtemu->glGetShaderInfoLog = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLsizei, CRTEMU_GLsizei*, CRTEMU_GLchar*) ) (uintptr_t) GetProcAddress( crtemu->gl_dll, "glGetShaderInfoLog" );
    #endif

    // Any opengl functions which didn't bind, try binding them using wglGetProcAddrss
    if( !crtemu->glTexParameterfv ) crtemu->glTexParameterfv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLfloat const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glTexParameterfv" );
    if( !crtemu->glDeleteFramebuffers ) crtemu->glDeleteFramebuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glDeleteFramebuffers" );
    if( !crtemu->glGetIntegerv ) crtemu->glGetIntegerv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLint *) ) (uintptr_t) crtemu->wglGetProcAddress( "glGetIntegerv" );
    if( !crtemu->glGenFramebuffers ) crtemu->glGenFramebuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint *) ) (uintptr_t) crtemu->wglGetProcAddress( "glGenFramebuffers" );
    if( !crtemu->glBindFramebuffer ) crtemu->glBindFramebuffer = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glBindFramebuffer" );    
    if( !crtemu->glUniform1f ) crtemu->glUniform1f = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLfloat) ) (uintptr_t) crtemu->wglGetProcAddress( "glUniform1f" );
    if( !crtemu->glUniform2f ) crtemu->glUniform2f = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLfloat, CRTEMU_GLfloat) ) (uintptr_t) crtemu->wglGetProcAddress( "glUniform2f" );
    if( !crtemu->glFramebufferTexture2D ) crtemu->glFramebufferTexture2D = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLuint, CRTEMU_GLint) ) (uintptr_t) crtemu->wglGetProcAddress( "glFramebufferTexture2D" );    
    if( !crtemu->glCreateShader ) crtemu->glCreateShader = ( CRTEMU_GLuint (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum) ) (uintptr_t) crtemu->wglGetProcAddress( "glCreateShader" );
    if( !crtemu->glShaderSource ) crtemu->glShaderSource = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLsizei, CRTEMU_GLchar const* const*, CRTEMU_GLint const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glShaderSource" );
    if( !crtemu->glCompileShader ) crtemu->glCompileShader = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glCompileShader" );
    if( !crtemu->glGetShaderiv ) crtemu->glGetShaderiv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLenum, CRTEMU_GLint*) ) (uintptr_t) crtemu->wglGetProcAddress( "glGetShaderiv" );
    if( !crtemu->glCreateProgram ) crtemu->glCreateProgram = ( CRTEMU_GLuint (CRTEMU_GLCALLTYPE*) (void) ) (uintptr_t) crtemu->wglGetProcAddress( "glCreateProgram" );
    if( !crtemu->glAttachShader ) crtemu->glAttachShader = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glAttachShader" );
    if( !crtemu->glBindAttribLocation ) crtemu->glBindAttribLocation = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLuint, CRTEMU_GLchar const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glBindAttribLocation" );
    if( !crtemu->glLinkProgram ) crtemu->glLinkProgram = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glLinkProgram" );
    if( !crtemu->glGetProgramiv ) crtemu->glGetProgramiv = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLenum, CRTEMU_GLint*) ) (uintptr_t) crtemu->wglGetProcAddress( "glGetProgramiv" );
    if( !crtemu->glGenBuffers ) crtemu->glGenBuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint*) ) (uintptr_t) crtemu->wglGetProcAddress( "glGenBuffers" );
    if( !crtemu->glBindBuffer ) crtemu->glBindBuffer = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glBindBuffer" );
    if( !crtemu->glEnableVertexAttribArray ) crtemu->glEnableVertexAttribArray = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glEnableVertexAttribArray" );
    if( !crtemu->glVertexAttribPointer ) crtemu->glVertexAttribPointer = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLint, CRTEMU_GLenum, CRTEMU_GLboolean, CRTEMU_GLsizei, void const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glVertexAttribPointer" );
    if( !crtemu->glGenTextures ) crtemu->glGenTextures = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint*) ) (uintptr_t) crtemu->wglGetProcAddress( "glGenTextures" );
    if( !crtemu->glEnable ) crtemu->glEnable = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum) ) (uintptr_t) crtemu->wglGetProcAddress( "glEnable" );
    if( !crtemu->glActiveTexture ) crtemu->glActiveTexture = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum) ) (uintptr_t) crtemu->wglGetProcAddress( "glActiveTexture" );
    if( !crtemu->glBindTexture ) crtemu->glBindTexture = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glBindTexture" );
    if( !crtemu->glTexParameteri ) crtemu->glTexParameteri = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLenum, CRTEMU_GLint) ) (uintptr_t) crtemu->wglGetProcAddress( "glTexParameteri" );
    if( !crtemu->glDeleteBuffers ) crtemu->glDeleteBuffers = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glDeleteBuffers" );
    if( !crtemu->glDeleteTextures ) crtemu->glDeleteTextures = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLsizei, CRTEMU_GLuint const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glDeleteTextures" );
    if( !crtemu->glBufferData ) crtemu->glBufferData = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLsizeiptr, void const *, CRTEMU_GLenum) ) (uintptr_t) crtemu->wglGetProcAddress( "glBufferData" );
    if( !crtemu->glUseProgram ) crtemu->glUseProgram = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glUseProgram" );
    if( !crtemu->glUniform1i ) crtemu->glUniform1i = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLint) ) (uintptr_t) crtemu->wglGetProcAddress( "glUniform1i" );
    if( !crtemu->glUniform3f ) crtemu->glUniform3f = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLfloat, CRTEMU_GLfloat, CRTEMU_GLfloat) ) (uintptr_t) crtemu->wglGetProcAddress( "glUniform3f" );
    if( !crtemu->glGetUniformLocation ) crtemu->glGetUniformLocation = ( CRTEMU_GLint (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLchar const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glGetUniformLocation" );
    if( !crtemu->glTexImage2D ) crtemu->glTexImage2D = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLint, CRTEMU_GLint, CRTEMU_GLsizei, CRTEMU_GLsizei, CRTEMU_GLint, CRTEMU_GLenum, CRTEMU_GLenum, void const*) ) (uintptr_t) crtemu->wglGetProcAddress( "glTexImage2D" );
    if( !crtemu->glClearColor ) crtemu->glClearColor = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLfloat, CRTEMU_GLfloat, CRTEMU_GLfloat, CRTEMU_GLfloat) ) (uintptr_t) crtemu->wglGetProcAddress( "glClearColor" );
    if( !crtemu->glClear ) crtemu->glClear = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLbitfield) ) (uintptr_t) crtemu->wglGetProcAddress( "glClear" );
    if( !crtemu->glDrawArrays ) crtemu->glDrawArrays = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLenum, CRTEMU_GLint, CRTEMU_GLsizei) ) (uintptr_t) crtemu->wglGetProcAddress( "glDrawArrays" );
    if( !crtemu->glViewport ) crtemu->glViewport = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLint, CRTEMU_GLint, CRTEMU_GLsizei, CRTEMU_GLsizei) ) (uintptr_t) crtemu->wglGetProcAddress( "glViewport" );
    if( !crtemu->glDeleteShader ) crtemu->glDeleteShader = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glDeleteShader" );
    if( !crtemu->glDeleteProgram ) crtemu->glDeleteProgram = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint) ) (uintptr_t) crtemu->wglGetProcAddress( "glDeleteProgram" );
    #ifdef CRTEMU_REPORT_SHADER_ERRORS
        if( !crtemu->glGetShaderInfoLog ) crtemu->glGetShaderInfoLog = ( void (CRTEMU_GLCALLTYPE*) (CRTEMU_GLuint, CRTEMU_GLsizei, CRTEMU_GLsizei*, CRTEMU_GLchar*) ) (uintptr_t) crtemu->wglGetProcAddress( "glGetShaderInfoLog" );
    #endif

    // Report error if any gl function was not found.
    if( !crtemu->glTexParameterfv ) goto failed;
    if( !crtemu->glDeleteFramebuffers ) goto failed;
    if( !crtemu->glGetIntegerv ) goto failed;
    if( !crtemu->glGenFramebuffers ) goto failed;
    if( !crtemu->glBindFramebuffer ) goto failed;
    if( !crtemu->glUniform1f ) goto failed;
    if( !crtemu->glUniform2f ) goto failed;
    if( !crtemu->glFramebufferTexture2D ) goto failed;
    if( !crtemu->glCreateShader ) goto failed;
    if( !crtemu->glShaderSource ) goto failed;
    if( !crtemu->glCompileShader ) goto failed;
    if( !crtemu->glGetShaderiv ) goto failed;
    if( !crtemu->glCreateProgram ) goto failed;
    if( !crtemu->glAttachShader ) goto failed;
    if( !crtemu->glBindAttribLocation ) goto failed;
    if( !crtemu->glLinkProgram ) goto failed;
    if( !crtemu->glGetProgramiv ) goto failed;
    if( !crtemu->glGenBuffers ) goto failed;
    if( !crtemu->glBindBuffer ) goto failed;
    if( !crtemu->glEnableVertexAttribArray ) goto failed;
    if( !crtemu->glVertexAttribPointer ) goto failed;
    if( !crtemu->glGenTextures ) goto failed;
    if( !crtemu->glEnable ) goto failed;
    if( !crtemu->glActiveTexture ) goto failed;
    if( !crtemu->glBindTexture ) goto failed;
    if( !crtemu->glTexParameteri ) goto failed;
    if( !crtemu->glDeleteBuffers ) goto failed;
    if( !crtemu->glDeleteTextures ) goto failed;
    if( !crtemu->glBufferData ) goto failed;
    if( !crtemu->glUseProgram ) goto failed;
    if( !crtemu->glUniform1i ) goto failed;
    if( !crtemu->glUniform3f ) goto failed;
    if( !crtemu->glGetUniformLocation ) goto failed;
    if( !crtemu->glTexImage2D ) goto failed;
    if( !crtemu->glClearColor ) goto failed;
    if( !crtemu->glClear ) goto failed;
    if( !crtemu->glDrawArrays ) goto failed;
    if( !crtemu->glViewport ) goto failed;
    if( !crtemu->glDeleteShader ) goto failed;
    if( !crtemu->glDeleteProgram ) goto failed;
    #ifdef CRTEMU_REPORT_SHADER_ERRORS
        if( !crtemu->glGetShaderInfoLog ) goto failed;
    #endif

    #define STR( x ) #x

	char const* vs_source = 
    STR(
        #version 330\n\n

        layout( location = 0 ) in vec4 pos;
        out vec2 uv;

		void main( void )
			{
			gl_Position = vec4( pos.xy, 0.0, 1.0 );
            uv = pos.zw;
			}
	);

    char const* crt_fs_source = 
	STR (
        #version 330\n\n

        in vec2 uv;
        out vec4 color;

        uniform vec3 modulate;
        uniform vec2 resolution;
        uniform float time;
		uniform sampler2D backbuffer;
        uniform sampler2D blurbuffer;
        uniform sampler2D frametexture;
        uniform float use_frame;

        vec3 tsample( sampler2D samp, vec2 tc, float offs, vec2 resolution )
	        {
	        tc = tc * vec2(1.035, 0.96) + vec2(-0.0125*0.75, 0.02);
			tc = tc * 1.2 - 0.1;
	        vec3 s = pow( abs( texture2D( samp, vec2( tc.x, 1.0-tc.y ) ).bgr), vec3( 2.2 ) );
	        return s*vec3(1.25);
	        }

        vec3 filmic( vec3 LinearColor )
	        {
	        vec3 x = max( vec3(0.0), LinearColor-vec3(0.004));
	        return (x*(6.2*x+0.5))/(x*(6.2*x+1.7)+0.06);
	        }

        vec2 curve( vec2 uv )
	        {
	        uv = (uv - 0.5) * 2.0;
	        uv *= 1.1;	
	        uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0);
	        uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0);
	        uv  = (uv / 2.0) + 0.5;
	        uv =  uv *0.92 + 0.04;
	        return uv;
	        }

        float rand(vec2 co)
	        {
            return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
	        }
	        
	) STR /* neeed to break up the string because of string limit on older compilers */ (

        void main(void)
			{
	        /* Curve */
	        vec2 curved_uv = mix( curve( uv ), uv, 0.8 );
	        float scale = 0.04;
	        vec2 scuv = curved_uv*(1.0-scale)+scale/2.0+vec2(0.003, -0.001);

	        /* Main color, Bleed */
	        vec3 col;
	        float x =  sin(0.1*time+curved_uv.y*13.0)*sin(0.23*time+curved_uv.y*19.0)*sin(0.3+0.11*time+curved_uv.y*23.0)*0.0012;
	        float o =sin(gl_FragCoord.y*1.5)/resolution.x;
	        x+=o*0.25;
			x *= 0.2f;
	        col.r = tsample(backbuffer,vec2(x+scuv.x+0.0009*0.25,scuv.y+0.0009*0.25),resolution.y/800.0, resolution ).x+0.02;
	        col.g = tsample(backbuffer,vec2(x+scuv.x+0.0000*0.25,scuv.y-0.0011*0.25),resolution.y/800.0, resolution ).y+0.02;
	        col.b = tsample(backbuffer,vec2(x+scuv.x-0.0015*0.25,scuv.y+0.0000*0.25),resolution.y/800.0, resolution ).z+0.02;
	        float i = clamp(col.r*0.299 + col.g*0.587 + col.b*0.114, 0.0, 1.0 );		
	        i = pow( 1.0 - pow(i,2.0), 1.0 );
	        i = (1.0-i) * 0.85 + 0.15;	

	        /* Ghosting */
            float ghs = 0.05;
	        vec3 r = tsample(blurbuffer, vec2(x-0.014*1.0, -0.027)*0.45+0.007*vec2( 0.35*sin(1.0/7.0 + 15.0*curved_uv.y + 0.9*time), 
                0.35*sin( 2.0/7.0 + 10.0*curved_uv.y + 1.37*time) )+vec2(scuv.x+0.001,scuv.y+0.001),
                5.5+1.3*sin( 3.0/9.0 + 31.0*curved_uv.x + 1.70*time),resolution).xyz*vec3(0.5,0.25,0.25);
	        vec3 g = tsample(blurbuffer, vec2(x-0.019*1.0, -0.020)*0.45+0.007*vec2( 0.35*cos(1.0/9.0 + 15.0*curved_uv.y + 0.5*time), 
                0.35*sin( 2.0/9.0 + 10.0*curved_uv.y + 1.50*time) )+vec2(scuv.x+0.000,scuv.y-0.002),
                5.4+1.3*sin( 3.0/3.0 + 71.0*curved_uv.x + 1.90*time),resolution).xyz*vec3(0.25,0.5,0.25);
	        vec3 b = tsample(blurbuffer, vec2(x-0.017*1.0, -0.003)*0.35+0.007*vec2( 0.35*sin(2.0/3.0 + 15.0*curved_uv.y + 0.7*time), 
                0.35*cos( 2.0/3.0 + 10.0*curved_uv.y + 1.63*time) )+vec2(scuv.x-0.002,scuv.y+0.000),
                5.3+1.3*sin( 3.0/7.0 + 91.0*curved_uv.x + 1.65*time),resolution).xyz*vec3(0.25,0.25,0.5);
	) STR /* neeed to break up the string because of string limit on older compilers */ (
	
	        col += vec3(ghs*(1.0-0.299))*pow(clamp(vec3(3.0)*r,vec3(0.0),vec3(1.0)),vec3(2.0))*vec3(i);
            col += vec3(ghs*(1.0-0.587))*pow(clamp(vec3(3.0)*g,vec3(0.0),vec3(1.0)),vec3(2.0))*vec3(i);
            col += vec3(ghs*(1.0-0.114))*pow(clamp(vec3(3.0)*b,vec3(0.0),vec3(1.0)),vec3(2.0))*vec3(i);
	
	        /* Level adjustment (curves) */
	        col *= vec3(0.95,1.05,0.95);
            col = clamp(col*1.3 + 0.75*col*col + 1.25*col*col*col*col*col,vec3(0.0),vec3(10.0));
	
	        /* Vignette */
            float vig = (0.1 + 1.0*16.0*curved_uv.x*curved_uv.y*(1.0-curved_uv.x)*(1.0-curved_uv.y));
	        vig = 1.3*pow(vig,0.5);
	        col *= vig;
	
	        /* Scanlines */
	        float scans = clamp( 0.35+0.18*sin(0.0*time+curved_uv.y*resolution.y*1.5), 0.0, 1.0);
	        float s = pow(scans,0.9);
	        col = col * vec3(s);

	        /* Vertical lines (shadow mask) */
	        col*=1.0-0.23*(clamp((mod(gl_FragCoord.xy.x, 3.0))/2.0,0.0,1.0));

	        /* Tone map */
	        col = filmic( col );

	) STR /* neeed to break up the string because of string limit on older compilers */ (

	        /* Noise */
	        //vec2 seed = floor(curved_uv*resolution.xy*vec2(0.5))/resolution.xy;
            vec2 seed = curved_uv*resolution.xy;;
	        /* seed = curved_uv; */
	        col -= 0.015*pow(vec3(rand( seed +time ), rand( seed +time*2.0 ), rand( seed +time * 3.0 ) ), vec3(1.5) );
	
	        /* Flicker */
            col *= (1.0-0.004*(sin(50.0*time+curved_uv.y*2.0)*0.5+0.5));

	        /* Clamp */
	        if (curved_uv.x < 0.0 || curved_uv.x > 1.0)
		        col *= 0.0;
	        if (curved_uv.y < 0.0 || curved_uv.y > 1.0)
		        col *= 0.0;
		
	        /* Frame */
	        vec2 fscale = vec2( -0.019, -0.018 );
			vec2 fuv=vec2( uv.x, 1.0 - uv.y)*((1.0)+2.0*fscale)-fscale-vec2(-0.0, 0.005);
	        vec4 f=texture2D(frametexture, fuv * vec2(0.91, 0.8) + vec2( 0.050, 0.093 ));
	        f.xyz = mix( f.xyz, vec3(0.5,0.5,0.5), 0.5 );
	        float fvig = clamp( -0.00+512.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y), 0.2, 0.85 );
			col *= fvig;
	        col = mix( col, mix( max( col, 0.0), pow( abs( f.xyz ), vec3( 1.4 ) ), f.w), vec3( use_frame) );
            
			color = vec4( col, 1.0 );
			}
			
	 ) /* STR */;

	crtemu->crt_shader = crtemu_internal_build_shader( crtemu, vs_source, crt_fs_source );
    if( crtemu->crt_shader == 0 ) goto failed;

	char const* blur_fs_source = 
	STR (
        #version 330\n\n

        in vec2 uv;
        out vec4 color;

        uniform vec2 blur;
        uniform sampler2D texture;

        void main( void )
	        {
	        vec4 sum = texture2D( texture, uv ) * 0.2270270270;
	        sum += texture2D(texture, vec2( uv.x - 4.0 * blur.x, uv.y - 4.0 * blur.y ) ) * 0.0162162162;
	        sum += texture2D(texture, vec2( uv.x - 3.0 * blur.x, uv.y - 3.0 * blur.y ) ) * 0.0540540541;
	        sum += texture2D(texture, vec2( uv.x - 2.0 * blur.x, uv.y - 2.0 * blur.y ) ) * 0.1216216216;
	        sum += texture2D(texture, vec2( uv.x - 1.0 * blur.x, uv.y - 1.0 * blur.y ) ) * 0.1945945946;
	        sum += texture2D(texture, vec2( uv.x + 1.0 * blur.x, uv.y + 1.0 * blur.y ) ) * 0.1945945946;
	        sum += texture2D(texture, vec2( uv.x + 2.0 * blur.x, uv.y + 2.0 * blur.y ) ) * 0.1216216216;
	        sum += texture2D(texture, vec2( uv.x + 3.0 * blur.x, uv.y + 3.0 * blur.y ) ) * 0.0540540541;
	        sum += texture2D(texture, vec2( uv.x + 4.0 * blur.x, uv.y + 4.0 * blur.y ) ) * 0.0162162162;
	        color = sum;
	        }	

    ) /* STR */;

    crtemu->blur_shader = crtemu_internal_build_shader( crtemu, vs_source, blur_fs_source );
    if( crtemu->blur_shader == 0 ) goto failed;

	char const* accumulate_fs_source = 
	STR (
        #version 330\n\n

        in vec2 uv;
        out vec4 color;

        uniform sampler2D tex0;
        uniform sampler2D tex1;
        uniform float modulate;

        void main( void )
	        {
	        vec4 a = texture2D( tex0, uv ) * vec4( modulate );
	        vec4 b = texture2D( tex1, uv );

	        color = max( a, b * 0.92 );
	        }	

    ) /* STR */;

    crtemu->accumulate_shader = crtemu_internal_build_shader( crtemu, vs_source, accumulate_fs_source );
    if( crtemu->accumulate_shader == 0 ) goto failed;

	char const* blend_fs_source = 
	STR (
        #version 330\n\n

        in vec2 uv;
        out vec4 color;

        uniform sampler2D tex0;
        uniform sampler2D tex1;
        uniform float modulate;

        void main( void )
	        {
	        vec4 a = texture2D( tex0, uv ) * vec4( modulate );
	        vec4 b = texture2D( tex1, uv );

	        color = max( a, b * 0.32 );
	        }	

    ) /* STR */;

    crtemu->blend_shader = crtemu_internal_build_shader( crtemu, vs_source, blend_fs_source );
    if( crtemu->blend_shader == 0 ) goto failed;

	char const* copy_fs_source = 
	STR (
        #version 330\n\n

        in vec2 uv;
        out vec4 color;

        uniform sampler2D tex0;

        void main( void )
	        {
	        color = texture2D( tex0, uv );
	        }	

    ) /* STR */;

    crtemu->copy_shader = crtemu_internal_build_shader( crtemu, vs_source, copy_fs_source );
    if( crtemu->copy_shader == 0 ) goto failed;

    #undef STR

	crtemu->glGenTextures( 1, &crtemu->accumulatetexture_a );
	crtemu->glGenFramebuffers( 1, &crtemu->accumulatebuffer_a );

	crtemu->glGenTextures( 1, &crtemu->accumulatetexture_b );
	crtemu->glGenFramebuffers( 1, &crtemu->accumulatebuffer_b );

	crtemu->glGenTextures( 1, &crtemu->blurtexture_a );
	crtemu->glGenFramebuffers( 1, &crtemu->blurbuffer_a );

	crtemu->glGenTextures( 1, &crtemu->blurtexture_b );
	crtemu->glGenFramebuffers( 1, &crtemu->blurbuffer_b );

    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );

    crtemu->glGenTextures( 1, &crtemu->frametexture ); 
    crtemu->glEnable( CRTEMU_GL_TEXTURE_2D ); 
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE2 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->frametexture );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );

    crtemu->glGenTextures( 1, &crtemu->backbuffer ); 
    crtemu->glEnable( CRTEMU_GL_TEXTURE_2D ); 
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->backbuffer );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_NEAREST );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_NEAREST );

    crtemu->glGenBuffers( 1, &crtemu->vertexbuffer );
	crtemu->glBindBuffer( CRTEMU_GL_ARRAY_BUFFER, crtemu->vertexbuffer );
	crtemu->glEnableVertexAttribArray( 0 );
	crtemu->glVertexAttribPointer( 0, 4, CRTEMU_GL_FLOAT, CRTEMU_GL_FALSE, 4 * sizeof( CRTEMU_GLfloat ), 0 );

    return crtemu;

failed:
    if( crtemu->accumulatetexture_a ) crtemu->glDeleteTextures( 1, &crtemu->accumulatetexture_a );
	if( crtemu->accumulatebuffer_a ) crtemu->glDeleteFramebuffers( 1, &crtemu->accumulatebuffer_a );
	if( crtemu->accumulatetexture_b ) crtemu->glDeleteTextures( 1, &crtemu->accumulatetexture_b );
	if( crtemu->accumulatebuffer_b ) crtemu->glDeleteFramebuffers( 1, &crtemu->accumulatebuffer_b );
	if( crtemu->blurtexture_a ) crtemu->glDeleteTextures( 1, &crtemu->blurtexture_a );
	if( crtemu->blurbuffer_a ) crtemu->glDeleteFramebuffers( 1, &crtemu->blurbuffer_a );
	if( crtemu->blurtexture_b ) crtemu->glDeleteTextures( 1, &crtemu->blurtexture_b );
	if( crtemu->blurbuffer_b ) crtemu->glDeleteFramebuffers( 1, &crtemu->blurbuffer_b );
    if( crtemu->frametexture ) crtemu->glDeleteTextures( 1, &crtemu->frametexture ); 
    if( crtemu->backbuffer ) crtemu->glDeleteTextures( 1, &crtemu->backbuffer ); 
    if( crtemu->vertexbuffer ) crtemu->glDeleteBuffers( 1, &crtemu->vertexbuffer );

    if( crtemu->gl_dll ) FreeLibrary( crtemu->gl_dll );
    CRTEMU_FREE( crtemu->memctx, crtemu );
    return 0;
    }


void crtemu_destroy( crtemu_t* crtemu )
    {
    crtemu->glDeleteTextures( 1, &crtemu->accumulatetexture_a );
	crtemu->glDeleteFramebuffers( 1, &crtemu->accumulatebuffer_a );
	crtemu->glDeleteTextures( 1, &crtemu->accumulatetexture_b );
	crtemu->glDeleteFramebuffers( 1, &crtemu->accumulatebuffer_b );
	crtemu->glDeleteTextures( 1, &crtemu->blurtexture_a );
	crtemu->glDeleteFramebuffers( 1, &crtemu->blurbuffer_a );
	crtemu->glDeleteTextures( 1, &crtemu->blurtexture_b );
	crtemu->glDeleteFramebuffers( 1, &crtemu->blurbuffer_b );
    crtemu->glDeleteTextures( 1, &crtemu->frametexture ); 
    crtemu->glDeleteTextures( 1, &crtemu->backbuffer ); 
    crtemu->glDeleteBuffers( 1, &crtemu->vertexbuffer );
    FreeLibrary( crtemu->gl_dll );
    CRTEMU_FREE( crtemu->memctx, crtemu );
    }


void crtemu_frame( crtemu_t* crtemu, CRTEMU_U32* frame_abgr, int frame_width, int frame_height )
    {
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE3 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->frametexture );   
    crtemu->glTexImage2D( CRTEMU_GL_TEXTURE_2D, 0, CRTEMU_GL_RGBA, frame_width, frame_height, 0, CRTEMU_GL_RGBA, CRTEMU_GL_UNSIGNED_BYTE, frame_abgr ); 
    if( frame_abgr )
        crtemu->use_frame = 1.0f;
    else
        crtemu->use_frame = 0.0f;
    }


static void crtemu_internal_blur( crtemu_t* crtemu, CRTEMU_GLuint source, CRTEMU_GLuint blurbuffer_a, CRTEMU_GLuint blurbuffer_b, 
    CRTEMU_GLuint blurtexture_b, float r, int width, int height )
    {
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, blurbuffer_b );
    crtemu->glUseProgram( crtemu->blur_shader );
    crtemu->glUniform2f( crtemu->glGetUniformLocation( crtemu->blur_shader, "blur" ), r / (float) width, 0 );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->blur_shader, "texture" ), 0 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, source );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
	crtemu->glDrawArrays( CRTEMU_GL_TRIANGLE_FAN, 0, 4 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );

    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, blurbuffer_a );
    crtemu->glUseProgram( crtemu->blur_shader );
    crtemu->glUniform2f( crtemu->glGetUniformLocation( crtemu->blur_shader, "blur" ), 0, r / (float) height );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->blur_shader, "texture" ), 0 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, blurtexture_b );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
	crtemu->glDrawArrays( CRTEMU_GL_TRIANGLE_FAN, 0, 4 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );
    }


void crtemu_present( crtemu_t* crtemu, CRTEMU_U64 time_us, CRTEMU_U32 const* pixels_xbgr, int width, int height, 
    CRTEMU_U32 mod_xbgr, CRTEMU_U32 border_xbgr )
    {
    int viewport[ 4 ];
    crtemu->glGetIntegerv( CRTEMU_GL_VIEWPORT, viewport );

	if( width != crtemu->last_present_width || height != crtemu->last_present_height )
		{
        crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
 
    	crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->accumulatetexture_a );
		crtemu->glTexImage2D( CRTEMU_GL_TEXTURE_2D, 0, CRTEMU_GL_RGB, width, height, 0, CRTEMU_GL_RGB, CRTEMU_GL_UNSIGNED_BYTE, 0 );
	    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, crtemu->accumulatebuffer_a );
	    crtemu->glFramebufferTexture2D( CRTEMU_GL_FRAMEBUFFER, CRTEMU_GL_COLOR_ATTACHMENT0, CRTEMU_GL_TEXTURE_2D, crtemu->accumulatetexture_a, 0 );
        crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );

		crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->accumulatetexture_b );
		crtemu->glTexImage2D( CRTEMU_GL_TEXTURE_2D, 0, CRTEMU_GL_RGB, width, height, 0, CRTEMU_GL_RGB, CRTEMU_GL_UNSIGNED_BYTE, 0 );
	    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, crtemu->accumulatebuffer_b );
	    crtemu->glFramebufferTexture2D( CRTEMU_GL_FRAMEBUFFER, CRTEMU_GL_COLOR_ATTACHMENT0, CRTEMU_GL_TEXTURE_2D, crtemu->accumulatetexture_b, 0 );
        crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );

		crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->blurtexture_a );
		crtemu->glTexImage2D( CRTEMU_GL_TEXTURE_2D, 0, CRTEMU_GL_RGB, width, height, 0, CRTEMU_GL_RGB, CRTEMU_GL_UNSIGNED_BYTE, 0 );
	    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, crtemu->blurbuffer_a );
	    crtemu->glFramebufferTexture2D( CRTEMU_GL_FRAMEBUFFER, CRTEMU_GL_COLOR_ATTACHMENT0, CRTEMU_GL_TEXTURE_2D, crtemu->blurtexture_a, 0 );
        crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );

		crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->blurtexture_b );
		crtemu->glTexImage2D( CRTEMU_GL_TEXTURE_2D, 0, CRTEMU_GL_RGB, width, height, 0, CRTEMU_GL_RGB, CRTEMU_GL_UNSIGNED_BYTE, 0 );
	    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, crtemu->blurbuffer_b );
	    crtemu->glFramebufferTexture2D( CRTEMU_GL_FRAMEBUFFER, CRTEMU_GL_COLOR_ATTACHMENT0, CRTEMU_GL_TEXTURE_2D, crtemu->blurtexture_b, 0 );
        crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );
		}

	
	crtemu->last_present_width = width;
	crtemu->last_present_height = height;

    {
	float x1 = -1.0f, y1 = -1.0f, x2 = 1.0f, y2 = 1.0f;

    CRTEMU_GLfloat vertices[] = 
        { 
        x1, y1, 0.0f, 0.0f,
        x2, y1, 1.0f, 0.0f,
        x2, y2, 1.0f, 1.0f,
        x1, y2, 0.0f, 1.0f,
        };
	crtemu->glBufferData( CRTEMU_GL_ARRAY_BUFFER, 4 * 4 * sizeof( CRTEMU_GLfloat ), vertices, CRTEMU_GL_STATIC_DRAW );
	crtemu->glBindBuffer( CRTEMU_GL_ARRAY_BUFFER, crtemu->vertexbuffer );
    }

    // Copy to backbuffer
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->backbuffer );
    crtemu->glTexImage2D( CRTEMU_GL_TEXTURE_2D, 0, CRTEMU_GL_RGBA, width, height, 0, CRTEMU_GL_RGBA, CRTEMU_GL_UNSIGNED_BYTE, pixels_xbgr ); 
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );

	crtemu->glViewport( 0, 0, width, height );

    // Blur the previous accumulation buffer
    crtemu_internal_blur( crtemu, crtemu->accumulatetexture_b, crtemu->blurbuffer_a, crtemu->blurbuffer_b, crtemu->blurtexture_b, 1.0f, width, height );

    // Update accumulation buffer
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, crtemu->accumulatebuffer_a );
    crtemu->glUseProgram( crtemu->accumulate_shader );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->accumulate_shader, "tex0" ), 0 );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->accumulate_shader, "tex1" ), 1 );
    crtemu->glUniform1f( crtemu->glGetUniformLocation( crtemu->accumulate_shader, "modulate" ), 1.0f );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->backbuffer );   
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE1 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->blurtexture_a );   
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
	crtemu->glDrawArrays( CRTEMU_GL_TRIANGLE_FAN, 0, 4 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE1 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );
  
    
    // Store a copy of the accumulation buffer
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, crtemu->accumulatebuffer_b );
    crtemu->glUseProgram( crtemu->copy_shader );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->copy_shader, "tex0" ), 0 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->accumulatetexture_a );   
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
	crtemu->glDrawArrays( CRTEMU_GL_TRIANGLE_FAN, 0, 4 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );

    // Blend accumulation and backbuffer
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, crtemu->accumulatebuffer_a );
    crtemu->glUseProgram( crtemu->blend_shader );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->blend_shader, "tex0" ), 0 );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->blend_shader, "tex1" ), 1 );
    crtemu->glUniform1f( crtemu->glGetUniformLocation( crtemu->blend_shader, "modulate" ), 1.0f );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->backbuffer );   
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE1 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->accumulatetexture_b );   
	crtemu->glDrawArrays( CRTEMU_GL_TRIANGLE_FAN, 0, 4 );
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE1 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );


    // Add slight blur to backbuffer
    crtemu_internal_blur( crtemu, crtemu->accumulatetexture_a, crtemu->accumulatebuffer_a, crtemu->blurbuffer_b, crtemu->blurtexture_b, 0.17f, width, height );

    // Create fully blurred version of backbuffer
    crtemu_internal_blur( crtemu, crtemu->accumulatetexture_a, crtemu->blurbuffer_a, crtemu->blurbuffer_b, crtemu->blurtexture_b, 1.0f, width, height );


    // Present to screen with CRT shader
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 );

    crtemu->glViewport( viewport[ 0 ], viewport[ 1 ], viewport[ 2 ], viewport[ 3 ] );

    int window_width = viewport[ 2 ] - viewport[ 0 ];
    int window_height = viewport[ 3 ] - viewport[ 1 ];
    int scaled_w = (int)( window_height * ( 4.3f / 3.0f ) );
    int scaled_h = (int)( window_width * ( 3.0f / 4.3f ) );

    if( scaled_h > window_height && scaled_w <= window_width) {
        window_width = scaled_w;
        window_height = (int)( scaled_w * ( 3.0f / 4.3f ) );
    } else if( scaled_w > window_width && scaled_h <= window_height ) {
        window_height = scaled_h;
        window_width = (int)( scaled_h * ( 4.3f / 3.0f ) );
    } 
 
    float hborder = ( ( viewport[ 2 ] - viewport[ 0 ] ) - window_width ) / 2.0f;
    float vborder = ( ( viewport[ 3 ] - viewport[ 1 ] ) - window_height ) / 2.0f;

    float x1 = hborder;
    float y1 = vborder;
    float x2 = x1 + window_width;
    float y2 = y1 + window_height;

    x1 = ( x1 / ( viewport[ 2 ] - viewport[ 0 ] )  ) * 2.0f - 1.0f;
    x2 = ( x2 / ( viewport[ 2 ] - viewport[ 0 ] )  ) * 2.0f - 1.0f;
    y1 = ( y1 / ( viewport[ 3 ] - viewport[ 1 ] ) ) * 2.0f - 1.0f;
    y2 = ( y2 / ( viewport[ 3 ] - viewport[ 1 ] ) ) * 2.0f - 1.0f;

    CRTEMU_GLfloat screen_vertices[] = 
        { 
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
        };
    screen_vertices[  0 ] = x1;
    screen_vertices[  1 ] = y1;
    screen_vertices[  4 ] = x2;
    screen_vertices[  5 ] = y1;
    screen_vertices[  8 ] = x2;
    screen_vertices[  9 ] = y2;
    screen_vertices[ 12 ] = x1;
    screen_vertices[ 13 ] = y2;

    crtemu->glBufferData( CRTEMU_GL_ARRAY_BUFFER, 4 * 4 * sizeof( CRTEMU_GLfloat ), screen_vertices, CRTEMU_GL_STATIC_DRAW );
    crtemu->glBindBuffer( CRTEMU_GL_ARRAY_BUFFER, crtemu->vertexbuffer );

    float r = ( ( border_xbgr >> 16 ) & 0xff ) / 255.0f;
    float g = ( ( border_xbgr >> 8  ) & 0xff ) / 255.0f;
    float b = ( ( border_xbgr       ) & 0xff ) / 255.0f;
    crtemu->glClearColor( r, g, b, 1.0f );
    crtemu->glClear( CRTEMU_GL_COLOR_BUFFER_BIT );

    crtemu->glUseProgram( crtemu->crt_shader );    
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->crt_shader, "backbuffer" ), 0 );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->crt_shader, "blurbuffer" ), 1 );
	crtemu->glUniform1i( crtemu->glGetUniformLocation( crtemu->crt_shader, "frametexture" ), 2 );
    crtemu->glUniform1f( crtemu->glGetUniformLocation( crtemu->crt_shader, "use_frame" ), crtemu->use_frame );
	crtemu->glUniform1f( crtemu->glGetUniformLocation( crtemu->crt_shader, "time" ), 1.5f * (CRTEMU_GLfloat)( ( (double) time_us ) / 1000000.0 ) );
    crtemu->glUniform2f( crtemu->glGetUniformLocation( crtemu->crt_shader, "resolution" ), (float) window_width, (float) window_height );

	float mod_r = ( ( mod_xbgr >> 16 ) & 0xff ) / 255.0f;
	float mod_g = ( ( mod_xbgr >> 8  ) & 0xff ) / 255.0f;
	float mod_b = ( ( mod_xbgr       ) & 0xff ) / 255.0f;
    crtemu->glUniform3f( crtemu->glGetUniformLocation( crtemu->crt_shader, "modulate" ), mod_r, mod_g, mod_b );

	float color[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->accumulatetexture_a );   
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
	crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_WRAP_S, CRTEMU_GL_CLAMP_TO_BORDER );
	crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_WRAP_T, CRTEMU_GL_CLAMP_TO_BORDER );
	crtemu->glTexParameterfv( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_BORDER_COLOR, color );	

	crtemu->glActiveTexture( CRTEMU_GL_TEXTURE1 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->blurtexture_a );   
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
	crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_WRAP_S, CRTEMU_GL_CLAMP_TO_BORDER );
	crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_WRAP_T, CRTEMU_GL_CLAMP_TO_BORDER );
	crtemu->glTexParameterfv( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_BORDER_COLOR, color );	

    crtemu->glActiveTexture( CRTEMU_GL_TEXTURE3 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, crtemu->frametexture );   
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MIN_FILTER, CRTEMU_GL_LINEAR );
    crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_MAG_FILTER, CRTEMU_GL_LINEAR );
	crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_WRAP_S, CRTEMU_GL_CLAMP_TO_BORDER );
	crtemu->glTexParameteri( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_WRAP_T, CRTEMU_GL_CLAMP_TO_BORDER );
	crtemu->glTexParameterfv( CRTEMU_GL_TEXTURE_2D, CRTEMU_GL_TEXTURE_BORDER_COLOR, color );	

    crtemu->glDrawArrays( CRTEMU_GL_TRIANGLE_FAN, 0, 4 );    

	crtemu->glActiveTexture( CRTEMU_GL_TEXTURE0 );
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 0 );   
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 1 );   
    crtemu->glBindTexture( CRTEMU_GL_TEXTURE_2D, 2 );   
    crtemu->glBindFramebuffer( CRTEMU_GL_FRAMEBUFFER, 0 ); 
    }

#endif /* CRTEMU_IMPLEMENTATION */

/*
------------------------------------------------------------------------------

This software is available under 2 licenses - you may choose the one you like.

------------------------------------------------------------------------------

ALTERNATIVE A - MIT License

Copyright (c) 2016 Mattias Gustavsson

Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.

------------------------------------------------------------------------------

ALTERNATIVE B - Public Domain (www.unlicense.org)

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.

In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

------------------------------------------------------------------------------
*/
