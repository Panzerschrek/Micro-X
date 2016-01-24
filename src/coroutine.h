#pragma once

// If this struct changed
// mxSetJump/mxLongJump must be changed too
struct mx_StateBuff
{
	/*
	In functions call registers EAX, ECX, EDX are trashed.
	ESP, EBP, EBX, ESI, EDI, must be saved.
	Additional registers, such
		ST(0)-ST(7) (and MM0-MM7), XMM0-XMM7, YMM0-YMM7, ZMM0-ZMM7, K0-K7,
	are trashed.
	*/
	unsigned int* esp; // 0
	unsigned int ebp; // 4
	unsigned int esi; // 8
	unsigned int edi; // 12
	void (*jump_p)(void*); // 16
	unsigned int ebx; // 20
};

class Coroutine
{
public:
	// Create coroutine with proper stack.
	// Warning! Beware stack overflowing.
	Coroutine( unsigned int stack_size= 65536 );

	// Call this, if you need coroutine execution.
	// Function returns, after working code calls Resume.
	void Exec() /*final*/;

protected:
	// Reimplement this
	virtual void ExecFunc()= 0;

	void Resume()/*final*/;

private:
	static void Do(void* this_p);

private:
	mx_StateBuff inner_state_;
	mx_StateBuff outer_state_;
	unsigned char* stack_;
	unsigned int stack_size_;
};