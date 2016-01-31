#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "main_loop.h"
#include "mx_math.h"

static void GetCommandLineParameter( const char* cmd_line, const char* parameter_name, float& out_parameter )
{
	const char* str= std::strstr( cmd_line, parameter_name );
	if( str == NULL ) return;

	while( *str != 0 && *str != ' ' ) str++;
	if( *str == 0 ) return;

	out_parameter= float(std::atof( str ));
}

#ifdef MX_DEBUG
int main()
#else
int WINAPI WinMain(
	HINSTANCE /*hInstance*/,
	HINSTANCE /*hPrevInstance*/,
	LPSTR /*lpCmdLine*/,
	int /*nCmdShow*/)
#endif
{
	// TODO - remove this code, if we reach 32kb limit
	const char* cmd= GetCommandLine();
	bool fullscreen= std::strstr( cmd, "--fullscreen" ) != NULL;
	bool vsync= std::strstr( cmd, "--no-vsync" ) == NULL;
	bool invert_mouse= std::strstr( cmd, "--invert-mouse-y" ) != NULL;

	float sens_x= 1.0f;
	float sens_y= 1.0f;
	GetCommandLineParameter( cmd, "--sx", sens_x );
	GetCommandLineParameter( cmd, "--sy", sens_y );
	sens_x= mxClamp( 0.1f, 10.0f, sens_x );
	sens_y= mxClamp( 0.1f, 10.0f, sens_y );

	mx_MainLoop::CreateInstance(
		1024, 768,
		fullscreen, vsync,
		invert_mouse,
		sens_x, sens_y );

	mx_MainLoop::Instance()->Loop();
	mx_MainLoop::DeleteInstance();

	return 0;
}