#include "thread.h"

#include "debug.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

thread_t* thread_create(int (*function)(void*), void* data)
{
	HANDLE h = CreateThread(NULL, 0, function, data, CREATE_SUSPENDED, NULL);
	if (h == INVALID_HANDLE_VALUE)
	{
		debug_print(k_print_warning, "Thread failed to create!\n");
		return NULL;
	}
	ResumeThread(h);
	return (thread_t*)h;
}

int thread_destroy(thread_t* thread)
{
	WaitForSingleObject(thread, INFINITE);
	int code = 0;
	GetExitCodeThread(thread, &code);
	CloseHandle(thread);
	return code;
}

void thread_sleep(uint32_t ms)
{
	Sleep(ms);
}
