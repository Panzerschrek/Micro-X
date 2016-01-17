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
	mx_MainLoop::CreateInstance(
		1024, 768,
		false, true,
		false,
		1.0f, 0.7f );

	mx_MainLoop::Instance()->Loop();
	mx_MainLoop::DeleteInstance();


	return 0;
}