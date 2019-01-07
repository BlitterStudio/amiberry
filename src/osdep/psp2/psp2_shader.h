//
// Created by cpasjuste on 18/12/16.
//

#ifndef UAE4ALL2_PSP2_SHADER_H
#define UAE4ALL2_PSP2_SHADER_H

#include "vita2d_fbo/includes/vita2d.h"

class PSP2Shader {

public:

    enum Shader {
        NONE = 0,
        LCD3X,
        SCALE2X,
        AAA,
        SHARP_BILINEAR,
        SHARP_BILINEAR_SIMPLE,
        FXAA,
    };

    PSP2Shader(Shader shaderType);
    ~PSP2Shader();

private:
    vita2d_shader *shader = NULL;
};

#endif //UAE4ALL2_PSP2_SHADER_H
