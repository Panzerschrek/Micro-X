#include "funcs.h"

#define PROCESS_OGL_FUNCTION( TYPE, NAME ) TYPE NAME= NULL
#include "funcs_list.h"
#undef PROCESS_OGL_FUNCTION

void mxGetGLFunctions(
	void* (*GetProcAddressFunc)(const char* func_name))
{
	#define PROCESS_OGL_FUNCTION( TYPE, NAME )\
	NAME= (TYPE) GetProcAddressFunc( #NAME )\

	#include "funcs_list.h"

	#undef PROCESS_OGL_FUNCTION
}
