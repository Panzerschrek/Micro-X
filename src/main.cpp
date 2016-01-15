#include <cstdlib>
#include <cstdio>

#include "main_loop.h"

int main()
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