
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#include <EGL/egl.h>

#include "shader_stuff.h"


typedef struct
{
    // Handle to a program object
    GLuint programObject;

   // Attribute locations
   GLint  positionLoc;
   GLint  texCoordLoc;

   // Sampler location
   GLint samplerLoc;

   // Other locations
   GLint frameCountLoc;
   GLint emulatorFrameSizeLoc;
   GLint outputFrameSizeLoc;

   // Texture handle
   GLuint textureId;

} UserData;

typedef struct STATE_T
{
    uint32_t width;
    uint32_t height;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;

    EGL_DISPMANX_WINDOW_T nativewindow;
    UserData *user_data;
    void (*draw_func) (struct STATE_T* );
} STATE_T;



// for checking if file has changed
#include <sys/types.h>
#include <sys/stat.h>


time_t get_file_date(const char *path)
{
    struct stat file_stat;
    int err = stat(path, &file_stat);
    if (err)
        return 0;
    else
        return file_stat.st_mtime;
}


void showlog(GLint shader)
{
   char log[1024];

   glGetShaderInfoLog(shader,sizeof log,NULL,log);
   printf("%d:shader:\n%s\n", shader, log);
}

static void showprogramlog(GLint shader)
{
   char log[1024];
   glGetProgramInfoLog(shader,sizeof log,NULL,log);
   printf("%d:program:\n%s\n", shader, log);
}

//static char fshader_file_name[10000];
static char fshader_file_name[] = "fshader.glsl";
static time_t fshader_file_date=0;
static GLchar *fshader_source = NULL;


static GLchar default_vShaderStr[] =  
      "attribute vec4 a_position;   \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = a_position; \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";
   
static GLchar default_fShaderStr[] =  
      "precision mediump float;\n"
      "varying vec2 v_texCoord;\n"
      "uniform sampler2D s_texture;\n"
      "uniform float u_framecount;\n"
      "uniform vec2 u_emulator_frame_size;\n"
      "uniform vec2 u_output_frame_size;\n"
      "void main()\n"
      "{\n"
//      "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord )\n"
//      "		* (cos(gl_FragCoord.y * 3.14159) * 0.5 + 0.5); \n"
//      "		* (cos(v_texCoord.y * 3.14159 * 262.0 * 2.0 * 2.0) * 0.45 + 0.55); \n"
      "		* (cos(gl_FragCoord.y * 3.1415926) * 0.35 + 0.65); \n"
      "}\n";


/*
void set_fshader_file_name(char *shaderdir)
{
    strcpy(fshader_file_name,shaderdir);
    if(shaderdir[strlen(shaderdir)-1]!='/')
        strcat(fshader_file_name,"/");
    strcat(fshader_file_name,"fshader.glsl");
}
*/

/* Returns non-zero, if the fragment shaders should be reloaded.
 * Because the file has been changed or deleted or something. 
 * Returns zero, if the shader doesn't need to be reloaded.
 */
int shader_stuff_shader_needs_reload()
{
	if ((fshader_file_name == NULL) || (fshader_source == default_fShaderStr))
		return 0;

    return (get_file_date(fshader_file_name) != fshader_file_date);
}


/* Tries to load file named file_name. 
 * 
 * If opening the file succeeds:
 *   Allocates memory for *file_data with malloc() and loads the file
 *   contents in the allocated buffer. An extra terminator char is added.
 *   Previous pointer value is overwritten.
 *    
 *   *file_date is set to the time/date stamp of the file.
 *   
 *   Returns 0.
 * 
 * If opening the file fails:
 *   *file_data and *file_date are left untouched.
 *   Returns -1.
 * 
 */ 
int load_file(char *file_name, GLchar **file_data, time_t *file_date)
{
    GLchar *data = NULL;

    FILE *f = fopen(file_name, "rb");
    if(f!=NULL)
    {
		fseek(f, 0, SEEK_END);
		int len = ftell(f);
		*file_data = (GLchar *)malloc(len+1);
		fseek(f, 0, SEEK_SET);
		fread(*file_data, 1, len, f);
		fclose(f);
		(*file_data)[len] = 0; // String terminator 

		*file_date = get_file_date(file_name);
	}
    else {
        printf("Fragment shader file %s won't open\n", file_name);
        return -1;
    }

    return 0;
}

void free_fshader_source()
{
	if ((fshader_source != NULL) && (fshader_source != default_fShaderStr)) {
		free(fshader_source);
		fshader_source = NULL;
	}
}

