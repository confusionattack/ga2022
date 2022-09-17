#include "mutex.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

mutex_t* mutex_create()
{
	return (mutex_t*)CreateMutex(NULL, FALSE, NULL);
}

void mutex_destroy(mutex_t* mutex)
{
	CloseHandle(mutex);
}

void mutex_lock(mutex_t* mutex)
{
	WaitForSingleObject(mutex, INFINITE);
}

void mutex_unlock(mutex_t* mutex)
{
	ReleaseMutex(mutex);
}
