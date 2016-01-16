#include "funcs.h"

#define PROCESS_OGL_FUNCTION( TYPE, NAME ) TYPE NAME= NULL
#include "funcs_list.h"
#undef PROCESS_OGL_FUNCTION

void mxGetGLFunctions()
{
	#define PROCESS_OGL_FUNCTION( TYPE, NAME )\
	NAME= (TYPE) wglGetProcAddress( #NAME )\

	#include "funcs_list.h"

	#undef PROCESS_OGL_FUNCTION
}