///
// Create a shader object, load the shader source, and
// compile the shader.
//
GLuint LoadShader(GLenum type, const GLchar *shaderSrc)
{
    GLuint shader;
    GLint compiled;
    // Create the shader object
    shader = glCreateShader(type);
    if(shader == 0)
	return 0;
    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
    glCompileShader(shader);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if(!compiled)
	{
	    GLint infoLen = 0;
	    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
	    if(infoLen > 1)
		{
		    char* infoLog = (char *)malloc(sizeof(char) * infoLen);
		    glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
		    fprintf(stderr, "Error compiling shader:\n%s\n", infoLog);
		    free(infoLog);
		}
	    glDeleteShader(shader);
	    return 0;
	}
    return shader;
}

GLuint LoadProgram ( const GLchar *vertShaderSrc, const GLchar *fragShaderSrc )
{
   GLuint vertexShader;
   GLuint fragmentShader;
   GLuint programObject;
   GLint linked;

   // Load the vertex/fragment shaders
   vertexShader = LoadShader ( GL_VERTEX_SHADER, vertShaderSrc );
   if ( vertexShader == 0 )
      return 0;

   fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, fragShaderSrc );
   // if it didn't compile, let's try the default shader
   if ((fragmentShader == 0) && (fshader_source != default_fShaderStr)) {
	   fshader_source = default_fShaderStr;
		fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, default_fShaderStr );
	}
		
   if ( fragmentShader == 0 )
   {
      glDeleteShader( vertexShader );
      return 0;
   }

   // Create the program object
   programObject = glCreateProgram ( );
   
   if ( programObject == 0 )
      return 0;

   glAttachShader ( programObject, vertexShader );
   glAttachShader ( programObject, fragmentShader );

   // Link the program
   glLinkProgram ( programObject );

   // Check the link status
   glGetProgramiv ( programObject, GL_LINK_STATUS, &linked );

   if ( !linked ) 
   {
      GLint infoLen = 0;
      glGetProgramiv ( programObject, GL_INFO_LOG_LENGTH, &infoLen );
      
      if ( infoLen > 1 )
      {
         char* infoLog = (char *)malloc (sizeof(char) * infoLen );

         glGetProgramInfoLog ( programObject, infoLen, NULL, infoLog );
         fprintf (stderr, "Error linking program:\n%s\n", infoLog );            
         
         free ( infoLog );
      }

      glDeleteProgram ( programObject );
      return 0;
   }

   // Free up no longer needed shader resources
   glDeleteShader ( vertexShader );
   glDeleteShader ( fragmentShader );

   return programObject;
}

static STATE_T shader_stuff_state;

int shader_stuff_init()
{
	STATE_T *p_state = &shader_stuff_state;
   p_state->user_data = (UserData *)malloc(sizeof(UserData));
   return GL_TRUE;
}

///
// Initialize the shader and program object
//
int shader_stuff_reload_shaders()
{
	STATE_T *p_state = &shader_stuff_state;
   UserData *userData = p_state->user_data;

// ----- these lines could be moved to a separate "delete program" routine

	free_fshader_source();
	
	// If there was an existing program object, delete it.
	if (userData->programObject != 0) {
		glDeleteProgram(userData->programObject);
		userData->programObject = 0;
	}
// -----


   if (load_file(fshader_file_name, &fshader_source, &fshader_file_date) != 0) {
	   printf("Cannot open %s. Using built-in default fragment shader.", fshader_file_name);
	   fshader_source = default_fShaderStr;
   }
		
   // Load the shaders and get a linked program object
   userData->programObject = LoadProgram ( default_vShaderStr, fshader_source );

   
   return GL_TRUE;
}

