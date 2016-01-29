#include <cstdlib>
#include <cstdio>

#include "main_loop.h"

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
	bool fullscreen= strstr( cmd, "--fullscreen" ) != NULL;

	mx_MainLoop::CreateInstance(
		1024, 768,
		fullscreen, true,
		false,
		1.0f, 0.7f );

	mx_MainLoop::Instance()->Loop();
	mx_MainLoop::DeleteInstance();

	return 0;
}