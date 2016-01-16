#pragma once

#include "glcorearb.h"
#include "wglext.h"
#include <GL/gl.h>

#define PROCESS_OGL_FUNCTION( TYPE, NAME ) extern TYPE NAME
#include "funcs_list.h"
#undef PROCESS_OGL_FUNCTION

void mxGetGLFunctions();