int shader_stuff_set_data(GLfloat *vertex_coords_3f, GLfloat *texture_coords_2f, GLuint texture_name)
{
	STATE_T *p_state = &shader_stuff_state;
   UserData *userData = p_state->user_data;

   glClearColor ( 0.0f, 0.0f, 0.0f, 0.0f );

   // Get the attribute locations
   userData->positionLoc = glGetAttribLocation ( userData->programObject, "a_position" );
   userData->texCoordLoc = glGetAttribLocation ( userData->programObject, "a_texCoord" );
   
   // Get the sampler location
   userData->samplerLoc = glGetUniformLocation ( userData->programObject, "s_texture" );

   // Get the sampler location
   userData->frameCountLoc = glGetUniformLocation ( userData->programObject, "u_framecount" );
   printf("frameCountLoc = %d\n", userData->frameCountLoc);
   userData->emulatorFrameSizeLoc = glGetUniformLocation ( userData->programObject, "u_emulator_frame_size" );
   printf("emulatorFrameSizeLoc = %d\n", userData->emulatorFrameSizeLoc);
   userData->outputFrameSizeLoc = glGetUniformLocation ( userData->programObject, "u_output_frame_size" );

   // Load the texture
   userData->textureId = texture_name;

   // Load the vertex position
   glVertexAttribPointer ( userData->positionLoc, 3, GL_FLOAT, 
                           GL_FALSE, 3 * sizeof(GLfloat), vertex_coords_3f );
   // Load the texture coordinate
   glVertexAttribPointer ( userData->texCoordLoc, 2, GL_FLOAT,
                           GL_FALSE, 2 * sizeof(GLfloat), texture_coords_2f );

   glEnableVertexAttribArray ( userData->positionLoc );
   glEnableVertexAttribArray ( userData->texCoordLoc );

   // Bind the texture
   glActiveTexture ( GL_TEXTURE0 );
   glBindTexture ( GL_TEXTURE_2D, userData->textureId );

   // Set the sampler texture unit to 0
   glUniform1i ( userData->samplerLoc, 0 );


   return GL_TRUE;
}

// call this for every frame
// todo: merge all this "stuff" properly to gl.cpp
int shader_stuff_frame(int framecount, int emu_width, int emu_height, int out_width, int out_height)
{
	STATE_T *p_state = &shader_stuff_state;
   UserData *userData = p_state->user_data;

   glUseProgram ( userData->programObject );
   
   glUniform1f ( userData->frameCountLoc, (GLfloat)(framecount) );
   glUniform2f( userData->emulatorFrameSizeLoc, (GLfloat)(emu_width), (GLfloat)(emu_height));
   glUniform2f ( userData->outputFrameSizeLoc, (GLfloat)(out_width), (GLfloat)(out_height));

}


/*

// for checking if file has changed
#include <sys/types.h>
#include <sys/stat.h>


void showlog(GLint shader)
{
   char log[1024];

   glGetShaderInfoLog(shader,sizeof log,NULL,log);
   printf("%d:shader:\n%s\n", shader, log);
}

static void showprogramlog(GLint shader)
{
   char log[1024];
   glGetProgramInfoLog(shader,sizeof log,NULL,log);
   printf("%d:program:\n%s\n", shader, log);
}

static char fshader_file_name[10000];
static time_t fshader_file_date=0;

void set_fshader_file_name(char *shaderdir)
{
    strcpy(fshader_file_name,shaderdir);
    if(shaderdir[strlen(shaderdir)-1]!='/')
        strcat(fshader_file_name,"/");
    strcat(fshader_file_name,"fshader.glsl");
}

static int init_shader()
{
    FILE *f;
    int len;
    static GLchar *fsource=NULL,*vsource=
           "attribute vec2 vertex;"
           "void main(void) {"
           " gl_Position = vec4(vertex.x,vertex.y,0.0,1.0);"
           "}";

    f=fopen(fshader_file_name,"rb");
    if(f==NULL)
    {
        printf("Fragment shader won't open\n");
        return -1;
    }
    fseek(f,0,SEEK_END);
    len=ftell(f);
    if(fsource!=NULL)
        free(fsource);
    fsource=malloc(len+1);
    fseek(f,0,SEEK_SET);
    fread(fsource,1,len,f);
    fclose(f);
    fsource[len]=0; // Need to terminate!

    fshader_file_date = get_file_date(fshader_file_name);

    vshader=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader,1,(const GLchar **)&vsource,0);
    glCompileShader(vshader);
    showlog(vshader);

    fshader=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader,1,(const GLchar **)&fsource,0);
    glCompileShader(fshader);
    showlog(fshader);

    program=glCreateProgram();
    glAttachShader(program,vshader);
    glAttachShader(program,fshader);
    glLinkProgram(program);

    return 0;
}

void delete_shader()
{
    glDetachShader(program, fshader);
    glDetachShader(program, vshader);
    glDeleteShader(fshader);
    glDeleteShader(vshader);
    glUseProgram(0);
    glDeleteProgram(program);
    glUseProgram(0);
}

void koelli_reload_shader()
{
    delete_shader();
    init_shader();
}
* 
int koelli_fshader_file_changed(void)
{
    return (get_file_date(fshader_file_name) != fshader_file_date);
}
*/
