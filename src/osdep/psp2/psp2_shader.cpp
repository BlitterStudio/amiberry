//
// Created by cpasjuste on 18/12/16.
//

// use https://github.com/frangarcj/vita2dlib/tree/fbo
// and https://github.com/frangarcj/vita-shader-collection/releases

#include <sysconfig.h>
#include "psp2_shader.h"

#include "vita-shader-collection/includes/lcd3x_v.h"
#include "vita-shader-collection/includes/lcd3x_f.h"
#include "vita-shader-collection/includes/gtu_v.h"
#include "vita-shader-collection/includes/gtu_f.h"
#include "vita-shader-collection/includes/texture_v.h"
#include "vita-shader-collection/includes/texture_f.h"
#include "vita-shader-collection/includes/opaque_v.h"
#include "vita-shader-collection/includes/bicubic_f.h"
#include "vita-shader-collection/includes/xbr_2x_v.h"
#include "vita-shader-collection/includes/xbr_2x_f.h"
#include "vita-shader-collection/includes/xbr_2x_fast_v.h"
#include "vita-shader-collection/includes/xbr_2x_fast_f.h"
#include "vita-shader-collection/includes/advanced_aa_v.h"
#include "vita-shader-collection/includes/advanced_aa_f.h"
#include "vita-shader-collection/includes/scale2x_f.h"
#include "vita-shader-collection/includes/scale2x_v.h"
#include "vita-shader-collection/includes/sharp_bilinear_f.h"
#include "vita-shader-collection/includes/sharp_bilinear_v.h"
#include "vita-shader-collection/includes/sharp_bilinear_simple_f.h"
#include "vita-shader-collection/includes/sharp_bilinear_simple_v.h"
//#include "xbr_2x_noblend_f.h"
//#include "xbr_2x_noblend_v.h"
#include "vita-shader-collection/includes/fxaa_v.h"
#include "vita-shader-collection/includes/fxaa_f.h"
#include "vita-shader-collection/includes/crt_easymode_f.h"

PSP2Shader::PSP2Shader(Shader shaderType) {

    //printf("PSP2Shader: %i\n", shaderType);

    switch (shaderType) {

        case Shader::LCD3X:
            shader = vita2d_create_shader((SceGxmProgram *) lcd3x_v, (SceGxmProgram *) lcd3x_f);
            break;
        case Shader::SCALE2X:
            shader = vita2d_create_shader((SceGxmProgram *) scale2x_v, (SceGxmProgram *) scale2x_f);
            break;
        case Shader::AAA:
            shader = vita2d_create_shader((SceGxmProgram *) advanced_aa_v, (SceGxmProgram *) advanced_aa_f);
            break;
        case Shader::SHARP_BILINEAR:
            shader = vita2d_create_shader((SceGxmProgram *) sharp_bilinear_v, (SceGxmProgram *) sharp_bilinear_f);
            break;
            case Shader::SHARP_BILINEAR_SIMPLE:
            shader = vita2d_create_shader((SceGxmProgram *) sharp_bilinear_simple_v, (SceGxmProgram *) sharp_bilinear_simple_f);
            break;
        case Shader::FXAA:
            shader = vita2d_create_shader((SceGxmProgram *) fxaa_v, (SceGxmProgram *) fxaa_f);
            break;
        default:
            shader = vita2d_create_shader((SceGxmProgram *) texture_v, (SceGxmProgram *) texture_f);
            break;
    }

    vita2d_texture_set_program(shader->vertexProgram, shader->fragmentProgram);
    vita2d_texture_set_wvp(shader->wvpParam);
    vita2d_texture_set_vertexInput(&shader->vertexInput);
    vita2d_texture_set_fragmentInput(&shader->fragmentInput);

    for(int i=0; i<3; i++) {
        vita2d_start_drawing();
        vita2d_clear_screen();
        vita2d_wait_rendering_done();
        vita2d_swap_buffers();
    }
}

PSP2Shader::~PSP2Shader() {
    if (shader != NULL) {
        vita2d_wait_rendering_done();
        vita2d_free_shader(shader);
    }
}
