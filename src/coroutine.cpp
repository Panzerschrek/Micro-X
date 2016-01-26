#include "coroutine.h"

__declspec(naked) int mxSetJump(mx_StateBuff* /*state*/)
{
	_asm
	{
		mov eax, DWORD PTR[esp +4] ; state
		mov DWORD PTR[eax   ], esp
		mov DWORD PTR[eax+ 4], ebp
		mov DWORD PTR[eax+ 8], esi
		mov DWORD PTR[eax+12], edi
		mov DWORD PTR[eax+20], ebx
		lea ecx, after_jmp
		mov DWORD PTR[eax+16], ecx  ; jump_p

		mov eax, 0
		ret

	after_jmp:
		mov eax,1
		ret
	}
}

__declspec(naked) void mxLongJump(const mx_StateBuff* /*state*/)
{
	_asm
	{
		mov eax, DWORD PTR[esp +4] ; state
		mov edi, DWORD PTR[eax+12]
		mov esi, DWORD PTR[eax+ 8]
		mov ebp, DWORD PTR[eax+ 4]
		mov ebx, DWORD PTR[eax+20]
		mov esp, DWORD PTR[eax   ]
		mov edx, DWORD PTR[eax+16] ; jump_p
		jmp edx
	}
}

mx_Coroutine::mx_Coroutine( unsigned int stack_size )
	: stack_( new unsigned char[stack_size] )
	, stack_size_(stack_size)
{
	// Setup initial state

	// Set our proper stack
	inner_state_.esp= (unsigned int*)(stack_ + stack_size_ - 16);
	// Set entry point
	inner_state_.jump_p= Do;
	// Move to stack first argument of function
	inner_state_.esp[1]= reinterpret_cast<unsigned int>(this);
}

void mx_Coroutine::Exec()
{
	if( mxSetJump(&outer_state_) )
		return;
	mxLongJump( &inner_state_ );
}

void mx_Coroutine::Resume()
{
	if( mxSetJump(&inner_state_) )
		return;
	mxLongJump( &outer_state_ );
}
void mx_Coroutine::Do(void* this_p)
{
	mx_Coroutine* th= static_cast<mx_Coroutine*>(this_p);
	th->ExecFunc();

	// We should never return, because this function is first in our stack
	for(;;)
		th->Resume();
}
